/*============================================================================
 *  ScopeManager.cpp - CScopeManager implementation.
 *
 *  Discovers model plugins with QPluginLoader, maps a scope number to a plugin
 *  instance and forwards every operation. Mirrors CELoadManager. A single
 *  mutex serialises access to the plugin/instance maps and to per-scope calls
 *  (so concurrent calls on the same scope number never interleave); calls on
 *  different scope numbers are correct under concurrency by construction.
 *==========================================================================*/
#include "ScopeManager.h"

#include <QDir>
#include <QFileInfo>

/*----------------------------------------------------------------------------
 * Forwarding helper: lock, resolve the plugin owning the scope number, call it.
 *--------------------------------------------------------------------------*/
#define SCP_FWD(scope, call)                                                     \
    do {                                                                        \
        QMutexLocker _lock(&m_mutex);                                           \
        CIScopePlugin* pPlugin = getPlugin(scope);                             \
        if (pPlugin == nullptr) {                                               \
            return ScopeError(Enum_Scope_ErrorCode::INVALID_SCOPE_NUMBER,       \
                QStringLiteral("no instance for scope number %1").arg(scope));  \
        }                                                                       \
        return pPlugin->call;                                                   \
    } while (0)

/*============================================================================
 *  Singleton / lifecycle
 *==========================================================================*/
CScopeManager& CScopeManager::instance()
{
    static CScopeManager s_instance;
    return s_instance;
}

CScopeManager::CScopeManager()
    : m_pfnTrace(nullptr)
    , m_pvTraceUser(nullptr)
{
}

CScopeManager::~CScopeManager()
{
    m_mapInstances.clear();
    for (auto it = m_mapPlugins.begin(); it != m_mapPlugins.end(); ++it)
    {
        if (it.value().m_pLoader != nullptr)
        {
            it.value().m_pLoader->unload();
            delete it.value().m_pLoader;
        }
    }
    m_mapPlugins.clear();
}

CIScopePlugin* CScopeManager::getPlugin(U32BIT in_u32ScopeNumber) const
{
    if (!m_mapInstances.contains(in_u32ScopeNumber))
    {
        return nullptr;
    }
    const QString strName = m_mapInstances.value(in_u32ScopeNumber);
    if (!m_mapPlugins.contains(strName))
    {
        return nullptr;
    }
    return m_mapPlugins.value(strName).m_pPlugin;
}

void CScopeManager::setTraceCallback(ScopeTraceCallback in_pfnCallback, void* in_pvUser)
{
    QMutexLocker lock(&m_mutex);
    m_pfnTrace = in_pfnCallback;
    m_pvTraceUser = in_pvUser;
}

/*============================================================================
 *  Plugin management
 *==========================================================================*/
void CScopeManager::loadPlugins(const QString& in_strPluginPath)
{
    QMutexLocker lock(&m_mutex);
    QDir dirPlugins(in_strPluginPath);
    QStringList lstFilters;
#ifdef Q_OS_WIN
    lstFilters << QStringLiteral("*.dll");
#else
    lstFilters << QStringLiteral("*.so") << QStringLiteral("*.so.*");
#endif
    dirPlugins.setNameFilters(lstFilters);

    const QStringList lstFiles = dirPlugins.entryList(QDir::Files);
    for (const QString& strFileName : lstFiles)
    {
        // The mock VISA library lives beside the plugins in some layouts;
        // never try to load it as a plugin.
        if (strFileName.contains(QStringLiteral("visa"), Qt::CaseInsensitive))
        {
            continue;
        }
        const QString strFullPath = dirPlugins.absoluteFilePath(strFileName);
        QPluginLoader* pLoader = new QPluginLoader(strFullPath);

        QObject* pObject = pLoader->instance();
        if (pObject == nullptr)
        {
            emit pluginLoadFailed(strFileName, pLoader->errorString());
            delete pLoader;
            continue;
        }

        CIScopePlugin* pPlugin = qobject_cast<CIScopePlugin*>(pObject);
        if (pPlugin == nullptr)
        {
            emit pluginLoadFailed(strFileName, QStringLiteral("not a Scope plugin"));
            pLoader->unload();
            delete pLoader;
            continue;
        }

        const S_Scope_PluginInfo sInfo = pPlugin->getPluginInfo();
        if (!sInfo.isCompatible(getCoreVersion()))
        {
            emit pluginLoadFailed(strFileName, QStringLiteral("version incompatible"));
            pLoader->unload();
            delete pLoader;
            continue;
        }

        const QString strName = QString::fromLatin1(sInfo.m_szName);
        if (m_mapPlugins.contains(strName))
        {
            pLoader->unload();
            delete pLoader;
            continue;
        }

        S_PluginData sData;
        sData.m_pLoader = pLoader;
        sData.m_pPlugin = pPlugin;
        sData.m_sInfo   = sInfo;
        sData.m_sCaps   = pPlugin->getCapabilities();
        m_mapPlugins.insert(strName, sData);
        emit pluginLoaded(strName);
    }
}

