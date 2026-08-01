/**
 * @file    TDS2024CPlugin.cpp
 * @brief   Tektronix TDS2024C (TDS2000C) oscilloscope - SCPI-over-VISA plugin implementation.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "TDS2024CPlugin.h"
#include "VisaHelper.h" // inline S_Scope_ConnectionConfig::toVisaResourceString()

#include <QByteArray>
#include <QStringList>
#include <cmath>
#include <cstdio>
#include <cstring>

/*============================================================================
 *  Construction / info / capabilities
 *==========================================================================*/
CTDS2024CPlugin::CTDS2024CPlugin() : m_pLimits(nullptr), m_strModel(QStringLiteral("TDS2024C"))
{
    const S_ScopeLimits* p = ScopeFindLimits("TDS2024C");
    if (p != nullptr)
    {
        m_limits = *p;
    }
    else
    {
        std::memset(&m_limits, 0, sizeof(m_limits));
    }
    m_pLimits = &m_limits;
}

CTDS2024CPlugin::~CTDS2024CPlugin()
{
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        if (it.value().m_vi != VI_NULL)
        {
            viClose(it.value().m_vi);
        }
        if (it.value().m_rm != VI_NULL)
        {
            viClose(it.value().m_rm);
        }
    }
    m_devices.clear();
}

S_Scope_PluginInfo CTDS2024CPlugin::getPluginInfo() const
{
    S_Scope_PluginInfo s;
    qstrncpy(s.m_szName, m_strModel.toLatin1().constData(), PLUGIN_INFO_NAME_SIZE);
    qstrncpy(s.m_szVersion, "1.0.0", PLUGIN_INFO_VERSION_SIZE);
    qstrncpy(s.m_szManufacturer, "Tektronix", PLUGIN_INFO_MANUFACTURER_SIZE);
    qstrncpy(s.m_szModelName, m_strModel.toLatin1().constData(), PLUGIN_INFO_MODEL_NAME_SIZE);
    qstrncpy(s.m_szSeries, "3 Series MDO", PLUGIN_INFO_SERIES_SIZE);
    qstrncpy(s.m_szDescription, "Tektronix TDS2024C mixed-domain oscilloscope, 1 GHz, 4 analog ch",
             PLUGIN_INFO_DESCRIPTION_SIZE);
    qstrncpy(s.m_szMinCoreVersion, "1.0.0", PLUGIN_INFO_MIN_CORE_VERSION);
    qstrncpy(s.m_szMaxCoreVersion, "2.0.0", PLUGIN_INFO_MAX_CORE_VERSION);
    s.m_StrlstSupportedProtocols << "USB" << "LAN" << "GPIB";
    s.m_StrlstSupportedModes << "SAMPLE" << "PEAK" << "AVERAGE" << "HIRES" << "ENVELOPE";
    return s;
}

S_Scope_Capabilities CTDS2024CPlugin::getCapabilities() const
{
    S_Scope_Capabilities caps;
    caps.m_u32NumberOfChannels = static_cast<U32BIT>(m_pLimits->m_iAnalogChannels);
    caps.m_u32NumberOfDigital = static_cast<U32BIT>(m_pLimits->m_iDigitalChannels);
    caps.m_dBandwidthHz = m_pLimits->m_dBandwidthHz;
    caps.m_dMaxSampleRate = m_pLimits->m_dMaxSampleRate;
    caps.m_u32MaxMemoryDepth = m_pLimits->m_u32MaxMemoryDepth;
    caps.m_dMinTimebaseScale = m_pLimits->m_dTimebaseMin;
    caps.m_dMaxTimebaseScale = m_pLimits->m_dTimebaseMax;
    caps.m_bHasDigital = m_pLimits->m_bHasDigital;
    caps.m_bHasAWG = m_pLimits->m_bHasAWG;
    caps.m_bHasFFT = m_pLimits->m_bHasFFT;
    caps.m_bHasSerialDecode = m_pLimits->m_bHasSerialDecode;
    caps.m_bHasSegmented = m_pLimits->m_bHasSegmented;
    for (int c = 1; c <= m_pLimits->m_iAnalogChannels; ++c)
    {
        S_Scope_ChannelCapabilities ch;
        ch.m_u32ChannelNumber = static_cast<U32BIT>(c);
        ch.m_dBandwidthHz = m_pLimits->m_dBandwidthHz;
        ch.m_dMaxVerticalScale = m_pLimits->m_dVertScaleMax;
        ch.m_dMinVerticalScale = m_pLimits->m_dVertScaleMin;
        ch.m_bHas50Ohm = true;
        ch.m_bHasBandwidthLimit = true;
        caps.m_QlistChannels.append(ch);
    }
    return caps;
}

/*============================================================================
 *  Instance / transport
 *==========================================================================*/
CTDS2024CPlugin::S_DeviceInstance* CTDS2024CPlugin::dev(U32BIT s)
{
    auto it = m_devices.find(s);
    return (it == m_devices.end()) ? nullptr : &it.value();
}
bool CTDS2024CPlugin::validChannel(U32BIT c) const
{
    return c >= 1 && static_cast<int>(c) <= m_pLimits->m_iAnalogChannels;
}
QByteArray CTDS2024CPlugin::fmtD(FDOUBLE v)
{
    char sz[40];
    std::snprintf(sz, sizeof(sz), "%.6G", v);
    return QByteArray(sz);
}
const char* CTDS2024CPlugin::srcTok(Enum_Scope_TriggerSource e)
{
    switch (e)
    {
    case Enum_Scope_TriggerSource::m_enumCh1:
        return "CH1";
    case Enum_Scope_TriggerSource::m_enumCh2:
        return "CH2";
    case Enum_Scope_TriggerSource::m_enumCh3:
        return "CH3";
    case Enum_Scope_TriggerSource::m_enumCh4:
        return "CH4";
    case Enum_Scope_TriggerSource::m_enumExt:
        return "AUX";
    case Enum_Scope_TriggerSource::m_enumLine:
        return "LINE";
    default:
        return "CH1";
    }
}

ScopeError CTDS2024CPlugin::visaError(ViStatus st, const QString& ctx)
{
    if (st == VI_ERROR_TMO)
    {
        return ScopeError(Enum_Scope_ErrorCode::COMMUNICATION_TIMEOUT,
                          QStringLiteral("%1: VISA timeout").arg(ctx));
    }
    return ScopeError(Enum_Scope_ErrorCode::COMMUNICATION_ERROR,
                      QStringLiteral("%1: VISA status 0x%2")
                          .arg(ctx)
                          .arg(static_cast<quint32>(st), 8, 16, QLatin1Char('0')));
}

ScopeError CTDS2024CPlugin::writeLine(U32BIT s, const QByteArray& cmd)
{
    S_DeviceInstance* d = dev(s);
    if (d == nullptr || !d->m_bConnected)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    QByteArray aby = cmd;
    aby.append('\n');
    ViUInt32 written = 0;
    const ViStatus st = viWrite(d->m_vi, reinterpret_cast<ViConstBuf>(aby.constData()),
                                static_cast<ViUInt32>(aby.size()), &written);
    if (st < VI_SUCCESS)
    {
        return visaError(st, QStringLiteral("viWrite"));
    }
    return ScopeError();
}

