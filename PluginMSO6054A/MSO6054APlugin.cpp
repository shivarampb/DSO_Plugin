/*============================================================================
 *  MSO6054APlugin.cpp - Keysight InfiniiVision 2000 X-Series plugin.
 *  SCPI over VISA (direct), one session per scope. The embedded m_limits row
 *  is copied from the shared catalog at construction (the plugin owns its own
 *  copy; no shared base class).
 *
 *  Dialect: Keysight InfiniiVision Programmer's Guide. Exercised against the
 *  MockVisa emulator; TODO(manual) constants await manuals/MSO6054A/.
 *==========================================================================*/
#include "MSO6054APlugin.h"
#include "VisaHelper.h"

#include <QByteArray>
#include <QStringList>
#include <cmath>
#include <cstdio>

/* The one model identity this class carries; clones override only this line. */
#define KS_MODEL_NAME "MSO6054A"

CMSO6054APlugin::CMSO6054APlugin()
    : m_pLimits(nullptr)
    , m_strModel(QStringLiteral(KS_MODEL_NAME))
{
    const S_ScopeLimits* p = ScopeFindLimits(KS_MODEL_NAME);
    if (p != nullptr) m_limits = *p;                 // own a copy of the row
    else { std::memset(&m_limits, 0, sizeof(m_limits)); m_limits.m_szModelName = KS_MODEL_NAME;
           m_limits.m_iAnalogChannels = 2; m_limits.m_dVertScaleMin = 1e-3; m_limits.m_dVertScaleMax = 5.0;
           m_limits.m_dTimebaseMin = 5e-9; m_limits.m_dTimebaseMax = 50.0; m_limits.m_dVertOffsetMax = 100.0;
           m_limits.m_u32MaxMemoryDepth = 100000; m_limits.m_dMaxSampleRate = 2e9; m_limits.m_iAvgCountMax = 65536;
           m_limits.m_szIdnMatch = KS_MODEL_NAME; m_limits.m_szManufacturer = "Keysight"; }
    m_pLimits = &m_limits;
}

CMSO6054APlugin::~CMSO6054APlugin()
{
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        if (it.value().m_vi != VI_NULL) viClose(it.value().m_vi);
        if (it.value().m_rm != VI_NULL) viClose(it.value().m_rm);
    }
    m_devices.clear();
}

S_Scope_PluginInfo CMSO6054APlugin::getPluginInfo() const
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

