/*============================================================================
 *  AutoTest/main.cpp - console end-to-end suite for the Scope framework.
 *
 *  Exit code = number of failed checks (0 = all pass), so CI can gate on it.
 *  Runs with zero hardware: SimScope synthesizes waveforms with no VISA, and
 *  the real model plugins route VISA to the bundled MockVisa.
 *
 *  Usage:  AutoTest [pluginDir]      (default: ./plugins)
 *==========================================================================*/
#include <QCoreApplication>
#include <QString>
#include <QThread>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <thread>
#include <vector>

#include "ScopeManager.h"
#include "visa.h"

/*----------------------------------------------------------------------------
 * tiny test harness
 *--------------------------------------------------------------------------*/
static int g_iPass = 0;
static int g_iFail = 0;

static void checkTrue(bool in_bCond, const QString& in_strWhat)
{
    if (in_bCond) { ++g_iPass; }
    else { ++g_iFail; std::printf("  [FAIL] %s\n", in_strWhat.toLatin1().constData()); }
}

static void checkOk(const ScopeError& in_e, const QString& in_strWhat)
{
    checkTrue(in_e.isSuccess(), QStringLiteral("%1 -> %2").arg(in_strWhat, in_e.toString()));
}

static void checkCode(const ScopeError& in_e, Enum_Scope_ErrorCode in_eExpect, const QString& in_strWhat)
{
    checkTrue(in_e.code() == in_eExpect,
              QStringLiteral("%1 expected code %2 got %3")
              .arg(in_strWhat).arg(static_cast<int>(in_eExpect)).arg(static_cast<int>(in_e.code())));
}

static bool near(double a, double b, double relTol, double absTol)
{
    return std::fabs(a - b) <= (absTol + relTol * std::fabs(b));
}

static void section(const char* in_szName) { std::printf("[ %s ]\n", in_szName); }

/*----------------------------------------------------------------------------
 * Test groups
 *--------------------------------------------------------------------------*/
static void testDiscovery(CScopeManager& mgr)
{
    section("Plugin discovery");
    const QStringList lst = mgr.getAvailablePlugins();
    std::printf("  discovered: %s\n", lst.join(QStringLiteral(", ")).toLatin1().constData());
    checkTrue(lst.contains(QStringLiteral("SimScope")), QStringLiteral("SimScope discovered"));
}

static void testInstances(CScopeManager& mgr)
{
    section("Instance management");
    checkOk(mgr.createInstance(1, QStringLiteral("SimScope")), QStringLiteral("createInstance(1)"));
    checkTrue(mgr.instanceExists(1), QStringLiteral("instanceExists(1)"));
    checkCode(mgr.createInstance(1, QStringLiteral("SimScope")),
              Enum_Scope_ErrorCode::ALREADY_CONNECTED, QStringLiteral("createInstance dup"));
    checkCode(mgr.createInstance(2, QStringLiteral("NoSuchModel")),
              Enum_Scope_ErrorCode::PLUGIN_NOT_FOUND, QStringLiteral("createInstance bad plugin"));
    checkCode(mgr.reset(99), Enum_Scope_ErrorCode::INVALID_SCOPE_NUMBER, QStringLiteral("op on missing instance"));
    checkOk(mgr.destroyInstance(1), QStringLiteral("destroyInstance(1)"));
    checkTrue(!mgr.instanceExists(1), QStringLiteral("instance gone after destroy"));
}