QStringList CScopeManager::getAvailablePlugins() const
{
    QMutexLocker lock(&m_mutex);
    return m_mapPlugins.keys();
}

S_Scope_PluginInfo CScopeManager::getPluginInfoByName(const QString& in_strPluginName) const
{
    QMutexLocker lock(&m_mutex);
    return m_mapPlugins.value(in_strPluginName).m_sInfo;
}

S_Scope_Capabilities CScopeManager::getPluginCapabilities(const QString& in_strPluginName) const
{
    QMutexLocker lock(&m_mutex);
    return m_mapPlugins.value(in_strPluginName).m_sCaps;
}

/*============================================================================
 *  Instance management
 *==========================================================================*/
ScopeError CScopeManager::createInstance(U32BIT in_u32ScopeNumber, const QString& in_strPluginName)
{
    QMutexLocker lock(&m_mutex);
    if (in_u32ScopeNumber < 1)
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_SCOPE_NUMBER,
                          QStringLiteral("scope number must be >= 1"));
    }
    if (m_mapInstances.contains(in_u32ScopeNumber))
    {
        return ScopeError(Enum_Scope_ErrorCode::ALREADY_CONNECTED,
                          QStringLiteral("scope number %1 already in use").arg(in_u32ScopeNumber));
    }
    if (!m_mapPlugins.contains(in_strPluginName))
    {
        return ScopeError(Enum_Scope_ErrorCode::PLUGIN_NOT_FOUND,
                          QStringLiteral("plugin \"%1\" not found").arg(in_strPluginName));
    }
    m_mapInstances.insert(in_u32ScopeNumber, in_strPluginName);
    emit instanceCreated(in_u32ScopeNumber, in_strPluginName);
    return ScopeError(Enum_Scope_ErrorCode::SUCCESS);
}

ScopeError CScopeManager::destroyInstance(U32BIT in_u32ScopeNumber)
{
    QMutexLocker lock(&m_mutex);
    if (!m_mapInstances.contains(in_u32ScopeNumber))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_SCOPE_NUMBER,
                          QStringLiteral("no instance for scope number %1").arg(in_u32ScopeNumber));
    }
    CIScopePlugin* pPlugin = getPlugin(in_u32ScopeNumber);
    if (pPlugin != nullptr && pPlugin->isConnected(in_u32ScopeNumber))
    {
        pPlugin->disconnect(in_u32ScopeNumber);
    }
    m_mapInstances.remove(in_u32ScopeNumber);
    emit instanceDestroyed(in_u32ScopeNumber);
    return ScopeError(Enum_Scope_ErrorCode::SUCCESS);
}

bool CScopeManager::instanceExists(U32BIT in_u32ScopeNumber) const
{
    QMutexLocker lock(&m_mutex);
    return m_mapInstances.contains(in_u32ScopeNumber);
}

QString CScopeManager::getInstancePlugin(U32BIT in_u32ScopeNumber) const
{
    QMutexLocker lock(&m_mutex);
    return m_mapInstances.value(in_u32ScopeNumber);
}

