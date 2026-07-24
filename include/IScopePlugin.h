/*============================================================================
 *  IScopePlugin.h
 *
 *  Abstract plugin interface for an oscilloscope model. Mirrors the ELoad
 *  framework's CIELoadPlugin: a Qt plugin interface loaded with QPluginLoader
 *  and resolved with qobject_cast. Each model plugin is a QObject deriving
 *  from CIScopePlugin and declaring:
 *      Q_PLUGIN_METADATA(IID ScopePlugin_iid)
 *      Q_INTERFACES(CIScopePlugin)
 *
 *  Every operation takes the logical scope number (1-based) and, where it
 *  applies, a 1-based channel (scopes are inherently multi-channel, so channel
 *  is first-class here). Non-mandatory operations default to NOT_SUPPORTED, so
 *  a plugin overrides only what its model provides (e.g. only MSO models
 *  override the digital-channel group; only models with a built-in generator
 *  override the AWG group).
 *
 *  \author  Scope framework
 *==========================================================================*/
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
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

class CIScopePlugin
{
public:
    virtual ~CIScopePlugin() {}

    /*==== plugin information (mandatory) ==================================*/
    virtual S_Scope_PluginInfo   getPluginInfo() const = 0;
    virtual S_Scope_Capabilities getCapabilities() const = 0;

    /*==== connection management (mandatory) ==============================*/
    virtual ScopeError connect(U32BIT in_u32ScopeNumber, const S_Scope_ConnectionConfig& in_sConfig) = 0;
    virtual ScopeError disconnect(U32BIT in_u32ScopeNumber) = 0;
    virtual bool       isConnected(U32BIT in_u32ScopeNumber) const = 0;
    virtual ScopeError reset(U32BIT in_u32ScopeNumber) = 0;

    /*==== plugin info / connection (optional core) =======================*/
    virtual ScopeError setTimeout(U32BIT in_u32ScopeNumber, U32BIT in_u32TimeoutMs) { SCP_NS(); }
    virtual ScopeError clearStatus(U32BIT in_u32ScopeNumber) { SCP_NS(); }
    virtual ScopeError getIdentification(U32BIT in_u32ScopeNumber, QString& out_strIdn) { SCP_NS(); }
    virtual ScopeError selfTest(U32BIT in_u32ScopeNumber, S32BIT& out_iResult) { SCP_NS(); }
    virtual ScopeError getOptions(U32BIT in_u32ScopeNumber, QString& out_strOptions) { SCP_NS(); }
    virtual ScopeError waitOperationComplete(U32BIT in_u32ScopeNumber) { SCP_NS(); }
    virtual ScopeError getScpiVersion(U32BIT in_u32ScopeNumber, QString& out_strVersion) { SCP_NS(); }
    virtual ScopeError getParameterRange(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                         Enum_Scope_ParamId in_eParam, S_Scope_ParameterRange& out_sRange) { SCP_NS(); }
    virtual ScopeError autoscale(U32BIT in_u32ScopeNumber) { SCP_NS(); }
    virtual ScopeError run(U32BIT in_u32ScopeNumber) { SCP_NS(); }
    virtual ScopeError stop(U32BIT in_u32ScopeNumber) { SCP_NS(); }
    virtual ScopeError single(U32BIT in_u32ScopeNumber) { SCP_NS(); }
    virtual ScopeError forceTrigger(U32BIT in_u32ScopeNumber) { SCP_NS(); }

