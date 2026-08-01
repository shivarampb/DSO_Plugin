/**
 * @file    DSOS204APlugin.cpp
 * @brief   Keysight DSOS204A (Infiniium S-Series) oscilloscope - SCPI-over-VISA plugin implementation.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "DSOS204APlugin.h"
#include "VisaHelper.h"

#include <QByteArray>
#include <QStringList>
#include <cmath>
#include <cstdio>

/* The one model identity this class carries; clones override only this line. */
#define KS_MODEL_NAME "DSOS204A"

CDSOS204APlugin::CDSOS204APlugin() : m_pLimits(nullptr), m_strModel(QStringLiteral(KS_MODEL_NAME))
{
    const S_ScopeLimits* p = ScopeFindLimits(KS_MODEL_NAME);
    if (p != nullptr)
    {
        m_limits = *p; // own a copy of the row
    }
    else
    {
        std::memset(&m_limits, 0, sizeof(m_limits));
        m_limits.m_szModelName = KS_MODEL_NAME;
        m_limits.m_iAnalogChannels = 2;
        m_limits.m_dVertScaleMin = 1e-3;
        m_limits.m_dVertScaleMax = 5.0;
        m_limits.m_dTimebaseMin = 5e-9;
        m_limits.m_dTimebaseMax = 50.0;
        m_limits.m_dVertOffsetMax = 100.0;
        m_limits.m_u32MaxMemoryDepth = 100000;
        m_limits.m_dMaxSampleRate = 2e9;
        m_limits.m_iAvgCountMax = 65536;
        m_limits.m_szIdnMatch = KS_MODEL_NAME;
        m_limits.m_szManufacturer = "Keysight";
    }
    m_pLimits = &m_limits;
}

CDSOS204APlugin::~CDSOS204APlugin()
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

S_Scope_PluginInfo CDSOS204APlugin::getPluginInfo() const
{
    S_Scope_PluginInfo s;
    qstrncpy(s.m_szName, m_strModel.toLatin1().constData(), PLUGIN_INFO_NAME_SIZE);
    qstrncpy(s.m_szVersion, "1.0.0", PLUGIN_INFO_VERSION_SIZE);
    qstrncpy(s.m_szManufacturer, m_pLimits->m_szManufacturer, PLUGIN_INFO_MANUFACTURER_SIZE);
    qstrncpy(s.m_szModelName, m_strModel.toLatin1().constData(), PLUGIN_INFO_MODEL_NAME_SIZE);
    qstrncpy(s.m_szSeries, m_pLimits->m_szSeries, PLUGIN_INFO_SERIES_SIZE);
    const QString desc = QStringLiteral("%1 %2 oscilloscope, %3 analog ch")
                             .arg(QString::fromLatin1(m_pLimits->m_szManufacturer), m_strModel)
                             .arg(m_pLimits->m_iAnalogChannels);
    qstrncpy(s.m_szDescription, desc.toLatin1().constData(), PLUGIN_INFO_DESCRIPTION_SIZE);
    qstrncpy(s.m_szMinCoreVersion, "1.0.0", PLUGIN_INFO_MIN_CORE_VERSION);
    qstrncpy(s.m_szMaxCoreVersion, "2.0.0", PLUGIN_INFO_MAX_CORE_VERSION);
    s.m_StrlstSupportedProtocols << "USB" << "LAN" << "GPIB";
    s.m_StrlstSupportedModes << "NORMAL" << "AVERAGE" << "HRES" << "PEAK";
    return s;
}

S_Scope_Capabilities CDSOS204APlugin::getCapabilities() const
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

CDSOS204APlugin::S_DeviceInstance* CDSOS204APlugin::dev(U32BIT s)
{
    auto it = m_devices.find(s);
    return (it == m_devices.end()) ? nullptr : &it.value();
}
bool CDSOS204APlugin::validChannel(U32BIT c) const
{
    return c >= 1 && static_cast<int>(c) <= m_pLimits->m_iAnalogChannels;
}
QByteArray CDSOS204APlugin::fmtD(FDOUBLE v)
{
    char sz[40];
    std::snprintf(sz, sizeof(sz), "%.6G", v);
    return QByteArray(sz);
}
int CDSOS204APlugin::chanOf(Enum_Scope_TriggerSource e)
{
    switch (e)
    {
    case Enum_Scope_TriggerSource::m_enumCh1:
        return 1;
    case Enum_Scope_TriggerSource::m_enumCh2:
        return 2;
    case Enum_Scope_TriggerSource::m_enumCh3:
        return 3;
    case Enum_Scope_TriggerSource::m_enumCh4:
        return 4;
    default:
        return 0;
    }
}