S_Scope_Capabilities CScopeManager::getCapabilities(U32BIT in_u32ScopeNumber) const
{
    QMutexLocker lock(&m_mutex);
    CIScopePlugin* pPlugin = getPlugin(in_u32ScopeNumber);
    if (pPlugin == nullptr)
    {
        return S_Scope_Capabilities();
    }
    return pPlugin->getCapabilities();
}

/*============================================================================
 *  Connection / core
 *==========================================================================*/
ScopeError CScopeManager::connect(U32BIT s, const S_Scope_ConnectionConfig& c) { SCP_FWD(s, connect(s, c)); }
ScopeError CScopeManager::disconnect(U32BIT s) { SCP_FWD(s, disconnect(s)); }
bool CScopeManager::isConnected(U32BIT s) const
{
    QMutexLocker lock(&m_mutex);
    CIScopePlugin* pPlugin = getPlugin(s);
    return (pPlugin != nullptr) && pPlugin->isConnected(s);
}
ScopeError CScopeManager::reset(U32BIT s) { SCP_FWD(s, reset(s)); }
ScopeError CScopeManager::setTimeout(U32BIT s, U32BIT t) { SCP_FWD(s, setTimeout(s, t)); }
ScopeError CScopeManager::clearStatus(U32BIT s) { SCP_FWD(s, clearStatus(s)); }
ScopeError CScopeManager::getIdentification(U32BIT s, QString& o) { SCP_FWD(s, getIdentification(s, o)); }
ScopeError CScopeManager::selfTest(U32BIT s, S32BIT& o) { SCP_FWD(s, selfTest(s, o)); }
ScopeError CScopeManager::getOptions(U32BIT s, QString& o) { SCP_FWD(s, getOptions(s, o)); }
ScopeError CScopeManager::waitOperationComplete(U32BIT s) { SCP_FWD(s, waitOperationComplete(s)); }
ScopeError CScopeManager::getScpiVersion(U32BIT s, QString& o) { SCP_FWD(s, getScpiVersion(s, o)); }
ScopeError CScopeManager::getParameterRange(U32BIT s, U32BIT c, Enum_Scope_ParamId p, S_Scope_ParameterRange& o) { SCP_FWD(s, getParameterRange(s, c, p, o)); }
ScopeError CScopeManager::autoscale(U32BIT s) { SCP_FWD(s, autoscale(s)); }
ScopeError CScopeManager::run(U32BIT s) { SCP_FWD(s, run(s)); }
ScopeError CScopeManager::stop(U32BIT s) { SCP_FWD(s, stop(s)); }
ScopeError CScopeManager::single(U32BIT s) { SCP_FWD(s, single(s)); }
ScopeError CScopeManager::forceTrigger(U32BIT s) { SCP_FWD(s, forceTrigger(s)); }

/*============================================================================
 *  Vertical
 *==========================================================================*/