ScopeError CTDS2024CPlugin::readLine(U32BIT s, QByteArray& resp)
{
    resp.clear();
    S_DeviceInstance* d = dev(s);
    if (d == nullptr || !d->m_bConnected)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    viSetAttribute(d->m_vi, VI_ATTR_TERMCHAR_EN, VI_TRUE);
    char chunk[8192];
    for (;;)
    {
        ViUInt32 got = 0;
        const ViStatus st =
            viRead(d->m_vi, reinterpret_cast<ViBuf>(chunk), static_cast<ViUInt32>(sizeof(chunk)), &got);
        if (st < VI_SUCCESS)
        {
            return visaError(st, QStringLiteral("viRead"));
        }
        resp.append(chunk, static_cast<int>(got));
        if (st != VI_SUCCESS_MAX_CNT)
        {
            break;
        }
        if (resp.size() > 16 * 1024 * 1024)
        {
            break;
        }
    }
    while (resp.endsWith('\n') || resp.endsWith('\r'))
    {
        resp.chop(1);
    }
    return ScopeError();
}

ScopeError CTDS2024CPlugin::queryLine(U32BIT s, const QByteArray& cmd, QByteArray& resp)
{
    ScopeError e = writeLine(s, cmd);
    if (!e.isSuccess())
    {
        return e;
    }
    return readLine(s, resp);
}

/* Read a counted/binary #<w><len><payload> block (waveform / screenshot). */
ScopeError CTDS2024CPlugin::readBinaryBlock(U32BIT s, QByteArray& payload)
{
    payload.clear();
    S_DeviceInstance* d = dev(s);
    if (d == nullptr || !d->m_bConnected)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    viSetAttribute(d->m_vi, VI_ATTR_TERMCHAR_EN, VI_FALSE); // binary: no termchar
    QByteArray raw;
    char chunk[65536];
    for (;;)
    {
        ViUInt32 got = 0;
        const ViStatus st =
            viRead(d->m_vi, reinterpret_cast<ViBuf>(chunk), static_cast<ViUInt32>(sizeof(chunk)), &got);
        if (st < VI_SUCCESS)
        {
            viSetAttribute(d->m_vi, VI_ATTR_TERMCHAR_EN, VI_TRUE);
            return visaError(st, QStringLiteral("viRead block"));
        }
        raw.append(chunk, static_cast<int>(got));
        if (st != VI_SUCCESS_MAX_CNT)
        {
            break;
        }
        if (raw.size() > 64 * 1024 * 1024)
        {
            break;
        }
    }
    viSetAttribute(d->m_vi, VI_ATTR_TERMCHAR_EN, VI_TRUE);

    const int iHash = raw.indexOf('#');
    if (iHash < 0 || iHash + 2 > raw.size())
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE, QStringLiteral("no # block header"));
    }
    const int w = raw[iHash + 1] - '0';
    if (w < 1 || iHash + 2 + w > raw.size())
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE, QStringLiteral("bad block length prefix"));
    }
    const int len = raw.mid(iHash + 2, w).toInt();
    const int start = iHash + 2 + w;
    payload = raw.mid(start, len);
    if (payload.size() != len)
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE,
                          QStringLiteral("short block: got %1 of %2").arg(payload.size()).arg(len));
    }
    return ScopeError();
}

ScopeError CTDS2024CPlugin::sendChecked(U32BIT s, const QByteArray& cmd)
{
    ScopeError e = writeLine(s, cmd);
    if (!e.isSuccess())
    {
        return e;
    }
    QByteArray resp;
    e = queryLine(s, QByteArrayLiteral("SYST:ERR?"), resp);
    if (!e.isSuccess())
    {
        return e;
    }
    const int iComma = resp.indexOf(',');
    const int code = (iComma < 0) ? resp.trimmed().toInt() : resp.left(iComma).trimmed().toInt();
    if (code != 0)
    {
        return ScopeError(Enum_Scope_ErrorCode::INSTRUMENT_ERROR,
                          QStringLiteral("instrument error %1: %2").arg(code).arg(QString::fromLatin1(resp)));
    }
    return ScopeError();
}

ScopeError CTDS2024CPlugin::queryDouble(U32BIT s, const QByteArray& cmd, FDOUBLE& o)
{
    QByteArray resp;
    ScopeError e = queryLine(s, cmd, resp);
    if (!e.isSuccess())
    {
        return e;
    }
    bool ok = false;
    const double v = resp.trimmed().toDouble(&ok);
    if (!ok)
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE,
                          QStringLiteral("bad numeric response \"%1\"").arg(QString::fromLatin1(resp)));
    }
    o = v;
    return ScopeError();
}

ScopeError CTDS2024CPlugin::setDouble(U32BIT s, const char* scpi, FDOUBLE v, FDOUBLE lo, FDOUBLE hi,
                                      const char* what)
{
    if (v < lo || v > hi)
    {
        return ScopeError(
            Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
            QStringLiteral("%1 %2 out of range [%3 .. %4]").arg(QLatin1String(what)).arg(v).arg(lo).arg(hi));
    }
    return sendChecked(s, QByteArray(scpi) + " " + fmtD(v));
}

/*============================================================================
 *  Connection
 *==========================================================================*/
ScopeError CTDS2024CPlugin::connect(U32BIT s, const S_Scope_ConnectionConfig& c)
{
    if (dev(s) != nullptr && m_devices[s].m_bConnected)
    {
        return ScopeError(Enum_Scope_ErrorCode::ALREADY_CONNECTED);
    }

    S_DeviceInstance d;
    d.m_sConfig = c;
    d.m_u32Timeout = c.m_u32Timeout ? c.m_u32Timeout : 5000;

    ViStatus st = viOpenDefaultRM(&d.m_rm);
    if (st < VI_SUCCESS)
    {
        return visaError(st, QStringLiteral("viOpenDefaultRM"));
    }
    const QByteArray abyRes = c.toVisaResourceString().toLatin1();
    st = viOpen(d.m_rm, abyRes.constData(), 0, 0, &d.m_vi);
    if (st < VI_SUCCESS)
    {
        viClose(d.m_rm);
        return visaError(st, QStringLiteral("viOpen"));
    }
    viSetAttribute(d.m_vi, VI_ATTR_TMO_VALUE, d.m_u32Timeout);
    viSetAttribute(d.m_vi, VI_ATTR_TERMCHAR, static_cast<ViAttrState>('\n'));
    viSetAttribute(d.m_vi, VI_ATTR_TERMCHAR_EN, VI_TRUE);
    viSetAttribute(d.m_vi, VI_ATTR_SEND_END_EN, VI_TRUE);
    d.m_bConnected = true;
    m_devices[s] = d;

    QByteArray idn;
    ScopeError e = queryLine(s, QByteArrayLiteral("*IDN?"), idn);
    if (!e.isSuccess())
    {
        disconnect(s);
        return e;
    }
    if (!QString::fromLatin1(idn).contains(m_pLimits->m_szIdnMatch, Qt::CaseInsensitive))
    {
        const QString strIdn = QString::fromLatin1(idn).trimmed();
        disconnect(s);
        return ScopeError(Enum_Scope_ErrorCode::CONNECTION_FAILED,
                          QStringLiteral("*IDN? \"%1\" does not name %2").arg(strIdn, m_strModel));
    }
    m_devices[s].m_strIdn = QString::fromLatin1(idn).trimmed();
    return ScopeError();
}

