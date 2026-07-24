# Architecture

The Scope framework is a plugin-based oscilloscope control library modeled on
the in-house ELoad electronic-load framework. It separates a small, stable
**core** from independently shipped **model plugins**.

```
Application ──#include "ScopeManager.h"──▶ CScopeManager (singleton)
                                              │  qobject_cast + forward (per scope number)
                                              ▼
                                        CIScopePlugin (QPluginLoader)
                                          plugins/<Model>.so ──SCPI/VISA──▶ instrument
```

## Components

| Component | Role |
|-----------|------|
| `ScopeCore` (`CScopeManager`, `ScopeError`, `VisaHelper`) | Front-end singleton: plugin discovery, scope-number → plugin mapping, operation forwarding, error type. `SCOPECORE_LIBRARY`. |
| `include/` public SDK | `ScopeManager.h`, `IScopePlugin.h`, `ScopeError.h`, `ScopeTypes.h`, `VisaHelper.h`, `visa.h` — the only headers that ship. |
| `Common/S_ScopeLimits.h` | INTERNAL per-model limits struct + catalog. Never shipped. |
| `Plugin<Model>/` | Self-contained model plugins. Each implements the full `CIScopePlugin`, embeds its own `S_ScopeLimits` row, links VISA directly. No shared base class. |
| `PluginSimScope/` | Virtual model — synthesizes waveforms, no VISA, reuses the catalog rows. |
| `MockVisa/` | `libvisa` exposing the `vi*` ABI + `CMockScopeEmulator`. Lets real plugins run with no hardware. |
| `AutoTest/`, `GuiTester/` | Console + GUI testers. |

## Design invariants

1. **Discovery via `QPluginLoader`.** `CIScopePlugin` is a pure abstract Qt
   interface (`Q_DECLARE_INTERFACE`). Each model is a `QObject` with
   `Q_PLUGIN_METADATA(IID ScopePlugin_iid)` + `Q_INTERFACES`.
   `CScopeManager::loadPlugins(dir)` scans, loads and `qobject_cast`s each library.
2. **Manager singleton, keyed by scope number (+ 1-based channel).** A
   `QMap<QString,S_PluginData>` (plugin name → loader/plugin/info/caps) and a
   `QMap<U32BIT,QString>` (scope number → plugin name). Every op takes
   `in_u32ScopeNumber`; channel-scoped ops also take a 1-based `in_u32Channel`.
   A mutex serialises map access and per-scope calls.
3. **Mandatory vs optional.** Only `getPluginInfo/getCapabilities/connect/
   disconnect/isConnected/reset` are pure-virtual. Every other operation
   defaults to `NOT_SUPPORTED` via `SCP_NS()`, so a plugin overrides only what
   its model provides (e.g. only MSO models override the digital group; only
   models with a generator override the AWG group).
4. **Self-contained per-model plugins** — no shared base class; each embeds its
   own `S_ScopeLimits` row.
5. **Direct VISA linkage.** Real plugins `#include <visa.h>` and link `-lvisa`,
   one `ViSession` per scope. In-repo that resolves to MockVisa; on a bench, to
   the vendor NI-VISA.
6. **Local range validation** before any SCPI is sent (`PARAMETER_OUT_OF_RANGE`).
7. **Errors as return values** (`ScopeError` = code + description). `ScopeError`
   and `S_Scope_DeviceErrorStatus` are `SCOPECORE_EXPORT`.
8. **Binary waveform transfer is first-class**: counted `#<w><len><payload>`
   block reads with `:WAV:PRE?`/`WFMOutpre?` preamble parsing → scaled
   volts-vs-time. MockVisa emulates the block and a PNG screenshot block.

## Naming conventions

- Classes `C…`; framework structs `S_Scope_<Name>`; enums `Enum_Scope_<Name>`.
- Members `m_…`; params `in_…`/`out_…` with type tags (`in_u32ScopeNumber`,
  `in_dVoltsPerDiv`, `out_sWaveform`). Fixed-width typedefs only — no `long`.

## Build / directory layout

See `README.md`. Top-level `Scope.pro` is `TEMPLATE = subdirs, CONFIG += ordered`
with `<plugin>.depends = ScopeCore MockVisa`. Shadow build to `build/{lib,plugins,bin}`.
