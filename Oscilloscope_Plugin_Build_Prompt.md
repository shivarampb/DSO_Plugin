# Build Prompt — Plugin-Based Oscilloscope Control Library (Qt 5.14.2 / MinGW / x64)

> Paste this whole document into a fresh Claude Code (or agent) session as the task.
> It is written to reproduce, for **oscilloscopes**, the exact architecture of an
> existing in-house **ELoad** electronic-load framework (itself modeled on a
> PowerSupply plugin framework). Fill in the two **INPUTS YOU MUST SUPPLY**
> sections before running, then let the agent execute the milestones.

---

## 0. Mission

Build a portable, **plugin-based oscilloscope control library** for
**Qt 5.14.2 (MinGW, Windows x64)**, also buildable with Qt 5.14+ on Linux for
development and CI. One core library (`ScopeCore`) exposes a `CScopeManager`
singleton to the application; each instrument model is an **independent Qt plugin
DLL** discovered and loaded at run time with `QPluginLoader`. Talking to hardware
is **SCPI over VISA**. Ship a hardware-free **MockVisa** and a virtual **SimScope**
model so the whole stack builds, tests and demos with **no instrument attached**,
plus a **console test suite** and a **dark-themed tabbed GUI test application**.

This must be a faithful sibling of the ELoad framework: same plugin mechanism,
same singleton/keying model, same "mandatory-few / optional-default-NOT_SUPPORTED"
interface, same self-contained-per-model plugin convention, same VISA + MockVisa
strategy, same tester/packaging/docs shape — only the **instrument domain**
changes from electronic load to oscilloscope, and the **type prefix** changes from
`S_ELoad_` / `Enum_ELoad_` to `S_Scope_` / `Enum_Scope_`.

---

## 1. INPUTS YOU MUST SUPPLY (fill these in before running)

### 1a. Target model list (build a self-contained plugin per model)

Unlike the single-vendor ELoad family, this is a **multi-vendor fleet** — each
model is its own self-contained plugin (`Plugin<Model>/`) with its own SCPI
dialect verified against its own manual. Build order: do the two **★ priority
models first** (they cover the two richest SCPI dialects — Tektronix and Rohde &
Schwarz — and both are 4-channel MSO/MDO with AWG), then fan out the rest.

| # | Model (plugin/discovery name) | Make | Family | Bandwidth | Analog ch | Notes |
|---|---|---|---|---|---|---|
| 1 | **MDO34** ★ | Tektronix | 3 Series MDO | up to 1 GHz | 4 | Mixed-**domain** (built-in spectrum analyzer), MSO option, optional 50 MHz AWG |
| 2 | **RTM3004** ★ | Rohde & Schwarz | RTM3000 | up to 1 GHz | 4 | MSO option, built-in pattern/function generator |
| 3 | DSO7104B | Keysight (Agilent) | InfiniiVision 7000 | 1 GHz | 4 | |
| 4 | DSOS204A | Keysight | Infiniium S-Series | 2 GHz | 4 | Deep-memory, high sample rate |
| 5 | DSOX2012A | Keysight (Agilent) | InfiniiVision 2000 X | 100 MHz | 2 | Optional WaveGen |
| 6 | MSO6054A | Agilent (Keysight) | 6000 Series MSO | 500 MHz | 4 + 16 digital | Mixed-signal |
| 7 | RTO2064 | Rohde & Schwarz | RTO2000 | 600 MHz | 4 | High-end |
| 8 | TDS1012B | Tektronix | TDS1000B | 100 MHz | 2 | Entry / legacy SCPI |
| 9 | TDS2024C | Tektronix | TDS2000C | 200 MHz | 4 | Entry / legacy SCPI |
| 10 | WaveSurfer 42Xs | Teledyne LeCroy | WaveSurfer Xs | 400 MHz | 4 | MAUI/X-Stream remote |

> The class name for each is `C<Model>Plugin` with non-identifier characters
> dropped/normalised — e.g. `CMDO34Plugin`, `CRTM3004Plugin`, `CDSO7104BPlugin`,
> `CDSOS204APlugin`, `CDSOX2012APlugin`, `CMSO6054APlugin`, `CRTO2064Plugin`,
> `CTDS1012BPlugin`, `CTDS2024CPlugin`, `CWaveSurfer42XsPlugin`. Folder =
> `Plugin<Model>/` (e.g. `PluginMDO34/`, `PluginRTM3004/`). Group them by vendor
> in the docs but keep each plugin fully self-contained (no shared base class).

Because the SCPI differs **per vendor**, treat the vendors as distinct dialects:
Keysight/Agilent = "Programmer's Guide" (IVI/InfiniiVision & Infiniium),
Tektronix = "Programmer Manual", Rohde & Schwarz = "User Manual" (Remote Control /
SCPI chapters), Teledyne LeCroy = "Remote Control / MAUI Automation Command
Reference". A model plugin only overrides what its model supports (e.g. the AWG
group only on MDO34/RTM3004/DSOX2012A with the gen option; the digital/MSO group
only on MSO6054A and MSO-equipped MDO34/RTM3004).