ScopeError CTDS2024CPlugin::disconnect(U32BIT s)
{
    S_DeviceInstance* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    if (d->m_vi != VI_NULL)
    {
        viClose(d->m_vi);
    }
    if (d->m_rm != VI_NULL)
    {
        viClose(d->m_rm);
    }
    m_devices.remove(s);
    return ScopeError();
}

bool CTDS2024CPlugin::isConnected(U32BIT s) const
{
    auto it = m_devices.find(s);
    return it != m_devices.end() && it.value().m_bConnected;
}

ScopeError CTDS2024CPlugin::reset(U32BIT s)
{
    return sendChecked(s, QByteArrayLiteral("*RST"));
}
ScopeError CTDS2024CPlugin::clearStatus(U32BIT s)
{
    return writeLine(s, QByteArrayLiteral("*CLS"));
}

ScopeError CTDS2024CPlugin::setTimeout(U32BIT s, U32BIT t)
{
    S_DeviceInstance* d = dev(s);
    if (d == nullptr)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_u32Timeout = t;
    viSetAttribute(d->m_vi, VI_ATTR_TMO_VALUE, t);
    return ScopeError();
}
ScopeError CTDS2024CPlugin::getIdentification(U32BIT s, QString& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("*IDN?"), r);
    if (e.isSuccess())
    {
        o = QString::fromLatin1(r).trimmed();
    }
    return e;
}
ScopeError CTDS2024CPlugin::selfTest(U32BIT s, S32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("*TST?"), d);
    if (e.isSuccess())
    {
        o = static_cast<S32BIT>(d);
    }
    return e;
}
ScopeError CTDS2024CPlugin::getOptions(U32BIT s, QString& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("*OPT?"), r);
    if (e.isSuccess())
    {
        o = QString::fromLatin1(r).trimmed();
    }
    return e;
}
ScopeError CTDS2024CPlugin::waitOperationComplete(U32BIT s)
{
    QByteArray r;
    return queryLine(s, QByteArrayLiteral("*OPC?"), r);
}
ScopeError CTDS2024CPlugin::getScpiVersion(U32BIT s, QString& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = QStringLiteral("1999.0");
    return ScopeError();
}

ScopeError CTDS2024CPlugin::getParameterRange(U32BIT s, U32BIT c, Enum_Scope_ParamId p,
                                              S_Scope_ParameterRange& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    const S_ScopeLimits* L = m_pLimits;
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
        break;
    case Enum_Scope_ParamId::m_enumTimebaseScale:
        o.m_dMin = L->m_dTimebaseMin;
        o.m_dMax = L->m_dTimebaseMax;
        break;
    case Enum_Scope_ParamId::m_enumTriggerLevel:
        o.m_dMin = -L->m_dVertOffsetMax;
        o.m_dMax = L->m_dVertOffsetMax;
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
        o.m_dMin = 1.0;
        o.m_dMax = static_cast<double>(L->m_u32MaxMemoryDepth);
        break;
    default:
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    (void)c;
    return ScopeError();
}

ScopeError CTDS2024CPlugin::autoscale(U32BIT s)
{
    return sendChecked(s, QByteArrayLiteral("AUTOSet EXECute"));
}
ScopeError CTDS2024CPlugin::run(U32BIT s)
{
    ScopeError e = sendChecked(s, QByteArrayLiteral("ACQuire:STATE RUN"));
    if (e.isSuccess())
    {
        S_DeviceInstance* d = dev(s);
        if (d)
        {
            d->m_bRunning = true;
        }
    }
    return e;
}
ScopeError CTDS2024CPlugin::stop(U32BIT s)
{
    ScopeError e = sendChecked(s, QByteArrayLiteral("ACQuire:STATE STOP"));
    if (e.isSuccess())
    {
        S_DeviceInstance* d = dev(s);
        if (d)
        {
            d->m_bRunning = false;
        }
    }
    return e;
}
ScopeError CTDS2024CPlugin::single(U32BIT s)
{
    ScopeError e = sendChecked(s, QByteArrayLiteral("ACQuire:STOPAfter SEQuence"));
    if (!e.isSuccess())
    {
        return e;
    }
    e = sendChecked(s, QByteArrayLiteral("ACQuire:STATE RUN"));
    if (e.isSuccess())
    {
        S_DeviceInstance* d = dev(s);
        if (d)
        {
            d->m_bRunning = false;
        }
    } // single completes
    return e;
}
ScopeError CTDS2024CPlugin::forceTrigger(U32BIT s)
{
    return sendChecked(s, QByteArrayLiteral("TRIGger FORCe"));
}

/*============================================================================
 *  Vertical
 *==========================================================================*/
ScopeError CTDS2024CPlugin::enableChannel(U32BIT s, U32BIT c, bool v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray("SELect:CH") + QByteArray::number(c) + (v ? " ON" : " OFF"));
}
ScopeError CTDS2024CPlugin::isChannelEnabled(U32BIT s, U32BIT c, bool& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArray("SELect:CH") + QByteArray::number(c) + "?", d);
    if (e.isSuccess())
    {
        o = (d != 0.0);
    }
    return e;
}
ScopeError CTDS2024CPlugin::setVerticalScale(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return setDouble(s, (QByteArray("CH") + QByteArray::number(c) + ":SCAle").constData(), v,
                     m_pLimits->m_dVertScaleMin, m_pLimits->m_dVertScaleMax, "vertical scale");
}
ScopeError CTDS2024CPlugin::getVerticalScale(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return queryDouble(s, QByteArray("CH") + QByteArray::number(c) + ":SCAle?", o);
}
ScopeError CTDS2024CPlugin::setVerticalOffset(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return setDouble(s, (QByteArray("CH") + QByteArray::number(c) + ":OFFSet").constData(), v,
                     -m_pLimits->m_dVertOffsetMax, m_pLimits->m_dVertOffsetMax, "vertical offset");
}
ScopeError CTDS2024CPlugin::getVerticalOffset(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return queryDouble(s, QByteArray("CH") + QByteArray::number(c) + ":OFFSet?", o);
}
ScopeError CTDS2024CPlugin::setVerticalPosition(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray("CH") + QByteArray::number(c) + ":POSition " + fmtD(v));
}
ScopeError CTDS2024CPlugin::getVerticalPosition(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return queryDouble(s, QByteArray("CH") + QByteArray::number(c) + ":POSition?", o);
}
ScopeError CTDS2024CPlugin::setCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    const char* tok = (v == Enum_Scope_Coupling::m_enumAC)    ? "AC"
                      : (v == Enum_Scope_Coupling::m_enumGND) ? "GND"
                                                              : "DC";
    return sendChecked(s, QByteArray("CH") + QByteArray::number(c) + ":COUPling " + tok);
}
ScopeError CTDS2024CPlugin::getCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    QByteArray r;
    ScopeError e = queryLine(s, QByteArray("CH") + QByteArray::number(c) + ":COUPling?", r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    o = t.startsWith(QStringLiteral("AC"))    ? Enum_Scope_Coupling::m_enumAC
        : t.startsWith(QStringLiteral("GND")) ? Enum_Scope_Coupling::m_enumGND
                                              : Enum_Scope_Coupling::m_enumDC;
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(c))
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
    return sendChecked(s, QByteArray("CH") + QByteArray::number(c) + ":PRObe:GAIN " + fmtD(1.0 / v));
}
ScopeError CTDS2024CPlugin::getProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    FDOUBLE gain = 0;
    ScopeError e = queryDouble(s, QByteArray("CH") + QByteArray::number(c) + ":PRObe:GAIN?", gain);
    if (!e.isSuccess())
    {
        return e;
    }
    o = (gain > 0.0) ? (1.0 / gain) : 0.0;
    return ScopeError();
}

