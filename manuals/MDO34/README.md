# MDO34 - Tektronix (3 Series MDO)

Place the following PDFs in this folder (see `../README.md` for the sourcing
recipe). They are **not shipped**.

1. **3 Series MDO Programmer Manual** - the SCPI reference (single source of truth).
2. **MDO34 datasheet** - drives this model's `S_ScopeLimits` row.

Expected files (any filename is fine; these are suggestions):
- `MDO34_ProgrammingManual.pdf`
- `MDO34_Datasheet.pdf`

**Status:** Headline specs VERIFIED vs datasheet (1 GHz, 4 ch, 5 GS/s, 10 M record, 16 digital opt, AWG opt).

## Verification checklist

Once the manual PDF is in this folder, verify **page-by-page** against it and
update this model's row in `Common/S_ScopeLimits.h` (and, for MDO34/RTM3004, the
literal row embedded in the plugin constructor). Remove the `TODO(manual)`
markers in the plugin as each item is confirmed.

**Numeric limits (`S_ScopeLimits` row):**
- [ ] analog channel count, digital channel count (if MSO)
- [ ] analog bandwidth (model variants)
- [ ] max real-time sample rate
- [ ] max record length / memory depth
- [ ] vertical scale range (V/div) min..max, vertical offset range
- [ ] timebase scale range (s/div) min..max
- [ ] trigger holdoff min..max
- [ ] max average count
- [ ] built-in AWG frequency/amplitude max (if fitted)
- [ ] USB VID/PID, `*IDN?` response layout, SCPI line terminator

**SCPI commands (per operation group in the plugin `.cpp`):**
- [ ] vertical (scale/offset/position/coupling/bandwidth/probe/impedance)
- [ ] horizontal / timebase, sample-rate & record-length queries
- [ ] trigger (mode/type/source/slope/level/holdoff, type-specific)
- [ ] acquisition (mode/average/state, run/stop/single)
- [ ] waveform transfer: preamble format + binary block encoding & byte order
- [ ] measurements (type tokens + value query)
- [ ] math/FFT, cursors, display, save/recall, screenshot
- [ ] digital/MSO group (MSO models), serial-bus decode readout
- [ ] AWG/Wavegen group (models with a generator)
- [ ] status/system (status registers, remote/keylock/beeper)