ScopeError CDSOS204APlugin::visaError(ViStatus st, const QString& ctx)
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
ScopeError CDSOS204APlugin::writeLine(U32BIT s, const QByteArray& cmd)
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
ScopeError CDSOS204APlugin::readLine(U32BIT s, QByteArray& resp)
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
ScopeError CDSOS204APlugin::queryLine(U32BIT s, const QByteArray& cmd, QByteArray& resp)
{
    ScopeError e = writeLine(s, cmd);
    if (!e.isSuccess())
    {
        return e;
    }
    return readLine(s, resp);
}
ScopeError CDSOS204APlugin::readBinaryBlock(U32BIT s, QByteArray& payload)
{
    payload.clear();
    S_DeviceInstance* d = dev(s);
    if (d == nullptr || !d->m_bConnected)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    viSetAttribute(d->m_vi, VI_ATTR_TERMCHAR_EN, VI_FALSE);
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
    payload = raw.mid(iHash + 2 + w, len);
    if (payload.size() != len)
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE, QStringLiteral("short block"));
    }
    return ScopeError();
}
ScopeError CDSOS204APlugin::sendChecked(U32BIT s, const QByteArray& cmd)
{
    ScopeError e = writeLine(s, cmd);
    if (!e.isSuccess())
    {
        return e;
    }
    QByteArray resp;
    e = queryLine(s, QByteArrayLiteral(":SYSTem:ERRor?"), resp);
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
ScopeError CDSOS204APlugin::queryDouble(U32BIT s, const QByteArray& cmd, FDOUBLE& o)
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
                          QStringLiteral("bad numeric \"%1\"").arg(QString::fromLatin1(resp)));
    }
    o = v;
    return ScopeError();
}
ScopeError CDSOS204APlugin::setDouble(U32BIT s, const char* scpi, FDOUBLE v, FDOUBLE lo, FDOUBLE hi,
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

ScopeError CDSOS204APlugin::connect(U32BIT s, const S_Scope_ConnectionConfig& c)
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
ScopeError CDSOS204APlugin::disconnect(U32BIT s)
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
bool CDSOS204APlugin::isConnected(U32BIT s) const
{
    auto it = m_devices.find(s);
    return it != m_devices.end() && it.value().m_bConnected;
}
ScopeError CDSOS204APlugin::reset(U32BIT s)
{
    return sendChecked(s, QByteArrayLiteral("*RST"));
}
ScopeError CDSOS204APlugin::clearStatus(U32BIT s)
{
    return writeLine(s, QByteArrayLiteral("*CLS"));
}
ScopeError CDSOS204APlugin::setTimeout(U32BIT s, U32BIT t)
{
    S_DeviceInstance* d = dev(s);
    if (!d)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    d->m_u32Timeout = t;
    viSetAttribute(d->m_vi, VI_ATTR_TMO_VALUE, t);
    return ScopeError();
}
ScopeError CDSOS204APlugin::getIdentification(U32BIT s, QString& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("*IDN?"), r);
    if (e.isSuccess())
    {
        o = QString::fromLatin1(r).trimmed();
    }
    return e;
}
ScopeError CDSOS204APlugin::selfTest(U32BIT s, S32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("*TST?"), d);
    if (e.isSuccess())
    {
        o = static_cast<S32BIT>(d);
    }
    return e;
}
ScopeError CDSOS204APlugin::getOptions(U32BIT s, QString& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral("*OPT?"), r);
    if (e.isSuccess())
    {
        o = QString::fromLatin1(r).trimmed();
    }
    return e;
}
ScopeError CDSOS204APlugin::waitOperationComplete(U32BIT s)
{
    QByteArray r;
    return queryLine(s, QByteArrayLiteral("*OPC?"), r);
}
ScopeError CDSOS204APlugin::getScpiVersion(U32BIT s, QString& o)
{
    if (!isConnected(s))
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = QStringLiteral("1999.0");
    return ScopeError();
}

