/**
 * @file    main.cpp
 * @brief   Minimal consumer of the Scope plugin library (hardware-free).
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "ScopeManager.h"

#include <cstdio>

int main(int argc, char** argv)
{
    const char* pluginDir = (argc > 1) ? argv[1] : "plugins";

    CScopeManager& mgr = CScopeManager::instance();
    mgr.loadPlugins(pluginDir); // discover model DLLs

    std::printf("Discovered plugins:\n");
    for (const QString& name : mgr.getAvailablePlugins())
    {
        std::printf("  - %s\n", name.toLatin1().constData());
    }

    // Bind scope number 1 to the virtual SimScope (no hardware, no VISA).
    if (!mgr.createInstance(1, QStringLiteral("SimScope")).isSuccess())
    {
        std::printf("SimScope plugin not available\n");
        return 1;
    }

    S_Scope_ConnectionConfig cfg;
    cfg.setResourceString(QStringLiteral("SIM::MDO34")); // virtual MDO34
    if (mgr.connect(1, cfg).isSuccess())
    {
        mgr.enableChannel(1, 1, true);
        mgr.setVerticalScale(1, 1, 0.5); // 0.5 V/div
        mgr.setTimebaseScale(1, 1e-6);   // 1 us/div
        mgr.setTriggerSource(1, Enum_Scope_TriggerSource::m_enumCh1);
        mgr.setTriggerLevel(1, 1, 1.0);
        mgr.setAcqMode(1, Enum_Scope_AcqMode::m_enumSample);
        mgr.single(1);

        S_Scope_Waveform wfm;
        if (mgr.readWaveform(1, 1, wfm).isSuccess())
        {
            std::printf("Captured %u points (xInc = %.3g s)\n", wfm.pointCount(),
                        wfm.m_sPreamble.m_dXIncrement);
        }

        S_Scope_MeasurementResult r;
        if (mgr.readMeasurement(1, 1, Enum_Scope_MeasType::m_enumFrequency, r).isSuccess())
        {
            std::printf("Frequency = %.4g %s\n", r.m_dValue, r.m_strUnits.toLatin1().constData());
        }

        mgr.disconnect(1);
    }
    mgr.destroyInstance(1);

    /* On a bench (or against MockVisa) bind a real model instead:
     *   mgr.createInstance(2, "MDO34");
     *   cfg.setResourceString("USB0::0x0699::0x0522::C010000::INSTR");
     *   mgr.connect(2, cfg); ... ; mgr.destroyInstance(2);
     */
    return 0;
}