### 1b. Get each model's Interface & Communication manual + datasheet (Digi-Key / Mouser)

For **every** model above, obtain two documents and place the PDFs in
`manuals/<Model>/` in the repo:

1. the **Programming / Interface & Communication manual** (the SCPI reference —
   the single source of truth for every command, parameter, range and response
   format), and
2. the **datasheet / user manual** (the source for the per-model spec table in §1c).

**Sourcing recipe — Digi-Key and Mouser first, manufacturer as fallback:**

- **Digi-Key** (`digikey.com`): search the exact model number → open the product
  page → **"Documents & Media"** section → download the *Datasheet* and any
  *User/Programming Manual* / *Reference Manual* listed.
- **Mouser** (`mouser.com`): search the exact model number → product page →
  **"Product Documents" / "Datasheets & Files"** tab → download the *Datasheet*
  and *Manual* entries.
- **Manufacturer fallback** (use when Digi-Key/Mouser lack the programming manual,
  which is common for bench instruments): Tektronix `tek.com` (Downloads →
  Manuals, search "MDO34 / TDS2024C Programmer Manual"), Keysight `keysight.com`
  (Search → the model → Manuals → "Programmer's Guide"), Rohde & Schwarz
  `rohde-schwarz.com` (the model → Manuals → "User Manual" incl. remote control),
  Teledyne LeCroy `teledynelecroy.com` (Support → the model → "Remote Control
  Manual" / MAUI Automation Command Reference).

> If a document cannot be retrieved through the available network/tools, **stop and
> report which model's manual is missing** rather than inventing SCPI. The agent may
> proceed against **SimScope + MockVisa only** for any model whose manual is not yet
> available, leaving that model's real limits/commands as clearly-marked `TODO`s
> until its manual is supplied.

> Discipline (same as ELoad used the Kikusui manuals): **every** SCPI command,
> numeric range, resolution and response format in the code must be verified
> page-by-page against that model's programming manual.

### 1c. Per-model spec table (drives `Common/S_ScopeLimits.h`)

Fill one row per model from the datasheets gathered in §1b (start with the two ★
models). Capture at least:

| Param | MDO34 | RTM3004 | … (one column per model) |
|---|---|---|---|
| Analog bandwidth (model variants) | e.g. 100/200/350/500 MHz, 1 GHz | 100 MHz–1 GHz | … |
| Analog channels | 4 | 4 | … |
| Digital channels (if MSO) | 16 (opt) | 16 (opt) | … |
| Max real-time sample rate | verify | verify | … |
| Max record length / memory depth | verify | verify | … |
| Vertical scale range (V/div) min–max | verify | verify | … |
| Vertical offset / position range | verify | verify | … |
| Timebase scale range (s/div) min–max | verify | verify | … |
| Input impedance options | 1 MΩ / 50 Ω | 1 MΩ / 50 Ω | … |
| Probe attenuation options (discrete) | verify | verify | … |
| Built-in AWG (function/BW) | opt 50 MHz | built-in | … |
| Interfaces | USB-TMC, LAN, (GPIB) | USB-TMC, LAN | … |

Also capture, once per model: supported trigger types, acquisition modes,
waveform formats (BYTE/WORD/ASCII), max/min record lengths per mode, the
measurement list, math/FFT capabilities, cursor types, screenshot format,
USB VID/PID, LAN raw-socket port, SCPI line terminator, and the `*IDN?` response
layout — these drive the per-model plugin, its `S_ScopeLimits` row and the
MockVisa emulator.

---

## 2. Naming & house-style conventions (mandatory)

Match the ELoad/PowerSupply house style exactly:

- **Classes** `C…` (e.g. `CScopeManager`, `CIScopePlugin`, `CMDO34Plugin`).
- **Framework structs** `S_Scope_<Name>` (e.g. `S_Scope_ConnectionConfig`,
  `S_Scope_Waveform`, `S_Scope_Capabilities`).
- **Enumerations** `Enum_Scope_<Name>` (e.g. `Enum_Scope_TriggerType`,
  `Enum_Scope_Coupling`, `Enum_Scope_ErrorCode`); enumerator values keep short
  names (`m_enumEdge`, `SUCCESS`, …).
- **Members** `m_…`; **params** `in_…` / `out_…` with type tags
  (`in_u32ScopeNumber`, `in_dScaleVoltsPerDiv`, `out_sWaveform`).
- **Typedefs** `U32BIT`, `S32BIT`, `U16BIT`, `S16BIT`, `U8BIT`, `S8BIT`,
  `FDOUBLE`, `FFLOAT` (define in `ScopeTypes.h`, exactly as ELoad does).
- **Core export macro** `SCOPECORE_EXPORT` = `Q_DECL_EXPORT` when
  `SCOPECORE_LIBRARY` is defined, else `Q_DECL_IMPORT` — **defined in the
  lowest-level header (`ScopeTypes.h`)** and applied to `ScopeError` and
  `S_Scope_DeviceErrorStatus` so constructing a `ScopeError` inside a plugin is
  never an unresolved symbol on MinGW/MSVC alike.
- **Plugin IID** `#define ScopePlugin_iid "com.automation.ScopePlugin/1.0"`.
- C++11; `CONFIG += c++11`; no absolute paths in any `.pro`; `$$PWD`-anchored
  DESTDIRs; zero compiler warnings (treat `-Wunused-parameter` on the interface
  with a scoped `#pragma GCC diagnostic ignored`).

---

## 3. Design invariants (non-negotiable — copy the ELoad framework)

1. **Interface via `QPluginLoader`.** `CIScopePlugin` is a pure abstract Qt
   interface with `Q_DECLARE_INTERFACE(CIScopePlugin, ScopePlugin_iid)`. Each
   model plugin is a `QObject` carrying `Q_PLUGIN_METADATA(IID ScopePlugin_iid)`
   + `Q_INTERFACES(CIScopePlugin)`. `CScopeManager::loadPlugins(dir)` scans the
   directory, loads each `*.so`/`*.dll` and resolves it with
   `qobject_cast<CIScopePlugin*>`.

2. **Manager singleton, keyed by scope number (+ 1-based channel).**
   `CScopeManager::instance()` owns a `QMap<QString, S_PluginData>` (plugin name →
   loader/plugin/info/caps) and a `QMap<U32BIT, QString>` (scope number → plugin
   name). Every operation takes `in_u32ScopeNumber` and, where it applies, a
   1-based `in_u32Channel` (scopes are inherently multi-channel — channel is
   first-class here, unlike the single-channel loads). The manager forwards to the
   plugin backing that scope number.

3. **Mandatory vs. optional operations.** Only `getPluginInfo`, `getCapabilities`,
   `connect`, `disconnect`, `isConnected` and `reset` are pure-virtual. **Every
   other operation defaults to `NOT_SUPPORTED`** via a `SCP_NS()` macro
   (`return CIScopePlugin::NotSupported();`) so a plugin overrides only what its
   model provides (e.g. only MSO models override the digital-channel group; only
   models with a built-in generator override the AWG group).

4. **Self-contained per-model plugins (no shared base).** Each model plugin is a
   complete standalone `CIScopePlugin` implementation — it declares and implements
   the full operation set itself and **embeds its own `S_ScopeLimits` row** —
   exactly like the ELoad `CPLZ4005WH2Plugin`. Folder `Plugin<Model>/` holds
   `<Model>Plugin.h` + `<Model>Plugin.cpp` + `Plugin<Model>.pro`, depending only
   on the public SDK headers, the core library and VISA. Models share the SCPI
   *pattern* but not code; the `S_ScopeLimits` struct + family constants live in
   `Common/S_ScopeLimits.h` (shared *data types*, not behaviour).

5. **Direct VISA linkage.** Real plugins `#include <visa.h>` and link `-lvisa`,
   holding one `ViSession` per scope in a map. On a bench that resolves to the
   vendor NI-VISA; in this repo it resolves to **MockVisa** — a `libvisa` that
   exports the standard `vi*` symbols and answers with an embedded oscilloscope
   SCPI emulator, so the whole stack builds and runs with no hardware.

6. **Local range validation.** Every setter validates its argument against the
   model's `S_ScopeLimits` row *before* touching the instrument
   (`PARAMETER_OUT_OF_RANGE`), then sends the SCPI command and checks `SYST:ERR?`.

7. **Errors as return values.** Every operation returns a `ScopeError` value
   carrying an `Enum_Scope_ErrorCode` + human-readable description.
   `readErrorStatus()` fills an `S_Scope_DeviceErrorStatus` (decoded status bytes +
   trigger/acquisition state). A `CScopeManager::errorOccurred(scope, channel,
   S_Scope_DeviceErrorStatus)` signal carries decoded events asynchronously.

8. **Binary waveform transfer is a first-class surface** (the scope analog of
   ELoad's ARB binary block / `DATA:R?`): dedicated counted/binary VISA read mode
   for `#<w><len><payload>` blocks, preamble parsing, and conversion to real
   volts-vs-time. MockVisa must emulate this (and the screenshot block).

---

## 4. Public SDK surface

Public headers live in `include/` and are the ONLY deliverable headers:
`ScopeManager.h`, `IScopePlugin.h`, `ScopeError.h`, `ScopeTypes.h`, `VisaHelper.h`,
`visa.h`.

### 4a. Value types (`ScopeTypes.h`) — mirror ELoadTypes.h, adapt to scope

- `Enum_Scope_CommunicationProtocol { RS232, USB, GPIB, ETHERNET, LXI, TCPIP, VXI11, HiSLIP }`
- `S_Scope_ConnectionConfig` — protocol + fields (resource string, USB VID/PID/SN,
  IP/port, GPIB board/addr, COM/baud, timeout) + `setResourceString()` +
  `toVisaResourceString()` (implemented in `VisaHelper.h`).
- `S_Scope_PluginInfo` — fixed-size POD metadata (name, version, manufacturer,
  model, series, description, min/max core version) + supported protocols/modes;
  `isCompatible()`.
- `S_Scope_ChannelCapabilities` / `S_Scope_Capabilities` — channel count, analog
  bandwidth, max sample rate, max memory depth, vertical/timebase ranges, flags
  (`m_bHasDigital`, `m_bHasAWG`, `m_bHasFFT`, `m_bHasSerialDecode`,
  `m_bHasSegmented`, …) + `getChannelCapabilities()`.
- `S_Scope_ParameterRange { m_dMin, m_dMax, m_dResolution, QList<FDOUBLE> m_QlistDiscreteValues }`
  and `Enum_Scope_ParamId` (VerticalScale, VerticalOffset, TimebaseScale,
  TimebasePosition, TriggerLevel, TriggerHoldoff, ProbeAttenuation, SampleRate,
  MemoryDepth, AverageCount, WaveformPoints, AwgFrequency, AwgAmplitude,
  MeasureThreshold, …) for auto-ranging UIs.
- **Scope-specific types:**
  - `S_Scope_WaveformPreamble { format, type, points, count, xIncrement, xOrigin,
    xReference, yIncrement, yOrigin, yReference }` (parsed from `:WAV:PRE?`).
  - `S_Scope_Waveform { S_Scope_WaveformPreamble m_sPreamble; QVector<FDOUBLE>
    m_vecTimeSeconds; QVector<FDOUBLE> m_vecVolts; U32BIT m_u32SourceChannel; }`.
  - `S_Scope_MeasurementResult { Enum_Scope_MeasType m_eType; FDOUBLE m_dValue;
    bool m_bValid; QString m_strUnits; }` (+ optional statistics min/max/mean/stddev).
- Domain enums: `Enum_Scope_Coupling { m_enumDC, m_enumAC, m_enumGND }`,
  `Enum_Scope_TriggerMode { m_enumAuto, m_enumNormal, m_enumSingle }`,
  `Enum_Scope_TriggerType { m_enumEdge, m_enumPulse, m_enumWidth, m_enumVideo,
  m_enumPattern, m_enumRunt, m_enumSlope, m_enumSetupHold, m_enumSerial }`,
  `Enum_Scope_TriggerSlope { m_enumRising, m_enumFalling, m_enumEither }`,
  `Enum_Scope_TriggerSource { m_enumCh1..m_enumChN, m_enumExt, m_enumLine, m_enumDigital }`,
  `Enum_Scope_AcqMode { m_enumSample, m_enumPeakDetect, m_enumAverage, m_enumHiRes, m_enumEnvelope }`,
  `Enum_Scope_WaveformFormat { m_enumByte, m_enumWord, m_enumAscii }`,
  `Enum_Scope_WaveformSource { m_enumChannel, m_enumMath, m_enumRef, m_enumDigital }`,
  `Enum_Scope_MeasType { m_enumVpp, m_enumVmax, m_enumVmin, m_enumVrms, m_enumVavg,
  m_enumFrequency, m_enumPeriod, m_enumRiseTime, m_enumFallTime, m_enumPosWidth,
  m_enumNegWidth, m_enumDutyCycle, m_enumOvershoot, m_enumPhase, m_enumDelay, … }`,
  `Enum_Scope_MathOp { m_enumAdd, m_enumSub, m_enumMult, m_enumDiv, m_enumFFT }`,
  `Enum_Scope_FftWindow { m_enumRect, m_enumHann, m_enumHamming, m_enumBlackman, m_enumFlattop }`,
  `Enum_Scope_CursorType { m_enumOff, m_enumHorizontal, m_enumVertical, m_enumTrack }`,
  `Enum_Scope_TimebaseMode { m_enumMain, m_enumZoom, m_enumRoll, m_enumXY }`,
  `Enum_Scope_RemoteState`, `Enum_Scope_ImageFormat { m_enumPng, m_enumBmp }`.

### 4b. `ScopeError.h` — mirror ELoadError.h

`Enum_Scope_ErrorCode` with the same code bands: framework/plugin (1000s),
communication (2000s), parameter/usage (3000s incl. `NOT_SUPPORTED`),
instrument-reported (4000s), `UNKNOWN_ERROR = 9999`.
`Enum_Scope_DeviceStatusFlag` + `Q_DECLARE_FLAGS(Scope_DeviceStatus, …)`,
`struct SCOPECORE_EXPORT S_Scope_DeviceErrorStatus` (status byte/ESR/OPER/QUES +
trigger state + message + `toString()`), and
`class SCOPECORE_EXPORT ScopeError` (`code()`, `description()`, `toString()`,
`isSuccess()`, static code/status stringifiers).

### 4c. `CIScopePlugin` interface + `CScopeManager` — operation groups

Both expose the same method set (interface = pure-virtual mandatory + default
`SCP_NS()` optional; manager = forwarders that look up the plugin and call
through). Group the ~150 methods like ELoad grouped its ~150:

- **Plugin info / connection (mandatory core):** `getPluginInfo`,
  `getCapabilities`, `connect(scope, S_Scope_ConnectionConfig)`, `disconnect`,
  `isConnected`, `reset`, `setTimeout`, `clearStatus`, `getIdentification`,
  `selfTest`, `getOptions`, `waitOperationComplete`, `getScpiVersion`,
  `getParameterRange(scope, channel, Enum_Scope_ParamId, S_Scope_ParameterRange&)`,
  `autoscale`, `run`, `stop`, `single`, `forceTrigger`.
- **Vertical (per channel):** `enableChannel/isChannelEnabled`,
  `setVerticalScale/getVerticalScale` (V/div), `setVerticalOffset/getVerticalOffset`,
  `setVerticalPosition/getVerticalPosition` (div), `setCoupling/getCoupling`,
  `setBandwidthLimit/getBandwidthLimit`, `setProbeAttenuation/getProbeAttenuation`,
  `setInputImpedance/getInputImpedance`, `setInvert/getInvert`,
  `setChannelLabel/getChannelLabel`, `setChannelUnits/getChannelUnits`,
  `setDeskew/getDeskew`.
- **Horizontal / timebase:** `setTimebaseScale/getTimebaseScale` (s/div),
  `setTimebasePosition/getTimebasePosition` (delay), `setTimebaseReference`,
  `setTimebaseMode/getTimebaseMode`, `getSampleRate`, `setMemoryDepth/getMemoryDepth`,
  `setAcquisitionPoints/getAcquisitionPoints`.
- **Trigger:** `setTriggerMode/getTriggerMode`, `setTriggerType/getTriggerType`,
  `setTriggerSource/getTriggerSource`, `setTriggerSlope/getTriggerSlope`,
  `setTriggerLevel/getTriggerLevel`, `setTriggerCoupling/getTriggerCoupling`,
  `setTriggerHoldoff/getTriggerHoldoff`, `getTriggerState`, plus type-specific
  setters (pulse width, video standard, pattern, runt, slope) as the model allows.
- **Acquisition:** `setAcqMode/getAcqMode`, `setAverageCount/getAverageCount`,
  `getAcquisitionState`, `setSegmented…` (if supported).
- **Waveform transfer:** `setWaveformSource/getWaveformSource`,
  `setWaveformFormat/getWaveformFormat`, `setWaveformPoints`,
  `getWaveformPreamble(scope, channel, S_Scope_WaveformPreamble&)`,
  `readWaveform(scope, channel, S_Scope_Waveform&)` (binary block → scaled
  volts-vs-time), `digitizeChannel`.
- **Automatic measurements:** `addMeasurement`, `readMeasurement(scope, channel,
  Enum_Scope_MeasType, S_Scope_MeasurementResult&)`, `clearMeasurements`,
  `setMeasureStatistics/getMeasurementStatistics`.
- **Math / FFT:** `setMathOperation`, `setMathSource1/2`, `enableMath`,
  `setFftWindow/getFftWindow`, `setFftSpan`, `setFftCenter`,
  `setMathScale/setMathPosition`.
- **Cursors:** `setCursorType/getCursorType`, `setCursorSource`,
  `setCursorPosition/getCursorPosition`, `readCursorValues` (X/Y + deltas).
- **Display:** `setPersistence`, `setGraticule`, `setIntensity`,
  `setDisplayFormat` (YT/XY), `setVectors`.
- **Save / recall / screenshot:** `saveSetup/recallSetup` (instrument memory),
  `saveWaveformToFile`, `captureScreenshot(scope, Enum_Scope_ImageFormat,
  QByteArray& out_imageBytes)`, `saveToReference/displayReference`.
- **Digital / MSO (optional group — only MSO models override):**
  `enableDigitalChannel`, `setDigitalThreshold`, `setPodThreshold`,
  `enableBus/setBusType/readBusDecode`.
- **AWG / Wavegen (optional group — only models with a generator override):**
  `setAwgFunction`, `setAwgFrequency`, `setAwgAmplitude`, `setAwgOffset`,
  `enableAwgOutput`.
- **Status / system:** `readErrorStatus`, `clearErrorStatus`, `queryErrorQueue`,
  `getInstrumentErrorCount`, `readStandardEventStatus`, `readStatusByte`,
  `readOperationStatus`, `readQuestionableStatus`, `setRemoteState/getRemoteState`,
  `setKeyLock/isKeyLocked`, `setBeeper`.
- **Debug escape hatch (diagnostics only):** `writeScpi(scope, cmd)`,
  `queryScpi(scope, cmd, out_response)`.

Plus `CScopeManager` extras: `loadPlugins`, `getAvailablePlugins`,
`getPluginInfoByName`, `getPluginCapabilities`, `getCoreVersion`,
`createInstance(scope, pluginName)`, `destroyInstance`, `instanceExists`,
`getInstancePlugin`, and signals `pluginLoaded/pluginLoadFailed/instanceCreated/
instanceDestroyed/errorOccurred`. Optionally a live SCPI trace tap
(`typedef void (*ScopeTraceCallback)(void*, U32BIT scope, int dir, const QString&)`).

---

## 5. Repository layout (mirror ELoad exactly)

```
Scope.pro                      top-level subdirs build (ordered, with .depends)
include/                       public SDK headers (ScopeManager/IScopePlugin/
                               ScopeError/ScopeTypes/VisaHelper/visa.h) — ONLY these ship
ScopeCore/                     CScopeManager + ScopeError + VisaHelper impl (SCOPECORE_LIBRARY)
Common/                        S_ScopeLimits.h (struct + per-model constants + SimScope catalog, internal)
manuals/<Model>/               per-model Programming/Interface manual + datasheet PDFs (§1b; not shipped)
PluginMDO34/ PluginRTM3004/     ★ first two self-contained model plugins (<Model>Plugin.h/.cpp/.pro)
PluginDSO7104B/ PluginDSOS204A/ PluginDSOX2012A/ PluginMSO6054A/
PluginRTO2064/ PluginTDS1012B/ PluginTDS2024C/ PluginWaveSurfer42Xs/   the remaining models
PluginSimScope/                virtual model plugin (SimScopePlugin.*; no hardware, no VISA)
MockVisa/                      hardware-free libvisa + oscilloscope SCPI emulator (dev/CI)
AutoTest/                      console end-to-end suite (exit code = failures; zero hardware)
GuiTester/                     dark-themed tabbed "Scope Plugin Test" app
examples/BasicUsage/           minimal consumer program (main.cpp + .pro)
scripts/package_deliverables.sh|.bat   build + assemble + leak-guard deliverables
docs/                          Architecture, API_Reference, Integration_Guide,
                               Adding_A_New_Model, Plugin_User_Manual
.github/workflows/ci.yml       Linux build + AutoTest + GuiTester --smoke + packaging
README.md
```

Top-level `Scope.pro`: `TEMPLATE = subdirs`, `CONFIG += ordered`, SUBDIRS =
`ScopeCore MockVisa Plugin<...> PluginSimScope AutoTest GuiTester` with
`<plugin>.depends = ScopeCore MockVisa` and `PluginSimScope.depends = ScopeCore`.
Shadow build: `mkdir build && cd build && qmake ../Scope.pro && make -j`; outputs
land in `build/lib`, `build/plugins`, `build/bin`.

---

## 6. VISA layer + MockVisa + SimScope

- **VISA:** real plugins include `<visa.h>` (a minimal VPP-4.3.2 declaration
  header in `include/`) and link `-lvisa`/`-lvisa64`. `VisaHelper.h` provides
  `toVisaResourceString()` and thin session helpers (open with timeout + LF
  termchar, write+LF, read line, **read counted/binary block for waveform &
  screenshot payloads**, query). Map VISA status → `COMMUNICATION_ERROR`,
  `VI_ERROR_TMO` → `COMMUNICATION_TIMEOUT`.
- **MockVisa** (`TEMPLATE = lib`, exports the `vi*` ABI subset): an embedded
  oscilloscope SCPI emulator (`CMockScopeEmulator`) that answers `*IDN?`,
  IEEE-488.2 status, `SYST:ERR?` queue semantics, a representative command from
  **every** group, and crucially returns a valid **binary waveform `#`-block**
  for `:WAV:DATA?` (with a matching `:WAV:PRE?` preamble that round-trips to a
  known synthetic signal) and a small PNG **screenshot block**. Provide a
  `MOCK0::<model>::INSTR` resource and a `MOCK0::TIMEOUT::INSTR` that forces a
  timeout, and a `viFindRsrc` iterator.
- **SimScope** (`PluginSimScope`, no VISA): a complete `CIScopePlugin` that
  **synthesizes waveforms** (sine/square/ramp/noise selectable via the resource
  string, e.g. `SIM::MDO34`), honours vertical/timebase/trigger/acquisition
  settings when generating the trace, computes automatic measurements from the
  synthesized buffer, and emits trace lines — so `readWaveform`, measurements and
  the GUI plot all work with zero hardware. It reuses the **same `S_ScopeLimits`
  rows**, giving `getParameterRange` parity with the real plugins by construction.

---

## 7. Test application (dark tabbed GUI) + console AutoTest

### 7a. GuiTester — "Scope Plugin Test" (Qt Widgets, dark Fusion theme)

A `QMainWindow` with a `QTabWidget`, a status bar showing **Disconnected /
Connected**, operation tabs disabled until connected, and `--smoke` /
`--screenshot` command-line switches for headless CI. Default target exercises the
first ★ model via `MOCK0::MDO34::INSTR`. Long instrument calls run off the UI
thread (worker per scope) so the UI never blocks. Tabs:

1. **Connection** — Model, Interface (LAN / USB / GPIB / *Mock (dev)*),
   Host/Device address, Connect (green) / Disconnect (red); on success show the
   `*IDN?` identity box and unlock the other tabs.
2. **Vertical** — per-channel enable, V/div, offset, coupling, bandwidth limit,
   probe attenuation, impedance; spin-box ranges pulled from `getParameterRange`.
3. **Horizontal & Trigger** — timebase s/div + position + mode; trigger
   mode/type/source/slope/level/holdoff + Force Trigger; live trigger state.
4. **Acquisition** — acq mode, average count, Run / Stop / Single; live
   sample-rate & memory-depth readout.
5. **Waveform** — source select, Fetch, and a **live plot of the captured trace**
   (custom `QWidget` painting volts-vs-time with a graticule — the signature
   scope view). A poll interval + Start streams repeated captures.
6. **Measurements & Cursors** — add/read automatic measurements in a table;
   cursor type/source/position controls with value & delta readout.
7. **Save / Screenshot** — save/recall setup; **Capture Screenshot** grabs the PNG
   block and displays it.

Reuse an ELoad-style `TesterCommon.h` (`TESTER_SCOPE`, `TESTER_CHANNEL`,
`tintButton()`), a dark palette + stylesheet in `applyDarkTheme()`, and a
`runSmokeTest()` / `screenshotTo()` path. **Avoid duplicate/repeated SCPI sends**
(cache reads; don't re-issue the same query in two code paths).

### 7b. AutoTest — console suite (returns nonzero on failure)

Cover: plugin discovery + bad-DLL rejection; resolve-completeness; every API group
against SimScope; MockVisa-backed real-model end-to-end incl. **binary waveform
round-trip** (preamble + block → expected synthetic signal within tolerance) and
**screenshot block**; per-model `getParameterRange` matrix (e.g. DSOX2012A, being
2-channel, rejects enabling channel 4 that MDO34 accepts, and TDS1012B rejects a
fast timebase that DSOS204A accepts); error paths (out-of-range, invalid scope/channel,
`NOT_SUPPORTED` via an un-overridden group, timeout resource); two simultaneous
instances (SimScope + real); two-thread concurrency (different scope numbers in
parallel, same number serialized); trace-log file + callback content.

---

## 8. Build / packaging / docs / CI

- **Packaging** (`scripts/package_deliverables.sh|.bat`): build the tree and
  assemble `deliverables/` = `include/` + `lib/` + `plugins/` + `bin/` +
  `examples/` + `docs/`. **Leak-guard: abort** if any internal header
  (`S_ScopeLimits.h`, `CMockScopeEmulator.h`) or any model/mock source would be
  copied. On Linux add an `nm -D` check that plugins export only the Qt plugin
  entry points (no accidental symbol leakage).
- **Docs** (`docs/`): `Architecture.md`, `API_Reference.md` (every method ↔ SCPI
  command ↔ per-model range), `Integration_Guide.md` (qmake/CMake consumer setup,
  VISA runtime discovery, Windows `objdump -p` export check), `Adding_A_New_Model.md`
  (limits row + info + `.pro` recipe), and a `Plugin_User_Manual.md` (discover →
  connect → configure vertical/horizontal/trigger → capture waveform → measure →
  screenshot; per-model spec table; error/troubleshooting table; hardware-free
  testing; naming convention; deployment).
- **CI** (`.github/workflows/ci.yml`, Linux, Qt 5.15): clean shadow build,
  `AutoTest` (exit code gates), `GuiTester --smoke` (offscreen), packaging.
- **examples/BasicUsage:** the minimal consumer from §9 below, built against the
  packaged tree only.

---

## 9. Quick-start shape the API must support (write this example)

```cpp
#include "ScopeManager.h"

int main() {
    CScopeManager& mgr = CScopeManager::instance();
    mgr.loadPlugins("plugins");                 // discover model DLLs
    mgr.createInstance(1, "MDO34");             // bind scope number 1 to a model

    S_Scope_ConnectionConfig cfg;
    cfg.setResourceString("USB0::0x0699::0x0522::C010000::INSTR");   // Tektronix VID 0x0699
    if (mgr.connect(1, cfg).isSuccess()) {
        mgr.enableChannel(1, 1, true);
        mgr.setVerticalScale(1, 1, 0.5);                        // 0.5 V/div
        mgr.setTimebaseScale(1, 1e-6);                          // 1 µs/div
        mgr.setTriggerSource(1, Enum_Scope_TriggerSource::m_enumCh1);
        mgr.setTriggerLevel(1, 1, 1.0);
        mgr.setAcqMode(1, Enum_Scope_AcqMode::m_enumSample);
        mgr.single(1);

        S_Scope_Waveform wfm;
        mgr.readWaveform(1, 1, wfm);                            // scaled V vs t

        S_Scope_MeasurementResult r;
        mgr.readMeasurement(1, 1, Enum_Scope_MeasType::m_enumFrequency, r);

        mgr.disconnect(1);
    }
    mgr.destroyInstance(1);
}
```

`examples/BasicUsage` must also open the virtual `SimScope`
(`cfg.setResourceString("SIM::MDO34")`) so it runs with no hardware.

---

## 10. Milestones (each = buildable + AutoTest-green + committed + pushed)

- **M0 scaffold:** tree + all `.pro` + `.gitignore`; clean `qmake && make`.
- **M1 contracts:** `ScopeTypes.h`, `ScopeError.h`, `IScopePlugin.h` (mandatory
  pure-virtual + `SCP_NS()` defaults), `ScopeManager.h`; a TU that instantiates
  the interface compiles.
- **M2 core + SimScope basics + AutoTest phase 1:** manager discovery/instances/
  errors/concurrency against a SimScope that does connect/vertical/timebase/
  trigger/acquire + synthesized `readWaveform`.
- **M3 VISA stack + MockVisa v1:** VisaHelper + MockVisa with `*IDN?`, status,
  and the binary waveform + screenshot blocks; AutoTest VISA-path tests.
- **M4 first real models:** the two ★ models `PluginMDO34` and `PluginRTM3004` —
  vertical, horizontal, trigger, acquisition, waveform transfer, IDN verify — with
  MockVisa answering every command each sends (Tektronix + R&S dialects).
- **M5 feature slices (one commit each, MockVisa + AutoTest grow in lockstep):**
  (a) measurements + cursors; (b) math/FFT; (c) display + save/recall + screenshot;
  (d) digital/MSO group; (e) AWG group; (f) status/system + remote/keylock.
- **M6 model fan-out:** the remaining 8 models (Keysight/Agilent, R&S RTO, legacy
  Tektronix TDS, LeCroy) + per-model `getParameterRange` matrix. Each fan-out model
  is gated on its §1b manual being available; otherwise land it against SimScope +
  MockVisa with `TODO`s for the hardware-verified limits.
- **M7 SimScope full API + SimScope/MockVisa parity harness.**
- **M8 GuiTester:** the 7 tabs + dark theme + waveform plot; offscreen `--smoke`.
- **M9 packaging + docs + examples + final full run.**

---

## 11. Verification & acceptance (Linux container)

1. Install `qtbase5-dev qt5-qmake qtbase5-dev-tools` once.
2. Every milestone: clean shadow build with **zero warnings**; run `AutoTest`
   (exit code gates the commit); M8+: `QT_QPA_PLATFORM=offscreen bin/GuiTester
   --smoke "$(pwd)/plugins"` (constructs UI, loads plugins, opens a SimScope,
   fetches one waveform, exits 0).
3. **Drop-in DLL test:** build a second model after the core, copy its DLL into
   `plugins/`, re-run discovery **without rebuilding the core**.
4. Two-instance + two-thread tests pass; grep-check that core sources never
   include model/Common headers; run the packaging leak guard; build
   `examples/` against `deliverables/` only.
5. **Windows/MinGW can't be exercised in a Linux container** — mitigate with
   strict C-ABI discipline (fixed-width typedefs, no `long`), reviewed `win32{}`
   qmake scopes, and the documented `objdump -p` export check; flag this residual
   risk in the final report. Avoid any "since Qt 5.15" API (target is 5.14.2).

---

## 12. Git workflow

- Develop on a dedicated feature branch (create locally if absent); commit each
  milestone with a clear message; push with `git push -u origin <branch>` (retry
  with exponential backoff on network errors). **Do not open a PR unless asked.**

---

## 13. Definition of done

- [ ] `qmake && make -j` clean, zero warnings, on Qt 5.14+/Linux.
- [ ] `AutoTest` exits 0 (all groups, both SimScope and MockVisa-backed real model,
      waveform round-trip, per-model ranges, error paths, 2 instances, concurrency).
- [ ] `GuiTester --smoke` exits 0 offscreen; interactive app drives a real capture
      and renders the waveform plot.
- [ ] 10 self-contained model plugins (MDO34, RTM3004, DSO7104B, DSOS204A,
      DSOX2012A, MSO6054A, RTO2064, TDS1012B, TDS2024C, WaveSurfer 42Xs) + SimScope,
      each with an embedded `S_ScopeLimits` row and no shared base class. (Models
      whose §1b manual is not yet available may land against SimScope + MockVisa
      with clearly-marked `TODO`s for hardware-verified limits.)
- [ ] All framework types are `S_Scope_<Name>` / `Enum_Scope_<Name>`; `ScopeError`
      + `S_Scope_DeviceErrorStatus` are `SCOPECORE_EXPORT`.
- [ ] Packaging leak-guard passes; `deliverables/` builds the example standalone.
- [ ] Five docs present incl. the user manual; CI green; committed and pushed.