/*============================================================================
 *  Horizontal
 *==========================================================================*/
ScopeError CTDS2024CPlugin::setTimebaseScale(U32BIT s, FDOUBLE v)
{
    return setDouble(s, "HORizontal:SCAle", v, m_pLimits->m_dTimebaseMin, m_pLimits->m_dTimebaseMax,
                     "timebase scale");
}
ScopeError CTDS2024CPlugin::getTimebaseScale(U32BIT s, FDOUBLE& o)
{
    return queryDouble(s, QByteArrayLiteral("HORizontal:SCAle?"), o);
}
ScopeError CTDS2024CPlugin::setTimebasePosition(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray("HORizontal:DELay:TIMe ") + fmtD(v));
}
ScopeError CTDS2024CPlugin::getTimebasePosition(U32BIT s, FDOUBLE& o)
{
    return queryDouble(s, QByteArrayLiteral("HORizontal:DELay:TIMe?"), o);
}
ScopeError CTDS2024CPlugin::getSampleRate(U32BIT s, FDOUBLE& o)
{
    FDOUBLE sr = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("HORizontal:SAMPLERate?"), sr);
    if (!e.isSuccess())
    {
        return e;
    }
    if (sr <= 0.0)
    {
        FDOUBLE tb = 0;
        getTimebaseScale(s, tb);
        U32BIT pts = 1000;
        getMemoryDepth(s, pts);
        sr = (tb > 0.0) ? (static_cast<double>(pts) / (ScopeShared::GRATICULE_HDIV * tb)) : 0.0;
        if (sr > m_pLimits->m_dMaxSampleRate)
        {
            sr = m_pLimits->m_dMaxSampleRate;
        }
    }
    o = sr;
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setMemoryDepth(U32BIT s, U32BIT v)
{
    if (v < 1 || v > m_pLimits->m_u32MaxMemoryDepth)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("memory depth %1 out of range [1 .. %2]")
                              .arg(v)
                              .arg(m_pLimits->m_u32MaxMemoryDepth));
    }
    return sendChecked(s, QByteArray("HORizontal:RECOrdlength ") + QByteArray::number(v));
}
ScopeError CTDS2024CPlugin::getMemoryDepth(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("HORizontal:RECOrdlength?"), d);
    if (!e.isSuccess())
    {
        return e;
    }
    o = (d > 0.0) ? static_cast<U32BIT>(d) : 1000u;
    return ScopeError();
}

/*============================================================================
 *  Trigger
 *==========================================================================*/