S_Scope_Capabilities CMSO6054APlugin::getCapabilities() const
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
    for (int c = 1; c <= m_pLimits->m_iAnalogChannels; ++c) {
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

CMSO6054APlugin::S_DeviceInstance* CMSO6054APlugin::dev(U32BIT s)
{ auto it = m_devices.find(s); return (it == m_devices.end()) ? nullptr : &it.value(); }
bool CMSO6054APlugin::validChannel(U32BIT c) const
{ return c >= 1 && static_cast<int>(c) <= m_pLimits->m_iAnalogChannels; }
QByteArray CMSO6054APlugin::fmtD(FDOUBLE v)
{ char sz[40]; std::snprintf(sz, sizeof(sz), "%.6G", v); return QByteArray(sz); }
int CMSO6054APlugin::chanOf(Enum_Scope_TriggerSource e)
{
    switch (e) {
    case Enum_Scope_TriggerSource::m_enumCh1: return 1;
    case Enum_Scope_TriggerSource::m_enumCh2: return 2;
    case Enum_Scope_TriggerSource::m_enumCh3: return 3;
    case Enum_Scope_TriggerSource::m_enumCh4: return 4;
    default: return 0;
    }
}

ScopeError CMSO6054APlugin::visaError(ViStatus st, const QString& ctx)
{
    if (st == VI_ERROR_TMO)
        return ScopeError(Enum_Scope_ErrorCode::COMMUNICATION_TIMEOUT, QStringLiteral("%1: VISA timeout").arg(ctx));
    return ScopeError(Enum_Scope_ErrorCode::COMMUNICATION_ERROR,
                      QStringLiteral("%1: VISA status 0x%2").arg(ctx).arg(static_cast<quint32>(st), 8, 16, QLatin1Char('0')));
}
ScopeError CMSO6054APlugin::writeLine(U32BIT s, const QByteArray& cmd)
{
    S_DeviceInstance* d = dev(s);
    if (d == nullptr || !d->m_bConnected) return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    QByteArray aby = cmd; aby.append('\n');
    ViUInt32 written = 0;
    const ViStatus st = viWrite(d->m_vi, reinterpret_cast<ViConstBuf>(aby.constData()), static_cast<ViUInt32>(aby.size()), &written);
    if (st < VI_SUCCESS) return visaError(st, QStringLiteral("viWrite"));
    return ScopeError();
}
ScopeError CMSO6054APlugin::readLine(U32BIT s, QByteArray& resp)
{
    resp.clear();
    S_DeviceInstance* d = dev(s);
    if (d == nullptr || !d->m_bConnected) return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    viSetAttribute(d->m_vi, VI_ATTR_TERMCHAR_EN, VI_TRUE);
    char chunk[8192];
    for (;;) {
        ViUInt32 got = 0;
        const ViStatus st = viRead(d->m_vi, reinterpret_cast<ViBuf>(chunk), static_cast<ViUInt32>(sizeof(chunk)), &got);
        if (st < VI_SUCCESS) return visaError(st, QStringLiteral("viRead"));
        resp.append(chunk, static_cast<int>(got));
        if (st != VI_SUCCESS_MAX_CNT) break;
        if (resp.size() > 16 * 1024 * 1024) break;
    }
    while (resp.endsWith('\n') || resp.endsWith('\r')) resp.chop(1);
    return ScopeError();
}
ScopeError CMSO6054APlugin::queryLine(U32BIT s, const QByteArray& cmd, QByteArray& resp)
{ ScopeError e = writeLine(s, cmd); if (!e.isSuccess()) return e; return readLine(s, resp); }
ScopeError CMSO6054APlugin::readBinaryBlock(U32BIT s, QByteArray& payload)
{
    payload.clear();
    S_DeviceInstance* d = dev(s);
    if (d == nullptr || !d->m_bConnected) return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    viSetAttribute(d->m_vi, VI_ATTR_TERMCHAR_EN, VI_FALSE);
    QByteArray raw; char chunk[65536];
    for (;;) {
        ViUInt32 got = 0;
        const ViStatus st = viRead(d->m_vi, reinterpret_cast<ViBuf>(chunk), static_cast<ViUInt32>(sizeof(chunk)), &got);
        if (st < VI_SUCCESS) { viSetAttribute(d->m_vi, VI_ATTR_TERMCHAR_EN, VI_TRUE); return visaError(st, QStringLiteral("viRead block")); }
        raw.append(chunk, static_cast<int>(got));
        if (st != VI_SUCCESS_MAX_CNT) break;
        if (raw.size() > 64 * 1024 * 1024) break;
    }
    viSetAttribute(d->m_vi, VI_ATTR_TERMCHAR_EN, VI_TRUE);
    const int iHash = raw.indexOf('#');
    if (iHash < 0 || iHash + 2 > raw.size()) return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE, QStringLiteral("no # block header"));
    const int w = raw[iHash + 1] - '0';
    if (w < 1 || iHash + 2 + w > raw.size()) return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE, QStringLiteral("bad block length prefix"));
    const int len = raw.mid(iHash + 2, w).toInt();
    payload = raw.mid(iHash + 2 + w, len);
    if (payload.size() != len) return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE, QStringLiteral("short block"));
    return ScopeError();
}
ScopeError CMSO6054APlugin::sendChecked(U32BIT s, const QByteArray& cmd)
{
    ScopeError e = writeLine(s, cmd); if (!e.isSuccess()) return e;
    QByteArray resp; e = queryLine(s, QByteArrayLiteral(":SYSTem:ERRor?"), resp);
    if (!e.isSuccess()) return e;
    const int iComma = resp.indexOf(',');
    const int code = (iComma < 0) ? resp.trimmed().toInt() : resp.left(iComma).trimmed().toInt();
    if (code != 0) return ScopeError(Enum_Scope_ErrorCode::INSTRUMENT_ERROR,
                          QStringLiteral("instrument error %1: %2").arg(code).arg(QString::fromLatin1(resp)));
    return ScopeError();
}
ScopeError CMSO6054APlugin::queryDouble(U32BIT s, const QByteArray& cmd, FDOUBLE& o)
{
    QByteArray resp; ScopeError e = queryLine(s, cmd, resp); if (!e.isSuccess()) return e;
    bool ok = false; const double v = resp.trimmed().toDouble(&ok);
    if (!ok) return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE, QStringLiteral("bad numeric \"%1\"").arg(QString::fromLatin1(resp)));
    o = v; return ScopeError();
}
ScopeError CMSO6054APlugin::setDouble(U32BIT s, const char* scpi, FDOUBLE v, FDOUBLE lo, FDOUBLE hi, const char* what)
{
    if (v < lo || v > hi) return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
        QStringLiteral("%1 %2 out of range [%3 .. %4]").arg(QLatin1String(what)).arg(v).arg(lo).arg(hi));
    return sendChecked(s, QByteArray(scpi) + " " + fmtD(v));
}