ScopeError CScopeManager::enableChannel(U32BIT s, U32BIT c, bool v) { SCP_FWD(s, enableChannel(s, c, v)); }
ScopeError CScopeManager::isChannelEnabled(U32BIT s, U32BIT c, bool& o) { SCP_FWD(s, isChannelEnabled(s, c, o)); }
ScopeError CScopeManager::setVerticalScale(U32BIT s, U32BIT c, FDOUBLE v) { SCP_FWD(s, setVerticalScale(s, c, v)); }
ScopeError CScopeManager::getVerticalScale(U32BIT s, U32BIT c, FDOUBLE& o) { SCP_FWD(s, getVerticalScale(s, c, o)); }
ScopeError CScopeManager::setVerticalOffset(U32BIT s, U32BIT c, FDOUBLE v) { SCP_FWD(s, setVerticalOffset(s, c, v)); }
ScopeError CScopeManager::getVerticalOffset(U32BIT s, U32BIT c, FDOUBLE& o) { SCP_FWD(s, getVerticalOffset(s, c, o)); }
ScopeError CScopeManager::setVerticalPosition(U32BIT s, U32BIT c, FDOUBLE v) { SCP_FWD(s, setVerticalPosition(s, c, v)); }
ScopeError CScopeManager::getVerticalPosition(U32BIT s, U32BIT c, FDOUBLE& o) { SCP_FWD(s, getVerticalPosition(s, c, o)); }
ScopeError CScopeManager::setCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling v) { SCP_FWD(s, setCoupling(s, c, v)); }
ScopeError CScopeManager::getCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling& o) { SCP_FWD(s, getCoupling(s, c, o)); }
ScopeError CScopeManager::setBandwidthLimit(U32BIT s, U32BIT c, Enum_Scope_BandwidthLimit v) { SCP_FWD(s, setBandwidthLimit(s, c, v)); }
ScopeError CScopeManager::getBandwidthLimit(U32BIT s, U32BIT c, Enum_Scope_BandwidthLimit& o) { SCP_FWD(s, getBandwidthLimit(s, c, o)); }
ScopeError CScopeManager::setProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE v) { SCP_FWD(s, setProbeAttenuation(s, c, v)); }
ScopeError CScopeManager::getProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE& o) { SCP_FWD(s, getProbeAttenuation(s, c, o)); }
ScopeError CScopeManager::setInputImpedance(U32BIT s, U32BIT c, Enum_Scope_InputImpedance v) { SCP_FWD(s, setInputImpedance(s, c, v)); }
ScopeError CScopeManager::getInputImpedance(U32BIT s, U32BIT c, Enum_Scope_InputImpedance& o) { SCP_FWD(s, getInputImpedance(s, c, o)); }
ScopeError CScopeManager::setInvert(U32BIT s, U32BIT c, bool v) { SCP_FWD(s, setInvert(s, c, v)); }
ScopeError CScopeManager::getInvert(U32BIT s, U32BIT c, bool& o) { SCP_FWD(s, getInvert(s, c, o)); }
ScopeError CScopeManager::setChannelLabel(U32BIT s, U32BIT c, const QString& v) { SCP_FWD(s, setChannelLabel(s, c, v)); }
ScopeError CScopeManager::getChannelLabel(U32BIT s, U32BIT c, QString& o) { SCP_FWD(s, getChannelLabel(s, c, o)); }
ScopeError CScopeManager::setChannelUnits(U32BIT s, U32BIT c, const QString& v) { SCP_FWD(s, setChannelUnits(s, c, v)); }
ScopeError CScopeManager::getChannelUnits(U32BIT s, U32BIT c, QString& o) { SCP_FWD(s, getChannelUnits(s, c, o)); }
ScopeError CScopeManager::setDeskew(U32BIT s, U32BIT c, FDOUBLE v) { SCP_FWD(s, setDeskew(s, c, v)); }
ScopeError CScopeManager::getDeskew(U32BIT s, U32BIT c, FDOUBLE& o) { SCP_FWD(s, getDeskew(s, c, o)); }

/*============================================================================
 *  Horizontal / timebase
 *==========================================================================*/
ScopeError CScopeManager::setTimebaseScale(U32BIT s, FDOUBLE v) { SCP_FWD(s, setTimebaseScale(s, v)); }
ScopeError CScopeManager::getTimebaseScale(U32BIT s, FDOUBLE& o) { SCP_FWD(s, getTimebaseScale(s, o)); }
ScopeError CScopeManager::setTimebasePosition(U32BIT s, FDOUBLE v) { SCP_FWD(s, setTimebasePosition(s, v)); }
ScopeError CScopeManager::getTimebasePosition(U32BIT s, FDOUBLE& o) { SCP_FWD(s, getTimebasePosition(s, o)); }
ScopeError CScopeManager::setTimebaseReference(U32BIT s, FDOUBLE v) { SCP_FWD(s, setTimebaseReference(s, v)); }
ScopeError CScopeManager::setTimebaseMode(U32BIT s, Enum_Scope_TimebaseMode v) { SCP_FWD(s, setTimebaseMode(s, v)); }
ScopeError CScopeManager::getTimebaseMode(U32BIT s, Enum_Scope_TimebaseMode& o) { SCP_FWD(s, getTimebaseMode(s, o)); }
ScopeError CScopeManager::getSampleRate(U32BIT s, FDOUBLE& o) { SCP_FWD(s, getSampleRate(s, o)); }
ScopeError CScopeManager::setMemoryDepth(U32BIT s, U32BIT v) { SCP_FWD(s, setMemoryDepth(s, v)); }
ScopeError CScopeManager::getMemoryDepth(U32BIT s, U32BIT& o) { SCP_FWD(s, getMemoryDepth(s, o)); }
ScopeError CScopeManager::setAcquisitionPoints(U32BIT s, U32BIT v) { SCP_FWD(s, setAcquisitionPoints(s, v)); }
ScopeError CScopeManager::getAcquisitionPoints(U32BIT s, U32BIT& o) { SCP_FWD(s, getAcquisitionPoints(s, o)); }