ScopeError CTDS2024CPlugin::setTriggerMode(U32BIT s, Enum_Scope_TriggerMode v)
{
    const char* tok = (v == Enum_Scope_TriggerMode::m_enumNormal) ? "NORMal" : "AUTO";
    if (v == Enum_Scope_TriggerMode::m_enumSingle)
    {
        return single(s);
    }
    return sendChecked(s, QByteArray("TRIGger:A:MODe ") + tok);
}
ScopeError CTDS2024CPlugin::getTriggerMode(U32BIT s, Enum_Scope_TriggerMode& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("TRIGger:A:MODe?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    o = QString::fromLatin1(r).trimmed().toUpper().startsWith(QStringLiteral("NORM"))
            ? Enum_Scope_TriggerMode::m_enumNormal
            : Enum_Scope_TriggerMode::m_enumAuto;
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setTriggerSource(U32BIT s, Enum_Scope_TriggerSource v)
{
    return sendChecked(s, QByteArray("TRIGger:A:EDGE:SOUrce ") + srcTok(v));
}
ScopeError CTDS2024CPlugin::getTriggerSource(U32BIT s, Enum_Scope_TriggerSource& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("TRIGger:A:EDGE:SOUrce?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    if (t.contains(QStringLiteral("CH2")))
    {
        o = Enum_Scope_TriggerSource::m_enumCh2;
    }
    else if (t.contains(QStringLiteral("CH3")))
    {
        o = Enum_Scope_TriggerSource::m_enumCh3;
    }
    else if (t.contains(QStringLiteral("CH4")))
    {
        o = Enum_Scope_TriggerSource::m_enumCh4;
    }
    else if (t.contains(QStringLiteral("AUX")) || t.contains(QStringLiteral("EXT")))
    {
        o = Enum_Scope_TriggerSource::m_enumExt;
    }
    else if (t.contains(QStringLiteral("LINE")))
    {
        o = Enum_Scope_TriggerSource::m_enumLine;
    }
    else
    {
        o = Enum_Scope_TriggerSource::m_enumCh1;
    }
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope v)
{
    const char* tok = (v == Enum_Scope_TriggerSlope::m_enumFalling)  ? "FALL"
                      : (v == Enum_Scope_TriggerSlope::m_enumEither) ? "EITher"
                                                                     : "RISe";
    return sendChecked(s, QByteArray("TRIGger:A:EDGE:SLOpe ") + tok);
}
ScopeError CTDS2024CPlugin::getTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("TRIGger:A:EDGE:SLOpe?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    o = t.startsWith(QStringLiteral("FALL"))  ? Enum_Scope_TriggerSlope::m_enumFalling
        : t.startsWith(QStringLiteral("EIT")) ? Enum_Scope_TriggerSlope::m_enumEither
                                              : Enum_Scope_TriggerSlope::m_enumRising;
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setTriggerLevel(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray("TRIGger:A:LEVel:CH") + QByteArray::number(c) + " " + fmtD(v));
}
ScopeError CTDS2024CPlugin::getTriggerLevel(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return queryDouble(s, QByteArray("TRIGger:A:LEVel:CH") + QByteArray::number(c) + "?", o);
}
ScopeError CTDS2024CPlugin::setTriggerHoldoff(U32BIT s, FDOUBLE v)
{
    return setDouble(s, "TRIGger:A:HOLDoff:TIMe", v, m_pLimits->m_dTrigHoldoffMin,
                     m_pLimits->m_dTrigHoldoffMax, "holdoff");
}
ScopeError CTDS2024CPlugin::getTriggerHoldoff(U32BIT s, FDOUBLE& o)
{
    return queryDouble(s, QByteArrayLiteral("TRIGger:A:HOLDoff:TIMe?"), o);
}
ScopeError CTDS2024CPlugin::getTriggerState(U32BIT s, Enum_Scope_TriggerState& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("TRIGger:STATE?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    if (t.contains(QStringLiteral("TRIG")) || t.contains(QStringLiteral("SAVE")))
    {
        o = Enum_Scope_TriggerState::m_enumTriggered;
    }
    else if (t.contains(QStringLiteral("READY")) || t.contains(QStringLiteral("ARM")))
    {
        o = Enum_Scope_TriggerState::m_enumReady;
    }
    else if (t.contains(QStringLiteral("AUTO")))
    {
        o = Enum_Scope_TriggerState::m_enumAuto;
    }
    else
    {
        o = Enum_Scope_TriggerState::m_enumTriggered;
    }
    return ScopeError();
}

/*============================================================================
 *  Acquisition
 *==========================================================================*/
ScopeError CTDS2024CPlugin::setAcqMode(U32BIT s, Enum_Scope_AcqMode v)
{
    const char* tok = "SAMple";
    switch (v)
    {
    case Enum_Scope_AcqMode::m_enumPeakDetect:
        tok = "PEAKdetect";
        break;
    case Enum_Scope_AcqMode::m_enumAverage:
        tok = "AVErage";
        break;
    case Enum_Scope_AcqMode::m_enumHiRes:
        tok = "HIRes";
        break;
    case Enum_Scope_AcqMode::m_enumEnvelope:
        tok = "ENVelope";
        break;
    default:
        break;
    }
    return sendChecked(s, QByteArray("ACQuire:MODe ") + tok);
}
ScopeError CTDS2024CPlugin::getAcqMode(U32BIT s, Enum_Scope_AcqMode& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("ACQuire:MODe?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    if (t.startsWith(QStringLiteral("PEAK")))
    {
        o = Enum_Scope_AcqMode::m_enumPeakDetect;
    }
    else if (t.startsWith(QStringLiteral("AVE")))
    {
        o = Enum_Scope_AcqMode::m_enumAverage;
    }
    else if (t.startsWith(QStringLiteral("HIR")))
    {
        o = Enum_Scope_AcqMode::m_enumHiRes;
    }
    else if (t.startsWith(QStringLiteral("ENV")))
    {
        o = Enum_Scope_AcqMode::m_enumEnvelope;
    }
    else
    {
        o = Enum_Scope_AcqMode::m_enumSample;
    }
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setAverageCount(U32BIT s, U32BIT v)
{
    if (v < 1 || static_cast<int>(v) > m_pLimits->m_iAvgCountMax)
    {
        return ScopeError(
            Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
            QStringLiteral("average count %1 out of range [1 .. %2]").arg(v).arg(m_pLimits->m_iAvgCountMax));
    }
    return sendChecked(s, QByteArray("ACQuire:NUMAVg ") + QByteArray::number(v));
}
ScopeError CTDS2024CPlugin::getAverageCount(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("ACQuire:NUMAVg?"), d);
    if (e.isSuccess())
    {
        o = static_cast<U32BIT>(d);
    }
    return e;
}
ScopeError CTDS2024CPlugin::getAcquisitionState(U32BIT s, Enum_Scope_AcqState& o)
{
    S_DeviceInstance* d = dev(s);
    if (d == nullptr || !d->m_bConnected)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_bRunning ? Enum_Scope_AcqState::m_enumRunning : Enum_Scope_AcqState::m_enumStopped;
    return ScopeError();
}

/*============================================================================
 *  Waveform
 *==========================================================================*/
ScopeError CTDS2024CPlugin::setWaveformSource(U32BIT s, Enum_Scope_WaveformSource v, U32BIT i)
{
    if (v != Enum_Scope_WaveformSource::m_enumChannel)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED,
                          QStringLiteral("only channel source implemented"));
    }
    if (!validChannel(i))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray("DATa:SOUrce CH") + QByteArray::number(i));
}
ScopeError CTDS2024CPlugin::setWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat v)
{
    if (v == Enum_Scope_WaveformFormat::m_enumAscii)
    {
        return sendChecked(s, QByteArrayLiteral("DATa:ENCdg ASCii"));
    }
    ScopeError e = sendChecked(s, QByteArrayLiteral("DATa:ENCdg RIBinary")); // signed, MSB first
    if (!e.isSuccess())
    {
        return e;
    }
    const int width = (v == Enum_Scope_WaveformFormat::m_enumByte) ? 1 : 2;
    return sendChecked(s, QByteArray("DATa:WIDth ") + QByteArray::number(width));
}
ScopeError CTDS2024CPlugin::setWaveformPoints(U32BIT s, U32BIT v)
{
    if (v < 1 || v > m_pLimits->m_u32MaxMemoryDepth)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("waveform points %1 out of range").arg(v));
    }
    ScopeError e = sendChecked(s, QByteArrayLiteral("DATa:STARt 1"));
    if (!e.isSuccess())
    {
        return e;
    }
    return sendChecked(s, QByteArray("DATa:STOP ") + QByteArray::number(v));
}
ScopeError CTDS2024CPlugin::getWaveformPreamble(U32BIT s, U32BIT c, S_Scope_WaveformPreamble& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("WFMOutpre?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QList<QByteArray> f = r.trimmed().split(',');
    if (f.size() < 10)
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE, QStringLiteral("short preamble"));
    }
    o.m_iFormat = f[0].trimmed().toInt();
    o.m_iType = f[1].trimmed().toInt();
    o.m_u32Points = static_cast<U32BIT>(f[2].trimmed().toInt());
    o.m_u32Count = static_cast<U32BIT>(f[3].trimmed().toInt());
    o.m_dXIncrement = f[4].trimmed().toDouble();
    o.m_dXOrigin = f[5].trimmed().toDouble();
    o.m_dXReference = f[6].trimmed().toDouble();
    o.m_dYIncrement = f[7].trimmed().toDouble();
    o.m_dYOrigin = f[8].trimmed().toDouble();
    o.m_dYReference = f[9].trimmed().toDouble();
    return ScopeError();
}
ScopeError CTDS2024CPlugin::readWaveform(U32BIT s, U32BIT c, S_Scope_Waveform& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    ScopeError e = setWaveformSource(s, Enum_Scope_WaveformSource::m_enumChannel, c);
    if (!e.isSuccess())
    {
        return e;
    }
    e = setWaveformFormat(s, Enum_Scope_WaveformFormat::m_enumWord);
    if (!e.isSuccess())
    {
        return e;
    }
    e = getWaveformPreamble(s, c, o.m_sPreamble);
    if (!e.isSuccess())
    {
        return e;
    }

    QByteArray payload;
    e = writeLine(s, QByteArrayLiteral("CURVe?"));
    if (!e.isSuccess())
    {
        return e;
    }
    e = readBinaryBlock(s, payload);
    if (!e.isSuccess())
    {
        return e;
    }

    const S_Scope_WaveformPreamble& pr = o.m_sPreamble;
    const int n = payload.size() / 2; // WORD = 2 bytes
    o.m_u32SourceChannel = c;
    o.m_vecTimeSeconds.resize(n);
    o.m_vecVolts.resize(n);
    for (int i = 0; i < n; ++i)
    {
        const qint16 code = static_cast<qint16>((static_cast<quint8>(payload[2 * i]) << 8) |
                                                static_cast<quint8>(payload[2 * i + 1]));
        o.m_vecVolts[i] = (static_cast<double>(code) - pr.m_dYReference) * pr.m_dYIncrement + pr.m_dYOrigin;
        o.m_vecTimeSeconds[i] =
            pr.m_dXOrigin + (static_cast<double>(i) - pr.m_dXReference) * pr.m_dXIncrement;
    }
    return ScopeError();
}
ScopeError CTDS2024CPlugin::digitizeChannel(U32BIT s, U32BIT c)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return single(s);
}