    /*==== vertical (per channel) =========================================*/
    virtual ScopeError enableChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bOn) { SCP_NS(); }
    virtual ScopeError isChannelEnabled(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool& out_bOn) { SCP_NS(); }
    virtual ScopeError setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv) { SCP_NS(); }
    virtual ScopeError getVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVoltsPerDiv) { SCP_NS(); }
    virtual ScopeError setVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVolts) { SCP_NS(); }
    virtual ScopeError getVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVolts) { SCP_NS(); }
    virtual ScopeError setVerticalPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dDiv) { SCP_NS(); }
    virtual ScopeError getVerticalPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dDiv) { SCP_NS(); }
    virtual ScopeError setCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_Coupling in_eCoupling) { SCP_NS(); }
    virtual ScopeError getCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_Coupling& out_eCoupling) { SCP_NS(); }
    virtual ScopeError setBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_BandwidthLimit in_eLimit) { SCP_NS(); }
    virtual ScopeError getBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_BandwidthLimit& out_eLimit) { SCP_NS(); }
    virtual ScopeError setProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dRatio) { SCP_NS(); }
    virtual ScopeError getProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dRatio) { SCP_NS(); }
    virtual ScopeError setInputImpedance(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_InputImpedance in_eImp) { SCP_NS(); }
    virtual ScopeError getInputImpedance(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_InputImpedance& out_eImp) { SCP_NS(); }
    virtual ScopeError setInvert(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bOn) { SCP_NS(); }
    virtual ScopeError getInvert(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool& out_bOn) { SCP_NS(); }
    virtual ScopeError setChannelLabel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const QString& in_strLabel) { SCP_NS(); }
    virtual ScopeError getChannelLabel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, QString& out_strLabel) { SCP_NS(); }
    virtual ScopeError setChannelUnits(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const QString& in_strUnits) { SCP_NS(); }
    virtual ScopeError getChannelUnits(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, QString& out_strUnits) { SCP_NS(); }
    virtual ScopeError setDeskew(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dSeconds) { SCP_NS(); }
    virtual ScopeError getDeskew(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dSeconds) { SCP_NS(); }

    /*==== horizontal / timebase ==========================================*/
    virtual ScopeError setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv) { SCP_NS(); }
    virtual ScopeError getTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSecondsPerDiv) { SCP_NS(); }
    virtual ScopeError setTimebasePosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds) { SCP_NS(); }
    virtual ScopeError getTimebasePosition(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSeconds) { SCP_NS(); }
    virtual ScopeError setTimebaseReference(U32BIT in_u32ScopeNumber, FDOUBLE in_dPercent) { SCP_NS(); }
    virtual ScopeError setTimebaseMode(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode in_eMode) { SCP_NS(); }
    virtual ScopeError getTimebaseMode(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode& out_eMode) { SCP_NS(); }
    virtual ScopeError getSampleRate(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSaPerSec) { SCP_NS(); }
    virtual ScopeError setMemoryDepth(U32BIT in_u32ScopeNumber, U32BIT in_u32Points) { SCP_NS(); }
    virtual ScopeError getMemoryDepth(U32BIT in_u32ScopeNumber, U32BIT& out_u32Points) { SCP_NS(); }
    virtual ScopeError setAcquisitionPoints(U32BIT in_u32ScopeNumber, U32BIT in_u32Points) { SCP_NS(); }
    virtual ScopeError getAcquisitionPoints(U32BIT in_u32ScopeNumber, U32BIT& out_u32Points) { SCP_NS(); }

    /*==== trigger ========================================================*/
    virtual ScopeError setTriggerMode(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerMode in_eMode) { SCP_NS(); }
    virtual ScopeError getTriggerMode(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerMode& out_eMode) { SCP_NS(); }
    virtual ScopeError setTriggerType(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerType in_eType) { SCP_NS(); }
    virtual ScopeError getTriggerType(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerType& out_eType) { SCP_NS(); }
    virtual ScopeError setTriggerSource(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSource in_eSource) { SCP_NS(); }
    virtual ScopeError getTriggerSource(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSource& out_eSource) { SCP_NS(); }
    virtual ScopeError setTriggerSlope(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSlope in_eSlope) { SCP_NS(); }
    virtual ScopeError getTriggerSlope(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerSlope& out_eSlope) { SCP_NS(); }
    virtual ScopeError setTriggerLevel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVolts) { SCP_NS(); }
    virtual ScopeError getTriggerLevel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE& out_dVolts) { SCP_NS(); }
    virtual ScopeError setTriggerCoupling(U32BIT in_u32ScopeNumber, Enum_Scope_Coupling in_eCoupling) { SCP_NS(); }
    virtual ScopeError getTriggerCoupling(U32BIT in_u32ScopeNumber, Enum_Scope_Coupling& out_eCoupling) { SCP_NS(); }
    virtual ScopeError setTriggerHoldoff(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds) { SCP_NS(); }
    virtual ScopeError getTriggerHoldoff(U32BIT in_u32ScopeNumber, FDOUBLE& out_dSeconds) { SCP_NS(); }
    virtual ScopeError getTriggerState(U32BIT in_u32ScopeNumber, Enum_Scope_TriggerState& out_eState) { SCP_NS(); }
    virtual ScopeError setTriggerPulseWidth(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds) { SCP_NS(); }
    virtual ScopeError setTriggerVideoStandard(U32BIT in_u32ScopeNumber, const QString& in_strStandard) { SCP_NS(); }
    virtual ScopeError setTriggerPattern(U32BIT in_u32ScopeNumber, const QString& in_strPattern) { SCP_NS(); }

    /*==== acquisition ====================================================*/
    virtual ScopeError setAcqMode(U32BIT in_u32ScopeNumber, Enum_Scope_AcqMode in_eMode) { SCP_NS(); }
    virtual ScopeError getAcqMode(U32BIT in_u32ScopeNumber, Enum_Scope_AcqMode& out_eMode) { SCP_NS(); }
    virtual ScopeError setAverageCount(U32BIT in_u32ScopeNumber, U32BIT in_u32Count) { SCP_NS(); }
    virtual ScopeError getAverageCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Count) { SCP_NS(); }
    virtual ScopeError getAcquisitionState(U32BIT in_u32ScopeNumber, Enum_Scope_AcqState& out_eState) { SCP_NS(); }
    virtual ScopeError setSegmentedCount(U32BIT in_u32ScopeNumber, U32BIT in_u32Segments) { SCP_NS(); }
    virtual ScopeError getSegmentedCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Segments) { SCP_NS(); }

    /*==== waveform transfer ==============================================*/
    virtual ScopeError setWaveformSource(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformSource in_eSource, U32BIT in_u32Index) { SCP_NS(); }
    virtual ScopeError getWaveformSource(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformSource& out_eSource, U32BIT& out_u32Index) { SCP_NS(); }
    virtual ScopeError setWaveformFormat(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformFormat in_eFormat) { SCP_NS(); }
    virtual ScopeError getWaveformFormat(U32BIT in_u32ScopeNumber, Enum_Scope_WaveformFormat& out_eFormat) { SCP_NS(); }
    virtual ScopeError setWaveformPoints(U32BIT in_u32ScopeNumber, U32BIT in_u32Points) { SCP_NS(); }
    virtual ScopeError getWaveformPreamble(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Scope_WaveformPreamble& out_sPreamble) { SCP_NS(); }
    virtual ScopeError readWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Scope_Waveform& out_sWaveform) { SCP_NS(); }
    virtual ScopeError digitizeChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel) { SCP_NS(); }

    /*==== automatic measurements =========================================*/
    virtual ScopeError addMeasurement(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_MeasType in_eType) { SCP_NS(); }
    virtual ScopeError readMeasurement(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_MeasType in_eType, S_Scope_MeasurementResult& out_sResult) { SCP_NS(); }
    virtual ScopeError clearMeasurements(U32BIT in_u32ScopeNumber) { SCP_NS(); }
    virtual ScopeError setMeasureStatistics(U32BIT in_u32ScopeNumber, bool in_bOn) { SCP_NS(); }
    virtual ScopeError getMeasurementStatistics(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Scope_MeasType in_eType, S_Scope_MeasurementResult& out_sResult) { SCP_NS(); }

    /*==== math / FFT =====================================================*/
    virtual ScopeError setMathOperation(U32BIT in_u32ScopeNumber, Enum_Scope_MathOp in_eOp) { SCP_NS(); }
    virtual ScopeError setMathSource1(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel) { SCP_NS(); }
    virtual ScopeError setMathSource2(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel) { SCP_NS(); }
    virtual ScopeError enableMath(U32BIT in_u32ScopeNumber, bool in_bOn) { SCP_NS(); }
    virtual ScopeError setFftWindow(U32BIT in_u32ScopeNumber, Enum_Scope_FftWindow in_eWindow) { SCP_NS(); }
    virtual ScopeError getFftWindow(U32BIT in_u32ScopeNumber, Enum_Scope_FftWindow& out_eWindow) { SCP_NS(); }
    virtual ScopeError setFftSpan(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz) { SCP_NS(); }
    virtual ScopeError setFftCenter(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz) { SCP_NS(); }
    virtual ScopeError setMathScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dScale) { SCP_NS(); }
    virtual ScopeError setMathPosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dPosition) { SCP_NS(); }

    /*==== cursors ========================================================*/
    virtual ScopeError setCursorType(U32BIT in_u32ScopeNumber, Enum_Scope_CursorType in_eType) { SCP_NS(); }
    virtual ScopeError getCursorType(U32BIT in_u32ScopeNumber, Enum_Scope_CursorType& out_eType) { SCP_NS(); }
    virtual ScopeError setCursorSource(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel) { SCP_NS(); }
    virtual ScopeError setCursorPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32CursorIndex, FDOUBLE in_dPosition) { SCP_NS(); }
    virtual ScopeError getCursorPosition(U32BIT in_u32ScopeNumber, U32BIT in_u32CursorIndex, FDOUBLE& out_dPosition) { SCP_NS(); }
    virtual ScopeError readCursorValues(U32BIT in_u32ScopeNumber, FDOUBLE& out_dX1, FDOUBLE& out_dX2, FDOUBLE& out_dY1, FDOUBLE& out_dY2) { SCP_NS(); }

    /*==== display ========================================================*/
    virtual ScopeError setPersistence(U32BIT in_u32ScopeNumber, FDOUBLE in_dSeconds) { SCP_NS(); }
    virtual ScopeError setGraticule(U32BIT in_u32ScopeNumber, const QString& in_strType) { SCP_NS(); }
    virtual ScopeError setIntensity(U32BIT in_u32ScopeNumber, FDOUBLE in_dPercent) { SCP_NS(); }
    virtual ScopeError setDisplayFormat(U32BIT in_u32ScopeNumber, Enum_Scope_TimebaseMode in_eFormat) { SCP_NS(); }
    virtual ScopeError setVectors(U32BIT in_u32ScopeNumber, bool in_bOn) { SCP_NS(); }

    /*==== save / recall / screenshot =====================================*/
    virtual ScopeError saveSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location) { SCP_NS(); }
    virtual ScopeError recallSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location) { SCP_NS(); }
    virtual ScopeError saveWaveformToFile(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, const QString& in_strPath) { SCP_NS(); }
    virtual ScopeError captureScreenshot(U32BIT in_u32ScopeNumber, Enum_Scope_ImageFormat in_eFormat, QByteArray& out_imageBytes) { SCP_NS(); }
    virtual ScopeError saveToReference(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, U32BIT in_u32RefSlot) { SCP_NS(); }
    virtual ScopeError displayReference(U32BIT in_u32ScopeNumber, U32BIT in_u32RefSlot, bool in_bOn) { SCP_NS(); }

    /*==== digital / MSO (optional group - only MSO models override) ======*/
    virtual ScopeError enableDigitalChannel(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel, bool in_bOn) { SCP_NS(); }
    virtual ScopeError setDigitalThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel, FDOUBLE in_dVolts) { SCP_NS(); }
    virtual ScopeError setPodThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32Pod, FDOUBLE in_dVolts) { SCP_NS(); }
    virtual ScopeError enableBus(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, bool in_bOn) { SCP_NS(); }
    virtual ScopeError setBusType(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, const QString& in_strType) { SCP_NS(); }
    virtual ScopeError readBusDecode(U32BIT in_u32ScopeNumber, U32BIT in_u32Bus, QString& out_strDecode) { SCP_NS(); }

    /*==== AWG / Wavegen (optional group - only models with a generator) ===*/
    virtual ScopeError setAwgFunction(U32BIT in_u32ScopeNumber, const QString& in_strFunction) { SCP_NS(); }
    virtual ScopeError setAwgFrequency(U32BIT in_u32ScopeNumber, FDOUBLE in_dHz) { SCP_NS(); }
    virtual ScopeError setAwgAmplitude(U32BIT in_u32ScopeNumber, FDOUBLE in_dVolts) { SCP_NS(); }
    virtual ScopeError setAwgOffset(U32BIT in_u32ScopeNumber, FDOUBLE in_dVolts) { SCP_NS(); }
    virtual ScopeError enableAwgOutput(U32BIT in_u32ScopeNumber, bool in_bOn) { SCP_NS(); }

    /*==== status / system ================================================*/
    virtual ScopeError readErrorStatus(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Scope_DeviceErrorStatus& out_sStatus) { SCP_NS(); }
    virtual ScopeError clearErrorStatus(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel) { SCP_NS(); }
    virtual ScopeError queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_strMessage) { SCP_NS(); }
    virtual ScopeError getInstrumentErrorCount(U32BIT in_u32ScopeNumber, U32BIT& out_u32Count) { SCP_NS(); }
    virtual ScopeError readStandardEventStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) { SCP_NS(); }
    virtual ScopeError readStatusByte(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) { SCP_NS(); }
    virtual ScopeError readOperationStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) { SCP_NS(); }
    virtual ScopeError readQuestionableStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) { SCP_NS(); }
    virtual ScopeError setRemoteState(U32BIT in_u32ScopeNumber, Enum_Scope_RemoteState in_eState) { SCP_NS(); }
    virtual ScopeError getRemoteState(U32BIT in_u32ScopeNumber, Enum_Scope_RemoteState& out_eState) { SCP_NS(); }
    virtual ScopeError setKeyLock(U32BIT in_u32ScopeNumber, bool in_bOn) { SCP_NS(); }
    virtual ScopeError isKeyLocked(U32BIT in_u32ScopeNumber, bool& out_bOn) { SCP_NS(); }
    virtual ScopeError setBeeper(U32BIT in_u32ScopeNumber, bool in_bOn) { SCP_NS(); }

    /*==== debug escape hatch (diagnostics only) ==========================*/
    virtual ScopeError writeScpi(U32BIT in_u32ScopeNumber, const QString& in_strCommand) { SCP_NS(); }
    virtual ScopeError queryScpi(U32BIT in_u32ScopeNumber, const QString& in_strCommand, QString& out_strResponse) { SCP_NS(); }

protected:
    // helper used by the default (not-overridden) bodies
    static ScopeError NotSupported()
    {
        return ScopeError(Enum_Scope_ErrorCode::NOT_SUPPORTED, QStringLiteral("operation not supported by this model"));
    }
};

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#undef SCP_NS

Q_DECLARE_INTERFACE(CIScopePlugin, ScopePlugin_iid)

#endif // ISCOPEPLUGIN_H
