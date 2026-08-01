/**
 * @file    SimScopePlugin.cpp
 * @brief   SimScope virtual model implementation - synthesized waveforms & measurements.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "SimScopePlugin.h"
#include "VisaHelper.h" // inline S_Scope_ConnectionConfig::toVisaResourceString()

#include <QByteArray>
#include <QStringList>
#include <cmath>

namespace
{
const double PI = 3.14159265358979323846;

/* value at fractional phase [0,1) for a given shape, 0-peak amplitude 1.0 */
double shapeValue(int in_eShape, double in_dPhase)
{
    switch (in_eShape)
    {
    case 0: /* SINE   */
        return std::sin(2.0 * PI * in_dPhase);
    case 1: /* SQUARE */
        return (in_dPhase < 0.5) ? 1.0 : -1.0;
    case 2: /* RAMP   */
        return 2.0 * in_dPhase - 1.0;
    case 3: /* NOISE  */
    {
        /* deterministic pseudo-noise from the phase so measurements repeat */
        const double x = std::sin(in_dPhase * 12.9898) * 43758.5453;
        return 2.0 * (x - std::floor(x)) - 1.0;
    }
    default:
        return 0.0;
    }
}
} // namespace

/*============================================================================
 *  Construction / info / capabilities
 *==========================================================================*/
CSimScopePlugin::CSimScopePlugin()
{
}
CSimScopePlugin::~CSimScopePlugin()
{
    m_devices.clear();
}

S_Scope_PluginInfo CSimScopePlugin::getPluginInfo() const
{
    S_Scope_PluginInfo s;
    qstrncpy(s.m_szName, "SimScope", PLUGIN_INFO_NAME_SIZE);
    qstrncpy(s.m_szVersion, "1.0.0", PLUGIN_INFO_VERSION_SIZE);
    qstrncpy(s.m_szManufacturer, "Scope Framework", PLUGIN_INFO_MANUFACTURER_SIZE);
    qstrncpy(s.m_szModelName, "SimScope", PLUGIN_INFO_MODEL_NAME_SIZE);
    qstrncpy(s.m_szSeries, "Virtual", PLUGIN_INFO_SERIES_SIZE);
    qstrncpy(s.m_szDescription, "Virtual oscilloscope (synthesized waveforms, no hardware)",
             PLUGIN_INFO_DESCRIPTION_SIZE);
    qstrncpy(s.m_szMinCoreVersion, "1.0.0", PLUGIN_INFO_MIN_CORE_VERSION);
    qstrncpy(s.m_szMaxCoreVersion, "2.0.0", PLUGIN_INFO_MAX_CORE_VERSION);
    s.m_StrlstSupportedProtocols << "SIM";
    s.m_StrlstSupportedModes << "SINE" << "SQUARE" << "RAMP" << "NOISE";
    return s;
}

S_Scope_Capabilities CSimScopePlugin::getCapabilities() const
{
    S_Scope_Capabilities caps;
    caps.m_u32NumberOfChannels = 4;
    caps.m_dBandwidthHz = 1.0e9;
    caps.m_dMaxSampleRate = 5.0e9;
    caps.m_u32MaxMemoryDepth = 10000000u;
    caps.m_dMinTimebaseScale = 1.0e-9;
    caps.m_dMaxTimebaseScale = 1000.0;
    caps.m_bHasFFT = true;
    caps.m_bHasAWG = true;
    for (U32BIT c = 1; c <= caps.m_u32NumberOfChannels; ++c)
    {
        S_Scope_ChannelCapabilities ch;
        ch.m_u32ChannelNumber = c;
        ch.m_dBandwidthHz = caps.m_dBandwidthHz;
        ch.m_dMaxVerticalScale = 10.0;
        ch.m_dMinVerticalScale = 1.0e-3;
        ch.m_bHas50Ohm = true;
        ch.m_bHasBandwidthLimit = true;
        caps.m_QlistChannels.append(ch);
    }
    return caps;
}

/*============================================================================
 *  Instance helpers
 *==========================================================================*/
CSimScopePlugin::S_SimDevice* CSimScopePlugin::dev(U32BIT s)
{
    auto it = m_devices.find(s);
    return (it == m_devices.end()) ? nullptr : &it.value();
}
const CSimScopePlugin::S_SimDevice* CSimScopePlugin::dev(U32BIT s) const
{
    auto it = m_devices.find(s);
    return (it == m_devices.end()) ? nullptr : &it.value();
}
bool CSimScopePlugin::validChannel(const S_SimDevice* d, U32BIT c) const
{
    return d != nullptr && d->m_pLimits != nullptr && c >= 1 &&
           static_cast<int>(c) <= d->m_pLimits->m_iAnalogChannels;
}

/*============================================================================
 *  Connection
 *==========================================================================*/