ScopeError CDSOS204APlugin::getParameterRange(U32BIT s, U32BIT c, Enum_Scope_ParamId p,
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
        o.m_dResolution = 1e-3;
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

ScopeError CDSOS204APlugin::autoscale(U32BIT s)
{
    return sendChecked(s, QByteArrayLiteral(":AUToscale"));
}
ScopeError CDSOS204APlugin::run(U32BIT s)
{
    ScopeError e = writeLine(s, QByteArrayLiteral(":RUN"));
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
ScopeError CDSOS204APlugin::stop(U32BIT s)
{
    ScopeError e = writeLine(s, QByteArrayLiteral(":STOP"));
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
ScopeError CDSOS204APlugin::single(U32BIT s)
{
    ScopeError e = writeLine(s, QByteArrayLiteral(":SINGle"));
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
ScopeError CDSOS204APlugin::forceTrigger(U32BIT s)
{
    return writeLine(s, QByteArrayLiteral(":TRIGger:FORCe"));
}

ScopeError CDSOS204APlugin::enableChannel(U32BIT s, U32BIT c, bool v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":DISPlay " + (v ? "ON" : "OFF"));
}
ScopeError CDSOS204APlugin::isChannelEnabled(U32BIT s, U32BIT c, bool& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":DISPlay?", d);
    if (e.isSuccess())
    {
        o = (d != 0.0);
    }
    return e;
}
ScopeError CDSOS204APlugin::setVerticalScale(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return setDouble(s, (QByteArray(":CHANnel") + QByteArray::number(c) + ":SCALe").constData(), v,
                     m_pLimits->m_dVertScaleMin, m_pLimits->m_dVertScaleMax, "vertical scale");
}
ScopeError CDSOS204APlugin::getVerticalScale(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return queryDouble(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":SCALe?", o);
}
ScopeError CDSOS204APlugin::setVerticalOffset(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return setDouble(s, (QByteArray(":CHANnel") + QByteArray::number(c) + ":OFFSet").constData(), v,
                     -m_pLimits->m_dVertOffsetMax, m_pLimits->m_dVertOffsetMax, "vertical offset");
}
ScopeError CDSOS204APlugin::getVerticalOffset(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return queryDouble(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":OFFSet?", o);
}
ScopeError CDSOS204APlugin::setCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    const char* tok = (v == Enum_Scope_Coupling::m_enumAC) ? "AC" : "DC"; // Keysight: AC|DC
    return sendChecked(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":COUPling " + tok);
}
ScopeError CDSOS204APlugin::getCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    QByteArray r;
    ScopeError e = queryLine(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":COUPling?", r);
    if (!e.isSuccess())
    {
        return e;
    }
    o = QString::fromLatin1(r).trimmed().toUpper().startsWith(QStringLiteral("AC"))
            ? Enum_Scope_Coupling::m_enumAC
            : Enum_Scope_Coupling::m_enumDC;
    return ScopeError();
}
ScopeError CDSOS204APlugin::setProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE v)
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
                          QStringLiteral("probe attenuation %1 invalid").arg(v));
    }
    return sendChecked(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":PROBe " + fmtD(v));
}
ScopeError CDSOS204APlugin::getProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return queryDouble(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":PROBe?", o);
}

ScopeError CDSOS204APlugin::setTimebaseScale(U32BIT s, FDOUBLE v)
{
    return setDouble(s, ":TIMebase:SCALe", v, m_pLimits->m_dTimebaseMin, m_pLimits->m_dTimebaseMax,
                     "timebase scale");
}
ScopeError CDSOS204APlugin::getTimebaseScale(U32BIT s, FDOUBLE& o)
{
    return queryDouble(s, QByteArrayLiteral(":TIMebase:SCALe?"), o);
}
ScopeError CDSOS204APlugin::setTimebasePosition(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray(":TIMebase:POSition ") + fmtD(v));
}
ScopeError CDSOS204APlugin::getTimebasePosition(U32BIT s, FDOUBLE& o)
{
    return queryDouble(s, QByteArrayLiteral(":TIMebase:POSition?"), o);
}
ScopeError CDSOS204APlugin::getSampleRate(U32BIT s, FDOUBLE& o)
{
    FDOUBLE sr = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral(":ACQuire:SRATe?"), sr);
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
ScopeError CDSOS204APlugin::setMemoryDepth(U32BIT s, U32BIT v)
{
    if (v < 1 || v > m_pLimits->m_u32MaxMemoryDepth)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("memory depth %1 out of range").arg(v));
    }
    return sendChecked(s, QByteArray(":ACQuire:POINts ") + QByteArray::number(v));
}
ScopeError CDSOS204APlugin::getMemoryDepth(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral(":ACQuire:POINts?"), d);
    if (!e.isSuccess())
    {
        return e;
    }
    o = (d > 0.0) ? static_cast<U32BIT>(d) : 1000u;
    return ScopeError();
}

