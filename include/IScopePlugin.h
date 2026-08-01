/**
 * @file    IScopePlugin.h
 * @brief   Abstract Qt plugin interface for an oscilloscope model.
 * @details Mirrors the ELoad framework's CIELoadPlugin: a Qt plugin interface
 *          loaded with QPluginLoader and resolved with qobject_cast. Each model
 *          plugin is a QObject deriving from CIScopePlugin and declaring
 *          @code Q_PLUGIN_METADATA(IID ScopePlugin_iid) @endcode and
 *          @code Q_INTERFACES(CIScopePlugin) @endcode.
 *
 *          Every operation takes the 1-based scope number and, where it applies,
 *          a 1-based channel (channel is first-class — scopes are inherently
 *          multi-channel).
 *
 *          @b Prerequisites (apply to every operation below): getPluginInfo /
 *          getCapabilities may be called any time; connect() requires the plugin
 *          to have been instantiated by the manager; every other operation
 *          requires a successful connect() first, and returns NOT_CONNECTED
 *          otherwise. Only getPluginInfo/getCapabilities/connect/disconnect/
 *          isConnected/reset are mandatory (pure virtual); every other operation
 *          defaults to NOT_SUPPORTED via SCP_NS(), so a plugin overrides only
 *          what its model provides (e.g. only MSO models override the
 *          digital-channel group; only models with a generator override AWG).
 *
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023 — the default (not-overridden) bodies deliberately
 *          ignore their named parameters; -Wunused-parameter is suppressed for
 *          this interface only, between the diagnostic push/pop below.
 */
#ifndef ISCOPEPLUGIN_H
#define ISCOPEPLUGIN_H

#include <QtPlugin>
#include <QVector>
#include <QByteArray>

#include "ScopeError.h"
#include "ScopeTypes.h"

#define ScopePlugin_iid "com.automation.ScopePlugin/1.0"

/* Default body for a not-overridden (unsupported) operation. Resolves to the
 * protected static CIScopePlugin::NotSupported() in complete-class context. */
#define SCP_NS() return CIScopePlugin::NotSupported()

/* The many default NOT_SUPPORTED stubs deliberately ignore their (named, for
 * documentation) parameters; silence -Wunused-parameter for this interface. */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

class CIScopePlugin
{
  public:
    virtual ~CIScopePlugin()
    {
    }

    /*==== plugin information (mandatory) ==================================*/
    /**
     * @brief  Return static identity/metadata for this plugin model.
     * @pre    None.
     */
    virtual S_Scope_PluginInfo getPluginInfo() const = 0;
    /**
     * @brief  Return the model capability set (channels, ranges, feature flags).
     * @pre    None.
     */
    virtual S_Scope_Capabilities getCapabilities() const = 0;