static void testSimApi(CScopeManager& mgr)
{
    section("SimScope full API + synthesized waveform");
    checkOk(mgr.createInstance(1, QStringLiteral("SimScope")), QStringLiteral("createInstance"));
    S_Scope_ConnectionConfig cfg;
    cfg.setResourceString(QStringLiteral("SIM::MDO34"));
    checkOk(mgr.connect(1, cfg), QStringLiteral("connect SIM::MDO34"));
    checkTrue(mgr.isConnected(1), QStringLiteral("isConnected"));

    QString idn;
    checkOk(mgr.getIdentification(1, idn), QStringLiteral("getIdentification"));
    checkTrue(idn.contains(QStringLiteral("MDO34")), QStringLiteral("IDN names model: ") + idn);

    checkOk(mgr.enableChannel(1, 1, true), QStringLiteral("enableChannel 1"));
    checkOk(mgr.setVerticalScale(1, 1, 0.1), QStringLiteral("setVerticalScale 0.1"));
    FDOUBLE vs = 0.0;
    checkOk(mgr.getVerticalScale(1, 1, vs), QStringLiteral("getVerticalScale"));
    checkTrue(near(vs, 0.1, 0.0, 1e-9), QStringLiteral("vertical scale readback"));

    checkOk(mgr.setTimebaseScale(1, 1.0e-4), QStringLiteral("setTimebaseScale 100us"));
    checkOk(mgr.setTriggerSource(1, Enum_Scope_TriggerSource::m_enumCh1), QStringLiteral("setTriggerSource"));
    checkOk(mgr.setTriggerLevel(1, 1, 0.0), QStringLiteral("setTriggerLevel"));
    checkOk(mgr.setAcqMode(1, Enum_Scope_AcqMode::m_enumSample), QStringLiteral("setAcqMode"));
    checkOk(mgr.single(1), QStringLiteral("single"));

    S_Scope_Waveform wfm;
    checkOk(mgr.readWaveform(1, 1, wfm), QStringLiteral("readWaveform"));
    checkTrue(wfm.pointCount() == 1000, QStringLiteral("waveform point count = 1000"));
    checkTrue(wfm.m_vecTimeSeconds.size() == wfm.m_vecVolts.size(), QStringLiteral("time/volts aligned"));

    // Vpp of a 0.1 V/div sine == 5 * V/div == 0.5 V
    double vmax = wfm.m_vecVolts[0], vmin = wfm.m_vecVolts[0];
    for (double v : wfm.m_vecVolts) { vmax = std::max(vmax, v); vmin = std::min(vmin, v); }
    checkTrue(near(vmax - vmin, 0.5, 0.05, 0.0), QStringLiteral("Vpp ~= 0.5 V (got %1)").arg(vmax - vmin));

    S_Scope_MeasurementResult r;
    checkOk(mgr.readMeasurement(1, 1, Enum_Scope_MeasType::m_enumFrequency, r), QStringLiteral("readMeasurement freq"));
    // 3 cycles across 10 * 100us = 1 ms  ->  3 kHz
    checkTrue(near(r.m_dValue, 3000.0, 0.1, 0.0), QStringLiteral("frequency ~= 3 kHz (got %1)").arg(r.m_dValue));

    checkOk(mgr.readMeasurement(1, 1, Enum_Scope_MeasType::m_enumVpp, r), QStringLiteral("readMeasurement Vpp"));
    checkTrue(near(r.m_dValue, 0.5, 0.05, 0.0), QStringLiteral("meas Vpp ~= 0.5 V"));

    checkOk(mgr.disconnect(1), QStringLiteral("disconnect"));
    checkOk(mgr.destroyInstance(1), QStringLiteral("destroyInstance"));
}

static void testRangeMatrix(CScopeManager& mgr)
{
    section("Per-model range matrix (via SimScope model selection)");

    // MDO34 has 4 channels -> enabling channel 4 succeeds
    mgr.createInstance(1, QStringLiteral("SimScope"));
    S_Scope_ConnectionConfig c1; c1.setResourceString(QStringLiteral("SIM::MDO34"));
    mgr.connect(1, c1);
    checkOk(mgr.enableChannel(1, 4, true), QStringLiteral("MDO34 enable ch4"));
    // DSOS204A accepts a very fast timebase
    mgr.destroyInstance(1);

    mgr.createInstance(2, QStringLiteral("SimScope"));
    S_Scope_ConnectionConfig c2; c2.setResourceString(QStringLiteral("SIM::DSOS204A"));
    mgr.connect(2, c2);
    checkOk(mgr.setTimebaseScale(2, 1.0e-10), QStringLiteral("DSOS204A accepts 100 ps/div"));
    mgr.destroyInstance(2);

    // DSOX2012A has only 2 channels -> enabling channel 4 is rejected
    mgr.createInstance(3, QStringLiteral("SimScope"));
    S_Scope_ConnectionConfig c3; c3.setResourceString(QStringLiteral("SIM::DSOX2012A"));
    mgr.connect(3, c3);
    checkCode(mgr.enableChannel(3, 4, true), Enum_Scope_ErrorCode::INVALID_CHANNEL,
              QStringLiteral("DSOX2012A rejects ch4"));
    mgr.destroyInstance(3);

    // TDS1012B rejects the fast timebase DSOS204A accepts
    mgr.createInstance(4, QStringLiteral("SimScope"));
    S_Scope_ConnectionConfig c4; c4.setResourceString(QStringLiteral("SIM::TDS1012B"));
    mgr.connect(4, c4);
    checkCode(mgr.setTimebaseScale(4, 1.0e-10), Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
              QStringLiteral("TDS1012B rejects 100 ps/div"));
    // getParameterRange reports the model's vertical scale band
    S_Scope_ParameterRange rng;
    checkOk(mgr.getParameterRange(4, 1, Enum_Scope_ParamId::m_enumVerticalScale, rng),
            QStringLiteral("getParameterRange vertical"));
    checkTrue(rng.m_dMax > rng.m_dMin, QStringLiteral("vertical range has span"));
    mgr.destroyInstance(4);
}

