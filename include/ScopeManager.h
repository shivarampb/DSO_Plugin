/**
 * @file    ScopeManager.h
 * @brief   CScopeManager — the framework front end (singleton facade).
 * @details Mirrors CELoadManager: a QObject singleton that discovers plugins
 *          with QPluginLoader, maps a 1-based scope number to a loaded plugin
 *          instance, and forwards every operation to the plugin that owns that
 *          scope number. All public operations are mutex-guarded.
 *
 *          @b Typical @b sequence (each step is a prerequisite of the next):
 *          @code
 *          CScopeManager& mgr = CScopeManager::instance();
 *          mgr.loadPlugins("plugins");                 // discover model DLLs
 *          mgr.createInstance(1, "MDO34");             // bind scope 1 to a model
 *          S_Scope_ConnectionConfig cfg;
 *          cfg.setResourceString("USB0::0x0699::0x0522::C0::INSTR");
 *          mgr.connect(1, cfg);                        // open the instrument
 *          mgr.enableChannel(1, 1, true);
 *          mgr.setVerticalScale(1, 1, 0.5);
 *          mgr.single(1);
 *          S_Scope_Waveform wfm; mgr.readWaveform(1, 1, wfm);
 *          @endcode
 *
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023 — the many per-operation members are thin,
 *          mutex-guarded forwarders (see ScopeManager.cpp / the SCP_FWD macro);
 *          their shared precondition is a bound instance (createInstance) and,
 *          for I/O, a successful connect().
 */
#ifndef SCOPEMANAGER_H
#define SCOPEMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QPluginLoader>
#include <QVector>
#include <QByteArray>

#include "IScopePlugin.h"
#include "ScopeError.h"
#include "ScopeTypes.h" // provides SCOPECORE_EXPORT

/* Live SCPI trace tap (optional). Direction: 0=TX 1=RX 2=ERR 3=INFO. */
typedef void (*ScopeTraceCallback)(void* in_pvUser, U32BIT in_u32ScopeNumber, int in_iDirection,
                                   const QString& in_strText);

class SCOPECORE_EXPORT CScopeManager : public QObject
{
    Q_OBJECT
  public:
    CScopeManager();
    ~CScopeManager();

    static CScopeManager& instance();

    /*==== plugin management ==============================================*/
    /**
     * @brief  Discover and load model plugins from a directory.
     * @pre    None.
     */
    void loadPlugins(const QString& in_strPluginPath);
    /**
     * @brief  List the names of the loaded plugins.
     * @pre    None.
     */
    QStringList getAvailablePlugins() const;
    /**
     * @brief  Return a loaded plugin's identity/metadata by name.
     * @pre    The named plugin is loaded.
     */
    S_Scope_PluginInfo getPluginInfoByName(const QString& in_strPluginName) const;
    /**
     * @brief  Return a loaded plugin's capability set by name.
     * @pre    The named plugin is loaded.
     */
    S_Scope_Capabilities getPluginCapabilities(const QString& in_strPluginName) const;
    /**
     * @brief  Return the core framework version string.
     * @pre    None.
     */
    QString getCoreVersion() const
    {
        return QStringLiteral("1.0.0");
    }