/*============================================================================
 *  Trigger
 *==========================================================================*/
ScopeError CScopeManager::setTriggerMode(U32BIT s, Enum_Scope_TriggerMode v) { SCP_FWD(s, setTriggerMode(s, v)); }
ScopeError CScopeManager::getTriggerMode(U32BIT s, Enum_Scope_TriggerMode& o) { SCP_FWD(s, getTriggerMode(s, o)); }
ScopeError CScopeManager::setTriggerType(U32BIT s, Enum_Scope_TriggerType v) { SCP_FWD(s, setTriggerType(s, v)); }
ScopeError CScopeManager::getTriggerType(U32BIT s, Enum_Scope_TriggerType& o) { SCP_FWD(s, getTriggerType(s, o)); }
ScopeError CScopeManager::setTriggerSource(U32BIT s, Enum_Scope_TriggerSource v) { SCP_FWD(s, setTriggerSource(s, v)); }
ScopeError CScopeManager::getTriggerSource(U32BIT s, Enum_Scope_TriggerSource& o) { SCP_FWD(s, getTriggerSource(s, o)); }
ScopeError CScopeManager::setTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope v) { SCP_FWD(s, setTriggerSlope(s, v)); }
ScopeError CScopeManager::getTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope& o) { SCP_FWD(s, getTriggerSlope(s, o)); }
ScopeError CScopeManager::setTriggerLevel(U32BIT s, U32BIT c, FDOUBLE v) { SCP_FWD(s, setTriggerLevel(s, c, v)); }
ScopeError CScopeManager::getTriggerLevel(U32BIT s, U32BIT c, FDOUBLE& o) { SCP_FWD(s, getTriggerLevel(s, c, o)); }
ScopeError CScopeManager::setTriggerCoupling(U32BIT s, Enum_Scope_Coupling v) { SCP_FWD(s, setTriggerCoupling(s, v)); }
ScopeError CScopeManager::getTriggerCoupling(U32BIT s, Enum_Scope_Coupling& o) { SCP_FWD(s, getTriggerCoupling(s, o)); }
ScopeError CScopeManager::setTriggerHoldoff(U32BIT s, FDOUBLE v) { SCP_FWD(s, setTriggerHoldoff(s, v)); }
ScopeError CScopeManager::getTriggerHoldoff(U32BIT s, FDOUBLE& o) { SCP_FWD(s, getTriggerHoldoff(s, o)); }
ScopeError CScopeManager::getTriggerState(U32BIT s, Enum_Scope_TriggerState& o) { SCP_FWD(s, getTriggerState(s, o)); }
ScopeError CScopeManager::setTriggerPulseWidth(U32BIT s, FDOUBLE v) { SCP_FWD(s, setTriggerPulseWidth(s, v)); }
ScopeError CScopeManager::setTriggerVideoStandard(U32BIT s, const QString& v) { SCP_FWD(s, setTriggerVideoStandard(s, v)); }
ScopeError CScopeManager::setTriggerPattern(U32BIT s, const QString& v) { SCP_FWD(s, setTriggerPattern(s, v)); }

/*============================================================================
 *  Acquisition
 *==========================================================================*/