ScopeError CSimScopePlugin::connect(U32BIT s, const S_Scope_ConnectionConfig& c)
{
    if (dev(s) != nullptr && m_devices[s].m_bConnected)
    {
        return ScopeError(Enum_Scope_ErrorCode::ALREADY_CONNECTED);
    }
    const QString strRes = c.toVisaResourceString().toUpper();
    const QStringList lstTok = strRes.split(QStringLiteral("::"), Qt::SkipEmptyParts);

    S_SimDevice d;
    d.m_u32Timeout = c.m_u32Timeout ? c.m_u32Timeout : 5000;

    // model token: the first token that matches a catalog name (case-insensitive;
    // the resource was upper-cased, but catalog names may be mixed case)
    const S_ScopeLimits* pLimits = ScopeFindLimits("MDO34");
    QString strModel = QStringLiteral("MDO34");
    int iCatCount = 0;
    const S_ScopeLimits* cat = ScopeLimitsCatalog(&iCatCount);
    bool bFound = false;
    for (const QString& t : lstTok)
    {
        for (int i = 0; i < iCatCount && !bFound; ++i)
        {
            const QString name = QString::fromLatin1(cat[i].m_szModelName);
            if (QString::compare(t, name, Qt::CaseInsensitive) == 0)
            {
                pLimits = &cat[i];
                strModel = name;
                bFound = true;
            }
        }
        if (bFound)
        {
            break;
        }
    }
    // shape token
    d.m_eShape = SHAPE_SINE;
    if (strRes.contains(QStringLiteral("SQUARE")))
    {
        d.m_eShape = SHAPE_SQUARE;
    }
    else if (strRes.contains(QStringLiteral("RAMP")))
    {
        d.m_eShape = SHAPE_RAMP;
    }
    else if (strRes.contains(QStringLiteral("NOISE")))
    {
        d.m_eShape = SHAPE_NOISE;
    }

    d.m_pLimits = pLimits;
    d.m_strModel = strModel;
    d.m_u32MemDepth = pLimits->m_u32MaxMemoryDepth > 1000 ? 1000u : pLimits->m_u32MaxMemoryDepth;

    const int n = pLimits->m_iAnalogChannels;
    for (int i = 0; i < n; ++i)
    {
        d.m_vChEnabled.append(i == 0); // ch1 on by default
        d.m_vVertScale.append(0.1);    // 100 mV/div
        d.m_vVertOffset.append(0.0);
        d.m_vCoupling.append(Enum_Scope_Coupling::m_enumDC);
        d.m_vProbeAtten.append(10.0);
        d.m_vBwLimit.append(Enum_Scope_BandwidthLimit::m_enumFull);
        d.m_vVertPosition.append(0.0);
        d.m_vImpedance.append(Enum_Scope_InputImpedance::m_enum1M);
        d.m_vInvert.append(false);
        d.m_vLabel.append(QStringLiteral("CH%1").arg(i + 1));
        d.m_vUnits.append(QStringLiteral("V"));
        d.m_vDeskew.append(0.0);
    }
    d.m_bConnected = true;
    m_devices[s] = d;
    return ScopeError();
}

ScopeError CSimScopePlugin::disconnect(U32BIT s)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    m_devices.remove(s);
    return ScopeError();
}

bool CSimScopePlugin::isConnected(U32BIT s) const
{
    const S_SimDevice* d = dev(s);
    return d != nullptr && d->m_bConnected;
}

ScopeError CSimScopePlugin::reset(U32BIT s)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    const S_ScopeLimits* p = d->m_pLimits;
    const QString model = d->m_strModel;
    const EWaveShape shape = d->m_eShape;
    const U32BIT tmo = d->m_u32Timeout;
    S_Scope_ConnectionConfig cfg;
    cfg.setResourceString(QStringLiteral("SIM::%1").arg(model));
    cfg.m_u32Timeout = tmo;
    m_devices.remove(s);
    ScopeError e = connect(s, cfg);
    if (e.isSuccess())
    {
        m_devices[s].m_eShape = shape;
        m_devices[s].m_pLimits = p;
    }
    return e;
}

/*============================================================================
 *  Core
 *==========================================================================*/
ScopeError CSimScopePlugin::setTimeout(U32BIT s, U32BIT t)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_u32Timeout = t;
    return ScopeError();
}

ScopeError CSimScopePlugin::getIdentification(U32BIT s, QString& o)
{
    const S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = QStringLiteral("%1,%2,SIM000000,1.0.0")
            .arg(QString::fromLatin1(d->m_pLimits->m_szManufacturer))
            .arg(d->m_strModel);
    return ScopeError();
}

ScopeError CSimScopePlugin::selfTest(U32BIT s, S32BIT& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = 0;
    return ScopeError();
}

ScopeError CSimScopePlugin::getScpiVersion(U32BIT s, QString& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = QStringLiteral("1999.0");
    return ScopeError();
}