    /*==== instance management ============================================*/
    /**
     * @brief  Bind a scope number to a loaded plugin (create an instance).
     * @pre    loadPlugins() has run and the named plugin is loaded.
     */
    ScopeError createInstance(U32BIT in_u32ScopeNumber, const QString& in_strPluginName);
    /**
     * @brief  Destroy the instance bound to a scope number.
     * @pre    None.
     */
    ScopeError destroyInstance(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Report whether a scope number has a bound instance.
     * @pre    None.
     */
    bool instanceExists(U32BIT in_u32ScopeNumber) const;
    /**
     * @brief  Return the plugin name bound to a scope number.
     * @pre    None.
     */
    QString getInstancePlugin(U32BIT in_u32ScopeNumber) const;
    /**
     * @brief  Return the capability set of the instance bound to a scope number.
     * @pre    The scope number has a bound instance (createInstance).
     */
    S_Scope_Capabilities getCapabilities(U32BIT in_u32ScopeNumber) const;

    /*==== connection / core ==============================================*/
    /**
     * @brief  Open a connection to the instrument bound to a scope number.
     * @pre    The scope number has a bound instance (createInstance).
     */
    ScopeError connect(U32BIT in_u32ScopeNumber, const S_Scope_ConnectionConfig& in_sConfig);
    /**
     * @brief  Close the connection on a scope number.
     * @pre    None.
     */
    ScopeError disconnect(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Report whether a scope number is connected.
     * @pre    None.
     */
    bool isConnected(U32BIT in_u32ScopeNumber) const;
    /**
     * @brief  Reset the instrument to a known default state (*RST).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError reset(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Set the I/O timeout in milliseconds for the scope slot.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTimeout(U32BIT in_u32ScopeNumber, U32BIT in_u32TimeoutMs);
    /**
     * @brief  Clear the instrument status registers and error queue (*CLS).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError clearStatus(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Read the instrument identification string (*IDN?).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getIdentification(U32BIT in_u32ScopeNumber, QString& out_strIdn);
    /**
     * @brief  Run the instrument self-test and return its result code (*TST?).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError selfTest(U32BIT in_u32ScopeNumber, S32BIT& out_iResult);
    /**
     * @brief  Read the installed instrument options (*OPT?).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getOptions(U32BIT in_u32ScopeNumber, QString& out_strOptions);
    /**
     * @brief  Block until pending operations complete (*OPC?).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError waitOperationComplete(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Read the instrument SCPI version string.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getScpiVersion(U32BIT in_u32ScopeNumber, QString& out_strVersion);
    /**
     * @brief  Report the valid range/resolution of a parameter for a channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getParameterRange(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_ParamId in_eParam,
                                 S_Scope_ParameterRange& out_sRange);
    /**
     * @brief  Auto-scale the display to the active signals.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError autoscale(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Start continuous acquisition.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError run(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Stop acquisition.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError stop(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Arm a single acquisition.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError single(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Force an immediate trigger.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError forceTrigger(U32BIT in_u32ScopeNumber);

    /*==== vertical (per channel) =========================================*/
    /**
     * @brief  Enable or disable an analog channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError enableChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bOn);
    /**
     * @brief  Report whether an analog channel is enabled.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError isChannelEnabled(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool& out_bOn);
    /**
     * @brief  Set a channel vertical scale (volts per division).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv);
    /**
     * @brief  Get a channel vertical scale (volts per division).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVoltsPerDiv);
    /**
     * @brief  Set a channel vertical offset in volts.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVolts);
    /**
     * @brief  Get a channel vertical offset in volts.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVolts);
    /**
     * @brief  Set a channel vertical position in divisions.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setVerticalPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dDiv);
    /**
     * @brief  Get a channel vertical position in divisions.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getVerticalPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dDiv);
    /**
     * @brief  Set a channel input coupling (DC/AC/GND).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_Coupling in_eCoupling);
    /**
     * @brief  Get a channel input coupling.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                           Enum_Scope_Coupling& out_eCoupling);
    /**
     * @brief  Set a channel bandwidth limit.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                 Enum_Scope_BandwidthLimit in_eLimit);
    /**
     * @brief  Get a channel bandwidth limit.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                 Enum_Scope_BandwidthLimit& out_eLimit);
    /**
     * @brief  Set a channel probe attenuation ratio.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dRatio);
    /**
     * @brief  Get a channel probe attenuation ratio.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dRatio);
    /**
     * @brief  Set a channel input impedance (1 MOhm / 50 Ohm).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setInputImpedance(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                 Enum_Scope_InputImpedance in_eImp);
    /**
     * @brief  Get a channel input impedance.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getInputImpedance(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                 Enum_Scope_InputImpedance& out_eImp);
    /**
     * @brief  Enable or disable channel inversion.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setInvert(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bOn);
    /**
     * @brief  Report whether a channel is inverted.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getInvert(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool& out_bOn);
    /**
     * @brief  Set a channel display label.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setChannelLabel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const QString& in_strLabel);
    /**
     * @brief  Get a channel display label.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getChannelLabel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, QString& out_strLabel);
    /**
     * @brief  Set a channel vertical units string.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setChannelUnits(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const QString& in_strUnits);
    /**
     * @brief  Get a channel vertical units string.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getChannelUnits(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, QString& out_strUnits);
    /**
     * @brief  Set a channel deskew time in seconds.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setDeskew(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dSeconds);
    /**
     * @brief  Get a channel deskew time in seconds.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getDeskew(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dSeconds);

    /*==== horizontal / timebase ==========================================*/
    /**
     * @brief  Set the main timebase scale (seconds per division).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv);
    /**
     * @brief  Get the main timebase scale (seconds per division).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSecondsPerDiv);
    /**
     * @brief  Set the horizontal (trigger) position in seconds.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTimebasePosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds);
    /**
     * @brief  Get the horizontal (trigger) position in seconds.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTimebasePosition(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSeconds);
    /**
     * @brief  Set the timebase reference point (percent of screen).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTimebaseReference(U32BIT in_u32ScopeNumber, FDOUBLE in_dPercent);
    /**
     * @brief  Set the timebase mode (main/zoom/roll/XY).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTimebaseMode(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode in_eMode);
    /**
     * @brief  Get the timebase mode.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTimebaseMode(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode& out_eMode);
    /**
     * @brief  Read the current sample rate in samples per second.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getSampleRate(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSaPerSec);
    /**
     * @brief  Set the acquisition memory depth in points.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setMemoryDepth(U32BIT in_u32ScopeNumber, U32BIT in_u32Points);
    /**
     * @brief  Get the acquisition memory depth in points.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getMemoryDepth(U32BIT in_u32ScopeNumber, U32BIT& out_u32Points);
    /**
     * @brief  Set the number of acquisition points.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setAcquisitionPoints(U32BIT in_u32ScopeNumber, U32BIT in_u32Points);
    /**
     * @brief  Get the number of acquisition points.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getAcquisitionPoints(U32BIT in_u32ScopeNumber, U32BIT& out_u32Points);

    /*==== trigger ========================================================*/
    /**
     * @brief  Set the trigger sweep mode (auto/normal).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerMode(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerMode in_eMode);
    /**
     * @brief  Get the trigger sweep mode.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTriggerMode(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerMode& out_eMode);
    /**
     * @brief  Set the trigger type (edge/pulse/video/pattern).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerType(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerType in_eType);
    /**
     * @brief  Get the trigger type.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTriggerType(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerType& out_eType);
    /**
     * @brief  Set the trigger source channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerSource(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSource in_eSource);
    /**
     * @brief  Get the trigger source channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTriggerSource(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSource& out_eSource);
    /**
     * @brief  Set the edge-trigger slope.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerSlope(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSlope in_eSlope);
    /**
     * @brief  Get the edge-trigger slope.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTriggerSlope(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSlope& out_eSlope);
    /**
     * @brief  Set the trigger level in volts for a source channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerLevel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVolts);
    /**
     * @brief  Get the trigger level in volts for a source channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTriggerLevel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVolts);
    /**
     * @brief  Set the trigger coupling.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerCoupling(U32BIT in_u32ScopeNumber, Enum_Scope_Coupling in_eCoupling);
    /**
     * @brief  Get the trigger coupling.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTriggerCoupling(U32BIT in_u32ScopeNumber, Enum_Scope_Coupling& out_eCoupling);
    /**
     * @brief  Set the trigger holdoff time in seconds.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerHoldoff(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds);
    /**
     * @brief  Get the trigger holdoff time in seconds.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTriggerHoldoff(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSeconds);
    /**
     * @brief  Read the current trigger state.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getTriggerState(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerState& out_eState);
    /**
     * @brief  Set the pulse-width trigger width in seconds.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerPulseWidth(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds);
    /**
     * @brief  Set the video-trigger standard.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerVideoStandard(U32BIT in_u32ScopeNumber, const QString& in_strStandard);
    /**
     * @brief  Set the pattern-trigger pattern string.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setTriggerPattern(U32BIT in_u32ScopeNumber, const QString& in_strPattern);

    /*==== acquisition ====================================================*/
    /**
     * @brief  Set the acquisition mode (sample/peak/average/hi-res/envelope).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setAcqMode(U32BIT in_u32ScopeNumber, Enum_Scope_AcqMode in_eMode);
    /**
     * @brief  Get the acquisition mode.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getAcqMode(U32BIT in_u32ScopeNumber, Enum_Scope_AcqMode& out_eMode);
    /**
     * @brief  Set the averaging count.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setAverageCount(U32BIT in_u32ScopeNumber, U32BIT in_u32Count);
    /**
     * @brief  Get the averaging count.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getAverageCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Count);
    /**
     * @brief  Read the acquisition state.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getAcquisitionState(U32BIT in_u32ScopeNumber, Enum_Scope_AcqState& out_eState);
    /**
     * @brief  Set the number of segmented-memory segments.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setSegmentedCount(U32BIT in_u32ScopeNumber, U32BIT in_u32Segments);
    /**
     * @brief  Get the number of segmented-memory segments.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getSegmentedCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Segments);

    /*==== waveform transfer ==============================================*/
    /**
     * @brief  Select the waveform transfer source.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setWaveformSource(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformSource in_eSource,
                                 U32BIT in_u32Index);
    /**
     * @brief  Get the waveform transfer source.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getWaveformSource(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformSource& out_eSource,
                                 U32BIT& out_u32Index);
    /**
     * @brief  Set the waveform transfer data format (byte/word/ascii).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setWaveformFormat(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformFormat in_eFormat);
    /**
     * @brief  Get the waveform transfer data format.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getWaveformFormat(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformFormat& out_eFormat);
    /**
     * @brief  Set the number of points to transfer.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setWaveformPoints(U32BIT in_u32ScopeNumber, U32BIT in_u32Points);
    /**
     * @brief  Read the waveform preamble (scaling/origin metadata).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getWaveformPreamble(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                   S_Scope_WaveformPreamble& out_sPreamble);
    /**
     * @brief  Fetch a channel waveform scaled to volts vs. time.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError readWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Scope_Waveform& out_sWaveform);
    /**
     * @brief  Force a synchronized single acquisition of a channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError digitizeChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);

    /*==== automatic measurements =========================================*/
    /**
     * @brief  Add an automatic measurement on a channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError addMeasurement(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_MeasType in_eType);
    /**
     * @brief  Read an automatic measurement value and units.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError readMeasurement(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_MeasType in_eType,
                               S_Scope_MeasurementResult& out_sResult);
    /**
     * @brief  Clear all active automatic measurements.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError clearMeasurements(U32BIT in_u32ScopeNumber);
    /**
     * @brief  Enable or disable measurement statistics.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setMeasureStatistics(U32BIT in_u32ScopeNumber, bool in_bOn);
    /**
     * @brief  Read measurement statistics (min/max/mean/stddev/count).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getMeasurementStatistics(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                        Enum_Scope_MeasType in_eType, S_Scope_MeasurementResult& out_sResult);

    /*==== math / FFT =====================================================*/
    /**
     * @brief  Set the math waveform operation.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setMathOperation(U32BIT in_u32ScopeNumber, Enum_Scope_MathOp in_eOp);
    /**
     * @brief  Set the first math source channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setMathSource1(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);
    /**
     * @brief  Set the second math source channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setMathSource2(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);
    /**
     * @brief  Enable or disable the math waveform.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError enableMath(U32BIT in_u32ScopeNumber, bool in_bOn);
    /**
     * @brief  Set the FFT window function.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setFftWindow(U32BIT in_u32ScopeNumber, Enum_Scope_FftWindow in_eWindow);
    /**
     * @brief  Get the FFT window function.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getFftWindow(U32BIT in_u32ScopeNumber, Enum_Scope_FftWindow& out_eWindow);
    /**
     * @brief  Set the FFT frequency span in hertz.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setFftSpan(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz);
    /**
     * @brief  Set the FFT center frequency in hertz.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setFftCenter(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz);
    /**
     * @brief  Set the math waveform vertical scale.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setMathScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dScale);
    /**
     * @brief  Set the math waveform vertical position.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setMathPosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dPosition);

    /*==== cursors ========================================================*/
    /**
     * @brief  Set the cursor type (off/horizontal/vertical/track).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setCursorType(U32BIT in_u32ScopeNumber, Enum_Scope_CursorType in_eType);
    /**
     * @brief  Get the cursor type.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getCursorType(U32BIT in_u32ScopeNumber, Enum_Scope_CursorType& out_eType);
    /**
     * @brief  Set the cursor source channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setCursorSource(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);
    /**
     * @brief  Set a cursor position.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setCursorPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32CursorIndex, FDOUBLE in_dPosition);
    /**
     * @brief  Get a cursor position.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getCursorPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32CursorIndex, FDOUBLE& out_dPosition);
    /**
     * @brief  Read the cursor X/Y values and their deltas.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError readCursorValues(U32BIT in_u32ScopeNumber, FDOUBLE& out_dX1, FDOUBLE& out_dX2,
                                FDOUBLE& out_dY1, FDOUBLE& out_dY2);

    /*==== display ========================================================*/
    /**
     * @brief  Set the display persistence time in seconds.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setPersistence(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds);
    /**
     * @brief  Set the display graticule type.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setGraticule(U32BIT in_u32ScopeNumber, const QString& in_strType);
    /**
     * @brief  Set the waveform display intensity (percent).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setIntensity(U32BIT in_u32ScopeNumber, FDOUBLE in_dPercent);
    /**
     * @brief  Set the display format.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setDisplayFormat(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode in_eFormat);
    /**
     * @brief  Enable or disable vector (connected-dot) drawing.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setVectors(U32BIT in_u32ScopeNumber, bool in_bOn);

    /*==== save / recall / screenshot =====================================*/
    /**
     * @brief  Save the instrument setup to an internal location.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError saveSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location);
    /**
     * @brief  Recall an instrument setup from an internal location.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError recallSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location);
    /**
     * @brief  Save a channel waveform to an instrument file.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError saveWaveformToFile(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const QString& in_strPath);
    /**
     * @brief  Capture a display screenshot in the given image format.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError captureScreenshot(U32BIT in_u32ScopeNumber, Enum_Scope_ImageFormat in_eFormat,
                                 QByteArray& out_imageBytes);
    /**
     * @brief  Store a channel waveform into a reference slot.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError saveToReference(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, U32BIT in_u32RefSlot);
    /**
     * @brief  Show or hide a reference waveform.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError displayReference(U32BIT in_u32ScopeNumber, U32BIT in_u32RefSlot, bool in_bOn);

    /*==== digital / MSO ==================================================*/
    /**
     * @brief  Enable or disable a digital (MSO) channel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError enableDigitalChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel, bool in_bOn);
    /**
     * @brief  Set a digital channel logic threshold.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setDigitalThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel, FDOUBLE in_dVolts);
    /**
     * @brief  Set a digital pod logic threshold.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setPodThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32Pod, FDOUBLE in_dVolts);
    /**
     * @brief  Enable or disable a serial-bus decoder.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError enableBus(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, bool in_bOn);
    /**
     * @brief  Set a serial-bus protocol type (I2C/SPI/UART/CAN/LIN).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setBusType(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, const QString& in_strType);
    /**
     * @brief  Read the decoded frames from a serial bus.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError readBusDecode(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, QString& out_strDecode);

    /*==== AWG / Wavegen ==================================================*/
    /**
     * @brief  Set the built-in generator waveform function.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setAwgFunction(U32BIT in_u32ScopeNumber, const QString& in_strFunction);
    /**
     * @brief  Set the generator frequency in hertz.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setAwgFrequency(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz);
    /**
     * @brief  Set the generator amplitude in volts.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setAwgAmplitude(U32BIT in_u32ScopeNumber, FDOUBLE in_dVolts);
    /**
     * @brief  Set the generator DC offset in volts.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setAwgOffset(U32BIT in_u32ScopeNumber, FDOUBLE in_dVolts);
    /**
     * @brief  Enable or disable the generator output.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError enableAwgOutput(U32BIT in_u32ScopeNumber, bool in_bOn);

    /*==== status / system ================================================*/
    /**
     * @brief  Populate the device error/status structure.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError readErrorStatus(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                               S_Scope_DeviceErrorStatus& out_sStatus);
    /**
     * @brief  Clear the device error/status state.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError clearErrorStatus(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel);
    /**
     * @brief  Pop one entry from the instrument error queue (SYST:ERR?).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_strMessage);
    /**
     * @brief  Report the number of queued instrument errors.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getInstrumentErrorCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Count);
    /**
     * @brief  Read the standard event status register (*ESR?).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError readStandardEventStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status);
    /**
     * @brief  Read the status byte register (*STB?).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError readStatusByte(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status);
    /**
     * @brief  Read the operation status register.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError readOperationStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status);
    /**
     * @brief  Read the questionable status register.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError readQuestionableStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status);
    /**
     * @brief  Set the instrument remote/local state.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setRemoteState(U32BIT in_u32ScopeNumber, Enum_Scope_RemoteState in_eState);
    /**
     * @brief  Get the instrument remote/local state.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError getRemoteState(U32BIT in_u32ScopeNumber, Enum_Scope_RemoteState& out_eState);
    /**
     * @brief  Lock or unlock the front panel.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setKeyLock(U32BIT in_u32ScopeNumber, bool in_bOn);
    /**
     * @brief  Report whether the front panel is locked.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError isKeyLocked(U32BIT in_u32ScopeNumber, bool& out_bOn);
    /**
     * @brief  Enable or disable the instrument beeper.
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError setBeeper(U32BIT in_u32ScopeNumber, bool in_bOn);

    /*==== debug ==========================================================*/
    /**
     * @brief  Send a raw SCPI command (debug escape hatch).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError writeScpi(U32BIT in_u32ScopeNumber, const QString& in_strCommand);
    /**
     * @brief  Send a raw SCPI query and return the response (debug escape hatch).
     * @pre    The scope number has a connected instance (createInstance + connect()).
     */
    ScopeError queryScpi(U32BIT in_u32ScopeNumber, const QString& in_strCommand, QString& out_strResponse);

    /*==== trace tap ======================================================*/
    /**
     * @brief  Install (or clear) the live SCPI trace callback.
     * @pre    None.
     */
    void setTraceCallback(ScopeTraceCallback in_pfnCallback, void* in_pvUser);

  signals:
    void pluginLoaded(const QString& in_strPluginName);
    void pluginLoadFailed(const QString& in_strFileName, const QString& in_strError);
    void instanceCreated(U32BIT in_u32ScopeNumber, const QString& in_strPluginName);
    void instanceDestroyed(U32BIT in_u32ScopeNumber);
    void errorOccurred(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                       const S_Scope_DeviceErrorStatus& in_sStatus);

  private:
    CScopeManager(const CScopeManager&) = delete;
    CScopeManager& operator=(const CScopeManager&) = delete;

    CIScopePlugin* getPlugin(U32BIT in_u32ScopeNumber) const;

    struct S_PluginData
    {
        QPluginLoader* m_pLoader;
        CIScopePlugin* m_pPlugin;
        S_Scope_PluginInfo m_sInfo;
        S_Scope_Capabilities m_sCaps;
        S_PluginData() : m_pLoader(nullptr), m_pPlugin(nullptr)
        {
        }
    };

    QMap<QString, S_PluginData> m_mapPlugins; // plugin name -> data
    QMap<U32BIT, QString> m_mapInstances;     // scope number -> plugin name
    mutable QMutex m_mutex;                   // guards the maps + per-scope calls
    ScopeTraceCallback m_pfnTrace;
    void* m_pvTraceUser;
};

#endif // SCOPEMANAGER_H