ScopeError CScopeManager::setAcqMode(U32BIT s, Enum_Scope_AcqMode v) { SCP_FWD(s, setAcqMode(s, v)); }
ScopeError CScopeManager::getAcqMode(U32BIT s, Enum_Scope_AcqMode& o) { SCP_FWD(s, getAcqMode(s, o)); }
ScopeError CScopeManager::setAverageCount(U32BIT s, U32BIT v) { SCP_FWD(s, setAverageCount(s, v)); }
ScopeError CScopeManager::getAverageCount(U32BIT s, U32BIT& o) { SCP_FWD(s, getAverageCount(s, o)); }
ScopeError CScopeManager::getAcquisitionState(U32BIT s, Enum_Scope_AcqState& o) { SCP_FWD(s, getAcquisitionState(s, o)); }
ScopeError CScopeManager::setSegmentedCount(U32BIT s, U32BIT v) { SCP_FWD(s, setSegmentedCount(s, v)); }
ScopeError CScopeManager::getSegmentedCount(U32BIT s, U32BIT& o) { SCP_FWD(s, getSegmentedCount(s, o)); }

/*============================================================================
 *  Waveform transfer
 *==========================================================================*/
ScopeError CScopeManager::setWaveformSource(U32BIT s, Enum_Scope_WaveformSource v, U32BIT i) { SCP_FWD(s, setWaveformSource(s, v, i)); }
ScopeError CScopeManager::getWaveformSource(U32BIT s, Enum_Scope_WaveformSource& o, U32BIT& i) { SCP_FWD(s, getWaveformSource(s, o, i)); }
ScopeError CScopeManager::setWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat v) { SCP_FWD(s, setWaveformFormat(s, v)); }
ScopeError CScopeManager::getWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat& o) { SCP_FWD(s, getWaveformFormat(s, o)); }
ScopeError CScopeManager::setWaveformPoints(U32BIT s, U32BIT v) { SCP_FWD(s, setWaveformPoints(s, v)); }
ScopeError CScopeManager::getWaveformPreamble(U32BIT s, U32BIT c, S_Scope_WaveformPreamble& o) { SCP_FWD(s, getWaveformPreamble(s, c, o)); }
ScopeError CScopeManager::readWaveform(U32BIT s, U32BIT c, S_Scope_Waveform& o) { SCP_FWD(s, readWaveform(s, c, o)); }
ScopeError CScopeManager::digitizeChannel(U32BIT s, U32BIT c) { SCP_FWD(s, digitizeChannel(s, c)); }

/*============================================================================
 *  Measurements
 *==========================================================================*/
ScopeError CScopeManager::addMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t) { SCP_FWD(s, addMeasurement(s, c, t)); }
ScopeError CScopeManager::readMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t, S_Scope_MeasurementResult& o) { SCP_FWD(s, readMeasurement(s, c, t, o)); }
ScopeError CScopeManager::clearMeasurements(U32BIT s) { SCP_FWD(s, clearMeasurements(s)); }
ScopeError CScopeManager::setMeasureStatistics(U32BIT s, bool v) { SCP_FWD(s, setMeasureStatistics(s, v)); }
ScopeError CScopeManager::getMeasurementStatistics(U32BIT s, U32BIT c, Enum_Scope_MeasType t, S_Scope_MeasurementResult& o) { SCP_FWD(s, getMeasurementStatistics(s, c, t, o)); }

/*============================================================================
 *  Math / FFT
 *==========================================================================*/
ScopeError CScopeManager::setMathOperation(U32BIT s, Enum_Scope_MathOp v) { SCP_FWD(s, setMathOperation(s, v)); }
ScopeError CScopeManager::setMathSource1(U32BIT s, U32BIT c) { SCP_FWD(s, setMathSource1(s, c)); }
ScopeError CScopeManager::setMathSource2(U32BIT s, U32BIT c) { SCP_FWD(s, setMathSource2(s, c)); }
ScopeError CScopeManager::enableMath(U32BIT s, bool v) { SCP_FWD(s, enableMath(s, v)); }
ScopeError CScopeManager::setFftWindow(U32BIT s, Enum_Scope_FftWindow v) { SCP_FWD(s, setFftWindow(s, v)); }
ScopeError CScopeManager::getFftWindow(U32BIT s, Enum_Scope_FftWindow& o) { SCP_FWD(s, getFftWindow(s, o)); }
ScopeError CScopeManager::setFftSpan(U32BIT s, FDOUBLE v) { SCP_FWD(s, setFftSpan(s, v)); }
ScopeError CScopeManager::setFftCenter(U32BIT s, FDOUBLE v) { SCP_FWD(s, setFftCenter(s, v)); }
ScopeError CScopeManager::setMathScale(U32BIT s, FDOUBLE v) { SCP_FWD(s, setMathScale(s, v)); }
ScopeError CScopeManager::setMathPosition(U32BIT s, FDOUBLE v) { SCP_FWD(s, setMathPosition(s, v)); }