/*============================================================================
 *  Status
 *==========================================================================*/
ScopeError CTDS2024CPlugin::readErrorStatus(U32BIT s, U32BIT, S_Scope_DeviceErrorStatus& o)
{
    o = S_Scope_DeviceErrorStatus();
    FDOUBLE esr = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("*ESR?"), esr);
    if (!e.isSuccess())
    {
        return e;
    }
    o.m_iStandardEventStatus = static_cast<S32BIT>(esr);
    Enum_Scope_AcqState acq = Enum_Scope_AcqState::m_enumStopped;
    getAcquisitionState(s, acq);
    o.m_EnumAcqState = acq;
    o.m_statusFlags |= (acq == Enum_Scope_AcqState::m_enumRunning) ? Enum_Scope_DeviceStatusFlag::Running
                                                                   : Enum_Scope_DeviceStatusFlag::Stopped;
    QByteArray q;
    queryLine(s, QByteArrayLiteral("SYST:ERR?"), q);
    o.m_StrErrorMessage = QString::fromLatin1(q).trimmed();
    return ScopeError();
}
ScopeError CTDS2024CPlugin::clearErrorStatus(U32BIT s, U32BIT)
{
    return writeLine(s, QByteArrayLiteral("*CLS"));
}
ScopeError CTDS2024CPlugin::queryErrorQueue(U32BIT s, QString& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("SYST:ERR?"), r);
    if (e.isSuccess())
    {
        o = QString::fromLatin1(r).trimmed();
    }
    return e;
}
ScopeError CTDS2024CPlugin::readStatusByte(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("*STB?"), d);
    if (e.isSuccess())
    {
        o = static_cast<U32BIT>(d);
    }
    return e;
}
ScopeError CTDS2024CPlugin::readStandardEventStatus(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("*ESR?"), d);
    if (e.isSuccess())
    {
        o = static_cast<U32BIT>(d);
    }
    return e;
}
ScopeError CTDS2024CPlugin::writeScpi(U32BIT s, const QString& v)
{
    return writeLine(s, v.toLatin1());
}
ScopeError CTDS2024CPlugin::queryScpi(U32BIT s, const QString& v, QString& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, v.toLatin1(), r);
    if (e.isSuccess())
    {
        o = QString::fromLatin1(r).trimmed();
    }
    return e;
}

/*============================================================================
 *  M5 feature slices - measurements / cursors / math / display / save /
 *  digital (MSO) / AWG / status. Tektronix dialect.
 *==========================================================================*/
namespace
{
const char* mdoMeasTok(Enum_Scope_MeasType t)
{
    switch (t)
    {
    case Enum_Scope_MeasType::m_enumVpp:
        return "PK2pk";
    case Enum_Scope_MeasType::m_enumVmax:
        return "MAXimum";
    case Enum_Scope_MeasType::m_enumVmin:
        return "MINImum";
    case Enum_Scope_MeasType::m_enumVrms:
        return "RMS";
    case Enum_Scope_MeasType::m_enumVavg:
        return "MEAN";
    case Enum_Scope_MeasType::m_enumFrequency:
        return "FREQuency";
    case Enum_Scope_MeasType::m_enumPeriod:
        return "PERIod";
    case Enum_Scope_MeasType::m_enumRiseTime:
        return "RISe";
    case Enum_Scope_MeasType::m_enumFallTime:
        return "FALL";
    case Enum_Scope_MeasType::m_enumPosWidth:
        return "PWIdth";
    case Enum_Scope_MeasType::m_enumNegWidth:
        return "NWIdth";
    case Enum_Scope_MeasType::m_enumDutyCycle:
        return "PDUty";
    case Enum_Scope_MeasType::m_enumPhase:
        return "PHAse";
    case Enum_Scope_MeasType::m_enumDelay:
        return "DELay";
    default:
        return "FREQuency";
    }
}
QString measUnits(Enum_Scope_MeasType t)
{
    switch (t)
    {
    case Enum_Scope_MeasType::m_enumFrequency:
        return QStringLiteral("Hz");
    case Enum_Scope_MeasType::m_enumPeriod:
    case Enum_Scope_MeasType::m_enumRiseTime:
    case Enum_Scope_MeasType::m_enumFallTime:
    case Enum_Scope_MeasType::m_enumPosWidth:
    case Enum_Scope_MeasType::m_enumNegWidth:
        return QStringLiteral("s");
    case Enum_Scope_MeasType::m_enumDutyCycle:
        return QStringLiteral("%");
    case Enum_Scope_MeasType::m_enumPhase:
        return QStringLiteral("deg");
    default:
        return QStringLiteral("V");
    }
}
} // namespace