ScopeError CMSO6054APlugin::connect(U32BIT s, const S_Scope_ConnectionConfig& c)
{
    if (dev(s) != nullptr && m_devices[s].m_bConnected) return ScopeError(Enum_Scope_ErrorCode::ALREADY_CONNECTED);
    S_DeviceInstance d; d.m_sConfig = c; d.m_u32Timeout = c.m_u32Timeout ? c.m_u32Timeout : 5000;
    ViStatus st = viOpenDefaultRM(&d.m_rm);
    if (st < VI_SUCCESS) return visaError(st, QStringLiteral("viOpenDefaultRM"));
    const QByteArray abyRes = c.toVisaResourceString().toLatin1();
    st = viOpen(d.m_rm, abyRes.constData(), 0, 0, &d.m_vi);
    if (st < VI_SUCCESS) { viClose(d.m_rm); return visaError(st, QStringLiteral("viOpen")); }
    viSetAttribute(d.m_vi, VI_ATTR_TMO_VALUE, d.m_u32Timeout);
    viSetAttribute(d.m_vi, VI_ATTR_TERMCHAR, static_cast<ViAttrState>('\n'));
    viSetAttribute(d.m_vi, VI_ATTR_TERMCHAR_EN, VI_TRUE);
    viSetAttribute(d.m_vi, VI_ATTR_SEND_END_EN, VI_TRUE);
    d.m_bConnected = true; m_devices[s] = d;
    QByteArray idn; ScopeError e = queryLine(s, QByteArrayLiteral("*IDN?"), idn);
    if (!e.isSuccess()) { disconnect(s); return e; }
    if (!QString::fromLatin1(idn).contains(m_pLimits->m_szIdnMatch, Qt::CaseInsensitive)) {
        const QString strIdn = QString::fromLatin1(idn).trimmed(); disconnect(s);
        return ScopeError(Enum_Scope_ErrorCode::CONNECTION_FAILED, QStringLiteral("*IDN? \"%1\" does not name %2").arg(strIdn, m_strModel));
    }
    m_devices[s].m_strIdn = QString::fromLatin1(idn).trimmed();
    return ScopeError();
}
ScopeError CMSO6054APlugin::disconnect(U32BIT s)
{
    S_DeviceInstance* d = dev(s);
    if (d == nullptr) return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    if (d->m_vi != VI_NULL) viClose(d->m_vi);
    if (d->m_rm != VI_NULL) viClose(d->m_rm);
    m_devices.remove(s);
    return ScopeError();
}
bool CMSO6054APlugin::isConnected(U32BIT s) const
{ auto it = m_devices.find(s); return it != m_devices.end() && it.value().m_bConnected; }
ScopeError CMSO6054APlugin::reset(U32BIT s)       { return sendChecked(s, QByteArrayLiteral("*RST")); }
ScopeError CMSO6054APlugin::clearStatus(U32BIT s) { return writeLine(s, QByteArrayLiteral("*CLS")); }
ScopeError CMSO6054APlugin::setTimeout(U32BIT s, U32BIT t)
{ S_DeviceInstance* d = dev(s); if (!d) return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED); d->m_u32Timeout = t; viSetAttribute(d->m_vi, VI_ATTR_TMO_VALUE, t); return ScopeError(); }
ScopeError CMSO6054APlugin::getIdentification(U32BIT s, QString& o)
{ QByteArray r; ScopeError e = queryLine(s, QByteArrayLiteral("*IDN?"), r); if (e.isSuccess()) o = QString::fromLatin1(r).trimmed(); return e; }
ScopeError CMSO6054APlugin::selfTest(U32BIT s, S32BIT& o)
{ FDOUBLE d = 0; ScopeError e = queryDouble(s, QByteArrayLiteral("*TST?"), d); if (e.isSuccess()) o = static_cast<S32BIT>(d); return e; }
ScopeError CMSO6054APlugin::getOptions(U32BIT s, QString& o)
{ QByteArray r; ScopeError e = queryLine(s, QByteArrayLiteral("*OPT?"), r); if (e.isSuccess()) o = QString::fromLatin1(r).trimmed(); return e; }
ScopeError CMSO6054APlugin::waitOperationComplete(U32BIT s) { QByteArray r; return queryLine(s, QByteArrayLiteral("*OPC?"), r); }
ScopeError CMSO6054APlugin::getScpiVersion(U32BIT s, QString& o)
{ if (!isConnected(s)) return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED); o = QStringLiteral("1999.0"); return ScopeError(); }

