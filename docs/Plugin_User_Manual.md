# Plugin User Manual

A task-oriented guide to driving oscilloscopes through the Scope library.

## 1. Discover → connect

```cpp
CScopeManager& mgr = CScopeManager::instance();
mgr.loadPlugins("plugins");                 // load model DLLs
mgr.createInstance(1, "MDO34");             // bind scope number 1 to a model
S_Scope_ConnectionConfig cfg;
cfg.setResourceString("USB0::0x0699::0x0522::C010000::INSTR");
ScopeError e = mgr.connect(1, cfg);         // verifies *IDN? names the model
```

`connect` fails with `CONNECTION_FAILED` if `*IDN?` does not name the expected
model, so a mismatched plugin never drives the wrong instrument.

## 2. Configure vertical (per channel)

```cpp
mgr.enableChannel(1, 1, true);
mgr.setVerticalScale(1, 1, 0.5);            // 0.5 V/div
mgr.setVerticalOffset(1, 1, 0.0);
mgr.setCoupling(1, 1, Enum_Scope_Coupling::m_enumDC);
mgr.setProbeAttenuation(1, 1, 10.0);        // discrete ratio
```

Setters validate against the model's limits **before** touching the instrument;
an out-of-range value returns `PARAMETER_OUT_OF_RANGE` and sends nothing. Query
the live limits for a UI with `getParameterRange(scope, channel, paramId, range)`.

## 3. Configure horizontal & trigger

```cpp
mgr.setTimebaseScale(1, 1e-6);              // 1 us/div
mgr.setTriggerSource(1, Enum_Scope_TriggerSource::m_enumCh1);
mgr.setTriggerSlope(1, Enum_Scope_TriggerSlope::m_enumRising);
mgr.setTriggerLevel(1, 1, 1.0);
mgr.setTriggerMode(1, Enum_Scope_TriggerMode::m_enumNormal);
```

## 4. Acquire & capture a waveform

```cpp
mgr.setAcqMode(1, Enum_Scope_AcqMode::m_enumSample);
mgr.single(1);                              // one acquisition
S_Scope_Waveform wfm;
mgr.readWaveform(1, 1, wfm);                // scaled volts vs. time
// wfm.m_vecVolts[i] at wfm.m_vecTimeSeconds[i]
```

`readWaveform` sets the source, selects a binary format, reads the preamble and
the `#`-block, and reconstructs real volts and time. `run(scope)` / `stop(scope)`
control continuous acquisition; `getAcquisitionState` reports it.

## 5. Automatic measurements

```cpp
S_Scope_MeasurementResult r;
mgr.readMeasurement(1, 1, Enum_Scope_MeasType::m_enumFrequency, r);
// r.m_dValue, r.m_strUnits, r.m_bValid
```

## 6. Screenshot

```cpp
QByteArray png;
if (mgr.captureScreenshot(1, Enum_Scope_ImageFormat::m_enumPng, png).isSuccess())
    /* png holds an image */;
```

## Per-model spec table (headline)

| Model | Vendor | BW | Analog ch | Digital | AWG | Notes |
|-------|--------|----|-----------|---------|-----|-------|
| MDO34 ★ | Tektronix | 1 GHz | 4 | 16 (opt) | 50 MHz (opt) | mixed-domain |
| RTM3004 ★ | Rohde & Schwarz | 1 GHz | 4 | 16 (opt) | built-in | |
| DSO7104B | Keysight | 1 GHz | 4 | — | — | InfiniiVision 7000 |
| DSOS204A | Keysight | 2 GHz | 4 | — | — | deep memory |
| DSOX2012A | Keysight | 100 MHz | 2 | — | 20 MHz (opt) | 2-channel |
| MSO6054A | Agilent | 500 MHz | 4 | 16 | — | mixed-signal |
| RTO2064 | Rohde & Schwarz | 600 MHz | 4 | — | — | high end |
| TDS1012B | Tektronix | 100 MHz | 2 | — | — | legacy SCPI |
| TDS2024C | Tektronix | 200 MHz | 4 | — | — | legacy SCPI |
| WaveSurfer 42Xs | Teledyne LeCroy | 400 MHz | 4 | — | — | MAUI |

Ranges tagged `TODO(manual)` in `Common/S_ScopeLimits.h` await verification
against each model's programming manual.

## Error / troubleshooting

| Code | Meaning | Typical fix |
|------|---------|-------------|
| `NOT_CONNECTED` | op before `connect` | connect first |
| `INVALID_SCOPE_NUMBER` | no instance for that scope | `createInstance` |
| `INVALID_CHANNEL` | channel absent on this model | check `getCapabilities` |
| `PARAMETER_OUT_OF_RANGE` | value outside model limits | query `getParameterRange` |
| `NOT_SUPPORTED` | model lacks that feature | check capability flags |
| `COMMUNICATION_TIMEOUT` | no response | check cabling / `setTimeout` |
| `CONNECTION_FAILED` | `*IDN?` mismatch | wrong plugin for the instrument |

## Hardware-free testing

- `SimScope`: `cfg.setResourceString("SIM::MDO34")` — synthesized waveforms, no VISA.
- Real plugin via MockVisa: `cfg.setResourceString("MOCK0::MDO34::INSTR")` — exercises
  the real SCPI/VISA path with no instrument. `MOCK0::TIMEOUT::INSTR` forces a timeout.

## Deployment

Ship `include/` + `libScopeCore` + the `plugins/` you need. Do **not** ship
`libvisa` (MockVisa) or any `Common/`/mock header to a bench — install the vendor
VISA there instead. See `Integration_Guide.md`.
