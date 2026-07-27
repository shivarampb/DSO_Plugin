# TDS2024C - Tektronix (TDS2000C)

Place the following PDFs in this folder (see `../README.md` for the sourcing
recipe). They are **not shipped**.

1. **TDS1000C/2000C Programmer Manual** - the SCPI reference (single source of truth).
2. **TDS2024C datasheet** - drives this model's `S_ScopeLimits` row.

Expected files (any filename is fine; these are suggestions):
- `TDS2024C_ProgrammingManual.pdf`
- `TDS2024C_Datasheet.pdf`

**Status:** headline specs seeded from the datasheet; not yet verified against the programming manual.

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
