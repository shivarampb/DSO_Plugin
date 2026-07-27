/*============================================================================
 *  TDS1012BPlugin.h
 *
 *  Tektronix TDS1012B (3 Series MDO) oscilloscope plugin - a complete,
 *  self-contained CIScopePlugin implementation (SCPI over VISA, direct
 *  linkage). One VISA session per scope is held in a map keyed by scope
 *  number; the model's limits are embedded in this plugin (m_limits).
 *
 *  Dialect: Tektronix Programmer Manual (CH<n>:..., HORizontal:..., TRIGger:A:...,
 *  ACQuire:..., DATa:.../WFMOutpre?/CURVe?). Commands verified against the mock
 *  emulator; hardware ranges tagged TODO(manual) until the MDO3 Series
 *  Programmer Manual is placed under manuals/TDS1012B/.
 *
 *  \author  Scope framework
 *==========================================================================*/
#ifndef TDS1012BPLUGIN_H
#define TDS1012BPLUGIN_H

#include <QObject>
#include <QMap>

#include "visa.h"
#include "IScopePlugin.h"
#include "S_ScopeLimits.h"

class CTDS1012BPlugin : public QObject, public CIScopePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)
public:
    CTDS1012BPlugin();
    ~CTDS1012BPlugin() override;

    /*==== mandatory ======================================================*/
    S_Scope_PluginInfo   getPluginInfo() const override;
    S_Scope_Capabilities getCapabilities() const override;
    ScopeError connect(U32BIT s, const S_Scope_ConnectionConfig& c) override;
    ScopeError disconnect(U32BIT s) override;
    bool       isConnected(U32BIT s) const override;
    ScopeError reset(U32BIT s) override;

    /*==== core ===========================================================*/
    ScopeError setTimeout(U32BIT s, U32BIT t) override;
    ScopeError clearStatus(U32BIT s) override;
    ScopeError getIdentification(U32BIT s, QString& o) override;
    ScopeError selfTest(U32BIT s, S32BIT& o) override;
    ScopeError getOptions(U32BIT s, QString& o) override;
    ScopeError waitOperationComplete(U32BIT s) override;
    ScopeError getScpiVersion(U32BIT s, QString& o) override;
    ScopeError getParameterRange(U32BIT s, U32BIT c, Enum_Scope_ParamId p, S_Scope_ParameterRange& o) override;
    ScopeError autoscale(U32BIT s) override;
    ScopeError run(U32BIT s) override;
    ScopeError stop(U32BIT s) override;
    ScopeError single(U32BIT s) override;
    ScopeError forceTrigger(U32BIT s) override;

    /*==== vertical =======================================================*/
    ScopeError enableChannel(U32BIT s, U32BIT c, bool v) override;
    ScopeError isChannelEnabled(U32BIT s, U32BIT c, bool& o) override;
    ScopeError setVerticalScale(U32BIT s, U32BIT c, FDOUBLE v) override;
    ScopeError getVerticalScale(U32BIT s, U32BIT c, FDOUBLE& o) override;
    ScopeError setVerticalOffset(U32BIT s, U32BIT c, FDOUBLE v) override;
    ScopeError getVerticalOffset(U32BIT s, U32BIT c, FDOUBLE& o) override;
    ScopeError setVerticalPosition(U32BIT s, U32BIT c, FDOUBLE v) override;
    ScopeError getVerticalPosition(U32BIT s, U32BIT c, FDOUBLE& o) override;
    ScopeError setCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling v) override;
    ScopeError getCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling& o) override;
    ScopeError setProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE v) override;
    ScopeError getProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE& o) override;

    /*==== horizontal =====================================================*/
    ScopeError setTimebaseScale(U32BIT s, FDOUBLE v) override;
    ScopeError getTimebaseScale(U32BIT s, FDOUBLE& o) override;
    ScopeError setTimebasePosition(U32BIT s, FDOUBLE v) override;
    ScopeError getTimebasePosition(U32BIT s, FDOUBLE& o) override;
    ScopeError getSampleRate(U32BIT s, FDOUBLE& o) override;
    ScopeError setMemoryDepth(U32BIT s, U32BIT v) override;
    ScopeError getMemoryDepth(U32BIT s, U32BIT& o) override;

    /*==== trigger ========================================================*/
    ScopeError setTriggerMode(U32BIT s, Enum_Scope_TriggerMode v) override;
    ScopeError getTriggerMode(U32BIT s, Enum_Scope_TriggerMode& o) override;
    ScopeError setTriggerSource(U32BIT s, Enum_Scope_TriggerSource v) override;
    ScopeError getTriggerSource(U32BIT s, Enum_Scope_TriggerSource& o) override;
    ScopeError setTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope v) override;
    ScopeError getTriggerSlope(U32BIT s, Enum_Scope_TriggerSlope& o) override;
    ScopeError setTriggerLevel(U32BIT s, U32BIT c, FDOUBLE v) override;
    ScopeError getTriggerLevel(U32BIT s, U32BIT c, FDOUBLE& o) override;
    ScopeError setTriggerHoldoff(U32BIT s, FDOUBLE v) override;
    ScopeError getTriggerHoldoff(U32BIT s, FDOUBLE& o) override;
    ScopeError getTriggerState(U32BIT s, Enum_Scope_TriggerState& o) override;

    /*==== acquisition ====================================================*/
    ScopeError setAcqMode(U32BIT s, Enum_Scope_AcqMode v) override;
    ScopeError getAcqMode(U32BIT s, Enum_Scope_AcqMode& o) override;
    ScopeError setAverageCount(U32BIT s, U32BIT v) override;
    ScopeError getAverageCount(U32BIT s, U32BIT& o) override;
    ScopeError getAcquisitionState(U32BIT s, Enum_Scope_AcqState& o) override;

    /*==== waveform =======================================================*/
    ScopeError setWaveformSource(U32BIT s, Enum_Scope_WaveformSource v, U32BIT i) override;
    ScopeError setWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat v) override;
    ScopeError setWaveformPoints(U32BIT s, U32BIT v) override;
    ScopeError getWaveformPreamble(U32BIT s, U32BIT c, S_Scope_WaveformPreamble& o) override;
    ScopeError readWaveform(U32BIT s, U32BIT c, S_Scope_Waveform& o) override;
    ScopeError digitizeChannel(U32BIT s, U32BIT c) override;

    /*==== measurements ===================================================*/
    ScopeError addMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t) override;
    ScopeError readMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t, S_Scope_MeasurementResult& o) override;
    ScopeError clearMeasurements(U32BIT s) override;
    ScopeError setMeasureStatistics(U32BIT s, bool v) override;
    ScopeError getMeasurementStatistics(U32BIT s, U32BIT c, Enum_Scope_MeasType t, S_Scope_MeasurementResult& o) override;

    /*==== cursors ========================================================*/
    ScopeError setCursorType(U32BIT s, Enum_Scope_CursorType v) override;
    ScopeError getCursorType(U32BIT s, Enum_Scope_CursorType& o) override;
    ScopeError setCursorSource(U32BIT s, U32BIT c) override;
    ScopeError setCursorPosition(U32BIT s, U32BIT i, FDOUBLE v) override;
    ScopeError getCursorPosition(U32BIT s, U32BIT i, FDOUBLE& o) override;
    ScopeError readCursorValues(U32BIT s, FDOUBLE& x1, FDOUBLE& x2, FDOUBLE& y1, FDOUBLE& y2) override;

    /*==== math / FFT =====================================================*/
    ScopeError setMathOperation(U32BIT s, Enum_Scope_MathOp v) override;
    ScopeError setMathSource1(U32BIT s, U32BIT c) override;
    ScopeError setMathSource2(U32BIT s, U32BIT c) override;
    ScopeError enableMath(U32BIT s, bool v) override;
    ScopeError setFftWindow(U32BIT s, Enum_Scope_FftWindow v) override;
    ScopeError getFftWindow(U32BIT s, Enum_Scope_FftWindow& o) override;
    ScopeError setFftSpan(U32BIT s, FDOUBLE v) override;
    ScopeError setFftCenter(U32BIT s, FDOUBLE v) override;
    ScopeError setMathScale(U32BIT s, FDOUBLE v) override;
    ScopeError setMathPosition(U32BIT s, FDOUBLE v) override;

    /*==== display ========================================================*/
    ScopeError setPersistence(U32BIT s, FDOUBLE v) override;
    ScopeError setGraticule(U32BIT s, const QString& v) override;
    ScopeError setIntensity(U32BIT s, FDOUBLE v) override;
    ScopeError setDisplayFormat(U32BIT s, Enum_Scope_TimebaseMode v) override;
    ScopeError setVectors(U32BIT s, bool v) override;

    /*==== save / recall / screenshot =====================================*/
    ScopeError saveSetup(U32BIT s, U32BIT loc) override;
    ScopeError recallSetup(U32BIT s, U32BIT loc) override;
    ScopeError saveWaveformToFile(U32BIT s, U32BIT c, const QString& path) override;
    ScopeError captureScreenshot(U32BIT s, Enum_Scope_ImageFormat f, QByteArray& o) override;
    ScopeError saveToReference(U32BIT s, U32BIT c, U32BIT slot) override;
    ScopeError displayReference(U32BIT s, U32BIT slot, bool v) override;

    /*==== digital / MSO (gated on capability) ============================*/
    ScopeError enableDigitalChannel(U32BIT s, U32BIT d, bool v) override;
    ScopeError setDigitalThreshold(U32BIT s, U32BIT d, FDOUBLE v) override;
    ScopeError setPodThreshold(U32BIT s, U32BIT p, FDOUBLE v) override;
    ScopeError enableBus(U32BIT s, U32BIT b, bool v) override;
    ScopeError setBusType(U32BIT s, U32BIT b, const QString& v) override;
    ScopeError readBusDecode(U32BIT s, U32BIT b, QString& o) override;

    /*==== AWG (gated on capability) ======================================*/
    ScopeError setAwgFunction(U32BIT s, const QString& v) override;
    ScopeError setAwgFrequency(U32BIT s, FDOUBLE v) override;
    ScopeError setAwgAmplitude(U32BIT s, FDOUBLE v) override;
    ScopeError setAwgOffset(U32BIT s, FDOUBLE v) override;
    ScopeError enableAwgOutput(U32BIT s, bool v) override;

    /*==== status / system extras =========================================*/
    ScopeError readOperationStatus(U32BIT s, U32BIT& o) override;
    ScopeError readQuestionableStatus(U32BIT s, U32BIT& o) override;
    ScopeError getInstrumentErrorCount(U32BIT s, U32BIT& o) override;
    ScopeError setRemoteState(U32BIT s, Enum_Scope_RemoteState v) override;
    ScopeError getRemoteState(U32BIT s, Enum_Scope_RemoteState& o) override;
    ScopeError setKeyLock(U32BIT s, bool v) override;
    ScopeError isKeyLocked(U32BIT s, bool& o) override;
    ScopeError setBeeper(U32BIT s, bool v) override;

    /*==== status =========================================================*/
    ScopeError readErrorStatus(U32BIT s, U32BIT c, S_Scope_DeviceErrorStatus& o) override;
    ScopeError clearErrorStatus(U32BIT s, U32BIT c) override;
    ScopeError queryErrorQueue(U32BIT s, QString& o) override;
    ScopeError readStatusByte(U32BIT s, U32BIT& o) override;
    ScopeError readStandardEventStatus(U32BIT s, U32BIT& o) override;
    ScopeError writeScpi(U32BIT s, const QString& v) override;
    ScopeError queryScpi(U32BIT s, const QString& v, QString& o) override;