ScopeError CTDS2024CPlugin::addMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    ScopeError e = sendChecked(s, QByteArray("MEASUrement:IMMed:SOUrce CH") + QByteArray::number(c));
    if (!e.isSuccess())
    {
        return e;
    }
    return sendChecked(s, QByteArray("MEASUrement:IMMed:TYPe ") + mdoMeasTok(t));
}
ScopeError CTDS2024CPlugin::readMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t,
                                            S_Scope_MeasurementResult& o)
{
    ScopeError e = addMeasurement(s, c, t);
    if (!e.isSuccess())
    {
        return e;
    }
    FDOUBLE v = 0.0;
    e = queryDouble(s, QByteArrayLiteral("MEASUrement:IMMed:VALue?"), v);
    if (!e.isSuccess())
    {
        return e;
    }
    o.m_eType = t;
    o.m_dValue = v;
    o.m_bValid = true;
    o.m_strUnits = measUnits(t);
    o.m_dMean = v;
    o.m_dMin = v;
    o.m_dMax = v;
    o.m_dStdDev = 0.0;
    return ScopeError();
}
ScopeError CTDS2024CPlugin::clearMeasurements(U32BIT s)
{
    return writeLine(s, QByteArrayLiteral("MEASUrement:CLEAR"));
}
ScopeError CTDS2024CPlugin::setMeasureStatistics(U32BIT s, bool v)
{
    return sendChecked(s, QByteArray("MEASUrement:STATIstics:MODe ") + (v ? "ALL" : "OFF"));
}
ScopeError CTDS2024CPlugin::getMeasurementStatistics(U32BIT s, U32BIT c, Enum_Scope_MeasType t,
                                                     S_Scope_MeasurementResult& o)
{
    return readMeasurement(s, c, t, o);
}

ScopeError CTDS2024CPlugin::setCursorType(U32BIT s, Enum_Scope_CursorType v)
{
    const char* tok = (v == Enum_Scope_CursorType::m_enumHorizontal) ? "HBArs"
                      : (v == Enum_Scope_CursorType::m_enumVertical) ? "VBArs"
                      : (v == Enum_Scope_CursorType::m_enumTrack)    ? "SCReen"
                                                                     : "OFF";
    return sendChecked(s, QByteArray("CURSor:FUNCtion ") + tok);
}
ScopeError CTDS2024CPlugin::getCursorType(U32BIT s, Enum_Scope_CursorType& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("CURSor:FUNCtion?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    if (t.startsWith(QStringLiteral("HBA")))
    {
        o = Enum_Scope_CursorType::m_enumHorizontal;
    }
    else if (t.startsWith(QStringLiteral("VBA")))
    {
        o = Enum_Scope_CursorType::m_enumVertical;
    }
    else if (t.startsWith(QStringLiteral("SCR")))
    {
        o = Enum_Scope_CursorType::m_enumTrack;
    }
    else
    {
        o = Enum_Scope_CursorType::m_enumOff;
    }
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setCursorSource(U32BIT s, U32BIT c)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray("CURSor:SOUrce CH") + QByteArray::number(c));
}
ScopeError CTDS2024CPlugin::setCursorPosition(U32BIT s, U32BIT i, FDOUBLE v)
{
    return sendChecked(s, QByteArray("CURSor:VBArs:POSITION") + QByteArray::number(i == 0 ? 1 : i) + " " +
                              fmtD(v));
}
ScopeError CTDS2024CPlugin::getCursorPosition(U32BIT s, U32BIT i, FDOUBLE& o)
{
    return queryDouble(s, QByteArray("CURSor:VBArs:POSITION") + QByteArray::number(i == 0 ? 1 : i) + "?", o);
}
ScopeError CTDS2024CPlugin::readCursorValues(U32BIT s, FDOUBLE& x1, FDOUBLE& x2, FDOUBLE& y1, FDOUBLE& y2)
{
    queryDouble(s, QByteArrayLiteral("CURSor:VBArs:POSITION1?"), x1);
    queryDouble(s, QByteArrayLiteral("CURSor:VBArs:POSITION2?"), x2);
    queryDouble(s, QByteArrayLiteral("CURSor:HBArs:POSITION1?"), y1);
    queryDouble(s, QByteArrayLiteral("CURSor:HBArs:POSITION2?"), y2);
    return ScopeError();
}

ScopeError CTDS2024CPlugin::setMathOperation(U32BIT s, Enum_Scope_MathOp v)
{
    return sendChecked(s, QByteArray("MATH:TYPe ") + ((v == Enum_Scope_MathOp::m_enumFFT) ? "FFT" : "DUAl"));
}
ScopeError CTDS2024CPlugin::setMathSource1(U32BIT s, U32BIT c)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray("MATH:SOURCE1 CH") + QByteArray::number(c));
}
ScopeError CTDS2024CPlugin::setMathSource2(U32BIT s, U32BIT c)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray("MATH:SOURCE2 CH") + QByteArray::number(c));
}
ScopeError CTDS2024CPlugin::enableMath(U32BIT s, bool v)
{
    return sendChecked(s, QByteArray("SELect:MATH ") + (v ? "ON" : "OFF"));
}
ScopeError CTDS2024CPlugin::setFftWindow(U32BIT s, Enum_Scope_FftWindow v)
{
    const char* tok = "HANning";
    switch (v)
    {
    case Enum_Scope_FftWindow::m_enumRect:
        tok = "RECTangular";
        break;
    case Enum_Scope_FftWindow::m_enumHamming:
        tok = "HAMMing";
        break;
    case Enum_Scope_FftWindow::m_enumBlackman:
        tok = "BLACKmanharris";
        break;
    case Enum_Scope_FftWindow::m_enumFlattop:
        tok = "FLATTOP2";
        break;
    default:
        break;
    }
    return sendChecked(s, QByteArray("MATH:FFT:WINdow ") + tok);
}
ScopeError CTDS2024CPlugin::getFftWindow(U32BIT s, Enum_Scope_FftWindow& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("MATH:FFT:WINdow?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    if (t.startsWith(QStringLiteral("RECT")))
    {
        o = Enum_Scope_FftWindow::m_enumRect;
    }
    else if (t.startsWith(QStringLiteral("HAMM")))
    {
        o = Enum_Scope_FftWindow::m_enumHamming;
    }
    else if (t.startsWith(QStringLiteral("BLACK")))
    {
        o = Enum_Scope_FftWindow::m_enumBlackman;
    }
    else if (t.startsWith(QStringLiteral("FLAT")))
    {
        o = Enum_Scope_FftWindow::m_enumFlattop;
    }
    else
    {
        o = Enum_Scope_FftWindow::m_enumHann;
    }
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setFftSpan(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray("MATH:FFT:HORizontal:SPAN ") + fmtD(v));
}
ScopeError CTDS2024CPlugin::setFftCenter(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray("MATH:FFT:HORizontal:CENTer ") + fmtD(v));
}
ScopeError CTDS2024CPlugin::setMathScale(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray("MATH:VERTical:SCAle ") + fmtD(v));
}
ScopeError CTDS2024CPlugin::setMathPosition(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray("MATH:VERTical:POSition ") + fmtD(v));
}

ScopeError CTDS2024CPlugin::setPersistence(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray("DISplay:PERSistence ") + fmtD(v));
}
ScopeError CTDS2024CPlugin::setGraticule(U32BIT s, const QString& v)
{
    return sendChecked(s, QByteArray("DISplay:GRAticule ") + v.toLatin1());
}
ScopeError CTDS2024CPlugin::setIntensity(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray("DISplay:INTENSITy:WAVEform ") + fmtD(v));
}
ScopeError CTDS2024CPlugin::setDisplayFormat(U32BIT s, Enum_Scope_TimebaseMode v)
{
    return sendChecked(s, QByteArray("DISplay:FORMat ") +
                              ((v == Enum_Scope_TimebaseMode::m_enumXY) ? "XY" : "YT"));
}
ScopeError CTDS2024CPlugin::setVectors(U32BIT s, bool v)
{
    return sendChecked(s, QByteArray("DISplay:STYLE:DOTsonly ") + (v ? "OFF" : "ON"));
}

