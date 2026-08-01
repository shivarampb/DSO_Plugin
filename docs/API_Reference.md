# API Reference

All operations return `ScopeError` (unless noted). `isSuccess()` is true on
`SUCCESS`. Every op takes `in_u32ScopeNumber`; channel-scoped ops also take a
1-based `in_u32Channel`. Methods not overridden by a model return
`NOT_SUPPORTED`.

## Plugin & instance management (`CScopeManager`)

| Method | Purpose |
|--------|---------|
| `loadPlugins(dir)` | Discover `*.so`/`*.dll` model plugins in `dir`. |
| `getAvailablePlugins()` | Names of loaded plugins. |
| `getPluginInfoByName` / `getPluginCapabilities` | Static metadata / capabilities. |
| `createInstance(scope, name)` / `destroyInstance(scope)` | Bind/unbind a scope number to a model. |
| `instanceExists` / `getInstancePlugin` / `getCapabilities(scope)` | Introspection. |
| signals `pluginLoaded/pluginLoadFailed/instanceCreated/instanceDestroyed/errorOccurred` | Async notifications. |

## Connection / core

`connect(scope, S_Scope_ConnectionConfig)`, `disconnect`, `isConnected`,
`reset`, `setTimeout`, `clearStatus`, `getIdentification`, `selfTest`,
`getOptions`, `waitOperationComplete`, `getScpiVersion`, `getParameterRange`,
`autoscale`, `run`, `stop`, `single`, `forceTrigger`.

## Operation groups (method ↔ SCPI ↔ range)

The SCPI column shows a representative command for each dialect; ranges come
from the model's `S_ScopeLimits` row and are enforced locally before the
command is sent.

| Group | Method | Tektronix (MDO34) | R&S (RTM3004) | Range source |
|-------|--------|-------------------|---------------|--------------|
| Vertical | `setVerticalScale` | `CH<n>:SCAle` | `CHANnel<n>:SCALe` | `VertScaleMin..Max` |
| | `setVerticalOffset` | `CH<n>:OFFSet` | `CHANnel<n>:OFFSet` | `±VertOffsetMax` |
| | `setCoupling` | `CH<n>:COUPling DC\|AC\|GND` | `CHANnel<n>:COUPling DCLimit\|ACLimit\|GND` | enum |
| | `enableChannel` | `SELect:CH<n>` | `CHANnel<n>:STATe` | channel count |
| | `setProbeAttenuation` | `CH<n>:PRObe:GAIN` | `PROBe<n>:SETup:ATTenuation:MANual` | discrete set |
| Horizontal | `setTimebaseScale` | `HORizontal:SCAle` | `TIMebase:SCALe` | `TimebaseMin..Max` |
| | `getSampleRate` | `HORizontal:SAMPLERate?` | `ACQuire:SRATe?` | ≤ `MaxSampleRate` |
| | `setMemoryDepth` | `HORizontal:RECOrdlength` | `ACQuire:POINts` | ≤ `MaxMemoryDepth` |
| Trigger | `setTriggerSource` | `TRIGger:A:EDGE:SOUrce` | `TRIGger:A:SOURce` | enum |
| | `setTriggerSlope` | `TRIGger:A:EDGE:SLOpe` | `TRIGger:A:EDGE:SLOPe` | enum |
| | `setTriggerLevel` | `TRIGger:A:LEVel:CH<n>` | `TRIGger:A:LEVel<n>:VALue` | `±VertOffsetMax` |
| | `setTriggerHoldoff` | `TRIGger:A:HOLDoff:TIMe` | `TRIGger:A:HOLDoff:TIME` | `TrigHoldoffMin..Max` |
| Acquisition | `setAcqMode` | `ACQuire:MODe` | `ACQuire:MODE` | enum |
| | `setAverageCount` | `ACQuire:NUMAVg` | `ACQuire:AVERage:COUNt` | ≤ `AvgCountMax` |
| Waveform | `getWaveformPreamble` | `WFMOutpre?` | `CHANnel<n>:DATA:HEADer?` | — |
| | `readWaveform` | `CURVe?` (`#`-block) | `CHANnel<n>:DATA?` (`#`-block) | — |
| Status | `readErrorStatus` | `*ESR?` + `SYST:ERR?` | `*ESR?` + `SYST:ERR?` | — |
| Debug | `writeScpi` / `queryScpi` | raw | raw | — |

Optional groups exposed by the interface (overridden only by models that
support them): **measurements** (`addMeasurement`, `readMeasurement`,
statistics), **math/FFT**, **cursors**, **display**, **save/recall/screenshot**
(`captureScreenshot` → PNG bytes), **digital/MSO**, **AWG/Wavegen**, and the
**status/system** group (remote/keylock/beeper).

## Waveform data types

- `S_Scope_WaveformPreamble` — `format,type,points,count,xInc,xOrigin,xRef,yInc,yOrigin,yRef`.
- `S_Scope_Waveform` — `m_sPreamble`, `m_vecTimeSeconds`, `m_vecVolts`,
  `m_u32SourceChannel`. Volts are reconstructed as
  `(code − yRef)·yInc + yOrigin`; time as `xOrigin + (i − xRef)·xInc`.
- `S_Scope_MeasurementResult` — `m_eType`, `m_dValue`, `m_bValid`, `m_strUnits`
  (+ optional statistics).

See `include/IScopePlugin.h` for the full, grouped method list.