ScopeError CDSOS204APlugin::setTriggerMode(U32BIT s, Enum_Scope_TriggerMode v)
{
    if (v == Enum_Scope_TriggerMode::m_enumSingle)
    {
        return single(s);
    }
    return sendChecked(s, QByteArray(":TRIGger:SWEep ") +
                              ((v == Enum_Scope_TriggerMode::m_enumNormal) ? "NORMal" : "AUTO"));
}
ScopeError CDSOS204APlugin::getTriggerMode(U32BIT s, Enum_Scope_TriggerMode& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":TRIGger:SWEep?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    o = QString::fromLatin1(r).trimmed().toUpper().startsWith(QStringLiteral("NORM"))
            ? Enum_Scope_TriggerMode::m_enumNormal
            : Enum_Scope_TriggerMode::m_enumAuto;
    return ScopeError();
}
ScopeError CDSOS204APlugin::setTriggerSource(U32BIT s, Enum_Scope_TriggerSource v)
{
    const int ch = chanOf(v);
    const QByteArray tok =
        (ch > 0) ? (QByteArray("CHANnel") + QByteArray::number(ch)) : QByteArray("EXTernal");
    return sendChecked(s, QByteArray(":TRIGger:EDGE:SOURce ") + tok);
}
ScopeError CDSOS204APlugin::getTriggerSource(U32BIT s, Enum_Scope_TriggerSource& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":TRIGger:EDGE:SOURce?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    if (t.contains(QStringLiteral("2")))
    {
        o = Enum_Scope_TriggerSource::m_enumCh2;
    }
    else if (t.contains(QStringLiteral("3")))
    {
        o = Enum_Scope_TriggerSource::m_enumCh3;
    }
    else if (t.contains(QStringLiteral("4")))
    {
        o = Enum_Scope_TriggerSource::m_enumCh4;
    }
    else if (t.contains(QStringLiteral("EXT")))
    {
        o = Enum_Scope_TriggerSource::m_enumExt;
    }
    else
    {
        o = Enum_Scope_TriggerSource::m_enumCh1;
    }
    return ScopeError();
}
ScopeError CDSOS204APlugin::setTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope v)
{
    const char* tok = (v == Enum_Scope_TriggerSlope::m_enumFalling)  ? "NEGative"
                      : (v == Enum_Scope_TriggerSlope::m_enumEither) ? "EITHer"
                                                                     : "POSitive";
    return sendChecked(s, QByteArray(":TRIGger:EDGE:SLOPe ") + tok);
}
ScopeError CDSOS204APlugin::getTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":TRIGger:EDGE:SLOPe?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    o = t.startsWith(QStringLiteral("NEG"))   ? Enum_Scope_TriggerSlope::m_enumFalling
        : t.startsWith(QStringLiteral("EIT")) ? Enum_Scope_TriggerSlope::m_enumEither
                                              : Enum_Scope_TriggerSlope::m_enumRising;
    return ScopeError();
}
ScopeError CDSOS204APlugin::setTriggerLevel(U32BIT s, U32BIT c, FDOUBLE v)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray(":TRIGger:EDGE:LEVel ") + fmtD(v) + ",CHANnel" + QByteArray::number(c));
}
ScopeError CDSOS204APlugin::getTriggerLevel(U32BIT s, U32BIT c, FDOUBLE& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return queryDouble(s, QByteArrayLiteral(":TRIGger:EDGE:LEVel?"), o);
}
ScopeError CDSOS204APlugin::setTriggerHoldoff(U32BIT s, FDOUBLE v)
{
    return setDouble(s, ":TRIGger:HOLDoff", v, m_pLimits->m_dTrigHoldoffMin, m_pLimits->m_dTrigHoldoffMax,
                     "holdoff");
}
ScopeError CDSOS204APlugin::getTriggerHoldoff(U32BIT s, FDOUBLE& o)
{
    return queryDouble(s, QByteArrayLiteral(":TRIGger:HOLDoff?"), o);
}
ScopeError CDSOS204APlugin::getTriggerState(U32BIT s, Enum_Scope_TriggerState& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":TRIGger:STATus?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    if (t.contains(QStringLiteral("TRIG")))
    {
        o = Enum_Scope_TriggerState::m_enumTriggered;
    }
    else if (t.contains(QStringLiteral("AUTO")))
    {
        o = Enum_Scope_TriggerState::m_enumAuto;
    }
    else if (t.contains(QStringLiteral("WAIT")) || t.contains(QStringLiteral("ARM")))
    {
        o = Enum_Scope_TriggerState::m_enumReady;
    }
    else
    {
        o = Enum_Scope_TriggerState::m_enumTriggered;
    }
    return ScopeError();
}

ScopeError CDSOS204APlugin::setAcqMode(U32BIT s, Enum_Scope_AcqMode v)
{
    const char* tok = "NORMal";
    switch (v)
    {
    case Enum_Scope_AcqMode::m_enumPeakDetect:
        tok = "PEAK";
        break;
    case Enum_Scope_AcqMode::m_enumAverage:
        tok = "AVERage";
        break;
    case Enum_Scope_AcqMode::m_enumHiRes:
        tok = "HRESolution";
        break;
    default:
        break;
    }
    return sendChecked(s, QByteArray(":ACQuire:TYPE ") + tok);
}
ScopeError CDSOS204APlugin::getAcqMode(U32BIT s, Enum_Scope_AcqMode& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":ACQuire:TYPE?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    if (t.startsWith(QStringLiteral("PEAK")))
    {
        o = Enum_Scope_AcqMode::m_enumPeakDetect;
    }
    else if (t.startsWith(QStringLiteral("AVER")))
    {
        o = Enum_Scope_AcqMode::m_enumAverage;
    }
    else if (t.startsWith(QStringLiteral("HRES")))
    {
        o = Enum_Scope_AcqMode::m_enumHiRes;
    }
    else
    {
        o = Enum_Scope_AcqMode::m_enumSample;
    }
    return ScopeError();
}
ScopeError CDSOS204APlugin::setAverageCount(U32BIT s, U32BIT v)
{
    if (v < 1 || static_cast<int>(v) > m_pLimits->m_iAvgCountMax)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("average count %1 out of range").arg(v));
    }
    return sendChecked(s, QByteArray(":ACQuire:COUNt ") + QByteArray::number(v));
}
ScopeError CDSOS204APlugin::getAverageCount(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral(":ACQuire:COUNt?"), d);
    if (e.isSuccess())
    {
        o = static_cast<U32BIT>(d);
    }
    return e;
}
ScopeError CDSOS204APlugin::getAcquisitionState(U32BIT s, Enum_Scope_AcqState& o)
{
    S_DeviceInstance* d = dev(s);
    if (!d || !d->m_bConnected)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    }
    o = d->m_bRunning ? Enum_Scope_AcqState::m_enumRunning : Enum_Scope_AcqState::m_enumStopped;
    return ScopeError();
}