ScopeError CTDS2024CPlugin::saveSetup(U32BIT s, U32BIT loc)
{
    return sendChecked(s, QByteArray("*SAV ") + QByteArray::number(loc));
}
ScopeError CTDS2024CPlugin::recallSetup(U32BIT s, U32BIT loc)
{
    return sendChecked(s, QByteArray("*RCL ") + QByteArray::number(loc));
}
ScopeError CTDS2024CPlugin::saveWaveformToFile(U32BIT s, U32BIT c, const QString& path)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray("SAVe:WAVEform CH") + QByteArray::number(c) + ",\"" + path.toLatin1() +
                              "\"");
}
ScopeError CTDS2024CPlugin::captureScreenshot(U32BIT s, Enum_Scope_ImageFormat, QByteArray& o)
{
    ScopeError e = writeLine(s, QByteArrayLiteral("HARDCopy STARt"));
    if (!e.isSuccess())
    {
        return e;
    }
    return readBinaryBlock(s, o);
}
ScopeError CTDS2024CPlugin::saveToReference(U32BIT s, U32BIT c, U32BIT slot)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray("SAVe:WAVEform CH") + QByteArray::number(c) + ",REF" +
                              QByteArray::number(slot));
}
ScopeError CTDS2024CPlugin::displayReference(U32BIT s, U32BIT slot, bool v)
{
    return sendChecked(s, QByteArray("SELect:REF") + QByteArray::number(slot) + (v ? " ON" : " OFF"));
}

ScopeError CTDS2024CPlugin::enableDigitalChannel(U32BIT s, U32BIT d, bool v)
{
    if (!m_pLimits->m_bHasDigital)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray("SELect:D") + QByteArray::number(d) + (v ? " ON" : " OFF"));
}
ScopeError CTDS2024CPlugin::setDigitalThreshold(U32BIT s, U32BIT d, FDOUBLE v)
{
    if (!m_pLimits->m_bHasDigital)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray("DIGital:THRESHold:D") + QByteArray::number(d) + " " + fmtD(v));
}
ScopeError CTDS2024CPlugin::setPodThreshold(U32BIT s, U32BIT p, FDOUBLE v)
{
    if (!m_pLimits->m_bHasDigital)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray("DIGital:POD") + QByteArray::number(p) + ":THRESHold " + fmtD(v));
}
ScopeError CTDS2024CPlugin::enableBus(U32BIT s, U32BIT b, bool v)
{
    if (!m_pLimits->m_bHasSerialDecode)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray("BUS:B") + QByteArray::number(b) + ":STATE " + (v ? "ON" : "OFF"));
}
ScopeError CTDS2024CPlugin::setBusType(U32BIT s, U32BIT b, const QString& v)
{
    if (!m_pLimits->m_bHasSerialDecode)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray("BUS:B") + QByteArray::number(b) + ":TYPe " + v.toLatin1());
}
ScopeError CTDS2024CPlugin::readBusDecode(U32BIT s, U32BIT b, QString& o)
{
    if (!m_pLimits->m_bHasSerialDecode)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    // TODO(manual): Tektronix exposes decoded frames via bus event tables; this
    // DATA? read is the mock-facing form until the Programmer Manual is applied.
    QByteArray r;
    ScopeError e = queryLine(s, QByteArray("BUS:B") + QByteArray::number(b) + ":DATA?", r);
    if (e.isSuccess())
    {
        o = QString::fromLatin1(r).trimmed();
    }
    return e;
}

ScopeError CTDS2024CPlugin::setAwgFunction(U32BIT s, const QString& v)
{
    if (!m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray("AFG:FUNCtion ") + v.toLatin1());
}
ScopeError CTDS2024CPlugin::setAwgFrequency(U32BIT s, FDOUBLE v)
{
    if (!m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    if (v < 0.0 || v > m_pLimits->m_dAwgFreqMax)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("AWG frequency %1 out of range").arg(v));
    }
    return sendChecked(s, QByteArray("AFG:FREQuency ") + fmtD(v));
}
ScopeError CTDS2024CPlugin::setAwgAmplitude(U32BIT s, FDOUBLE v)
{
    if (!m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    if (v < 0.0 || v > m_pLimits->m_dAwgAmplMax)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("AWG amplitude %1 out of range").arg(v));
    }
    return sendChecked(s, QByteArray("AFG:AMPLitude ") + fmtD(v));
}
ScopeError CTDS2024CPlugin::setAwgOffset(U32BIT s, FDOUBLE v)
{
    if (!m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray("AFG:OFFSet ") + fmtD(v));
}
ScopeError CTDS2024CPlugin::enableAwgOutput(U32BIT s, bool v)
{
    if (!m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray("AFG:OUTPut:STATE ") + (v ? "ON" : "OFF"));
}

ScopeError CTDS2024CPlugin::readOperationStatus(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("BUSY?"), d);
    o = e.isSuccess() ? static_cast<U32BIT>(d) : 0u;
    return ScopeError();
}
ScopeError CTDS2024CPlugin::readQuestionableStatus(U32BIT s, U32BIT& o)
{
    (void)s;
    o = 0u;
    return ScopeError();
}
ScopeError CTDS2024CPlugin::getInstrumentErrorCount(U32BIT s, U32BIT& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("SYST:ERR?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    o = r.trimmed().startsWith('0') ? 0u : 1u;
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setRemoteState(U32BIT s, Enum_Scope_RemoteState v)
{
    return sendChecked(s,
                       QByteArray("LOCk ") + ((v == Enum_Scope_RemoteState::m_enumLocal) ? "NONe" : "ALL"));
}
ScopeError CTDS2024CPlugin::getRemoteState(U32BIT s, Enum_Scope_RemoteState& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("LOCk?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    o = QString::fromLatin1(r).trimmed().toUpper().startsWith(QStringLiteral("NON"))
            ? Enum_Scope_RemoteState::m_enumLocal
            : Enum_Scope_RemoteState::m_enumRemote;
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setKeyLock(U32BIT s, bool v)
{
    return sendChecked(s, QByteArray("LOCk ") + (v ? "ALL" : "NONe"));
}
ScopeError CTDS2024CPlugin::isKeyLocked(U32BIT s, bool& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("LOCk?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    o = !QString::fromLatin1(r).trimmed().toUpper().startsWith(QStringLiteral("NON"));
    return ScopeError();
}
ScopeError CTDS2024CPlugin::setBeeper(U32BIT s, bool v)
{
    (void)v;
    return writeLine(s, QByteArrayLiteral("*CLS"));
}