ScopeError CSimScopePlugin::getParameterRange(U32BIT s, U32BIT c, Enum_Scope_ParamId p,
                                              S_Scope_ParameterRange& o)
{
    const S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    const S_ScopeLimits* L = d->m_pLimits;
    o = S_Scope_ParameterRange();
    switch (p)
    {
    case Enum_Scope_ParamId::m_enumVerticalScale:
        o.m_dMin = L->m_dVertScaleMin;
        o.m_dMax = L->m_dVertScaleMax;
        o.m_dResolution = 1.0e-3;
        break;
    case Enum_Scope_ParamId::m_enumVerticalOffset:
    case Enum_Scope_ParamId::m_enumVerticalPosition:
        o.m_dMin = -L->m_dVertOffsetMax;
        o.m_dMax = L->m_dVertOffsetMax;
        o.m_dResolution = 1.0e-3;
        break;
    case Enum_Scope_ParamId::m_enumTimebaseScale:
        o.m_dMin = L->m_dTimebaseMin;
        o.m_dMax = L->m_dTimebaseMax;
        o.m_dResolution = 1.0e-12;
        break;
    case Enum_Scope_ParamId::m_enumTimebasePosition:
        o.m_dMin = -L->m_dTimebaseMax * 10.0;
        o.m_dMax = L->m_dTimebaseMax * 10.0;
        break;
    case Enum_Scope_ParamId::m_enumTriggerLevel:
        o.m_dMin = -L->m_dVertOffsetMax;
        o.m_dMax = L->m_dVertOffsetMax;
        o.m_dResolution = 1.0e-3;
        break;
    case Enum_Scope_ParamId::m_enumTriggerHoldoff:
        o.m_dMin = L->m_dTrigHoldoffMin;
        o.m_dMax = L->m_dTrigHoldoffMax;
        break;
    case Enum_Scope_ParamId::m_enumProbeAttenuation:
        for (int i = 0; i < ScopeShared::PROBE_ATTEN_COUNT; ++i)
        {
            o.m_QlistDiscreteValues.append(ScopeShared::PROBE_ATTEN_VALUES[i]);
        }
        o.m_dMin = ScopeShared::PROBE_ATTEN_VALUES[0];
        o.m_dMax = ScopeShared::PROBE_ATTEN_VALUES[ScopeShared::PROBE_ATTEN_COUNT - 1];
        break;
    case Enum_Scope_ParamId::m_enumSampleRate:
        o.m_dMin = 1.0;
        o.m_dMax = L->m_dMaxSampleRate;
        break;
    case Enum_Scope_ParamId::m_enumMemoryDepth:
        o.m_dMin = 1.0;
        o.m_dMax = static_cast<double>(L->m_u32MaxMemoryDepth);
        break;
    case Enum_Scope_ParamId::m_enumAverageCount:
        o.m_dMin = 1.0;
        o.m_dMax = static_cast<double>(L->m_iAvgCountMax);
        break;
    case Enum_Scope_ParamId::m_enumWaveformPoints:
        for (int i = 0; i < ScopeShared::WAVEFORM_POINTS_COUNT; ++i)
        {
            o.m_QlistDiscreteValues.append(ScopeShared::WAVEFORM_POINTS_VALUES[i]);
        }
        o.m_dMin = ScopeShared::WAVEFORM_POINTS_VALUES[0];
        o.m_dMax = static_cast<double>(L->m_u32MaxMemoryDepth);
        break;
    case Enum_Scope_ParamId::m_enumAwgFrequency:
        if (!L->m_bHasAWG)
        {
            return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
        }
        o.m_dMin = 0.1;
        o.m_dMax = L->m_dAwgFreqMax;
        break;
    case Enum_Scope_ParamId::m_enumAwgAmplitude:
        if (!L->m_bHasAWG)
        {
            return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
        }
        o.m_dMin = 0.0;
        o.m_dMax = L->m_dAwgAmplMax;
        break;
    case Enum_Scope_ParamId::m_enumMeasureThreshold:
        o.m_dMin = ScopeShared::MEASURE_THRESHOLD_MIN;
        o.m_dMax = ScopeShared::MEASURE_THRESHOLD_MAX;
        break;
    default:
        return ScopeError(Enum_Scope_ErrorCode::INVALID_PARAMETER);
    }
    (void)c;
    return ScopeError();
}

ScopeError CSimScopePlugin::autoscale(U32BIT s)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    for (int i = 0; i < d->m_vVertScale.size(); ++i)
    {
        d->m_vVertScale[i] = 0.1;
        d->m_vVertOffset[i] = 0.0;
    }
    d->m_dTimebase = 1.0e-4;
    d->m_eTrigMode = Enum_Scope_TriggerMode::m_enumAuto;
    return ScopeError();
}

ScopeError CSimScopePlugin::run(U32BIT s)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eAcqState = Enum_Scope_AcqState::m_enumRunning;
    return ScopeError();
}
ScopeError CSimScopePlugin::stop(U32BIT s)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eAcqState = Enum_Scope_AcqState::m_enumStopped;
    return ScopeError();
}
ScopeError CSimScopePlugin::single(U32BIT s)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eAcqState = Enum_Scope_AcqState::m_enumComplete;
    return ScopeError();
}
ScopeError CSimScopePlugin::forceTrigger(U32BIT s)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eAcqState = Enum_Scope_AcqState::m_enumComplete;
    return ScopeError();
}

/*============================================================================
 *  Vertical
 *==========================================================================*/