ScopeError CMSO6054APlugin::getParameterRange(U32BIT s, U32BIT c, Enum_Scope_ParamId p, S_Scope_ParameterRange& o)
{
    if (!isConnected(s)) return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
    const S_ScopeLimits* L = m_pLimits; o = S_Scope_ParameterRange();
    switch (p) {
    case Enum_Scope_ParamId::m_enumVerticalScale: o.m_dMin = L->m_dVertScaleMin; o.m_dMax = L->m_dVertScaleMax; o.m_dResolution = 1e-3; break;
    case Enum_Scope_ParamId::m_enumVerticalOffset:
    case Enum_Scope_ParamId::m_enumVerticalPosition: o.m_dMin = -L->m_dVertOffsetMax; o.m_dMax = L->m_dVertOffsetMax; break;
    case Enum_Scope_ParamId::m_enumTimebaseScale: o.m_dMin = L->m_dTimebaseMin; o.m_dMax = L->m_dTimebaseMax; break;
    case Enum_Scope_ParamId::m_enumTriggerLevel: o.m_dMin = -L->m_dVertOffsetMax; o.m_dMax = L->m_dVertOffsetMax; break;
    case Enum_Scope_ParamId::m_enumTriggerHoldoff: o.m_dMin = L->m_dTrigHoldoffMin; o.m_dMax = L->m_dTrigHoldoffMax; break;
    case Enum_Scope_ParamId::m_enumProbeAttenuation:
        for (int i = 0; i < ScopeShared::PROBE_ATTEN_COUNT; ++i) o.m_QlistDiscreteValues.append(ScopeShared::PROBE_ATTEN_VALUES[i]);
        o.m_dMin = ScopeShared::PROBE_ATTEN_VALUES[0]; o.m_dMax = ScopeShared::PROBE_ATTEN_VALUES[ScopeShared::PROBE_ATTEN_COUNT - 1]; break;
    case Enum_Scope_ParamId::m_enumSampleRate: o.m_dMin = 1.0; o.m_dMax = L->m_dMaxSampleRate; break;
    case Enum_Scope_ParamId::m_enumMemoryDepth: o.m_dMin = 1.0; o.m_dMax = static_cast<double>(L->m_u32MaxMemoryDepth); break;
    case Enum_Scope_ParamId::m_enumAverageCount: o.m_dMin = 1.0; o.m_dMax = static_cast<double>(L->m_iAvgCountMax); break;
    case Enum_Scope_ParamId::m_enumWaveformPoints: o.m_dMin = 1.0; o.m_dMax = static_cast<double>(L->m_u32MaxMemoryDepth); break;
    default: return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
    }
    (void)c; return ScopeError();
}

ScopeError CMSO6054APlugin::autoscale(U32BIT s)    { return sendChecked(s, QByteArrayLiteral(":AUToscale")); }
ScopeError CMSO6054APlugin::run(U32BIT s)          { ScopeError e = writeLine(s, QByteArrayLiteral(":RUN")); if (e.isSuccess()) { S_DeviceInstance* d = dev(s); if (d) d->m_bRunning = true; } return e; }
ScopeError CMSO6054APlugin::stop(U32BIT s)         { ScopeError e = writeLine(s, QByteArrayLiteral(":STOP")); if (e.isSuccess()) { S_DeviceInstance* d = dev(s); if (d) d->m_bRunning = false; } return e; }
ScopeError CMSO6054APlugin::single(U32BIT s)       { ScopeError e = writeLine(s, QByteArrayLiteral(":SINGle")); if (e.isSuccess()) { S_DeviceInstance* d = dev(s); if (d) d->m_bRunning = false; } return e; }
ScopeError CMSO6054APlugin::forceTrigger(U32BIT s) { return writeLine(s, QByteArrayLiteral(":TRIGger:FORCe")); }

ScopeError CMSO6054APlugin::enableChannel(U32BIT s, U32BIT c, bool v)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return sendChecked(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":DISPlay " + (v ? "ON" : "OFF")); }
ScopeError CMSO6054APlugin::isChannelEnabled(U32BIT s, U32BIT c, bool& o)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  FDOUBLE d = 0; ScopeError e = queryDouble(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":DISPlay?", d); if (e.isSuccess()) o = (d != 0.0); return e; }
ScopeError CMSO6054APlugin::setVerticalScale(U32BIT s, U32BIT c, FDOUBLE v)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return setDouble(s, (QByteArray(":CHANnel") + QByteArray::number(c) + ":SCALe").constData(), v, m_pLimits->m_dVertScaleMin, m_pLimits->m_dVertScaleMax, "vertical scale"); }
ScopeError CMSO6054APlugin::getVerticalScale(U32BIT s, U32BIT c, FDOUBLE& o)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return queryDouble(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":SCALe?", o); }
ScopeError CMSO6054APlugin::setVerticalOffset(U32BIT s, U32BIT c, FDOUBLE v)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return setDouble(s, (QByteArray(":CHANnel") + QByteArray::number(c) + ":OFFSet").constData(), v, -m_pLimits->m_dVertOffsetMax, m_pLimits->m_dVertOffsetMax, "vertical offset"); }
ScopeError CMSO6054APlugin::getVerticalOffset(U32BIT s, U32BIT c, FDOUBLE& o)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return queryDouble(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":OFFSet?", o); }
ScopeError CMSO6054APlugin::setCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling v)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  const char* tok = (v == Enum_Scope_Coupling::m_enumAC) ? "AC" : "DC";   // Keysight: AC|DC
  return sendChecked(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":COUPling " + tok); }