ScopeError CDSOS204APlugin::setWaveformSource(U32BIT s, Enum_Scope_WaveformSource v, U32BIT i)
{
    if (v != Enum_Scope_WaveformSource::m_enumChannel)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    if (!validChannel(i))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray(":WAVeform:SOURce CHANnel") + QByteArray::number(i));
}
ScopeError CDSOS204APlugin::setWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat v)
{
    const char* tok = (v == Enum_Scope_WaveformFormat::m_enumByte)    ? "BYTE"
                      : (v == Enum_Scope_WaveformFormat::m_enumAscii) ? "ASCii"
                                                                      : "WORD";
    ScopeError e = sendChecked(s, QByteArray(":WAVeform:FORMat ") + tok);
    if (!e.isSuccess())
    {
        return e;
    }
    return sendChecked(s, QByteArrayLiteral(":WAVeform:BYTeorder MSBFirst"));
}
ScopeError CDSOS204APlugin::setWaveformPoints(U32BIT s, U32BIT v)
{
    if (v < 1 || v > m_pLimits->m_u32MaxMemoryDepth)
    {
        return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
                          QStringLiteral("waveform points %1 out of range").arg(v));
    }
    return sendChecked(s, QByteArray(":WAVeform:POINts ") + QByteArray::number(v));
}
ScopeError CDSOS204APlugin::getWaveformPreamble(U32BIT s, U32BIT c, S_Scope_WaveformPreamble& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":WAVeform:PREamble?"), r);
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
ScopeError CDSOS204APlugin::readWaveform(U32BIT s, U32BIT c, S_Scope_Waveform& o)
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
    e = writeLine(s, QByteArrayLiteral(":WAVeform:DATA?"));
    if (!e.isSuccess())
    {
        return e;
    }
    QByteArray payload;
    e = readBinaryBlock(s, payload);
    if (!e.isSuccess())
    {
        return e;
    }
    const S_Scope_WaveformPreamble& pr = o.m_sPreamble;
    const int n = payload.size() / 2;
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
ScopeError CDSOS204APlugin::digitizeChannel(U32BIT s, U32BIT c)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray(":DIGitize CHANnel") + QByteArray::number(c));
}

ScopeError CDSOS204APlugin::captureScreenshot(U32BIT s, Enum_Scope_ImageFormat f, QByteArray& o)
{
    const char* fmt = (f == Enum_Scope_ImageFormat::m_enumBmp) ? "BMP" : "PNG";
    ScopeError e = writeLine(s, QByteArray(":DISPlay:DATA? ") + fmt);
    if (!e.isSuccess())
    {
        return e;
    }
    return readBinaryBlock(s, o);
}
ScopeError CDSOS204APlugin::readErrorStatus(U32BIT s, U32BIT, S_Scope_DeviceErrorStatus& o)
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
    queryLine(s, QByteArrayLiteral(":SYSTem:ERRor?"), q);
    o.m_StrErrorMessage = QString::fromLatin1(q).trimmed();
    return ScopeError();
}
ScopeError CDSOS204APlugin::clearErrorStatus(U32BIT s, U32BIT)
{
    return writeLine(s, QByteArrayLiteral("*CLS"));
}
ScopeError CDSOS204APlugin::queryErrorQueue(U32BIT s, QString& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":SYSTem:ERRor?"), r);
    if (e.isSuccess())
    {
        o = QString::fromLatin1(r).trimmed();
    }
    return e;
}
ScopeError CDSOS204APlugin::readStatusByte(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("*STB?"), d);
    if (e.isSuccess())
    {
        o = static_cast<U32BIT>(d);
    }
    return e;
}
ScopeError CDSOS204APlugin::readStandardEventStatus(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral("*ESR?"), d);
    if (e.isSuccess())
    {
        o = static_cast<U32BIT>(d);
    }
    return e;
}
ScopeError CDSOS204APlugin::writeScpi(U32BIT s, const QString& v)
{
    return writeLine(s, v.toLatin1());
}
ScopeError CDSOS204APlugin::queryScpi(U32BIT s, const QString& v, QString& o)
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
 *  M5 feature slices - Keysight InfiniiVision dialect.
 *==========================================================================*/
