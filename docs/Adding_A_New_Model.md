# Adding a New Model

Each model is a **self-contained plugin** — its own class, its own SCPI, and its
own embedded `S_ScopeLimits` row. There is no shared base class to subclass.

## 1. Obtain the manual (source of truth)

Place the model's programming/interface manual and datasheet under
`manuals/<Model>/`. Every SCPI command, numeric range, resolution and response
format in the plugin must be verified page-by-page against that manual. Until
the manual is available, land the model against SimScope + MockVisa and mark the
unverified constants with `// TODO(manual): ...`.

## 2. Add the limits row

Two places use the model's limits:

- **The plugin embeds its own row** in its constructor (initializer list), so it
  is self-contained.
- **The catalog** in `Common/S_ScopeLimits.h` (`ScopeLimitsCatalog`) — add a row
  so `SimScope` can simulate the model and `getParameterRange` matches the real
  plugin by construction.

Row fields: name, manufacturer, series, `*IDN?` match token, analog/digital
channel counts, bandwidth, max sample rate, max memory depth, vertical
scale min/max + offset max, timebase min/max, trigger holdoff min/max, average
count max, feature flags (AWG/FFT/digital/serial/segmented), AWG freq/ampl max,
USB VID/PID.

## 3. Create the plugin

Folder `Plugin<Model>/` with `<Model>Plugin.h`, `<Model>Plugin.cpp`,
`Plugin<Model>.pro`. Fastest path: copy `PluginMDO34/` (Tektronix dialect) or
`PluginRTM3004/` (R&S dialect), then:

1. Rename the class to `C<Model>Plugin` (drop non-identifier chars), the include
   guard, and the `TARGET`.
2. Replace the embedded `m_limits{...}` row and `m_strModel`.
3. Adjust the SCPI tokens to the model's dialect (vertical/horizontal/trigger/
   acquisition/waveform). Reuse the transport helpers verbatim
   (`writeLine/readLine/queryLine/readBinaryBlock/sendChecked/queryDouble/setDouble`).
4. Override only the groups the model supports; leave the rest as the interface's
   `NOT_SUPPORTED` defaults (e.g. override the digital/MSO group only for MSO
   models, the AWG group only for models with a generator).

`.pro` recipe (copy an existing one):

```pro
QT -= gui
TEMPLATE = lib
CONFIG += plugin c++11
unix: QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
TARGET = Plugin<Model>
DESTDIR = ../plugins
INCLUDEPATH += ../include ../Common
LIBS += -L../lib -lScopeCore
win32 { LIBS += -L"C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc" -lvisa64 }
unix  { LIBS += -L../lib -lvisa }
HEADERS += <Model>Plugin.h
SOURCES += <Model>Plugin.cpp
```

## 4. Register in the build

Add `Plugin<Model>` to `Scope.pro` `SUBDIRS` with
`Plugin<Model>.depends = ScopeCore MockVisa`.

## 5. Teach MockVisa (if needed)

If the model uses a waveform/preamble/screenshot command the emulator does not
yet recognise, extend the matchers in `MockVisa/CMockScopeEmulator.cpp`. The
emulator keys `*IDN?` off the resource-string model token, so `MOCK0::<Model>::INSTR`
returns the correct identity automatically once the catalog row exists.

## 6. Test

Add the model to `AutoTest` (discovery, a `getParameterRange` row, an
end-to-end waveform round-trip via `MOCK0::<Model>::INSTR`, and any
range-boundary case that distinguishes it from another model — e.g. a
channel-count or fast-timebase rejection). Build the plugin, drop its `.so` into
`plugins/`, and re-run discovery **without rebuilding the core** to confirm the
drop-in contract.