ScopeError CMSO6054APlugin::getCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling& o)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  QByteArray r; ScopeError e = queryLine(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":COUPling?", r); if (!e.isSuccess()) return e;
  o = QString::fromLatin1(r).trimmed().toUpper().startsWith(QStringLiteral("AC")) ? Enum_Scope_Coupling::m_enumAC : Enum_Scope_Coupling::m_enumDC; return ScopeError(); }
ScopeError CMSO6054APlugin::setProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE v)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  bool found = false; for (int i = 0; i < ScopeShared::PROBE_ATTEN_COUNT; ++i) if (std::fabs(v - ScopeShared::PROBE_ATTEN_VALUES[i]) < 1e-9) { found = true; break; }
  if (!found) return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE, QStringLiteral("probe attenuation %1 invalid").arg(v));
  return sendChecked(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":PROBe " + fmtD(v)); }
ScopeError CMSO6054APlugin::getProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE& o)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return queryDouble(s, QByteArray(":CHANnel") + QByteArray::number(c) + ":PROBe?", o); }

ScopeError CMSO6054APlugin::setTimebaseScale(U32BIT s, FDOUBLE v)
{ return setDouble(s, ":TIMebase:SCALe", v, m_pLimits->m_dTimebaseMin, m_pLimits->m_dTimebaseMax, "timebase scale"); }
ScopeError CMSO6054APlugin::getTimebaseScale(U32BIT s, FDOUBLE& o) { return queryDouble(s, QByteArrayLiteral(":TIMebase:SCALe?"), o); }
ScopeError CMSO6054APlugin::setTimebasePosition(U32BIT s, FDOUBLE v) { return sendChecked(s, QByteArray(":TIMebase:POSition ") + fmtD(v)); }
ScopeError CMSO6054APlugin::getTimebasePosition(U32BIT s, FDOUBLE& o) { return queryDouble(s, QByteArrayLiteral(":TIMebase:POSition?"), o); }
ScopeError CMSO6054APlugin::getSampleRate(U32BIT s, FDOUBLE& o)
{
    FDOUBLE sr = 0; ScopeError e = queryDouble(s, QByteArrayLiteral(":ACQuire:SRATe?"), sr); if (!e.isSuccess()) return e;
    if (sr <= 0.0) { FDOUBLE tb = 0; getTimebaseScale(s, tb); U32BIT pts = 1000; getMemoryDepth(s, pts);
        sr = (tb > 0.0) ? (static_cast<double>(pts) / (ScopeShared::GRATICULE_HDIV * tb)) : 0.0;
        if (sr > m_pLimits->m_dMaxSampleRate) sr = m_pLimits->m_dMaxSampleRate; }
    o = sr; return ScopeError();
}
ScopeError CMSO6054APlugin::setMemoryDepth(U32BIT s, U32BIT v)
{ if (v < 1 || v > m_pLimits->m_u32MaxMemoryDepth) return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE, QStringLiteral("memory depth %1 out of range").arg(v));
  return sendChecked(s, QByteArray(":ACQuire:POINts ") + QByteArray::number(v)); }
ScopeError CMSO6054APlugin::getMemoryDepth(U32BIT s, U32BIT& o)
{ FDOUBLE d = 0; ScopeError e = queryDouble(s, QByteArrayLiteral(":ACQuire:POINts?"), d); if (!e.isSuccess()) return e; o = (d > 0.0) ? static_cast<U32BIT>(d) : 1000u; return ScopeError(); }

ScopeError CMSO6054APlugin::setTriggerMode(U32BIT s, Enum_Scope_TriggerMode v)
{ if (v == Enum_Scope_TriggerMode::m_enumSingle) return single(s);
  return sendChecked(s, QByteArray(":TRIGger:SWEep ") + ((v == Enum_Scope_TriggerMode::m_enumNormal) ? "NORMal" : "AUTO")); }
ScopeError CMSO6054APlugin::getTriggerMode(U32BIT s, Enum_Scope_TriggerMode& o)
{ QByteArray r; ScopeError e = queryLine(s, QByteArrayLiteral(":TRIGger:SWEep?"), r); if (!e.isSuccess()) return e;
  o = QString::fromLatin1(r).trimmed().toUpper().startsWith(QStringLiteral("NORM")) ? Enum_Scope_TriggerMode::m_enumNormal : Enum_Scope_TriggerMode::m_enumAuto; return ScopeError(); }