namespace
{
const char* ksMeasTok(Enum_Scope_MeasType t)
{
    switch (t)
    {
    case Enum_Scope_MeasType::m_enumVpp:
        return "VPP";
    case Enum_Scope_MeasType::m_enumVmax:
        return "VMAX";
    case Enum_Scope_MeasType::m_enumVmin:
        return "VMIN";
    case Enum_Scope_MeasType::m_enumVrms:
        return "VRMS";
    case Enum_Scope_MeasType::m_enumVavg:
        return "VAVerage";
    case Enum_Scope_MeasType::m_enumFrequency:
        return "FREQuency";
    case Enum_Scope_MeasType::m_enumPeriod:
        return "PERiod";
    case Enum_Scope_MeasType::m_enumRiseTime:
        return "RISetime";
    case Enum_Scope_MeasType::m_enumFallTime:
        return "FALLtime";
    case Enum_Scope_MeasType::m_enumPosWidth:
        return "PWIDth";
    case Enum_Scope_MeasType::m_enumNegWidth:
        return "NWIDth";
    case Enum_Scope_MeasType::m_enumDutyCycle:
        return "DUTYcycle";
    case Enum_Scope_MeasType::m_enumPhase:
        return "PHASe";
    case Enum_Scope_MeasType::m_enumDelay:
        return "DELay";
    default:
        return "FREQuency";
    }
}
QString ksMeasUnits(Enum_Scope_MeasType t)
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

ScopeError CDSOS204APlugin::addMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray(":MEASure:") + ksMeasTok(t) + " CHANnel" + QByteArray::number(c));
}
ScopeError CDSOS204APlugin::readMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t,
                                            S_Scope_MeasurementResult& o)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    FDOUBLE v = 0.0;
    ScopeError e =
        queryDouble(s, QByteArray(":MEASure:") + ksMeasTok(t) + "? CHANnel" + QByteArray::number(c), v);
    if (!e.isSuccess())
    {
        return e;
    }
    o.m_eType = t;
    o.m_dValue = v;
    o.m_bValid = true;
    o.m_strUnits = ksMeasUnits(t);
    o.m_dMean = v;
    o.m_dMin = v;
    o.m_dMax = v;
    o.m_dStdDev = 0.0;
    return ScopeError();
}
ScopeError CDSOS204APlugin::clearMeasurements(U32BIT s)
{
    return writeLine(s, QByteArrayLiteral(":MEASure:CLEar"));
}
ScopeError CDSOS204APlugin::setMeasureStatistics(U32BIT s, bool v)
{
    return sendChecked(s, QByteArray(":MEASure:STATistics ") + (v ? "ON" : "OFF"));
}
ScopeError CDSOS204APlugin::getMeasurementStatistics(U32BIT s, U32BIT c, Enum_Scope_MeasType t,
                                                     S_Scope_MeasurementResult& o)
{
    return readMeasurement(s, c, t, o);
}

ScopeError CDSOS204APlugin::setCursorType(U32BIT s, Enum_Scope_CursorType v)
{
    const char* tok = (v == Enum_Scope_CursorType::m_enumOff)     ? "OFF"
                      : (v == Enum_Scope_CursorType::m_enumTrack) ? "WAVeform"
                                                                  : "MANual";
    return sendChecked(s, QByteArray(":MARKer:MODE ") + tok);
}
ScopeError CDSOS204APlugin::getCursorType(U32BIT s, Enum_Scope_CursorType& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":MARKer:MODE?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    const QString t = QString::fromLatin1(r).trimmed().toUpper();
    if (t.startsWith(QStringLiteral("OFF")))
    {
        o = Enum_Scope_CursorType::m_enumOff;
    }
    else if (t.startsWith(QStringLiteral("WAV")))
    {
        o = Enum_Scope_CursorType::m_enumTrack;
    }
    else
    {
        o = Enum_Scope_CursorType::m_enumVertical;
    }
    return ScopeError();
}
ScopeError CDSOS204APlugin::setCursorSource(U32BIT s, U32BIT c)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray(":MARKer:X1Y1source CHANnel") + QByteArray::number(c));
}
ScopeError CDSOS204APlugin::setCursorPosition(U32BIT s, U32BIT i, FDOUBLE v)
{
    return sendChecked(s,
                       QByteArray(":MARKer:X") + QByteArray::number(i == 0 ? 1 : i) + "Position " + fmtD(v));
}
ScopeError CDSOS204APlugin::getCursorPosition(U32BIT s, U32BIT i, FDOUBLE& o)
{
    return queryDouble(s, QByteArray(":MARKer:X") + QByteArray::number(i == 0 ? 1 : i) + "Position?", o);
}
ScopeError CDSOS204APlugin::readCursorValues(U32BIT s, FDOUBLE& x1, FDOUBLE& x2, FDOUBLE& y1, FDOUBLE& y2)
{
    queryDouble(s, QByteArrayLiteral(":MARKer:X1Position?"), x1);
    queryDouble(s, QByteArrayLiteral(":MARKer:X2Position?"), x2);
    queryDouble(s, QByteArrayLiteral(":MARKer:Y1Position?"), y1);
    queryDouble(s, QByteArrayLiteral(":MARKer:Y2Position?"), y2);
    return ScopeError();
}

