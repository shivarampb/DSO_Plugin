# Integration Guide

How to consume the Scope library from your application.

## What you need

From the packaged `deliverables/`:
- `include/` — the six public SDK headers.
- `lib/libScopeCore.so` (`ScopeCore.dll` + import lib on Windows).
- `plugins/` — the model plugin DLLs you want to ship.
- `lib/libvisa.*` — **only for hardware-free testing**. On a bench, do not ship
  it; the real vendor VISA is discovered at runtime instead.

## qmake consumer

```pro
QT += core
INCLUDEPATH += $$PWD/deliverables/include
LIBS += -L$$PWD/deliverables/lib -lScopeCore
```

## CMake consumer

```cmake
find_package(Qt5 COMPONENTS Core REQUIRED)
add_executable(myapp main.cpp)
target_include_directories(myapp PRIVATE ${DELIVERABLES}/include)
target_link_libraries(myapp PRIVATE Qt5::Core ${DELIVERABLES}/lib/libScopeCore.so)
```

## Minimal usage

```cpp
#include "ScopeManager.h"

CScopeManager& mgr = CScopeManager::instance();
mgr.loadPlugins("plugins");
mgr.createInstance(1, "MDO34");

S_Scope_ConnectionConfig cfg;
cfg.setResourceString("USB0::0x0699::0x0522::C010000::INSTR");
if (mgr.connect(1, cfg).isSuccess()) {
    mgr.enableChannel(1, 1, true);
    mgr.setVerticalScale(1, 1, 0.5);
    mgr.setTimebaseScale(1, 1e-6);
    mgr.single(1);
    S_Scope_Waveform wfm;
    mgr.readWaveform(1, 1, wfm);
    mgr.disconnect(1);
}
mgr.destroyInstance(1);
```

See `examples/BasicUsage/` for a complete, buildable program that also opens the
virtual `SimScope` (`SIM::MDO34`) so it runs with no hardware.

## VISA runtime discovery

Real plugins link `-lvisa`/`-lvisa64`. At runtime the loader resolves `libvisa`:
- **Bench:** install the vendor NI-VISA / Keysight IO Libraries; ensure the VISA
  shared library is on the loader path (`LD_LIBRARY_PATH` / `PATH`).
- **Hardware-free:** put the bundled `libvisa` on the loader path and use a
  `MOCK0::<model>::INSTR` resource; `SimScope` needs no VISA at all.

## Windows / MinGW export check

Confirm a plugin DLL exports only the Qt plugin entry points:

```
objdump -p plugins\PluginMDO34.dll | findstr /i "qt_plugin"
```

Only `qt_plugin_instance` / `qt_plugin_query_metadata` should appear — no
internal symbols (the Linux build enforces this with `-fvisibility=hidden` and
an `nm -D` check in the packaging script).

## Resource strings

| Interface | Example resource |
|-----------|------------------|
| USB-TMC | `USB0::0x0699::0x0522::<serial>::INSTR` |
| LAN (VXI-11/INSTR) | `TCPIP0::192.168.0.10::INSTR` |
| LAN raw socket | `TCPIP0::192.168.0.10::5025::SOCKET` |
| GPIB | `GPIB0::7::INSTR` |
| Mock (dev) | `MOCK0::MDO34::INSTR` |
| SimScope | `SIM::MDO34` (append `::SQUARE`/`::RAMP`/`::NOISE` to pick a shape) |

`S_Scope_ConnectionConfig::toVisaResourceString()` builds these from structured
fields when `setResourceString()` is not used.