ScopeError CMSO6054APlugin::setTriggerSource(U32BIT s, Enum_Scope_TriggerSource v)
{ const int ch = chanOf(v); const QByteArray tok = (ch > 0) ? (QByteArray("CHANnel") + QByteArray::number(ch)) : QByteArray("EXTernal");
  return sendChecked(s, QByteArray(":TRIGger:EDGE:SOURce ") + tok); }
ScopeError CMSO6054APlugin::getTriggerSource(U32BIT s, Enum_Scope_TriggerSource& o)
{ QByteArray r; ScopeError e = queryLine(s, QByteArrayLiteral(":TRIGger:EDGE:SOURce?"), r); if (!e.isSuccess()) return e;
  const QString t = QString::fromLatin1(r).trimmed().toUpper();
  if (t.contains(QStringLiteral("2"))) o = Enum_Scope_TriggerSource::m_enumCh2; else if (t.contains(QStringLiteral("3"))) o = Enum_Scope_TriggerSource::m_enumCh3;
  else if (t.contains(QStringLiteral("4"))) o = Enum_Scope_TriggerSource::m_enumCh4; else if (t.contains(QStringLiteral("EXT"))) o = Enum_Scope_TriggerSource::m_enumExt;
  else o = Enum_Scope_TriggerSource::m_enumCh1;
  return ScopeError(); }
ScopeError CMSO6054APlugin::setTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope v)
{ const char* tok = (v == Enum_Scope_TriggerSlope::m_enumFalling) ? "NEGative" : (v == Enum_Scope_TriggerSlope::m_enumEither) ? "EITHer" : "POSitive";
  return sendChecked(s, QByteArray(":TRIGger:EDGE:SLOPe ") + tok); }
ScopeError CMSO6054APlugin::getTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope& o)
{ QByteArray r; ScopeError e = queryLine(s, QByteArrayLiteral(":TRIGger:EDGE:SLOPe?"), r); if (!e.isSuccess()) return e;
  const QString t = QString::fromLatin1(r).trimmed().toUpper();
  o = t.startsWith(QStringLiteral("NEG")) ? Enum_Scope_TriggerSlope::m_enumFalling : t.startsWith(QStringLiteral("EIT")) ? Enum_Scope_TriggerSlope::m_enumEither : Enum_Scope_TriggerSlope::m_enumRising;
  return ScopeError(); }
ScopeError CMSO6054APlugin::setTriggerLevel(U32BIT s, U32BIT c, FDOUBLE v)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return sendChecked(s, QByteArray(":TRIGger:EDGE:LEVel ") + fmtD(v) + ",CHANnel" + QByteArray::number(c)); }
ScopeError CMSO6054APlugin::getTriggerLevel(U32BIT s, U32BIT c, FDOUBLE& o)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return queryDouble(s, QByteArrayLiteral(":TRIGger:EDGE:LEVel?"), o); }
ScopeError CMSO6054APlugin::setTriggerHoldoff(U32BIT s, FDOUBLE v)
{ return setDouble(s, ":TRIGger:HOLDoff", v, m_pLimits->m_dTrigHoldoffMin, m_pLimits->m_dTrigHoldoffMax, "holdoff"); }
ScopeError CMSO6054APlugin::getTriggerHoldoff(U32BIT s, FDOUBLE& o) { return queryDouble(s, QByteArrayLiteral(":TRIGger:HOLDoff?"), o); }
ScopeError CMSO6054APlugin::getTriggerState(U32BIT s, Enum_Scope_TriggerState& o)
{ QByteArray r; ScopeError e = queryLine(s, QByteArrayLiteral(":TRIGger:STATus?"), r); if (!e.isSuccess()) return e;
  const QString t = QString::fromLatin1(r).trimmed().toUpper();
  if (t.contains(QStringLiteral("TRIG"))) o = Enum_Scope_TriggerState::m_enumTriggered; else if (t.contains(QStringLiteral("AUTO"))) o = Enum_Scope_TriggerState::m_enumAuto;
  else if (t.contains(QStringLiteral("WAIT")) || t.contains(QStringLiteral("ARM"))) o = Enum_Scope_TriggerState::m_enumReady;
  else o = Enum_Scope_TriggerState::m_enumTriggered;
  return ScopeError(); }

ScopeError CMSO6054APlugin::setAcqMode(U32BIT s, Enum_Scope_AcqMode v)
{ const char* tok = "NORMal";
  switch (v) { case Enum_Scope_AcqMode::m_enumPeakDetect: tok = "PEAK"; break; case Enum_Scope_AcqMode::m_enumAverage: tok = "AVERage"; break;
    case Enum_Scope_AcqMode::m_enumHiRes: tok = "HRESolution"; break; default: break; }
  return sendChecked(s, QByteArray(":ACQuire:TYPE ") + tok); }