ScopeError CDSOS204APlugin::setMathOperation(U32BIT s, Enum_Scope_MathOp v)
{
    const char* op = "ADD";
    switch (v)
    {
    case Enum_Scope_MathOp::m_enumSub:
        op = "SUBTract";
        break;
    case Enum_Scope_MathOp::m_enumMult:
        op = "MULTiply";
        break;
    case Enum_Scope_MathOp::m_enumDiv:
        op = "DIVide";
        break;
    case Enum_Scope_MathOp::m_enumFFT:
        op = "FFT";
        break;
    default:
        break;
    }
    return sendChecked(s, QByteArray(":FUNCtion:OPERation ") + op);
}
ScopeError CDSOS204APlugin::setMathSource1(U32BIT s, U32BIT c)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray(":FUNCtion:SOURce1 CHANnel") + QByteArray::number(c));
}
ScopeError CDSOS204APlugin::setMathSource2(U32BIT s, U32BIT c)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray(":FUNCtion:SOURce2 CHANnel") + QByteArray::number(c));
}
ScopeError CDSOS204APlugin::enableMath(U32BIT s, bool v)
{
    return sendChecked(s, QByteArray(":FUNCtion:DISPlay ") + (v ? "ON" : "OFF"));
}
ScopeError CDSOS204APlugin::setFftWindow(U32BIT s, Enum_Scope_FftWindow v)
{
    const char* tok = "HANNing";
    switch (v)
    {
    case Enum_Scope_FftWindow::m_enumRect:
        tok = "RECTangular";
        break;
    case Enum_Scope_FftWindow::m_enumHamming:
        tok = "HAMMing";
        break;
    case Enum_Scope_FftWindow::m_enumBlackman:
        tok = "BHARris";
        break;
    case Enum_Scope_FftWindow::m_enumFlattop:
        tok = "FLATtop";
        break;
    default:
        break;
    }
    return sendChecked(s, QByteArray(":FUNCtion:FFT:WINDow ") + tok);
}
ScopeError CDSOS204APlugin::getFftWindow(U32BIT s, Enum_Scope_FftWindow& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":FUNCtion:FFT:WINDow?"), r);
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
    else if (t.startsWith(QStringLiteral("BHAR")))
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
ScopeError CDSOS204APlugin::setFftSpan(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray(":FUNCtion:FFT:SPAN ") + fmtD(v));
}
ScopeError CDSOS204APlugin::setFftCenter(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray(":FUNCtion:FFT:CENTer ") + fmtD(v));
}
ScopeError CDSOS204APlugin::setMathScale(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray(":FUNCtion:SCALe ") + fmtD(v));
}
ScopeError CDSOS204APlugin::setMathPosition(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray(":FUNCtion:OFFSet ") + fmtD(v));
}

ScopeError CDSOS204APlugin::setPersistence(U32BIT s, FDOUBLE v)
{
    return sendChecked(s,
                       QByteArray(":DISPlay:PERSistence ") + ((v > 0.0) ? fmtD(v) : QByteArray("MINimum")));
}
ScopeError CDSOS204APlugin::setGraticule(U32BIT s, const QString& v)
{
    return sendChecked(s, QByteArray(":DISPlay:GRATicule ") + v.toLatin1());
}
ScopeError CDSOS204APlugin::setIntensity(U32BIT s, FDOUBLE v)
{
    return sendChecked(s, QByteArray(":DISPlay:INTensity:WAVeform ") + fmtD(v));
}
ScopeError CDSOS204APlugin::setDisplayFormat(U32BIT s, Enum_Scope_TimebaseMode v)
{
    return sendChecked(s, QByteArray(":TIMebase:MODE ") +
                              ((v == Enum_Scope_TimebaseMode::m_enumXY) ? "XY" : "MAIN"));
}
ScopeError CDSOS204APlugin::setVectors(U32BIT s, bool v)
{
    return sendChecked(s, QByteArray(":DISPlay:VECTors ") + (v ? "ON" : "OFF"));
}

ScopeError CDSOS204APlugin::saveSetup(U32BIT s, U32BIT loc)
{
    return sendChecked(s, QByteArray("*SAV ") + QByteArray::number(loc));
}
ScopeError CDSOS204APlugin::recallSetup(U32BIT s, U32BIT loc)
{
    return sendChecked(s, QByteArray("*RCL ") + QByteArray::number(loc));
}
ScopeError CDSOS204APlugin::saveWaveformToFile(U32BIT s, U32BIT c, const QString& path)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    return sendChecked(s, QByteArray(":SAVE:WAVeform:STARt \"") + path.toLatin1() + "\"");
}
ScopeError CDSOS204APlugin::saveToReference(U32BIT s, U32BIT c, U32BIT slot)
{
    if (!validChannel(c))
    {
        return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
    }
    ScopeError e = sendChecked(s, QByteArray(":REFerence:SOURce CHANnel") + QByteArray::number(c));
    if (!e.isSuccess())
    {
        return e;
    }
    return sendChecked(s, QByteArray(":REFerence:SAVE ") + QByteArray::number(slot));
}
ScopeError CDSOS204APlugin::displayReference(U32BIT s, U32BIT slot, bool v)
{
    (void)slot;
    return sendChecked(s, QByteArray(":REFerence:DISPlay ") + (v ? "ON" : "OFF"));
}