/*============================================================================
 *  Cursors
 *==========================================================================*/
ScopeError CScopeManager::setCursorType(U32BIT s, Enum_Scope_CursorType v) { SCP_FWD(s, setCursorType(s, v)); }
ScopeError CScopeManager::getCursorType(U32BIT s, Enum_Scope_CursorType& o) { SCP_FWD(s, getCursorType(s, o)); }
ScopeError CScopeManager::setCursorSource(U32BIT s, U32BIT c) { SCP_FWD(s, setCursorSource(s, c)); }
ScopeError CScopeManager::setCursorPosition(U32BIT s, U32BIT i, FDOUBLE v) { SCP_FWD(s, setCursorPosition(s, i, v)); }
ScopeError CScopeManager::getCursorPosition(U32BIT s, U32BIT i, FDOUBLE& o) { SCP_FWD(s, getCursorPosition(s, i, o)); }
ScopeError CScopeManager::readCursorValues(U32BIT s, FDOUBLE& x1, FDOUBLE& x2, FDOUBLE& y1, FDOUBLE& y2) { SCP_FWD(s, readCursorValues(s, x1, x2, y1, y2)); }

/*============================================================================
 *  Display
 *==========================================================================*/
ScopeError CScopeManager::setPersistence(U32BIT s, FDOUBLE v) { SCP_FWD(s, setPersistence(s, v)); }
ScopeError CScopeManager::setGraticule(U32BIT s, const QString& v) { SCP_FWD(s, setGraticule(s, v)); }
ScopeError CScopeManager::setIntensity(U32BIT s, FDOUBLE v) { SCP_FWD(s, setIntensity(s, v)); }
ScopeError CScopeManager::setDisplayFormat(U32BIT s, Enum_Scope_TimebaseMode v) { SCP_FWD(s, setDisplayFormat(s, v)); }
ScopeError CScopeManager::setVectors(U32BIT s, bool v) { SCP_FWD(s, setVectors(s, v)); }

/*============================================================================
 *  Save / recall / screenshot
 *==========================================================================*/
ScopeError CScopeManager::saveSetup(U32BIT s, U32BIT v) { SCP_FWD(s, saveSetup(s, v)); }
ScopeError CScopeManager::recallSetup(U32BIT s, U32BIT v) { SCP_FWD(s, recallSetup(s, v)); }
ScopeError CScopeManager::saveWaveformToFile(U32BIT s, U32BIT c, const QString& v) { SCP_FWD(s, saveWaveformToFile(s, c, v)); }
ScopeError CScopeManager::captureScreenshot(U32BIT s, Enum_Scope_ImageFormat f, QByteArray& o) { SCP_FWD(s, captureScreenshot(s, f, o)); }
ScopeError CScopeManager::saveToReference(U32BIT s, U32BIT c, U32BIT r) { SCP_FWD(s, saveToReference(s, c, r)); }
ScopeError CScopeManager::displayReference(U32BIT s, U32BIT r, bool v) { SCP_FWD(s, displayReference(s, r, v)); }

/*============================================================================
 *  Digital / MSO
 *==========================================================================*/
