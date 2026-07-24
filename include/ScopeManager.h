/*============================================================================
 *  ScopeManager.h
 *
 *  CScopeManager - the framework front end. Mirrors CELoadManager: a QObject
 *  singleton that discovers plugins with QPluginLoader, maps a logical scope
 *  number to a loaded plugin instance, and forwards every operation to the
 *  plugin that owns that scope number.
 *
 *  Usage:
 *      CScopeManager& mgr = CScopeManager::instance();
 *      mgr.loadPlugins("plugins");
 *      mgr.createInstance(1, "MDO34");
 *      S_Scope_ConnectionConfig cfg; cfg.setResourceString("USB0::0x0699::0x0522::C0::INSTR");
 *      mgr.connect(1, cfg);
 *      mgr.enableChannel(1, 1, true);
 *      mgr.setVerticalScale(1, 1, 0.5);
 *      mgr.single(1);
 *      S_Scope_Waveform wfm; mgr.readWaveform(1, 1, wfm);
 *
 *  \author  Scope framework
 *==========================================================================*/
#ifndef SCOPEMANAGER_H
#define SCOPEMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QPluginLoader>
#include <QVector>
#include <QByteArray>

#include "IScopePlugin.h"
#include "ScopeError.h"
#include "ScopeTypes.h"   // provides SCOPECORE_EXPORT

/* Live SCPI trace tap (optional). Direction: 0=TX 1=RX 2=ERR 3=INFO. */
typedef void (*ScopeTraceCallback)(void* in_pvUser, U32BIT in_u32ScopeNumber,
                                   int in_iDirection, const QString& in_strText);

class SCOPECORE_EXPORT CScopeManager : public QObject
{
    Q_OBJECT
public:
    CScopeManager();
    ~CScopeManager();

    static CScopeManager& instance();

    /*==== plugin management ==============================================*/
    void        loadPlugins(const QString& in_strPluginPath);
    QStringList getAvailablePlugins() const;
    S_Scope_PluginInfo   getPluginInfoByName(const QString& in_strPluginName) const;
    S_Scope_Capabilities getPluginCapabilities(const QString& in_strPluginName) const;
    QString     getCoreVersion() const { return QStringLiteral("1.0.0"); }

    /*==== instance management ============================================*/
    ScopeError createInstance(U32BIT in_u32ScopeNumber, const QString& in_strPluginName);
    ScopeError destroyInstance(U32BIT in_u32ScopeNumber);
    bool       instanceExists(U32BIT in_u32ScopeNumber) const;
    QString    getInstancePlugin(U32BIT in_u32ScopeNumber) const;
    S_Scope_Capabilities getCapabilities(U32BIT in_u32ScopeNumber) const;

    /*==== connection / core ==============================================*/
    ScopeError connect(U32BIT in_u32ScopeNumber, const S_Scope_ConnectionConfig& in_sConfig);
    ScopeError disconnect(U32BIT in_u32ScopeNumber);
    bool       isConnected(U32BIT in_u32ScopeNumber) const;
    ScopeError reset(U32BIT in_u32ScopeNumber);
    ScopeError setTimeout(U32BIT in_u32ScopeNumber, U32BIT in_u32TimeoutMs);
    ScopeError clearStatus(U32BIT in_u32ScopeNumber);
    ScopeError getIdentification(U32BIT in_u32ScopeNumber, QString& out_strIdn);
    ScopeError selfTest(U32BIT in_u32ScopeNumber, S32BIT& out_iResult);
    ScopeError getOptions(U32BIT in_u32ScopeNumber, QString& out_strOptions);
    ScopeError waitOperationComplete(U32BIT in_u32ScopeNumber);
    ScopeError getScpiVersion(U32BIT in_u32ScopeNumber, QString& out_strVersion);
    ScopeError getParameterRange(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                 Enum_Scope_ParamId in_eParam, S_Scope_ParameterRange& out_sRange);
    ScopeError autoscale(U32BIT in_u32ScopeNumber);
    ScopeError run(U32BIT in_u32ScopeNumber);
    ScopeError stop(U32BIT in_u32ScopeNumber);
    ScopeError single(U32BIT in_u32ScopeNumber);
    ScopeError forceTrigger(U32BIT in_u32ScopeNumber);