static void testErrorPaths(CScopeManager& mgr)
{
    section("Error paths");
    mgr.createInstance(1, QStringLiteral("SimScope"));
    S_Scope_ConnectionConfig cfg; cfg.setResourceString(QStringLiteral("SIM::MDO34"));
    mgr.connect(1, cfg);

    checkCode(mgr.setVerticalScale(1, 1, 1.0e6), Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
              QStringLiteral("vertical scale too large"));
    checkCode(mgr.setVerticalScale(1, 9, 0.1), Enum_Scope_ErrorCode::INVALID_CHANNEL,
              QStringLiteral("invalid channel 9"));
    // setAwgFunction is not overridden by SimScope -> NOT_SUPPORTED default
    checkCode(mgr.setAwgFunction(1, QStringLiteral("SIN")), Enum_Scope_ErrorCode::NOT_SUPPORTED,
              QStringLiteral("un-overridden op returns NOT_SUPPORTED"));
    checkCode(mgr.reset(77), Enum_Scope_ErrorCode::INVALID_SCOPE_NUMBER,
              QStringLiteral("op on non-existent scope"));
    mgr.destroyInstance(1);
}

static void testTwoInstances(CScopeManager& mgr)
{
    section("Two simultaneous instances");
    mgr.createInstance(1, QStringLiteral("SimScope"));
    mgr.createInstance(2, QStringLiteral("SimScope"));
    S_Scope_ConnectionConfig a; a.setResourceString(QStringLiteral("SIM::MDO34::SINE"));
    S_Scope_ConnectionConfig b; b.setResourceString(QStringLiteral("SIM::RTM3004::SQUARE"));
    checkOk(mgr.connect(1, a), QStringLiteral("connect scope 1"));
    checkOk(mgr.connect(2, b), QStringLiteral("connect scope 2"));
    mgr.setVerticalScale(1, 1, 0.2);
    mgr.setVerticalScale(2, 1, 0.5);
    FDOUBLE v1 = 0.0, v2 = 0.0;
    mgr.getVerticalScale(1, 1, v1);
    mgr.getVerticalScale(2, 1, v2);
    checkTrue(near(v1, 0.2, 0, 1e-9) && near(v2, 0.5, 0, 1e-9), QStringLiteral("independent state"));
    mgr.destroyInstance(1);
    mgr.destroyInstance(2);
}

static void testConcurrency(CScopeManager& mgr)
{
    section("Two-thread concurrency (different scope numbers)");
    mgr.createInstance(1, QStringLiteral("SimScope"));
    mgr.createInstance(2, QStringLiteral("SimScope"));
    S_Scope_ConnectionConfig a; a.setResourceString(QStringLiteral("SIM::MDO34"));
    S_Scope_ConnectionConfig b; b.setResourceString(QStringLiteral("SIM::RTM3004"));
    mgr.connect(1, a);
    mgr.connect(2, b);

    std::atomic<int> iErrors(0);
    auto worker = [&mgr, &iErrors](U32BIT scope, double scale) {
        for (int i = 0; i < 2000; ++i) {
            if (!mgr.setVerticalScale(scope, 1, scale).isSuccess()) { ++iErrors; return; }
            FDOUBLE got = 0.0;
            if (!mgr.getVerticalScale(scope, 1, got).isSuccess() || !near(got, scale, 0, 1e-9)) {
                ++iErrors; return;
            }
        }
    };
    std::thread t1(worker, 1, 0.05);
    std::thread t2(worker, 2, 0.02);
    t1.join();
    t2.join();
    checkTrue(iErrors.load() == 0, QStringLiteral("no concurrency errors"));
    mgr.destroyInstance(1);
    mgr.destroyInstance(2);
}

/*----------------------------------------------------------------------------
 * MockVisa direct-path test: exercise the VISA C ABI + emulator that the real
 * model plugins depend on, without a plugin - IDN, binary waveform block
 * round-trip, screenshot block, and the forced-timeout resource.
 *--------------------------------------------------------------------------*/