ScopeError CMSO6054APlugin::getAcqMode(U32BIT s, Enum_Scope_AcqMode& o)
{ QByteArray r; ScopeError e = queryLine(s, QByteArrayLiteral(":ACQuire:TYPE?"), r); if (!e.isSuccess()) return e;
  const QString t = QString::fromLatin1(r).trimmed().toUpper();
  if (t.startsWith(QStringLiteral("PEAK"))) o = Enum_Scope_AcqMode::m_enumPeakDetect; else if (t.startsWith(QStringLiteral("AVER"))) o = Enum_Scope_AcqMode::m_enumAverage;
  else if (t.startsWith(QStringLiteral("HRES"))) o = Enum_Scope_AcqMode::m_enumHiRes; else o = Enum_Scope_AcqMode::m_enumSample;
  return ScopeError(); }
ScopeError CMSO6054APlugin::setAverageCount(U32BIT s, U32BIT v)
{ if (v < 1 || static_cast<int>(v) > m_pLimits->m_iAvgCountMax) return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE, QStringLiteral("average count %1 out of range").arg(v));
  return sendChecked(s, QByteArray(":ACQuire:COUNt ") + QByteArray::number(v)); }
ScopeError CMSO6054APlugin::getAverageCount(U32BIT s, U32BIT& o)
{ FDOUBLE d = 0; ScopeError e = queryDouble(s, QByteArrayLiteral(":ACQuire:COUNt?"), d); if (e.isSuccess()) o = static_cast<U32BIT>(d); return e; }
ScopeError CMSO6054APlugin::getAcquisitionState(U32BIT s, Enum_Scope_AcqState& o)
{ S_DeviceInstance* d = dev(s); if (!d || !d->m_bConnected) return ScopeError(Enum_Scope_ErrorCode::NOT_CONNECTED);
  o = d->m_bRunning ? Enum_Scope_AcqState::m_enumRunning : Enum_Scope_AcqState::m_enumStopped; return ScopeError(); }

ScopeError CMSO6054APlugin::setWaveformSource(U32BIT s, Enum_Scope_WaveformSource v, U32BIT i)
{ if (v != Enum_Scope_WaveformSource::m_enumChannel) return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED);
  if (!validChannel(i)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return sendChecked(s, QByteArray(":WAVeform:SOURce CHANnel") + QByteArray::number(i)); }
ScopeError CMSO6054APlugin::setWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat v)
{ const char* tok = (v == Enum_Scope_WaveformFormat::m_enumByte) ? "BYTE" : (v == Enum_Scope_WaveformFormat::m_enumAscii) ? "ASCii" : "WORD";
  ScopeError e = sendChecked(s, QByteArray(":WAVeform:FORMat ") + tok); if (!e.isSuccess()) return e;
  return sendChecked(s, QByteArrayLiteral(":WAVeform:BYTeorder MSBFirst")); }
ScopeError CMSO6054APlugin::setWaveformPoints(U32BIT s, U32BIT v)
{ if (v < 1 || v > m_pLimits->m_u32MaxMemoryDepth) return ScopeError(Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE, QStringLiteral("waveform points %1 out of range").arg(v));
  return sendChecked(s, QByteArray(":WAVeform:POINts ") + QByteArray::number(v)); }
ScopeError CMSO6054APlugin::getWaveformPreamble(U32BIT s, U32BIT c, S_Scope_WaveformPreamble& o)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  QByteArray r; ScopeError e = queryLine(s, QByteArrayLiteral(":WAVeform:PREamble?"), r); if (!e.isSuccess()) return e;
  const QList<QByteArray> f = r.trimmed().split(','); if (f.size() < 10) return ScopeError(Enum_Scope_ErrorCode::INVALID_RESPONSE, QStringLiteral("short preamble"));
  o.m_iFormat = f[0].trimmed().toInt(); o.m_iType = f[1].trimmed().toInt(); o.m_u32Points = static_cast<U32BIT>(f[2].trimmed().toInt());
  o.m_u32Count = static_cast<U32BIT>(f[3].trimmed().toInt()); o.m_dXIncrement = f[4].trimmed().toDouble(); o.m_dXOrigin = f[5].trimmed().toDouble();
  o.m_dXReference = f[6].trimmed().toDouble(); o.m_dYIncrement = f[7].trimmed().toDouble(); o.m_dYOrigin = f[8].trimmed().toDouble(); o.m_dYReference = f[9].trimmed().toDouble();
  return ScopeError(); }