private:
    struct S_DeviceInstance {
        ViSession m_vi;
        ViSession m_rm;
        S_Scope_ConnectionConfig m_sConfig;
        bool      m_bConnected;
        bool      m_bRunning;
        U32BIT    m_u32Timeout;
        QString   m_strIdn;
        S_DeviceInstance() : m_vi(VI_NULL), m_rm(VI_NULL), m_bConnected(false)
            , m_bRunning(false), m_u32Timeout(5000) {}
    };

    S_DeviceInstance* dev(U32BIT s);
    bool validChannel(U32BIT c) const;

    // transport (direct VISA)
    ScopeError visaError(ViStatus st, const QString& ctx);
    ScopeError writeLine(U32BIT s, const QByteArray& cmd);
    ScopeError readLine(U32BIT s, QByteArray& resp);
    ScopeError queryLine(U32BIT s, const QByteArray& cmd, QByteArray& resp);
    ScopeError readBinaryBlock(U32BIT s, QByteArray& payload);
    ScopeError sendChecked(U32BIT s, const QByteArray& cmd);
    ScopeError queryDouble(U32BIT s, const QByteArray& cmd, FDOUBLE& o);
    ScopeError setDouble(U32BIT s, const char* scpi, FDOUBLE v, FDOUBLE lo, FDOUBLE hi, const char* what);
    static QByteArray fmtD(FDOUBLE v);
    static const char* srcTok(Enum_Scope_TriggerSource e);

    S_ScopeLimits        m_limits;
    const S_ScopeLimits* m_pLimits;
    QString              m_strModel;
    QMap<U32BIT, S_DeviceInstance> m_devices;
};

#endif // TDS1012BPLUGIN_H