static void testMockVisa()
{
    section("MockVisa VISA-path (IDN, waveform block, screenshot, timeout)");
    ViSession rm = 0;
    checkTrue(viOpenDefaultRM(&rm) == VI_SUCCESS, QStringLiteral("viOpenDefaultRM"));

    // find resources
    ViFindList fl = 0; ViUInt32 cnt = 0; char desc[256];
    checkTrue(viFindRsrc(rm, "?*INSTR", &fl, &cnt, desc) == VI_SUCCESS && cnt >= 2,
              QStringLiteral("viFindRsrc lists mock resources"));

    ViSession vi = 0;
    checkTrue(viOpen(rm, "MOCK0::MDO34::INSTR", 0, 0, &vi) == VI_SUCCESS, QStringLiteral("viOpen MDO34"));
    viSetAttribute(vi, VI_ATTR_TMO_VALUE, 2000);
    viSetAttribute(vi, VI_ATTR_TERMCHAR, static_cast<ViAttrState>('\n'));
    viSetAttribute(vi, VI_ATTR_TERMCHAR_EN, VI_TRUE);

    // *IDN?
    ViUInt32 nw = 0;
    const char* idnCmd = "*IDN?\n";
    viWrite(vi, reinterpret_cast<ViConstBuf>(idnCmd), 6, &nw);
    char rd[512]; ViUInt32 got = 0;
    ViStatus st = viRead(vi, reinterpret_cast<ViBuf>(rd), sizeof(rd), &got);
    QByteArray idn(rd, static_cast<int>(got));
    checkTrue(st >= VI_SUCCESS && idn.contains("MDO3"), QStringLiteral("mock *IDN? -> ") + QString::fromLatin1(idn.trimmed()));

    // waveform points + preamble + binary block round-trip
    const char* setPts = ":WAV:POIN 500\n";
    viWrite(vi, reinterpret_cast<ViConstBuf>(setPts), static_cast<ViUInt32>(strlen(setPts)), &nw);
    const char* preQ = ":WAV:PRE?\n";
    viWrite(vi, reinterpret_cast<ViConstBuf>(preQ), static_cast<ViUInt32>(strlen(preQ)), &nw);
    got = 0; st = viRead(vi, reinterpret_cast<ViBuf>(rd), sizeof(rd), &got);
    QByteArray pre(rd, static_cast<int>(got));
    const QList<QByteArray> preFields = pre.trimmed().split(',');
    checkTrue(preFields.size() >= 10, QStringLiteral("preamble has 10 fields"));
    const int nPoints = preFields.value(2).toInt();
    checkTrue(nPoints == 500, QStringLiteral("preamble points reflects :WAV:POIN 500"));
    const double yInc = preFields.value(7).toDouble();

    // read the binary data block with termchar disabled
    viSetAttribute(vi, VI_ATTR_TERMCHAR_EN, VI_FALSE);
    const char* dataQ = ":WAV:DATA?\n";
    viWrite(vi, reinterpret_cast<ViConstBuf>(dataQ), static_cast<ViUInt32>(strlen(dataQ)), &nw);
    QByteArray block;
    for (int i = 0; i < 64; ++i) {
        got = 0;
        st = viRead(vi, reinterpret_cast<ViBuf>(rd), sizeof(rd), &got);
        block.append(rd, static_cast<int>(got));
        if (st != VI_SUCCESS_MAX_CNT) break;
    }
    // parse #<w><len><payload>
    bool blockOk = block.size() > 2 && block[0] == '#';
    int payloadLen = 0, hdr = 0;
    if (blockOk) {
        const int w = block[1] - '0';
        payloadLen = block.mid(2, w).toInt();
        hdr = 2 + w;
    }
    checkTrue(blockOk && payloadLen == 2 * nPoints,
              QStringLiteral("waveform block length = 2*points (%1)").arg(payloadLen));

    // decode WORD big-endian codes -> volts, check amplitude ~ 0.4 Vpk
    double vmax = -1e9, vmin = 1e9;
    for (int i = 0; i < payloadLen && hdr + i + 1 < block.size(); i += 2) {
        const qint16 code = static_cast<qint16>(
            (static_cast<quint8>(block[hdr + i]) << 8) | static_cast<quint8>(block[hdr + i + 1]));
        const double v = code * yInc;
        vmax = std::max(vmax, v); vmin = std::min(vmin, v);
    }
    checkTrue(near(vmax, 0.4, 0.05, 0.0) && near(vmin, -0.4, 0.05, 0.0),
              QStringLiteral("decoded sine amplitude ~= +/-0.4 V (got %1/%2)").arg(vmax).arg(vmin));

    // screenshot block (PNG signature)
    viSetAttribute(vi, VI_ATTR_TERMCHAR_EN, VI_FALSE);
    const char* shotQ = ":DISP:DATA?\n";
    viWrite(vi, reinterpret_cast<ViConstBuf>(shotQ), static_cast<ViUInt32>(strlen(shotQ)), &nw);
    QByteArray shot;
    for (int i = 0; i < 8; ++i) {
        got = 0; st = viRead(vi, reinterpret_cast<ViBuf>(rd), sizeof(rd), &got);
        shot.append(rd, static_cast<int>(got));
        if (st != VI_SUCCESS_MAX_CNT) break;
    }
    checkTrue(shot.contains("\x89PNG"), QStringLiteral("screenshot block carries a PNG"));
    viClose(vi);

    // forced-timeout resource never answers a read
    ViSession viT = 0;
    checkTrue(viOpen(rm, "MOCK0::TIMEOUT::INSTR", 0, 0, &viT) == VI_SUCCESS, QStringLiteral("viOpen TIMEOUT"));
    viWrite(viT, reinterpret_cast<ViConstBuf>(idnCmd), 6, &nw);
    got = 0; st = viRead(viT, reinterpret_cast<ViBuf>(rd), sizeof(rd), &got);
    checkTrue(st == VI_ERROR_TMO, QStringLiteral("timeout resource returns VI_ERROR_TMO"));
    viClose(viT);
    viClose(rm);
}