ScopeError CScopeManager::enableDigitalChannel(U32BIT s, U32BIT d, bool v) { SCP_FWD(s, enableDigitalChannel(s, d, v)); }
ScopeError CScopeManager::setDigitalThreshold(U32BIT s, U32BIT d, FDOUBLE v) { SCP_FWD(s, setDigitalThreshold(s, d, v)); }
ScopeError CScopeManager::setPodThreshold(U32BIT s, U32BIT p, FDOUBLE v) { SCP_FWD(s, setPodThreshold(s, p, v)); }
ScopeError CScopeManager::enableBus(U32BIT s, U32BIT b, bool v) { SCP_FWD(s, enableBus(s, b, v)); }
ScopeError CScopeManager::setBusType(U32BIT s, U32BIT b, const QString& v) { SCP_FWD(s, setBusType(s, b, v)); }
ScopeError CScopeManager::readBusDecode(U32BIT s, U32BIT b, QString& o) { SCP_FWD(s, readBusDecode(s, b, o)); }

/*============================================================================
 *  AWG / Wavegen
 *==========================================================================*/
ScopeError CScopeManager::setAwgFunction(U32BIT s, const QString& v) { SCP_FWD(s, setAwgFunction(s, v)); }
ScopeError CScopeManager::setAwgFrequency(U32BIT s, FDOUBLE v) { SCP_FWD(s, setAwgFrequency(s, v)); }
ScopeError CScopeManager::setAwgAmplitude(U32BIT s, FDOUBLE v) { SCP_FWD(s, setAwgAmplitude(s, v)); }
ScopeError CScopeManager::setAwgOffset(U32BIT s, FDOUBLE v) { SCP_FWD(s, setAwgOffset(s, v)); }
ScopeError CScopeManager::enableAwgOutput(U32BIT s, bool v) { SCP_FWD(s, enableAwgOutput(s, v)); }

/*============================================================================
 *  Status / system
 *==========================================================================*/
ScopeError CScopeManager::readErrorStatus(U32BIT s, U32BIT c, S_Scope_DeviceErrorStatus& o) { SCP_FWD(s, readErrorStatus(s, c, o)); }
ScopeError CScopeManager::clearErrorStatus(U32BIT s, U32BIT c) { SCP_FWD(s, clearErrorStatus(s, c)); }
ScopeError CScopeManager::queryErrorQueue(U32BIT s, QString& o) { SCP_FWD(s, queryErrorQueue(s, o)); }
ScopeError CScopeManager::getInstrumentErrorCount(U32BIT s, U32BIT& o) { SCP_FWD(s, getInstrumentErrorCount(s, o)); }
ScopeError CScopeManager::readStandardEventStatus(U32BIT s, U32BIT& o) { SCP_FWD(s, readStandardEventStatus(s, o)); }
ScopeError CScopeManager::readStatusByte(U32BIT s, U32BIT& o) { SCP_FWD(s, readStatusByte(s, o)); }
ScopeError CScopeManager::readOperationStatus(U32BIT s, U32BIT& o) { SCP_FWD(s, readOperationStatus(s, o)); }
ScopeError CScopeManager::readQuestionableStatus(U32BIT s, U32BIT& o) { SCP_FWD(s, readQuestionableStatus(s, o)); }
ScopeError CScopeManager::setRemoteState(U32BIT s, Enum_Scope_RemoteState v) { SCP_FWD(s, setRemoteState(s, v)); }
ScopeError CScopeManager::getRemoteState(U32BIT s, Enum_Scope_RemoteState& o) { SCP_FWD(s, getRemoteState(s, o)); }
ScopeError CScopeManager::setKeyLock(U32BIT s, bool v) { SCP_FWD(s, setKeyLock(s, v)); }
ScopeError CScopeManager::isKeyLocked(U32BIT s, bool& o) { SCP_FWD(s, isKeyLocked(s, o)); }
ScopeError CScopeManager::setBeeper(U32BIT s, bool v) { SCP_FWD(s, setBeeper(s, v)); }

/*============================================================================
 *  Debug
 *==========================================================================*/
ScopeError CScopeManager::writeScpi(U32BIT s, const QString& v) { SCP_FWD(s, writeScpi(s, v)); }
ScopeError CScopeManager::queryScpi(U32BIT s, const QString& v, QString& o) { SCP_FWD(s, queryScpi(s, v, o)); }