    /*==== connection management (mandatory) ==============================*/
    /**
     * @brief  Open a connection to the instrument on the given scope slot.
     * @pre    The plugin is instantiated by the manager and the slot is not already connected.
     */
    virtual ScopeError connect(U32BIT in_u32ScopeNumber, const S_Scope_ConnectionConfig& in_sConfig) = 0;
    /**
     * @brief  Close the connection on the given scope slot.
     * @pre    None.
     */
    virtual ScopeError disconnect(U32BIT in_u32ScopeNumber) = 0;
    /**
     * @brief  Report whether the given scope slot is connected.
     * @pre    None.
     */
    virtual bool isConnected(U32BIT in_u32ScopeNumber) const = 0;
    /**
     * @brief  Reset the instrument to a known default state (*RST).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError reset(U32BIT in_u32ScopeNumber) = 0;

    /*==== plugin info / connection (optional core) =======================*/
    /**
     * @brief  Set the I/O timeout in milliseconds for the scope slot.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTimeout(U32BIT in_u32ScopeNumber, U32BIT in_u32TimeoutMs)
    {
        SCP_NS();
    }
    /**
     * @brief  Clear the instrument status registers and error queue (*CLS).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError clearStatus(U32BIT in_u32ScopeNumber)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the instrument identification string (*IDN?).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getIdentification(U32BIT in_u32ScopeNumber, QString& out_strIdn)
    {
        SCP_NS();
    }
    /**
     * @brief  Run the instrument self-test and return its result code (*TST?).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError selfTest(U32BIT in_u32ScopeNumber, S32BIT& out_iResult)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the installed instrument options (*OPT?).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getOptions(U32BIT in_u32ScopeNumber, QString& out_strOptions)
    {
        SCP_NS();
    }
    /**
     * @brief  Block until pending operations complete (*OPC?).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError waitOperationComplete(U32BIT in_u32ScopeNumber)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the instrument SCPI version string.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getScpiVersion(U32BIT in_u32ScopeNumber, QString& out_strVersion)
    {
        SCP_NS();
    }
    /**
     * @brief  Report the valid range/resolution of a parameter for a channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getParameterRange(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                         Enum_Scope_ParamId in_eParam, S_Scope_ParameterRange& out_sRange)
    {
        SCP_NS();
    }
    /**
     * @brief  Auto-scale the display to the active signals.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError autoscale(U32BIT in_u32ScopeNumber)
    {
        SCP_NS();
    }
    /**
     * @brief  Start continuous acquisition.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError run(U32BIT in_u32ScopeNumber)
    {
        SCP_NS();
    }
    /**
     * @brief  Stop acquisition.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError stop(U32BIT in_u32ScopeNumber)
    {
        SCP_NS();
    }
    /**
     * @brief  Arm a single acquisition.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError single(U32BIT in_u32ScopeNumber)
    {
        SCP_NS();
    }
    /**
     * @brief  Force an immediate trigger.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError forceTrigger(U32BIT in_u32ScopeNumber)
    {
        SCP_NS();
    }

    /*==== vertical (per channel) =========================================*/
    /**
     * @brief  Enable or disable an analog channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError enableChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Report whether an analog channel is enabled.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError isChannelEnabled(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool& out_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel vertical scale (volts per division).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                        FDOUBLE in_dVoltsPerDiv)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel vertical scale (volts per division).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                        FDOUBLE& out_dVoltsPerDiv)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel vertical offset in volts.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVolts)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel vertical offset in volts.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVolts)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel vertical position in divisions.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setVerticalPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dDiv)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel vertical position in divisions.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getVerticalPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dDiv)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel input coupling (DC/AC/GND).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                   Enum_Scope_Coupling in_eCoupling)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel input coupling.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                   Enum_Scope_Coupling& out_eCoupling)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel bandwidth limit.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                         Enum_Scope_BandwidthLimit in_eLimit)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel bandwidth limit.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                         Enum_Scope_BandwidthLimit& out_eLimit)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel probe attenuation ratio.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dRatio)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel probe attenuation ratio.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                           FDOUBLE& out_dRatio)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel input impedance (1 MOhm / 50 Ohm).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setInputImpedance(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                         Enum_Scope_InputImpedance in_eImp)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel input impedance.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getInputImpedance(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                         Enum_Scope_InputImpedance& out_eImp)
    {
        SCP_NS();
    }
    /**
     * @brief  Enable or disable channel inversion.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setInvert(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Report whether a channel is inverted.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getInvert(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool& out_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel display label.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setChannelLabel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                       const QString& in_strLabel)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel display label.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getChannelLabel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, QString& out_strLabel)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel vertical units string.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setChannelUnits(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                       const QString& in_strUnits)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel vertical units string.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getChannelUnits(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, QString& out_strUnits)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a channel deskew time in seconds.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setDeskew(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dSeconds)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a channel deskew time in seconds.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getDeskew(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dSeconds)
    {
        SCP_NS();
    }

    /*==== horizontal / timebase ==========================================*/
    /**
     * @brief  Set the main timebase scale (seconds per division).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the main timebase scale (seconds per division).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSecondsPerDiv)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the horizontal (trigger) position in seconds.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTimebasePosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the horizontal (trigger) position in seconds.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTimebasePosition(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSeconds)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the timebase reference point (percent of screen).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTimebaseReference(U32BIT in_u32ScopeNumber, FDOUBLE in_dPercent)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the timebase mode (main/zoom/roll/XY).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTimebaseMode(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode in_eMode)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the timebase mode.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTimebaseMode(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode& out_eMode)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the current sample rate in samples per second.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getSampleRate(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSaPerSec)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the acquisition memory depth in points.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setMemoryDepth(U32BIT in_u32ScopeNumber, U32BIT in_u32Points)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the acquisition memory depth in points.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getMemoryDepth(U32BIT in_u32ScopeNumber, U32BIT& out_u32Points)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the number of acquisition points.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setAcquisitionPoints(U32BIT in_u32ScopeNumber, U32BIT in_u32Points)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the number of acquisition points.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getAcquisitionPoints(U32BIT in_u32ScopeNumber, U32BIT& out_u32Points)
    {
        SCP_NS();
    }

    /*==== trigger ========================================================*/
    /**
     * @brief  Set the trigger sweep mode (auto/normal).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerMode(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerMode in_eMode)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the trigger sweep mode.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTriggerMode(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerMode& out_eMode)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the trigger type (edge/pulse/video/pattern).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerType(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerType in_eType)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the trigger type.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTriggerType(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerType& out_eType)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the trigger source channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerSource(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSource in_eSource)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the trigger source channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTriggerSource(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSource& out_eSource)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the edge-trigger slope.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerSlope(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSlope in_eSlope)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the edge-trigger slope.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTriggerSlope(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSlope& out_eSlope)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the trigger level in volts for a source channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerLevel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVolts)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the trigger level in volts for a source channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTriggerLevel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVolts)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the trigger coupling.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerCoupling(U32BIT in_u32ScopeNumber, Enum_Scope_Coupling in_eCoupling)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the trigger coupling.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTriggerCoupling(U32BIT in_u32ScopeNumber, Enum_Scope_Coupling& out_eCoupling)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the trigger holdoff time in seconds.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerHoldoff(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the trigger holdoff time in seconds.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTriggerHoldoff(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSeconds)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the current trigger state.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getTriggerState(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerState& out_eState)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the pulse-width trigger width in seconds.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerPulseWidth(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the video-trigger standard.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerVideoStandard(U32BIT in_u32ScopeNumber, const QString& in_strStandard)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the pattern-trigger pattern string.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setTriggerPattern(U32BIT in_u32ScopeNumber, const QString& in_strPattern)
    {
        SCP_NS();
    }

    /*==== acquisition ====================================================*/
    /**
     * @brief  Set the acquisition mode (sample/peak/average/hi-res/envelope).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setAcqMode(U32BIT in_u32ScopeNumber, Enum_Scope_AcqMode in_eMode)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the acquisition mode.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getAcqMode(U32BIT in_u32ScopeNumber, Enum_Scope_AcqMode& out_eMode)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the averaging count.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setAverageCount(U32BIT in_u32ScopeNumber, U32BIT in_u32Count)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the averaging count.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getAverageCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Count)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the acquisition state.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getAcquisitionState(U32BIT in_u32ScopeNumber, Enum_Scope_AcqState& out_eState)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the number of segmented-memory segments.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setSegmentedCount(U32BIT in_u32ScopeNumber, U32BIT in_u32Segments)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the number of segmented-memory segments.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getSegmentedCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Segments)
    {
        SCP_NS();
    }

    /*==== waveform transfer ==============================================*/
    /**
     * @brief  Select the waveform transfer source.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setWaveformSource(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformSource in_eSource,
                                         U32BIT in_u32Index)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the waveform transfer source.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getWaveformSource(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformSource& out_eSource,
                                         U32BIT& out_u32Index)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the waveform transfer data format (byte/word/ascii).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setWaveformFormat(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformFormat in_eFormat)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the waveform transfer data format.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getWaveformFormat(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformFormat& out_eFormat)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the number of points to transfer.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setWaveformPoints(U32BIT in_u32ScopeNumber, U32BIT in_u32Points)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the waveform preamble (scaling/origin metadata).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getWaveformPreamble(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                           S_Scope_WaveformPreamble& out_sPreamble)
    {
        SCP_NS();
    }
    /**
     * @brief  Fetch a channel waveform scaled to volts vs. time.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError readWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                    S_Scope_Waveform& out_sWaveform)
    {
        SCP_NS();
    }
    /**
     * @brief  Force a synchronized single acquisition of a channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError digitizeChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel)
    {
        SCP_NS();
    }

    /*==== automatic measurements =========================================*/
    /**
     * @brief  Add an automatic measurement on a channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError addMeasurement(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                      Enum_Scope_MeasType in_eType)
    {
        SCP_NS();
    }
    /**
     * @brief  Read an automatic measurement value and units.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError readMeasurement(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                       Enum_Scope_MeasType in_eType, S_Scope_MeasurementResult& out_sResult)
    {
        SCP_NS();
    }
    /**
     * @brief  Clear all active automatic measurements.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError clearMeasurements(U32BIT in_u32ScopeNumber)
    {
        SCP_NS();
    }
    /**
     * @brief  Enable or disable measurement statistics.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setMeasureStatistics(U32BIT in_u32ScopeNumber, bool in_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Read measurement statistics (min/max/mean/stddev/count).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getMeasurementStatistics(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                                Enum_Scope_MeasType in_eType,
                                                S_Scope_MeasurementResult& out_sResult)
    {
        SCP_NS();
    }

    /*==== math / FFT =====================================================*/
    /**
     * @brief  Set the math waveform operation.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setMathOperation(U32BIT in_u32ScopeNumber, Enum_Scope_MathOp in_eOp)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the first math source channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setMathSource1(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the second math source channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setMathSource2(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel)
    {
        SCP_NS();
    }
    /**
     * @brief  Enable or disable the math waveform.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError enableMath(U32BIT in_u32ScopeNumber, bool in_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the FFT window function.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setFftWindow(U32BIT in_u32ScopeNumber, Enum_Scope_FftWindow in_eWindow)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the FFT window function.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getFftWindow(U32BIT in_u32ScopeNumber, Enum_Scope_FftWindow& out_eWindow)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the FFT frequency span in hertz.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setFftSpan(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the FFT center frequency in hertz.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setFftCenter(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the math waveform vertical scale.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setMathScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dScale)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the math waveform vertical position.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setMathPosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dPosition)
    {
        SCP_NS();
    }

    /*==== cursors ========================================================*/
    /**
     * @brief  Set the cursor type (off/horizontal/vertical/track).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setCursorType(U32BIT in_u32ScopeNumber, Enum_Scope_CursorType in_eType)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the cursor type.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getCursorType(U32BIT in_u32ScopeNumber, Enum_Scope_CursorType& out_eType)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the cursor source channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setCursorSource(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a cursor position.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setCursorPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32CursorIndex,
                                         FDOUBLE in_dPosition)
    {
        SCP_NS();
    }
    /**
     * @brief  Get a cursor position.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getCursorPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32CursorIndex,
                                         FDOUBLE& out_dPosition)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the cursor X/Y values and their deltas.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError readCursorValues(U32BIT in_u32ScopeNumber, FDOUBLE& out_dX1, FDOUBLE& out_dX2,
                                        FDOUBLE& out_dY1, FDOUBLE& out_dY2)
    {
        SCP_NS();
    }

    /*==== display ========================================================*/
    /**
     * @brief  Set the display persistence time in seconds.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setPersistence(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the display graticule type.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setGraticule(U32BIT in_u32ScopeNumber, const QString& in_strType)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the waveform display intensity (percent).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setIntensity(U32BIT in_u32ScopeNumber, FDOUBLE in_dPercent)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the display format.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setDisplayFormat(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode in_eFormat)
    {
        SCP_NS();
    }
    /**
     * @brief  Enable or disable vector (connected-dot) drawing.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setVectors(U32BIT in_u32ScopeNumber, bool in_bOn)
    {
        SCP_NS();
    }

    /*==== save / recall / screenshot =====================================*/
    /**
     * @brief  Save the instrument setup to an internal location.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError saveSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location)
    {
        SCP_NS();
    }
    /**
     * @brief  Recall an instrument setup from an internal location.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError recallSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location)
    {
        SCP_NS();
    }
    /**
     * @brief  Save a channel waveform to an instrument file.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError saveWaveformToFile(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                          const QString& in_strPath)
    {
        SCP_NS();
    }
    /**
     * @brief  Capture a display screenshot in the given image format.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError captureScreenshot(U32BIT in_u32ScopeNumber, Enum_Scope_ImageFormat in_eFormat,
                                         QByteArray& out_imageBytes)
    {
        SCP_NS();
    }
    /**
     * @brief  Store a channel waveform into a reference slot.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError saveToReference(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, U32BIT in_u32RefSlot)
    {
        SCP_NS();
    }
    /**
     * @brief  Show or hide a reference waveform.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError displayReference(U32BIT in_u32ScopeNumber, U32BIT in_u32RefSlot, bool in_bOn)
    {
        SCP_NS();
    }

    /*==== digital / MSO (optional group - only MSO models override) ======*/
    /**
     * @brief  Enable or disable a digital (MSO) channel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError enableDigitalChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel,
                                            bool in_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a digital channel logic threshold.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setDigitalThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel,
                                           FDOUBLE in_dVolts)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a digital pod logic threshold.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setPodThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32Pod, FDOUBLE in_dVolts)
    {
        SCP_NS();
    }
    /**
     * @brief  Enable or disable a serial-bus decoder.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError enableBus(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, bool in_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Set a serial-bus protocol type (I2C/SPI/UART/CAN/LIN).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setBusType(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, const QString& in_strType)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the decoded frames from a serial bus.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError readBusDecode(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, QString& out_strDecode)
    {
        SCP_NS();
    }

    /*==== AWG / Wavegen (optional group - only models with a generator) ===*/
    /**
     * @brief  Set the built-in generator waveform function.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setAwgFunction(U32BIT in_u32ScopeNumber, const QString& in_strFunction)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the generator frequency in hertz.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setAwgFrequency(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the generator amplitude in volts.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setAwgAmplitude(U32BIT in_u32ScopeNumber, FDOUBLE in_dVolts)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the generator DC offset in volts.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setAwgOffset(U32BIT in_u32ScopeNumber, FDOUBLE in_dVolts)
    {
        SCP_NS();
    }
    /**
     * @brief  Enable or disable the generator output.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError enableAwgOutput(U32BIT in_u32ScopeNumber, bool in_bOn)
    {
        SCP_NS();
    }

    /*==== status / system ================================================*/
    /**
     * @brief  Populate the device error/status structure.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError readErrorStatus(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                       S_Scope_DeviceErrorStatus& out_sStatus)
    {
        SCP_NS();
    }
    /**
     * @brief  Clear the device error/status state.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError clearErrorStatus(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel)
    {
        SCP_NS();
    }
    /**
     * @brief  Pop one entry from the instrument error queue (SYST:ERR?).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_strMessage)
    {
        SCP_NS();
    }
    /**
     * @brief  Report the number of queued instrument errors.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getInstrumentErrorCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Count)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the standard event status register (*ESR?).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError readStandardEventStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the status byte register (*STB?).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError readStatusByte(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the operation status register.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError readOperationStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
    {
        SCP_NS();
    }
    /**
     * @brief  Read the questionable status register.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError readQuestionableStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
    {
        SCP_NS();
    }
    /**
     * @brief  Set the instrument remote/local state.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setRemoteState(U32BIT in_u32ScopeNumber, Enum_Scope_RemoteState in_eState)
    {
        SCP_NS();
    }
    /**
     * @brief  Get the instrument remote/local state.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError getRemoteState(U32BIT in_u32ScopeNumber, Enum_Scope_RemoteState& out_eState)
    {
        SCP_NS();
    }
    /**
     * @brief  Lock or unlock the front panel.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setKeyLock(U32BIT in_u32ScopeNumber, bool in_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Report whether the front panel is locked.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError isKeyLocked(U32BIT in_u32ScopeNumber, bool& out_bOn)
    {
        SCP_NS();
    }
    /**
     * @brief  Enable or disable the instrument beeper.
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError setBeeper(U32BIT in_u32ScopeNumber, bool in_bOn)
    {
        SCP_NS();
    }

    /*==== debug escape hatch (diagnostics only) ==========================*/
    /**
     * @brief  Send a raw SCPI command (debug escape hatch).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError writeScpi(U32BIT in_u32ScopeNumber, const QString& in_strCommand)
    {
        SCP_NS();
    }
    /**
     * @brief  Send a raw SCPI query and return the response (debug escape hatch).
     * @pre    A connected scope (see connect()).
     */
    virtual ScopeError queryScpi(U32BIT in_u32ScopeNumber, const QString& in_strCommand,
                                 QString& out_strResponse)
    {
        SCP_NS();
    }

  protected:
    // helper used by the default (not-overridden) bodies
    static ScopeError NotSupported()
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED,
                          QStringLiteral("operation not supported by this model"));
    }
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#undef SCP_NS

Q_DECLARE_INTERFACE(CIScopePlugin, ScopePlugin_iid)

#endif // ISCOPEPLUGIN_H