/*----------------------------------------------------------------------------
 * Real model plugins (MDO34, RTM3004) end-to-end via MockVisa, including the
 * binary waveform round-trip (preamble + block -> scaled volts).
 *--------------------------------------------------------------------------*/
static void testRealModel(CScopeManager& mgr, const QString& model, const QString& resource,
                          int analogChannels)
{
    section(QStringLiteral("Real model %1 via MockVisa").arg(model).toLatin1().constData());
    if (!mgr.getAvailablePlugins().contains(model)) {
        std::printf("  [SKIP] %s plugin not discovered\n", model.toLatin1().constData());
        return;
    }
    const U32BIT scope = 10;
    checkOk(mgr.createInstance(scope, model), QStringLiteral("createInstance %1").arg(model));
    S_Scope_ConnectionConfig cfg; cfg.setResourceString(resource);
    checkOk(mgr.connect(scope, cfg), QStringLiteral("connect %1").arg(resource));
    checkTrue(mgr.isConnected(scope), QStringLiteral("isConnected"));

    QString idn;
    checkOk(mgr.getIdentification(scope, idn), QStringLiteral("getIdentification"));

    checkOk(mgr.reset(scope), QStringLiteral("reset (*RST)"));
    checkOk(mgr.enableChannel(scope, 1, true), QStringLiteral("enableChannel 1"));
    // channel-count matrix: a channel beyond this model's count is rejected
    if (analogChannels < 4)
        checkCode(mgr.enableChannel(scope, 4, true), Enum_Scope_ErrorCode::INVALID_CHANNEL,
                  QStringLiteral("%1 (%2ch) rejects ch4").arg(model).arg(analogChannels));
    else
        checkOk(mgr.enableChannel(scope, 4, true), QStringLiteral("%1 accepts ch4").arg(model));
    checkOk(mgr.setVerticalScale(scope, 1, 0.2), QStringLiteral("setVerticalScale"));
    FDOUBLE vs = 0.0;
    checkOk(mgr.getVerticalScale(scope, 1, vs), QStringLiteral("getVerticalScale"));
    checkTrue(near(vs, 0.2, 0, 1e-9), QStringLiteral("vertical scale round-trips through SCPI"));
    checkCode(mgr.setVerticalScale(scope, 1, 1.0e6), Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE,
              QStringLiteral("out-of-range vertical scale rejected locally"));

    checkOk(mgr.setTimebaseScale(scope, 1.0e-6), QStringLiteral("setTimebaseScale"));
    checkOk(mgr.setTriggerSource(scope, Enum_Scope_TriggerSource::m_enumCh1), QStringLiteral("setTriggerSource"));
    checkOk(mgr.setTriggerSlope(scope, Enum_Scope_TriggerSlope::m_enumRising), QStringLiteral("setTriggerSlope"));
    checkOk(mgr.setTriggerLevel(scope, 1, 0.1), QStringLiteral("setTriggerLevel"));
    checkOk(mgr.setAcqMode(scope, Enum_Scope_AcqMode::m_enumSample), QStringLiteral("setAcqMode"));
    checkOk(mgr.single(scope), QStringLiteral("single"));

    // waveform point count control + binary round-trip
    checkOk(mgr.setWaveformPoints(scope, 500), QStringLiteral("setWaveformPoints 500"));
    S_Scope_Waveform wfm;
    checkOk(mgr.readWaveform(scope, 1, wfm), QStringLiteral("readWaveform"));
    checkTrue(wfm.pointCount() == 500, QStringLiteral("waveform reflects 500 points (got %1)").arg(wfm.pointCount()));
    double vmax = -1e9, vmin = 1e9;
    for (double v : wfm.m_vecVolts) { vmax = std::max(vmax, v); vmin = std::min(vmin, v); }
    // mock synth sine is 0.4 Vpk (Vpp 0.8), independent of V/div
    checkTrue(near(vmax - vmin, 0.8, 0.05, 0.0), QStringLiteral("decoded Vpp ~= 0.8 V (got %1)").arg(vmax - vmin));
    checkTrue(wfm.m_sPreamble.m_dXIncrement > 0.0, QStringLiteral("preamble xIncrement > 0"));

    // status + screenshot
    S_Scope_DeviceErrorStatus stx;
    checkOk(mgr.readErrorStatus(scope, 1, stx), QStringLiteral("readErrorStatus"));
    QByteArray png;
    ScopeError shot = mgr.captureScreenshot(scope, Enum_Scope_ImageFormat::m_enumPng, png);
    // captureScreenshot is not in the M4 override set for these models -> NOT_SUPPORTED is acceptable
    checkTrue(shot.isSuccess() || shot.code() == Enum_Scope_ErrorCode::NOT_SUPPORTED,
              QStringLiteral("captureScreenshot ok or NOT_SUPPORTED"));

    checkOk(mgr.disconnect(scope), QStringLiteral("disconnect"));
    checkOk(mgr.destroyInstance(scope), QStringLiteral("destroyInstance"));
}

