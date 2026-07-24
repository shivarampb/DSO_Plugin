# Scope — plugin-based oscilloscope control library

A portable, plugin-based oscilloscope (DSO) control library for
**Qt 5.14.2 (MinGW, Windows x64)**, also buildable with Qt 5.14+ on Linux for
development and CI. It is a faithful sibling of the in-house **ELoad**
electronic-load framework: a single core library (`ScopeCore`) exposes the
`CScopeManager` singleton to the application, and each instrument model is an
independent Qt plugin DLL discovered and loaded at run time with
`QPluginLoader`.

Target fleet (multi-vendor, one self-contained plugin per model): **MDO34**
(Tektronix) ★, **RTM3004** (Rohde & Schwarz) ★, DSO7104B, DSOS204A, DSOX2012A,
MSO6054A, RTO2064, TDS1012B, TDS2024C, WaveSurfer 42Xs (Teledyne LeCroy), plus a
hardware-free **SimScope** virtual model for offline development.

```
Application ──#include "ScopeManager.h"──▶ CScopeManager (singleton)
                                              │  qobject_cast + forward
                                              ▼
                                        CIScopePlugin (QPluginLoader)
                                          plugins/<Model> ──SCPI/VISA──▶ instrument
```

Operations are keyed by **scope number** and, where relevant, a **1-based
channel** (channel is first-class — scopes are inherently multi-channel). The
real model plugins link VISA directly (`-lvisa`); each model plugin is a
complete, self-contained implementation with its own SCPI dialect and its own
embedded limits row (the same one-class-per-instrument convention as ELoad).

## Repository layout

```
Scope.pro                     top-level build (subdirs, mirrors ELoad.pro)
include/                      public SDK headers (framework, types, error, VISA)
ScopeCore/                    CScopeManager + ScopeError + VisaHelper
Common/                       S_ScopeLimits (struct + per-model rows + catalog, internal)
Plugin<Model>/               self-contained model plugins (<Model>Plugin.*)
PluginSimScope/              virtual model plugin (no hardware, no VISA)
MockVisa/                    hardware-free libvisa + oscilloscope SCPI emulator (dev/CI)
AutoTest/                    console end-to-end suite (zero hardware)
GuiTester/                   dark-themed tabbed "Scope Plugin Test" app
examples/BasicUsage/         minimal consumer program
scripts/                     build + assemble + leak-guard deliverables
docs/                        architecture, API reference, integration, add-a-model, user manual
```

## Build

```sh
mkdir build && cd build
qmake ../Scope.pro
make -j        # (mingw32-make on Windows)
```

Outputs land in `build/lib` (core + `libvisa`), `build/plugins` (model DLLs) and
`build/bin` (testers).

## Test (zero hardware)

```sh
cd build
LD_LIBRARY_PATH=lib bin/AutoTest "$(pwd)/plugins"     # exit code = failures
QT_QPA_PLATFORM=offscreen LD_LIBRARY_PATH=lib bin/GuiTester --smoke "$(pwd)/plugins"
```

The real plugins route VISA to the bundled **MockVisa** (`libvisa`), and
`SimScope` needs no VISA at all, so the whole suite runs with no instrument
attached. On a real bench, replace `libvisa` with the vendor NI-VISA.

## Reference manuals

Per-model programming/interface manuals and datasheets belong under
`manuals/<Model>/`. Where a manual is not yet available, the model's
SCPI/limits carry `TODO(manual)` markers and are exercised against SimScope +
MockVisa until the PDF is supplied.