    /*==== vertical (per channel) =========================================*/
    ScopeError enableChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bOn);
    ScopeError isChannelEnabled(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool& out_bOn);
    ScopeError setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv);
    ScopeError getVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVoltsPerDiv);
    ScopeError setVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVolts);
    ScopeError getVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVolts);
    ScopeError setVerticalPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dDiv);
    ScopeError getVerticalPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dDiv);
    ScopeError setCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_Coupling in_eCoupling);
    ScopeError getCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_Coupling& out_eCoupling);
    ScopeError setBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_BandwidthLimit in_eLimit);
    ScopeError getBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_BandwidthLimit& out_eLimit);
    ScopeError setProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dRatio);
    ScopeError getProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dRatio);
    ScopeError setInputImpedance(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_InputImpedance in_eImp);
    ScopeError getInputImpedance(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_InputImpedance& out_eImp);
    ScopeError setInvert(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bOn);
    ScopeError getInvert(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool& out_bOn);
    ScopeError setChannelLabel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const QString& in_strLabel);
    ScopeError getChannelLabel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, QString& out_strLabel);
    ScopeError setChannelUnits(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const QString& in_strUnits);
    ScopeError getChannelUnits(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, QString& out_strUnits);
    ScopeError setDeskew(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dSeconds);
    ScopeError getDeskew(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dSeconds);

    /*==== horizontal / timebase ==========================================*/
    ScopeError setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv);
    ScopeError getTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSecondsPerDiv);
    ScopeError setTimebasePosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds);
    ScopeError getTimebasePosition(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSeconds);
    ScopeError setTimebaseReference(U32BIT in_u32ScopeNumber, FDOUBLE in_dPercent);
    ScopeError setTimebaseMode(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode in_eMode);
    ScopeError getTimebaseMode(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode& out_eMode);
    ScopeError getSampleRate(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSaPerSec);
    ScopeError setMemoryDepth(U32BIT in_u32ScopeNumber, U32BIT in_u32Points);
    ScopeError getMemoryDepth(U32BIT in_u32ScopeNumber, U32BIT& out_u32Points);
    ScopeError setAcquisitionPoints(U32BIT in_u32ScopeNumber, U32BIT in_u32Points);
    ScopeError getAcquisitionPoints(U32BIT in_u32ScopeNumber, U32BIT& out_u32Points);

    /*==== trigger ========================================================*/
    ScopeError setTriggerMode(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerMode in_eMode);
    ScopeError getTriggerMode(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerMode& out_eMode);
    ScopeError setTriggerType(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerType in_eType);
    ScopeError getTriggerType(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerType& out_eType);
    ScopeError setTriggerSource(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSource in_eSource);
    ScopeError getTriggerSource(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSource& out_eSource);
    ScopeError setTriggerSlope(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSlope in_eSlope);
    ScopeError getTriggerSlope(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSlope& out_eSlope);
    ScopeError setTriggerLevel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVolts);
    ScopeError getTriggerLevel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVolts);
    ScopeError setTriggerCoupling(U32BIT in_u32ScopeNumber, Enum_Scope_Coupling in_eCoupling);
    ScopeError getTriggerCoupling(U32BIT in_u32ScopeNumber, Enum_Scope_Coupling& out_eCoupling);
    ScopeError setTriggerHoldoff(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds);
    ScopeError getTriggerHoldoff(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSeconds);
    ScopeError getTriggerState(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerState& out_eState);
    ScopeError setTriggerPulseWidth(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds);
    ScopeError setTriggerVideoStandard(U32BIT in_u32ScopeNumber, const QString& in_strStandard);
    ScopeError setTriggerPattern(U32BIT in_u32ScopeNumber, const QString& in_strPattern);

    /*==== acquisition ====================================================*/
    ScopeError setAcqMode(U32BIT in_u32ScopeNumber, Enum_Scope_AcqMode in_eMode);
    ScopeError getAcqMode(U32BIT in_u32ScopeNumber, Enum_Scope_AcqMode& out_eMode);
    ScopeError setAverageCount(U32BIT in_u32ScopeNumber, U32BIT in_u32Count);
    ScopeError getAverageCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Count);
    ScopeError getAcquisitionState(U32BIT in_u32ScopeNumber, Enum_Scope_AcqState& out_eState);
    ScopeError setSegmentedCount(U32BIT in_u32ScopeNumber, U32BIT in_u32Segments);
    ScopeError getSegmentedCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Segments);

    /*==== waveform transfer ==============================================*/
    ScopeError setWaveformSource(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformSource in_eSource, U32BIT in_u32Index);
    ScopeError getWaveformSource(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformSource& out_eSource, U32BIT& out_u32Index);
    ScopeError setWaveformFormat(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformFormat in_eFormat);
    ScopeError getWaveformFormat(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformFormat& out_eFormat);
    ScopeError setWaveformPoints(U32BIT in_u32ScopeNumber, U32BIT in_u32Points);
    ScopeError getWaveformPreamble(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Scope_WaveformPreamble& out_sPreamble);
    ScopeError readWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Scope_Waveform& out_sWaveform);
    ScopeError digitizeChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);

    /*==== automatic measurements =========================================*/
    ScopeError addMeasurement(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_MeasType in_eType);
    ScopeError readMeasurement(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_MeasType in_eType, S_Scope_MeasurementResult& out_sResult);
    ScopeError clearMeasurements(U32BIT in_u32ScopeNumber);
    ScopeError setMeasureStatistics(U32BIT in_u32ScopeNumber, bool in_bOn);
    ScopeError getMeasurementStatistics(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_MeasType in_eType, S_Scope_MeasurementResult& out_sResult);

    /*==== math / FFT =====================================================*/
    ScopeError setMathOperation(U32BIT in_u32ScopeNumber, Enum_Scope_MathOp in_eOp);
    ScopeError setMathSource1(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);
    ScopeError setMathSource2(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);
    ScopeError enableMath(U32BIT in_u32ScopeNumber, bool in_bOn);
    ScopeError setFftWindow(U32BIT in_u32ScopeNumber, Enum_Scope_FftWindow in_eWindow);
    ScopeError getFftWindow(U32BIT in_u32ScopeNumber, Enum_Scope_FftWindow& out_eWindow);
    ScopeError setFftSpan(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz);
    ScopeError setFftCenter(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz);
    ScopeError setMathScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dScale);
    ScopeError setMathPosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dPosition);

    /*==== cursors ========================================================*/
    ScopeError setCursorType(U32BIT in_u32ScopeNumber, Enum_Scope_CursorType in_eType);
    ScopeError getCursorType(U32BIT in_u32ScopeNumber, Enum_Scope_CursorType& out_eType);
    ScopeError setCursorSource(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);
    ScopeError setCursorPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32CursorIndex, FDOUBLE in_dPosition);
    ScopeError getCursorPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32CursorIndex, FDOUBLE& out_dPosition);
    ScopeError readCursorValues(U32BIT in_u32ScopeNumber, FDOUBLE& out_dX1, FDOUBLE& out_dX2, FDOUBLE& out_dY1, FDOUBLE& out_dY2);

    /*==== display ========================================================*/
    ScopeError setPersistence(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds);
    ScopeError setGraticule(U32BIT in_u32ScopeNumber, const QString& in_strType);
    ScopeError setIntensity(U32BIT in_u32ScopeNumber, FDOUBLE in_dPercent);
    ScopeError setDisplayFormat(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode in_eFormat);
    ScopeError setVectors(U32BIT in_u32ScopeNumber, bool in_bOn);

    /*==== save / recall / screenshot =====================================*/
    ScopeError saveSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location);
    ScopeError recallSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location);
    ScopeError saveWaveformToFile(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const QString& in_strPath);
    ScopeError captureScreenshot(U32BIT in_u32ScopeNumber, Enum_Scope_ImageFormat in_eFormat, QByteArray& out_imageBytes);
    ScopeError saveToReference(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, U32BIT in_u32RefSlot);
    ScopeError displayReference(U32BIT in_u32ScopeNumber, U32BIT in_u32RefSlot, bool in_bOn);

    /*==== digital / MSO ==================================================*/
    ScopeError enableDigitalChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel, bool in_bOn);
    ScopeError setDigitalThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel, FDOUBLE in_dVolts);
    ScopeError setPodThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32Pod, FDOUBLE in_dVolts);
    ScopeError enableBus(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, bool in_bOn);
    ScopeError setBusType(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, const QString& in_strType);
    ScopeError readBusDecode(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, QString& out_strDecode);

    /*==== AWG / Wavegen ==================================================*/
    ScopeError setAwgFunction(U32BIT in_u32ScopeNumber, const QString& in_strFunction);
    ScopeError setAwgFrequency(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz);
    ScopeError setAwgAmplitude(U32BIT in_u32ScopeNumber, FDOUBLE in_dVolts);
    ScopeError setAwgOffset(U32BIT in_u32ScopeNumber, FDOUBLE in_dVolts);
    ScopeError enableAwgOutput(U32BIT in_u32ScopeNumber, bool in_bOn);

    /*==== status / system ================================================*/
    ScopeError readErrorStatus(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Scope_DeviceErrorStatus& out_sStatus);
    ScopeError clearErrorStatus(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);
    ScopeError queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_strMessage);
    ScopeError getInstrumentErrorCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Count);
    ScopeError readStandardEventStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status);
    ScopeError readStatusByte(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status);
    ScopeError readOperationStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status);
    ScopeError readQuestionableStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status);
    ScopeError setRemoteState(U32BIT in_u32ScopeNumber, Enum_Scope_RemoteState in_eState);
    ScopeError getRemoteState(U32BIT in_u32ScopeNumber, Enum_Scope_RemoteState& out_eState);
    ScopeError setKeyLock(U32BIT in_u32ScopeNumber, bool in_bOn);
    ScopeError isKeyLocked(U32BIT in_u32ScopeNumber, bool& out_bOn);
    ScopeError setBeeper(U32BIT in_u32ScopeNumber, bool in_bOn);

    /*==== debug ==========================================================*/
    ScopeError writeScpi(U32BIT in_u32ScopeNumber, const QString& in_strCommand);
    ScopeError queryScpi(U32BIT in_u32ScopeNumber, const QString& in_strCommand, QString& out_strResponse);

    /*==== trace tap ======================================================*/
    void setTraceCallback(ScopeTraceCallback in_pfnCallback, void* in_pvUser);