static void testRealModels(CScopeManager& mgr)
{
    // model, analog channel count (drives the channel-count matrix check)
    struct Row { const char* model; int ch; };
    static const Row rows[] = {
        { "MDO34", 4 }, { "RTM3004", 4 }, { "DSO7104B", 4 }, { "DSOS204A", 4 },
        { "DSOX2012A", 2 }, { "MSO6054A", 4 }, { "RTO2064", 4 },
        { "TDS1012B", 2 }, { "TDS2024C", 4 }, { "WaveSurfer42Xs", 4 },
    };
    for (const Row& r : rows) {
        const QString model = QString::fromLatin1(r.model);
        testRealModel(mgr, model, QStringLiteral("MOCK0::%1::INSTR").arg(model), r.ch);
    }
}

/*----------------------------------------------------------------------------
 * M5 feature slices exercised on one model per dialect (Tek/R&S/Keysight),
 * end-to-end through MockVisa: measurements (values checked), cursors, math,
 * display, save/recall, screenshot, AWG (capability-gated), status/keylock.
 *--------------------------------------------------------------------------*/
static void testFeatureSlicesFor(CScopeManager& mgr, const QString& model)
{
    section(QStringLiteral("M5 feature slices: %1").arg(model).toLatin1().constData());
    if (!mgr.getAvailablePlugins().contains(model)) { std::printf("  [SKIP] %s\n", model.toLatin1().constData()); return; }
    const U32BIT sc = 12;
    if (mgr.instanceExists(sc)) mgr.destroyInstance(sc);
    checkOk(mgr.createInstance(sc, model), QStringLiteral("createInstance"));
    S_Scope_ConnectionConfig cfg; cfg.setResourceString(QStringLiteral("MOCK0::%1::INSTR").arg(model));
    checkOk(mgr.connect(sc, cfg), QStringLiteral("connect"));
    mgr.enableChannel(sc, 1, true);

    // measurements (default 1000 pts -> 3 kHz, 0.8 Vpp, 0.4 Vmax)
    S_Scope_MeasurementResult r;
    checkOk(mgr.addMeasurement(sc, 1, Enum_Scope_MeasType::m_enumFrequency), QStringLiteral("addMeasurement"));
    checkOk(mgr.readMeasurement(sc, 1, Enum_Scope_MeasType::m_enumFrequency, r), QStringLiteral("readMeasurement freq"));
    checkTrue(near(r.m_dValue, 3000.0, 0.02, 0.0), QStringLiteral("%1 freq ~= 3kHz (got %2)").arg(model).arg(r.m_dValue));
    checkOk(mgr.readMeasurement(sc, 1, Enum_Scope_MeasType::m_enumVpp, r), QStringLiteral("readMeasurement Vpp"));
    checkTrue(near(r.m_dValue, 0.8, 0.02, 0.0), QStringLiteral("%1 Vpp ~= 0.8 (got %2)").arg(model).arg(r.m_dValue));
    checkOk(mgr.readMeasurement(sc, 1, Enum_Scope_MeasType::m_enumVmax, r), QStringLiteral("readMeasurement Vmax"));
    checkTrue(near(r.m_dValue, 0.4, 0.02, 0.0), QStringLiteral("%1 Vmax ~= 0.4").arg(model));
    checkOk(mgr.setMeasureStatistics(sc, true), QStringLiteral("setMeasureStatistics"));
    checkOk(mgr.clearMeasurements(sc), QStringLiteral("clearMeasurements"));

    // cursors
    checkOk(mgr.setCursorType(sc, Enum_Scope_CursorType::m_enumVertical), QStringLiteral("setCursorType"));
    Enum_Scope_CursorType ct = Enum_Scope_CursorType::m_enumOff;
    checkOk(mgr.getCursorType(sc, ct), QStringLiteral("getCursorType"));
    checkOk(mgr.setCursorSource(sc, 1), QStringLiteral("setCursorSource"));
    FDOUBLE x1, x2, y1, y2;
    checkOk(mgr.readCursorValues(sc, x1, x2, y1, y2), QStringLiteral("readCursorValues"));

    // math / FFT
    checkOk(mgr.setMathOperation(sc, Enum_Scope_MathOp::m_enumFFT), QStringLiteral("setMathOperation FFT"));
    checkOk(mgr.enableMath(sc, true), QStringLiteral("enableMath"));
    checkOk(mgr.setFftWindow(sc, Enum_Scope_FftWindow::m_enumHann), QStringLiteral("setFftWindow"));
    Enum_Scope_FftWindow fw = Enum_Scope_FftWindow::m_enumRect;
    checkOk(mgr.getFftWindow(sc, fw), QStringLiteral("getFftWindow"));
    checkTrue(fw == Enum_Scope_FftWindow::m_enumHann, QStringLiteral("FFT window round-trips"));

    // display + save/recall + screenshot
    checkOk(mgr.setDisplayFormat(sc, Enum_Scope_TimebaseMode::m_enumMain), QStringLiteral("setDisplayFormat"));
    checkOk(mgr.setVectors(sc, true), QStringLiteral("setVectors"));
    checkOk(mgr.saveSetup(sc, 1), QStringLiteral("saveSetup"));
    checkOk(mgr.recallSetup(sc, 1), QStringLiteral("recallSetup"));
    QByteArray png;
    checkOk(mgr.captureScreenshot(sc, Enum_Scope_ImageFormat::m_enumPng, png), QStringLiteral("captureScreenshot"));
    checkTrue(png.contains("\x89PNG"), QStringLiteral("screenshot is a PNG"));

    // AWG (capability-gated)
    S_Scope_Capabilities caps = mgr.getCapabilities(sc);
    if (caps.m_bHasAWG) {
        checkOk(mgr.setAwgFunction(sc, QStringLiteral("SINE")), QStringLiteral("setAwgFunction"));
        checkOk(mgr.setAwgFrequency(sc, 1000.0), QStringLiteral("setAwgFrequency in-range"));
        checkCode(mgr.setAwgFrequency(sc, 1e12), Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE, QStringLiteral("AWG freq out-of-range rejected"));
        checkOk(mgr.enableAwgOutput(sc, true), QStringLiteral("enableAwgOutput"));
    } else {
        checkCode(mgr.setAwgFrequency(sc, 1000.0), Enum_Scope_ErrorCode::NOT_SUPPORTED, QStringLiteral("AWG NOT_SUPPORTED (no generator)"));
    }

    // digital (capability-gated)
    if (caps.m_bHasDigital)
        checkOk(mgr.enableDigitalChannel(sc, 0, true), QStringLiteral("enableDigitalChannel"));
    else
        checkCode(mgr.enableDigitalChannel(sc, 0, true), Enum_Scope_ErrorCode::NOT_SUPPORTED, QStringLiteral("digital NOT_SUPPORTED (no MSO)"));

    // status / keylock
    checkOk(mgr.setKeyLock(sc, true), QStringLiteral("setKeyLock"));
    bool locked = false;
    checkOk(mgr.isKeyLocked(sc, locked), QStringLiteral("isKeyLocked"));
    U32BIT op = 0;
    checkOk(mgr.readOperationStatus(sc, op), QStringLiteral("readOperationStatus"));

    mgr.disconnect(sc);
    mgr.destroyInstance(sc);
}