ScopeError CDSOS204APlugin::enableDigitalChannel(U32BIT s, U32BIT d, bool v)
{
    if (!m_pLimits->m_bHasDigital)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray(":DIGital") + QByteArray::number(d) + ":DISPlay " + (v ? "ON" : "OFF"));
}
ScopeError CDSOS204APlugin::setDigitalThreshold(U32BIT s, U32BIT d, FDOUBLE v)
{
    if (!m_pLimits->m_bHasDigital)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray(":DIGital") + QByteArray::number(d) + ":THReshold " + fmtD(v));
}
ScopeError CDSOS204APlugin::setPodThreshold(U32BIT s, U32BIT p, FDOUBLE v)
{
    if (!m_pLimits->m_bHasDigital)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray(":POD") + QByteArray::number(p) + ":THReshold " + fmtD(v));
}
ScopeError CDSOS204APlugin::enableBus(U32BIT s, U32BIT b, bool v)
{
    if (!m_pLimits->m_bHasSerialDecode)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray(":SBUS") + QByteArray::number(b) + ":DISPlay " + (v ? "ON" : "OFF"));
}
ScopeError CDSOS204APlugin::setBusType(U32BIT s, U32BIT b, const QString& v)
{
    if (!m_pLimits->m_bHasSerialDecode)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray(":SBUS") + QByteArray::number(b) + ":MODE " + v.toLatin1());
}
ScopeError CDSOS204APlugin::readBusDecode(U32BIT s, U32BIT b, QString& o)
{
    if (!m_pLimits->m_bHasSerialDecode)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    // TODO(manual): Keysight exposes decode via :SBUS<n>:...:DATA / lister; this
    // DATA? read is the mock-facing form until the Programmer's Guide is applied.
    QByteArray r;
    ScopeError e = queryLine(s, QByteArray(":SBUS") + QByteArray::number(b) + ":DATA?", r);
    if (e.isSuccess())
    {
        o = QString::fromLatin1(r).trimmed();
    }
    return e;
}

ScopeError CDSOS204APlugin::setAwgFunction(U32BIT s, const QString& v)
{
    if (!m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray(":WGEN:FUNCtion ") + v.toLatin1());
}
ScopeError CDSOS204APlugin::setAwgFrequency(U32BIT s, FDOUBLE v)
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
    return sendChecked(s, QByteArray(":WGEN:FREQuency ") + fmtD(v));
}
ScopeError CDSOS204APlugin::setAwgAmplitude(U32BIT s, FDOUBLE v)
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
    return sendChecked(s, QByteArray(":WGEN:VOLTage ") + fmtD(v));
}
ScopeError CDSOS204APlugin::setAwgOffset(U32BIT s, FDOUBLE v)
{
    if (!m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray(":WGEN:VOLTage:OFFSet ") + fmtD(v));
}
ScopeError CDSOS204APlugin::enableAwgOutput(U32BIT s, bool v)
{
    if (!m_pLimits->m_bHasAWG)
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    return sendChecked(s, QByteArray(":WGEN:OUTPut ") + (v ? "ON" : "OFF"));
}

ScopeError CDSOS204APlugin::readOperationStatus(U32BIT s, U32BIT& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral(":OPERegister:CONDition?"), d);
    o = e.isSuccess() ? static_cast<U32BIT>(d) : 0u;
    return ScopeError();
}
ScopeError CDSOS204APlugin::readQuestionableStatus(U32BIT s, U32BIT& o)
{
    (void)s;
    o = 0u;
    return ScopeError();
}
ScopeError CDSOS204APlugin::getInstrumentErrorCount(U32BIT s, U32BIT& o)
{
    QByteArray r;
    ScopeError e = queryLine(s, QByteArrayLiteral(":SYSTem:ERRor?"), r);
    if (!e.isSuccess())
    {
        return e;
    }
    o = r.trimmed().startsWith('0') ? 0u : 1u;
    return ScopeError();
}
ScopeError CDSOS204APlugin::setRemoteState(U32BIT s, Enum_Scope_RemoteState v)
{
    (void)v;
    return writeLine(s, QByteArrayLiteral("*OPC"));
}
ScopeError CDSOS204APlugin::getRemoteState(U32BIT s, Enum_Scope_RemoteState& o)
{
    (void)s;
    o = Enum_Scope_RemoteState::m_enumRemote;
    return ScopeError();
}
ScopeError CDSOS204APlugin::setKeyLock(U32BIT s, bool v)
{
    return sendChecked(s, QByteArray(":SYSTem:LOCK ") + (v ? "ON" : "OFF"));
}
ScopeError CDSOS204APlugin::isKeyLocked(U32BIT s, bool& o)
{
    FDOUBLE d = 0;
    ScopeError e = queryDouble(s, QByteArrayLiteral(":SYSTem:LOCK?"), d);
    if (e.isSuccess())
    {
        o = (d != 0.0);
    }
    return e;
}
ScopeError CDSOS204APlugin::setBeeper(U32BIT s, bool v)
{
    (void)v;
    return writeLine(s, QByteArrayLiteral("*CLS"));
}
