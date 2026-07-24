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
#include <atomic>
#include <thread>
#include <vector>

#include "ScopeManager.h"

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
    testSimApi(mgr);
    testRangeMatrix(mgr);
    testErrorPaths(mgr);
    testTwoInstances(mgr);
    testConcurrency(mgr);

    std::printf("\n=== %d passed, %d failed ===\n", g_iPass, g_iFail);
    return g_iFail;
}