ScopeError CMSO6054APlugin::readWaveform(U32BIT s, U32BIT c, S_Scope_Waveform& o)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  ScopeError e = setWaveformSource(s, Enum_Scope_WaveformSource::m_enumChannel, c); if (!e.isSuccess()) return e;
  e = setWaveformFormat(s, Enum_Scope_WaveformFormat::m_enumWord); if (!e.isSuccess()) return e;
  e = getWaveformPreamble(s, c, o.m_sPreamble); if (!e.isSuccess()) return e;
  e = writeLine(s, QByteArrayLiteral(":WAVeform:DATA?")); if (!e.isSuccess()) return e;
  QByteArray payload; e = readBinaryBlock(s, payload); if (!e.isSuccess()) return e;
  const S_Scope_WaveformPreamble& pr = o.m_sPreamble; const int n = payload.size() / 2;
  o.m_u32SourceChannel = c; o.m_vecTimeSeconds.resize(n); o.m_vecVolts.resize(n);
  for (int i = 0; i < n; ++i) {
      const qint16 code = static_cast<qint16>((static_cast<quint8>(payload[2 * i]) << 8) | static_cast<quint8>(payload[2 * i + 1]));
      o.m_vecVolts[i] = (static_cast<double>(code) - pr.m_dYReference) * pr.m_dYIncrement + pr.m_dYOrigin;
      o.m_vecTimeSeconds[i] = pr.m_dXOrigin + (static_cast<double>(i) - pr.m_dXReference) * pr.m_dXIncrement; }
  return ScopeError(); }
ScopeError CMSO6054APlugin::digitizeChannel(U32BIT s, U32BIT c)
{ if (!validChannel(c)) return ScopeError(Enum_Scope_ErrorCode::INVALID_CHANNEL);
  return sendChecked(s, QByteArray(":DIGitize CHANnel") + QByteArray::number(c)); }

ScopeError CMSO6054APlugin::captureScreenshot(U32BIT s, Enum_Scope_ImageFormat f, QByteArray& o)
{
    const char* fmt = (f == Enum_Scope_ImageFormat::m_enumBmp) ? "BMP" : "PNG";
    ScopeError e = writeLine(s, QByteArray(":DISPlay:DATA? ") + fmt); if (!e.isSuccess()) return e;
    return readBinaryBlock(s, o);
}
ScopeError CMSO6054APlugin::readErrorStatus(U32BIT s, U32BIT, S_Scope_DeviceErrorStatus& o)
{ o = S_Scope_DeviceErrorStatus(); FDOUBLE esr = 0; ScopeError e = queryDouble(s, QByteArrayLiteral("*ESR?"), esr); if (!e.isSuccess()) return e;
  o.m_iStandardEventStatus = static_cast<S32BIT>(esr); Enum_Scope_AcqState acq = Enum_Scope_AcqState::m_enumStopped; getAcquisitionState(s, acq); o.m_EnumAcqState = acq;
  o.m_statusFlags |= (acq == Enum_Scope_AcqState::m_enumRunning) ? Enum_Scope_DeviceStatusFlag::Running : Enum_Scope_DeviceStatusFlag::Stopped;
  QByteArray q; queryLine(s, QByteArrayLiteral(":SYSTem:ERRor?"), q); o.m_StrErrorMessage = QString::fromLatin1(q).trimmed(); return ScopeError(); }
ScopeError CMSO6054APlugin::clearErrorStatus(U32BIT s, U32BIT) { return writeLine(s, QByteArrayLiteral("*CLS")); }
ScopeError CMSO6054APlugin::queryErrorQueue(U32BIT s, QString& o)
{ QByteArray r; ScopeError e = queryLine(s, QByteArrayLiteral(":SYSTem:ERRor?"), r); if (e.isSuccess()) o = QString::fromLatin1(r).trimmed(); return e; }
ScopeError CMSO6054APlugin::readStatusByte(U32BIT s, U32BIT& o)
{ FDOUBLE d = 0; ScopeError e = queryDouble(s, QByteArrayLiteral("*STB?"), d); if (e.isSuccess()) o = static_cast<U32BIT>(d); return e; }
ScopeError CMSO6054APlugin::readStandardEventStatus(U32BIT s, U32BIT& o)
{ FDOUBLE d = 0; ScopeError e = queryDouble(s, QByteArrayLiteral("*ESR?"), d); if (e.isSuccess()) o = static_cast<U32BIT>(d); return e; }
ScopeError CMSO6054APlugin::writeScpi(U32BIT s, const QString& v) { return writeLine(s, v.toLatin1()); }
ScopeError CMSO6054APlugin::queryScpi(U32BIT s, const QString& v, QString& o)
{ QByteArray r; ScopeError e = queryLine(s, v.toLatin1(), r); if (e.isSuccess()) o = QString::fromLatin1(r).trimmed(); return e; }