static void testFeatureSlices(CScopeManager& mgr)
{
    testFeatureSlicesFor(mgr, QStringLiteral("MDO34"));      // Tektronix
    testFeatureSlicesFor(mgr, QStringLiteral("RTM3004"));    // R&S
    testFeatureSlicesFor(mgr, QStringLiteral("DSOX2012A"));  // Keysight (2ch, no digital)
    testFeatureSlicesFor(mgr, QStringLiteral("MSO6054A"));   // Keysight MSO (digital, no AWG)
}

/*----------------------------------------------------------------------------
 * M7 parity: SimScope and the MockVisa-backed real plugin must report the
 * SAME getParameterRange for a model (both keyed off the shared catalog row),
 * and both must produce a non-empty scaled waveform.
 *--------------------------------------------------------------------------*/
static void testParity(CScopeManager& mgr)
{
    section("M7 SimScope/MockVisa parity (getParameterRange + waveform)");
    static const char* models[] = { "MDO34","RTM3004","DSO7104B","DSOS204A","DSOX2012A",
                                     "MSO6054A","RTO2064","TDS1012B","TDS2024C","WaveSurfer42Xs" };
    for (const char* m : models) {
        const QString model = QString::fromLatin1(m);
        if (!mgr.getAvailablePlugins().contains(model)) continue;
        const U32BIT simN = 20, realN = 21;
        if (mgr.instanceExists(simN)) mgr.destroyInstance(simN);
        if (mgr.instanceExists(realN)) mgr.destroyInstance(realN);
        mgr.createInstance(simN, QStringLiteral("SimScope"));
        mgr.createInstance(realN, model);
        S_Scope_ConnectionConfig cs; cs.setResourceString(QStringLiteral("SIM::%1").arg(model));
        S_Scope_ConnectionConfig cr; cr.setResourceString(QStringLiteral("MOCK0::%1::INSTR").arg(model));
        const bool okS = mgr.connect(simN, cs).isSuccess();
        const bool okR = mgr.connect(realN, cr).isSuccess();
        checkTrue(okS && okR, QStringLiteral("%1: both Sim and real connect").arg(model));
        if (okS && okR) {
            const Enum_Scope_ParamId params[] = {
                Enum_Scope_ParamId::m_enumVerticalScale, Enum_Scope_ParamId::m_enumTimebaseScale,
                Enum_Scope_ParamId::m_enumVerticalOffset, Enum_Scope_ParamId::m_enumAverageCount };
            bool parity = true;
            for (Enum_Scope_ParamId p : params) {
                S_Scope_ParameterRange rs, rr;
                mgr.getParameterRange(simN, 1, p, rs);
                mgr.getParameterRange(realN, 1, p, rr);
                if (!near(rs.m_dMin, rr.m_dMin, 0.0, 1e-9) || !near(rs.m_dMax, rr.m_dMax, 0.0, 1e-9))
                    parity = false;
            }
            checkTrue(parity, QStringLiteral("%1: getParameterRange parity Sim vs real").arg(model));

            mgr.enableChannel(simN, 1, true);
            mgr.enableChannel(realN, 1, true);
            S_Scope_Waveform ws, wr;
            const bool wS = mgr.readWaveform(simN, 1, ws).isSuccess() && ws.pointCount() > 1;
            const bool wR = mgr.readWaveform(realN, 1, wr).isSuccess() && wr.pointCount() > 1;
            checkTrue(wS && wR, QStringLiteral("%1: both produce a non-empty waveform").arg(model));
        }
        mgr.destroyInstance(simN);
        mgr.destroyInstance(realN);
    }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const QString strPluginDir = (argc > 1) ? QString::fromLocal8Bit(argv[1])
                                             : QStringLiteral("plugins");

    std::printf("=== Scope framework AutoTest ===\n");
    std::printf("plugin dir: %s\n", strPluginDir.toLatin1().constData());

    CScopeManager& mgr = CScopeManager::instance();
    mgr.loadPlugins(strPluginDir);

    testDiscovery(mgr);
    testInstances(mgr);
    testMockVisa();
    testRealModels(mgr);
    testFeatureSlices(mgr);
    testParity(mgr);
    testSimApi(mgr);
    testRangeMatrix(mgr);
    testErrorPaths(mgr);
    testTwoInstances(mgr);
    testConcurrency(mgr);

    std::printf("\n=== %d passed, %d failed ===\n", g_iPass, g_iFail);
    return g_iFail;
}