signals:
    void pluginLoaded(const QString& in_strPluginName);
    void pluginLoadFailed(const QString& in_strFileName, const QString& in_strError);
    void instanceCreated(U32BIT in_u32ScopeNumber, const QString& in_strPluginName);
    void instanceDestroyed(U32BIT in_u32ScopeNumber);
    void errorOccurred(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const S_Scope_DeviceErrorStatus& in_sStatus);

private:
    CScopeManager(const CScopeManager&) = delete;
    CScopeManager& operator=(const CScopeManager&) = delete;

    CIScopePlugin* getPlugin(U32BIT in_u32ScopeNumber) const;

    struct S_PluginData
    {
        QPluginLoader*       m_pLoader;
        CIScopePlugin*       m_pPlugin;
        S_Scope_PluginInfo   m_sInfo;
        S_Scope_Capabilities m_sCaps;
        S_PluginData() : m_pLoader(nullptr), m_pPlugin(nullptr) {}
    };

    QMap<QString, S_PluginData> m_mapPlugins;    // plugin name -> data
    QMap<U32BIT, QString>       m_mapInstances;  // scope number -> plugin name
    mutable QMutex              m_mutex;         // guards the maps + per-scope calls
    ScopeTraceCallback          m_pfnTrace;
    void*                       m_pvTraceUser;
};

#endif // SCOPEMANAGER_H