ScopeError CSimScopePlugin::enableChannel(U32BIT s, U32BIT c, bool v)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL,
                          QStringLiteral("channel %1 not present on %2").arg(c).arg(d->m_strModel));
    }
    d->m_vChEnabled[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::isChannelEnabled(U32BIT s, U32BIT c, bool& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = d->m_vChEnabled[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setVerticalScale(U32BIT s, U32BIT c, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    if (v < d->m_pLimits->m_dVertScaleMin || v > d->m_pLimits->m_dVertScaleMax)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("vertical scale %1 out of range [%2 .. %3]")
                              .arg(v)
                              .arg(d->m_pLimits->m_dVertScaleMin)
                              .arg(d->m_pLimits->m_dVertScaleMax));
    }
    d->m_vVertScale[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getVerticalScale(U32BIT s, U32BIT c, FDOUBLE& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = d->m_vVertScale[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setVerticalOffset(U32BIT s, U32BIT c, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    if (std::fabs(v) > d->m_pLimits->m_dVertOffsetMax)
    {
        return ScopeError(
            Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
            QStringLiteral("vertical offset %1 exceeds +/-%2").arg(v).arg(d->m_pLimits->m_dVertOffsetMax));
    }
    d->m_vVertOffset[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getVerticalOffset(U32BIT s, U32BIT c, FDOUBLE& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = d->m_vVertOffset[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling v)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    d->m_vCoupling[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = d->m_vCoupling[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    bool found = false;
    for (int i = 0; i < ScopeShared::PROBE_ATTEN_COUNT; ++i)
    {
        if (std::fabs(v - ScopeShared::PROBE_ATTEN_VALUES[i]) < 1e-9)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("probe attenuation %1 is not a valid ratio").arg(v));
    }
    d->m_vProbeAtten[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = d->m_vProbeAtten[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setBandwidthLimit(U32BIT s, U32BIT c, Enum_Scope_BandwidthLimit v)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    d->m_vBwLimit[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getBandwidthLimit(U32BIT s, U32BIT c, Enum_Scope_BandwidthLimit& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = d->m_vBwLimit[c - 1];
    return ScopeError();
}

/*============================================================================
 *  Horizontal
 *==========================================================================*/
ScopeError CSimScopePlugin::setTimebaseScale(U32BIT s, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (v < d->m_pLimits->m_dTimebaseMin || v > d->m_pLimits->m_dTimebaseMax)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("timebase %1 s/div out of range [%2 .. %3]")
                              .arg(v)
                              .arg(d->m_pLimits->m_dTimebaseMin)
                              .arg(d->m_pLimits->m_dTimebaseMax));
    }
    d->m_dTimebase = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTimebaseScale(U32BIT s, FDOUBLE& o)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_dTimebase;
    return ScopeError();
}
ScopeError CSimScopePlugin::setTimebasePosition(U32BIT s, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_dTimebasePos = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTimebasePosition(U32BIT s, FDOUBLE& o)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_dTimebasePos;
    return ScopeError();
}
ScopeError CSimScopePlugin::getSampleRate(U32BIT s, FDOUBLE& o)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    const double totalTime = ScopeShared::GRATICULE_HDIV * d->m_dTimebase;
    o = (totalTime > 0.0) ? (static_cast<double>(d->m_iWfmPoints) / totalTime) : 0.0;
    if (o > d->m_pLimits->m_dMaxSampleRate)
    {
        o = d->m_pLimits->m_dMaxSampleRate;
    }
    return ScopeError();
}
ScopeError CSimScopePlugin::setMemoryDepth(U32BIT s, U32BIT v)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (v < 1 || v > d->m_pLimits->m_u32MaxMemoryDepth)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("memory depth %1 out of range [1 .. %2]")
                              .arg(v)
                              .arg(d->m_pLimits->m_u32MaxMemoryDepth));
    }
    d->m_u32MemDepth = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getMemoryDepth(U32BIT s, U32BIT& o)
{
    S_SimDevice* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_u32MemDepth;
    return ScopeError();
}

/*============================================================================
 *  Trigger
 *==========================================================================*/
ScopeError CSimScopePlugin::setTriggerMode(U32BIT s, Enum_Scope_TriggerMode v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eTrigMode = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTriggerMode(U32BIT s, Enum_Scope_TriggerMode& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eTrigMode;
    return ScopeError();
}
ScopeError CSimScopePlugin::setTriggerType(U32BIT s, Enum_Scope_TriggerType v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eTrigType = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTriggerType(U32BIT s, Enum_Scope_TriggerType& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eTrigType;
    return ScopeError();
}
ScopeError CSimScopePlugin::setTriggerSource(U32BIT s, Enum_Scope_TriggerSource v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eTrigSource = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTriggerSource(U32BIT s, Enum_Scope_TriggerSource& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eTrigSource;
    return ScopeError();
}
ScopeError CSimScopePlugin::setTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eTrigSlope = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eTrigSlope;
    return ScopeError();
}
ScopeError CSimScopePlugin::setTriggerLevel(U32BIT s, U32BIT c, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    if (std::fabs(v) > d->m_pLimits->m_dVertOffsetMax)
    {
        return ScopeError(
            Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
            QStringLiteral("trigger level %1 exceeds +/-%2").arg(v).arg(d->m_pLimits->m_dVertOffsetMax));
    }
    d->m_dTrigLevel = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTriggerLevel(U32BIT s, U32BIT c, FDOUBLE& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = d->m_dTrigLevel;
    return ScopeError();
}
ScopeError CSimScopePlugin::setTriggerHoldoff(U32BIT s, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (v < d->m_pLimits->m_dTrigHoldoffMin || v > d->m_pLimits->m_dTrigHoldoffMax)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("holdoff %1 out of range").arg(v));
    }
    d->m_dTrigHoldoff = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTriggerHoldoff(U32BIT s, FDOUBLE& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_dTrigHoldoff;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTriggerState(U32BIT s, Enum_Scope_TriggerState& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = (d->m_eAcqState == Enum_Scope_AcqState::m_enumRunning) ? Enum_Scope_TriggerState::m_enumAuto
                                                               : Enum_Scope_TriggerState::m_enumTriggered;
    return ScopeError();
}

/*============================================================================
 *  Acquisition
 *==========================================================================*/
ScopeError CSimScopePlugin::setAcqMode(U32BIT s, Enum_Scope_AcqMode v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eAcqMode = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getAcqMode(U32BIT s, Enum_Scope_AcqMode& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eAcqMode;
    return ScopeError();
}
ScopeError CSimScopePlugin::setAverageCount(U32BIT s, U32BIT v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (v < 1 || static_cast<int>(v) > d->m_pLimits->m_iAvgCountMax)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("average count %1 out of range [1 .. %2]")
                              .arg(v)
                              .arg(d->m_pLimits->m_iAvgCountMax));
    }
    d->m_iAvgCount = static_cast<int>(v);
    return ScopeError();
}
ScopeError CSimScopePlugin::getAverageCount(U32BIT s, U32BIT& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = static_cast<U32BIT>(d->m_iAvgCount);
    return ScopeError();
}
ScopeError CSimScopePlugin::getAcquisitionState(U32BIT s, Enum_Scope_AcqState& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eAcqState;
    return ScopeError();
}

/*============================================================================
 *  Waveform
 *==========================================================================*/
ScopeError CSimScopePlugin::setWaveformSource(U32BIT s, Enum_Scope_WaveformSource v, U32BIT i)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eWfmSource = v;
    d->m_iWfmSourceIndex = static_cast<int>(i);
    return ScopeError();
}
ScopeError CSimScopePlugin::getWaveformSource(U32BIT s, Enum_Scope_WaveformSource& o, U32BIT& i)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eWfmSource;
    i = static_cast<U32BIT>(d->m_iWfmSourceIndex);
    return ScopeError();
}
ScopeError CSimScopePlugin::setWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eWfmFormat = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eWfmFormat;
    return ScopeError();
}
ScopeError CSimScopePlugin::setWaveformPoints(U32BIT s, U32BIT v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (v < 1 || v > d->m_pLimits->m_u32MaxMemoryDepth)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("waveform points %1 out of range").arg(v));
    }
    d->m_iWfmPoints = static_cast<int>(v);
    return ScopeError();
}

void CSimScopePlugin::synthesize(const S_SimDevice* d, U32BIT c, QVector<FDOUBLE>& out_time,
                                 QVector<FDOUBLE>& out_volts) const
{
    const int n = d->m_iWfmPoints;
    const double totalTime = ScopeShared::GRATICULE_HDIV * d->m_dTimebase;
    const double xInc = (n > 1) ? totalTime / (n - 1) : totalTime;
    const double xOrigin = -totalTime / 2.0 + d->m_dTimebasePos;
    const double vScale = d->m_vVertScale.value(c - 1, 0.1);
    const double vOff = d->m_vVertOffset.value(c - 1, 0.0);
    const double amp = 2.5 * vScale;                        // 0-peak; Vpp = 5 * V/div
    const double cycles = 3.0 + static_cast<double>(c - 1); // distinct per channel

    out_time.resize(n);
    out_volts.resize(n);
    for (int i = 0; i < n; ++i)
    {
        const double phase = std::fmod(cycles * static_cast<double>(i) / static_cast<double>(n), 1.0);
        double v = amp * shapeValue(static_cast<int>(d->m_eShape), phase) + vOff;
        if (d->m_vCoupling.value(c - 1) == Enum_Scope_Coupling::m_enumGND)
        {
            v = 0.0;
        }
        out_time[i] = xOrigin + xInc * i;
        out_volts[i] = v;
    }
}

ScopeError CSimScopePlugin::getWaveformPreamble(U32BIT s, U32BIT c, S_Scope_WaveformPreamble& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    const int n = d->m_iWfmPoints;
    const double totalTime = ScopeShared::GRATICULE_HDIV * d->m_dTimebase;
    o.m_iFormat = (d->m_eWfmFormat == Enum_Scope_WaveformFormat::m_enumByte)   ? 0
                  : (d->m_eWfmFormat == Enum_Scope_WaveformFormat::m_enumWord) ? 1
                                                                               : 4;
    o.m_iType = (d->m_eAcqMode == Enum_Scope_AcqMode::m_enumAverage) ? 2 : 0;
    o.m_u32Points = static_cast<U32BIT>(n);
    o.m_u32Count = 1;
    o.m_dXIncrement = (n > 1) ? totalTime / (n - 1) : totalTime;
    o.m_dXOrigin = -totalTime / 2.0 + d->m_dTimebasePos;
    o.m_dXReference = 0.0;
    const double vScale = d->m_vVertScale.value(c - 1, 0.1);
    o.m_dYIncrement = (vScale * ScopeShared::GRATICULE_VDIV) / 65536.0; // WORD full-scale
    o.m_dYOrigin = d->m_vVertOffset.value(c - 1, 0.0);
    o.m_dYReference = 32768.0;
    return ScopeError();
}

ScopeError CSimScopePlugin::readWaveform(U32BIT s, U32BIT c, S_Scope_Waveform& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    if (!d->m_vChEnabled.value(c - 1))
    {
        return ScopeError(Enum_Scope_ErrorCode::ACQUISITION_ERROR,
                          QStringLiteral("channel %1 is not enabled").arg(c));
    }
    getWaveformPreamble(s, c, o.m_sPreamble);
    o.m_u32SourceChannel = c;
    synthesize(d, c, o.m_vecTimeSeconds, o.m_vecVolts);
    return ScopeError();
}

ScopeError CSimScopePlugin::digitizeChannel(U32BIT s, U32BIT c)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    d->m_eAcqState = Enum_Scope_AcqState::m_enumComplete;
    return ScopeError();
}

/*============================================================================
 *  Measurements (computed from the synthesized buffer)
 *==========================================================================*/
double CSimScopePlugin::computeMeasurement(const S_SimDevice* d, U32BIT c, Enum_Scope_MeasType t,
                                           bool& out_bValid) const
{
    out_bValid = true;
    QVector<FDOUBLE> time, volts;
    synthesize(d, c, time, volts);
    const int n = volts.size();
    if (n < 2)
    {
        out_bValid = false;
        return 0.0;
    }

    double vmax = volts[0], vmin = volts[0], sum = 0.0, sumSq = 0.0;
    for (int i = 0; i < n; ++i)
    {
        vmax = std::max(vmax, volts[i]);
        vmin = std::min(vmin, volts[i]);
        sum += volts[i];
        sumSq += volts[i] * volts[i];
    }
    const double vavg = sum / n;
    const double vrms = std::sqrt(sumSq / n);
    const double xInc = (n > 1) ? (time.last() - time.first()) / (n - 1) : 0.0;

    // rising-edge crossings of the mean; period = mean spacing between them
    // (using the spacing avoids the boundary off-by-one of a raw cycle count)
    int iFirst = -1, iLast = -1, cross = 0;
    for (int i = 1; i < n; ++i)
    {
        if (volts[i - 1] < vavg && volts[i] >= vavg)
        {
            if (iFirst < 0)
            {
                iFirst = i;
            }
            iLast = i;
            ++cross;
        }
    }
    double period = 0.0;
    if (cross >= 2 && iLast > iFirst)
    {
        period = (static_cast<double>(iLast - iFirst) / (cross - 1)) * xInc;
    }
    const double freq = (period > 0.0) ? 1.0 / period : 0.0;

    switch (t)
    {
    case Enum_Scope_MeasType::m_enumVpp:
        return vmax - vmin;
    case Enum_Scope_MeasType::m_enumVmax:
        return vmax;
    case Enum_Scope_MeasType::m_enumVmin:
        return vmin;
    case Enum_Scope_MeasType::m_enumVrms:
        return vrms;
    case Enum_Scope_MeasType::m_enumVavg:
        return vavg;
    case Enum_Scope_MeasType::m_enumFrequency:
        return freq;
    case Enum_Scope_MeasType::m_enumPeriod:
        return period;
    case Enum_Scope_MeasType::m_enumRiseTime:
        return period * 0.1;
    case Enum_Scope_MeasType::m_enumFallTime:
        return period * 0.1;
    case Enum_Scope_MeasType::m_enumPosWidth:
        return period * 0.5;
    case Enum_Scope_MeasType::m_enumNegWidth:
        return period * 0.5;
    case Enum_Scope_MeasType::m_enumDutyCycle:
        return 50.0;
    case Enum_Scope_MeasType::m_enumOvershoot:
        return 0.0;
    case Enum_Scope_MeasType::m_enumPhase:
        return 0.0;
    case Enum_Scope_MeasType::m_enumDelay:
        return 0.0;
    default:
        out_bValid = false;
        return 0.0;
    }
}

ScopeError CSimScopePlugin::addMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return ScopeError();
}
ScopeError CSimScopePlugin::readMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t,
                                            S_Scope_MeasurementResult& o)
{
    S_SimDevice* d = dev(s);
    if (!validChannel(d, c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    bool valid = false;
    o.m_eType = t;
    o.m_dValue = computeMeasurement(d, c, t, valid);
    o.m_bValid = valid;
    o.m_dMean = o.m_dValue;
    o.m_dMin = o.m_dValue;
    o.m_dMax = o.m_dValue;
    o.m_dStdDev = 0.0;
    switch (t)
    {
    case Enum_Scope_MeasType::m_enumFrequency:
        o.m_strUnits = QStringLiteral("Hz");
        break;
    case Enum_Scope_MeasType::m_enumPeriod:
    case Enum_Scope_MeasType::m_enumRiseTime:
    case Enum_Scope_MeasType::m_enumFallTime:
    case Enum_Scope_MeasType::m_enumPosWidth:
    case Enum_Scope_MeasType::m_enumNegWidth:
        o.m_strUnits = QStringLiteral("s");
        break;
    case Enum_Scope_MeasType::m_enumDutyCycle:
        o.m_strUnits = QStringLiteral("%");
        break;
    case Enum_Scope_MeasType::m_enumPhase:
        o.m_strUnits = QStringLiteral("deg");
        break;
    default:
        o.m_strUnits = QStringLiteral("V");
        break;
    }
    return valid ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
}
ScopeError CSimScopePlugin::clearMeasurements(U32BIT s)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    return ScopeError();
}

/*============================================================================
 *  Status / debug
 *==========================================================================*/
ScopeError CSimScopePlugin::readErrorStatus(U32BIT s, U32BIT, S_Scope_DeviceErrorStatus& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = S_Scope_DeviceErrorStatus();
    o.m_EnumAcqState = d->m_eAcqState;
    o.m_EnumTriggerState = (d->m_eAcqState == Enum_Scope_AcqState::m_enumRunning)
                               ? Enum_Scope_TriggerState::m_enumAuto
                               : Enum_Scope_TriggerState::m_enumTriggered;
    if (d->m_eAcqState == Enum_Scope_AcqState::m_enumRunning)
    {
        o.m_statusFlags |= Enum_Scope_DeviceStatusFlag::Running;
    }
    else
    {
        o.m_statusFlags |= Enum_Scope_DeviceStatusFlag::Stopped;
    }
    return ScopeError();
}
ScopeError CSimScopePlugin::readStatusByte(U32BIT s, U32BIT& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = 0;
    return ScopeError();
}
ScopeError CSimScopePlugin::queryErrorQueue(U32BIT s, QString& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = QStringLiteral("0,\"No error\"");
    return ScopeError();
}
ScopeError CSimScopePlugin::writeScpi(U32BIT s, const QString&)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    return ScopeError();
}
ScopeError CSimScopePlugin::queryScpi(U32BIT s, const QString& v, QString& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (v.trimmed().compare(QStringLiteral("*IDN?"), Qt::CaseInsensitive) == 0)
    {
        return getIdentification(s, o);
    }
    o = QStringLiteral("0");
    return ScopeError();
}

/*============================================================================
 *  Full-API completeness: the remaining interface groups, simulated with
 *  stored state (getter-backed) or benign success. Range/channel/capability
 *  checks match the real plugins so behaviour is consistent hardware-free.
 *==========================================================================*/
ScopeError CSimScopePlugin::setVerticalPosition(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    dev(s)->m_vVertPosition[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getVerticalPosition(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = dev(s)->m_vVertPosition[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setInputImpedance(U32BIT s, U32BIT c, Enum_Scope_InputImpedance v)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    dev(s)->m_vImpedance[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getInputImpedance(U32BIT s, U32BIT c, Enum_Scope_InputImpedance& o)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = dev(s)->m_vImpedance[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setInvert(U32BIT s, U32BIT c, bool v)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    dev(s)->m_vInvert[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getInvert(U32BIT s, U32BIT c, bool& o)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = dev(s)->m_vInvert[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setChannelLabel(U32BIT s, U32BIT c, const QString& v)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    dev(s)->m_vLabel[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getChannelLabel(U32BIT s, U32BIT c, QString& o)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = dev(s)->m_vLabel[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setChannelUnits(U32BIT s, U32BIT c, const QString& v)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    dev(s)->m_vUnits[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getChannelUnits(U32BIT s, U32BIT c, QString& o)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = dev(s)->m_vUnits[c - 1];
    return ScopeError();
}
ScopeError CSimScopePlugin::setDeskew(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    dev(s)->m_vDeskew[c - 1] = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getDeskew(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(dev(s), c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    o = dev(s)->m_vDeskew[c - 1];
    return ScopeError();
}

ScopeError CSimScopePlugin::setTimebaseReference(U32BIT s, FDOUBLE)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setTimebaseMode(U32BIT s, Enum_Scope_TimebaseMode v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eTimebaseMode = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTimebaseMode(U32BIT s, Enum_Scope_TimebaseMode& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eTimebaseMode;
    return ScopeError();
}
ScopeError CSimScopePlugin::setAcquisitionPoints(U32BIT s, U32BIT v)
{
    return setWaveformPoints(s, v);
}
ScopeError CSimScopePlugin::getAcquisitionPoints(U32BIT s, U32BIT& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = static_cast<U32BIT>(d->m_iWfmPoints);
    return ScopeError();
}

ScopeError CSimScopePlugin::setTriggerCoupling(U32BIT s, Enum_Scope_Coupling v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eTrigCoupling = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getTriggerCoupling(U32BIT s, Enum_Scope_Coupling& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eTrigCoupling;
    return ScopeError();
}
ScopeError CSimScopePlugin::setTriggerPulseWidth(U32BIT s, FDOUBLE)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setTriggerVideoStandard(U32BIT s, const QString&)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setTriggerPattern(U32BIT s, const QString&)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}

ScopeError CSimScopePlugin::setSegmentedCount(U32BIT s, U32BIT v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_u32SegmentCount = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getSegmentedCount(U32BIT s, U32BIT& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_u32SegmentCount;
    return ScopeError();
}

ScopeError CSimScopePlugin::setMeasureStatistics(U32BIT s, bool)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::getMeasurementStatistics(U32BIT s, U32BIT c, Enum_Scope_MeasType t,
                                                     S_Scope_MeasurementResult& o)
{
    return readMeasurement(s, c, t, o);
}

ScopeError CSimScopePlugin::setMathOperation(U32BIT s, Enum_Scope_MathOp v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eMathOp = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::setMathSource1(U32BIT s, U32BIT c)
{
    return validChannel(dev(s), c) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
}
ScopeError CSimScopePlugin::setMathSource2(U32BIT s, U32BIT c)
{
    return validChannel(dev(s), c) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
}
ScopeError CSimScopePlugin::enableMath(U32BIT s, bool)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setFftWindow(U32BIT s, Enum_Scope_FftWindow v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eFftWindow = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getFftWindow(U32BIT s, Enum_Scope_FftWindow& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eFftWindow;
    return ScopeError();
}
ScopeError CSimScopePlugin::setFftSpan(U32BIT s, FDOUBLE)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setFftCenter(U32BIT s, FDOUBLE)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setMathScale(U32BIT s, FDOUBLE)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setMathPosition(U32BIT s, FDOUBLE)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}

ScopeError CSimScopePlugin::setCursorType(U32BIT s, Enum_Scope_CursorType v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eCursorType = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getCursorType(U32BIT s, Enum_Scope_CursorType& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eCursorType;
    return ScopeError();
}
ScopeError CSimScopePlugin::setCursorSource(U32BIT s, U32BIT c)
{
    return validChannel(dev(s), c) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
}
ScopeError CSimScopePlugin::setCursorPosition(U32BIT s, U32BIT i, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (i <= 1)
    {
        d->m_dCursorX1 = v;
    }
    else
    {
        d->m_dCursorX2 = v;
    }
    return ScopeError();
}
ScopeError CSimScopePlugin::getCursorPosition(U32BIT s, U32BIT i, FDOUBLE& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = (i <= 1) ? d->m_dCursorX1 : d->m_dCursorX2;
    return ScopeError();
}
ScopeError CSimScopePlugin::readCursorValues(U32BIT s, FDOUBLE& x1, FDOUBLE& x2, FDOUBLE& y1, FDOUBLE& y2)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    x1 = d->m_dCursorX1;
    x2 = d->m_dCursorX2;
    y1 = d->m_dCursorY1;
    y2 = d->m_dCursorY2;
    return ScopeError();
}

ScopeError CSimScopePlugin::setPersistence(U32BIT s, FDOUBLE)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setGraticule(U32BIT s, const QString&)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setIntensity(U32BIT s, FDOUBLE)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::setDisplayFormat(U32BIT s, Enum_Scope_TimebaseMode v)
{
    return setTimebaseMode(s, v);
}
ScopeError CSimScopePlugin::setVectors(U32BIT s, bool)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}

ScopeError CSimScopePlugin::saveSetup(U32BIT s, U32BIT)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::recallSetup(U32BIT s, U32BIT)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::saveWaveformToFile(U32BIT s, U32BIT c, const QString&)
{
    return validChannel(dev(s), c) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
}
ScopeError CSimScopePlugin::captureScreenshot(U32BIT s, Enum_Scope_ImageFormat, QByteArray& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    static const char* const kPngB64 = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+M8"
                                       "AAAMBAQAY3Y2wAAAAAElFTkSuQmCC";
    o = QByteArray::fromBase64(QByteArray(kPngB64));
    return ScopeError();
}
ScopeError CSimScopePlugin::saveToReference(U32BIT s, U32BIT c, U32BIT)
{
    return validChannel(dev(s), c) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
}
ScopeError CSimScopePlugin::displayReference(U32BIT s, U32BIT, bool)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}

ScopeError CSimScopePlugin::enableDigitalChannel(U32BIT s, U32BIT, bool)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    return d->m_pLimits->m_bHasDigital ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
}
ScopeError CSimScopePlugin::setDigitalThreshold(U32BIT s, U32BIT, FDOUBLE)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    return d->m_pLimits->m_bHasDigital ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
}
ScopeError CSimScopePlugin::setPodThreshold(U32BIT s, U32BIT, FDOUBLE)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    return d->m_pLimits->m_bHasDigital ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
}
ScopeError CSimScopePlugin::enableBus(U32BIT s, U32BIT, bool)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    return d->m_pLimits->m_bHasSerialDecode ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
}
ScopeError CSimScopePlugin::setBusType(U32BIT s, U32BIT, const QString&)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    return d->m_pLimits->m_bHasSerialDecode ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
}
ScopeError CSimScopePlugin::readBusDecode(U32BIT s, U32BIT, QString& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (!d->m_pLimits->m_bHasSerialDecode)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    o = QStringLiteral("(no bus data)");
    return ScopeError();
}

ScopeError CSimScopePlugin::setAwgFunction(U32BIT s, const QString& v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (!d->m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    d->m_strAwgFunction = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::setAwgFrequency(U32BIT s, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (!d->m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    if (v < 0.0 || v > d->m_pLimits->m_dAwgFreqMax)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("AWG frequency %1 out of range").arg(v));
    }
    d->m_dAwgFreq = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::setAwgAmplitude(U32BIT s, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (!d->m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    if (v < 0.0 || v > d->m_pLimits->m_dAwgAmplMax)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("AWG amplitude %1 out of range").arg(v));
    }
    d->m_dAwgAmpl = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::setAwgOffset(U32BIT s, FDOUBLE v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (!d->m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    d->m_dAwgOffset = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::enableAwgOutput(U32BIT s, bool v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (!d->m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    d->m_bAwgOn = v;
    return ScopeError();
}

ScopeError CSimScopePlugin::clearStatus(U32BIT s)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::getOptions(U32BIT s, QString& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = QStringLiteral("SIM");
    return ScopeError();
}
ScopeError CSimScopePlugin::waitOperationComplete(U32BIT s)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::clearErrorStatus(U32BIT s, U32BIT)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
ScopeError CSimScopePlugin::readStandardEventStatus(U32BIT s, U32BIT& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = 0;
    return ScopeError();
}
ScopeError CSimScopePlugin::readOperationStatus(U32BIT s, U32BIT& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = (d->m_eAcqState == Enum_Scope_AcqState::m_enumRunning) ? 0x10u : 0u;
    return ScopeError();
}
ScopeError CSimScopePlugin::readQuestionableStatus(U32BIT s, U32BIT& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = 0;
    return ScopeError();
}
ScopeError CSimScopePlugin::getInstrumentErrorCount(U32BIT s, U32BIT& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = 0;
    return ScopeError();
}
ScopeError CSimScopePlugin::setRemoteState(U32BIT s, Enum_Scope_RemoteState v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_eRemote = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::getRemoteState(U32BIT s, Enum_Scope_RemoteState& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_eRemote;
    return ScopeError();
}
ScopeError CSimScopePlugin::setKeyLock(U32BIT s, bool v)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_bKeyLocked = v;
    return ScopeError();
}
ScopeError CSimScopePlugin::isKeyLocked(U32BIT s, bool& o)
{
    S_SimDevice* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_bKeyLocked;
    return ScopeError();
}
ScopeError CSimScopePlugin::setBeeper(U32BIT s, bool)
{
    return isConnected(s) ? ScopeError() : ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
}
