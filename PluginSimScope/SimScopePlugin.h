/*============================================================================
 *  SimScopePlugin.h
 *
 *  SimScope - a virtual oscilloscope model plugin (no VISA, no hardware). A
 *  complete CIScopePlugin implementation that SYNTHESIZES waveforms
 *  (sine/square/ramp/noise, selectable via the resource string e.g.
 *  "SIM::MDO34"), honours the vertical / timebase / trigger / acquisition
 *  settings when generating the trace, and computes automatic measurements
 *  from the synthesized buffer.
 *
 *  It reuses the SAME S_ScopeLimits rows as the real plugins (catalog lookup
 *  by name), giving getParameterRange parity with the real plugins by
 *  construction. Mirrors the ELoad framework's SimLoad.
 *
 *  \author  Scope framework
 *==========================================================================*/
#ifndef SIMSCOPEPLUGIN_H
#define SIMSCOPEPLUGIN_H

#include <QObject>
#include <QMap>
#include <QVector>

#include "IScopePlugin.h"
#include "S_ScopeLimits.h"

class CSimScopePlugin : public QObject, public CIScopePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)
public:
    CSimScopePlugin();
    ~CSimScopePlugin() override;

    /*==== mandatory ======================================================*/
    S_Scope_PluginInfo   getPluginInfo() const override;
    S_Scope_Capabilities getCapabilities() const override;
    ScopeError connect(U32BIT s, const S_Scope_ConnectionConfig& c) override;
    ScopeError disconnect(U32BIT s) override;
    bool       isConnected(U32BIT s) const override;
    ScopeError reset(U32BIT s) override;

    /*==== core ===========================================================*/
    ScopeError setTimeout(U32BIT s, U32BIT t) override;
    ScopeError getIdentification(U32BIT s, QString& o) override;
    ScopeError selfTest(U32BIT s, S32BIT& o) override;
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
    ScopeError setCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling v) override;
    ScopeError getCoupling(U32BIT s, U32BIT c, Enum_Scope_Coupling& o) override;
    ScopeError setProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE v) override;
    ScopeError getProbeAttenuation(U32BIT s, U32BIT c, FDOUBLE& o) override;
    ScopeError setBandwidthLimit(U32BIT s, U32BIT c, Enum_Scope_BandwidthLimit v) override;
    ScopeError getBandwidthLimit(U32BIT s, U32BIT c, Enum_Scope_BandwidthLimit& o) override;

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
    ScopeError setTriggerType(U32BIT s, Enum_Scope_TriggerType v) override;
    ScopeError getTriggerType(U32BIT s, Enum_Scope_TriggerType& o) override;
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
    ScopeError getWaveformSource(U32BIT s, Enum_Scope_WaveformSource& o, U32BIT& i) override;
    ScopeError setWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat v) override;
    ScopeError getWaveformFormat(U32BIT s, Enum_Scope_WaveformFormat& o) override;
    ScopeError setWaveformPoints(U32BIT s, U32BIT v) override;
    ScopeError getWaveformPreamble(U32BIT s, U32BIT c, S_Scope_WaveformPreamble& o) override;
    ScopeError readWaveform(U32BIT s, U32BIT c, S_Scope_Waveform& o) override;
    ScopeError digitizeChannel(U32BIT s, U32BIT c) override;

    /*==== vertical extras ================================================*/
    ScopeError setVerticalPosition(U32BIT s, U32BIT c, FDOUBLE v) override;
    ScopeError getVerticalPosition(U32BIT s, U32BIT c, FDOUBLE& o) override;
    ScopeError setInputImpedance(U32BIT s, U32BIT c, Enum_Scope_InputImpedance v) override;
    ScopeError getInputImpedance(U32BIT s, U32BIT c, Enum_Scope_InputImpedance& o) override;
    ScopeError setInvert(U32BIT s, U32BIT c, bool v) override;
    ScopeError getInvert(U32BIT s, U32BIT c, bool& o) override;
    ScopeError setChannelLabel(U32BIT s, U32BIT c, const QString& v) override;
    ScopeError getChannelLabel(U32BIT s, U32BIT c, QString& o) override;
    ScopeError setChannelUnits(U32BIT s, U32BIT c, const QString& v) override;
    ScopeError getChannelUnits(U32BIT s, U32BIT c, QString& o) override;
    ScopeError setDeskew(U32BIT s, U32BIT c, FDOUBLE v) override;
    ScopeError getDeskew(U32BIT s, U32BIT c, FDOUBLE& o) override;

    /*==== horizontal extras ==============================================*/
    ScopeError setTimebaseReference(U32BIT s, FDOUBLE v) override;
    ScopeError setTimebaseMode(U32BIT s, Enum_Scope_TimebaseMode v) override;
    ScopeError getTimebaseMode(U32BIT s, Enum_Scope_TimebaseMode& o) override;
    ScopeError setAcquisitionPoints(U32BIT s, U32BIT v) override;
    ScopeError getAcquisitionPoints(U32BIT s, U32BIT& o) override;

    /*==== trigger extras =================================================*/
    ScopeError setTriggerCoupling(U32BIT s, Enum_Scope_Coupling v) override;
    ScopeError getTriggerCoupling(U32BIT s, Enum_Scope_Coupling& o) override;
    ScopeError setTriggerPulseWidth(U32BIT s, FDOUBLE v) override;
    ScopeError setTriggerVideoStandard(U32BIT s, const QString& v) override;
    ScopeError setTriggerPattern(U32BIT s, const QString& v) override;

    /*==== acquisition extras =============================================*/
    ScopeError setSegmentedCount(U32BIT s, U32BIT v) override;
    ScopeError getSegmentedCount(U32BIT s, U32BIT& o) override;

    /*==== measurements ===================================================*/
    ScopeError addMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t) override;
    ScopeError readMeasurement(U32BIT s, U32BIT c, Enum_Scope_MeasType t, S_Scope_MeasurementResult& o) override;
    ScopeError clearMeasurements(U32BIT s) override;
    ScopeError setMeasureStatistics(U32BIT s, bool v) override;
    ScopeError getMeasurementStatistics(U32BIT s, U32BIT c, Enum_Scope_MeasType t, S_Scope_MeasurementResult& o) override;

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

    /*==== cursors ========================================================*/
    ScopeError setCursorType(U32BIT s, Enum_Scope_CursorType v) override;
    ScopeError getCursorType(U32BIT s, Enum_Scope_CursorType& o) override;
    ScopeError setCursorSource(U32BIT s, U32BIT c) override;
    ScopeError setCursorPosition(U32BIT s, U32BIT i, FDOUBLE v) override;
    ScopeError getCursorPosition(U32BIT s, U32BIT i, FDOUBLE& o) override;
    ScopeError readCursorValues(U32BIT s, FDOUBLE& x1, FDOUBLE& x2, FDOUBLE& y1, FDOUBLE& y2) override;

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

    /*==== status / system ================================================*/
    ScopeError clearStatus(U32BIT s) override;
    ScopeError getOptions(U32BIT s, QString& o) override;
    ScopeError waitOperationComplete(U32BIT s) override;
    ScopeError readErrorStatus(U32BIT s, U32BIT c, S_Scope_DeviceErrorStatus& o) override;
    ScopeError clearErrorStatus(U32BIT s, U32BIT c) override;
    ScopeError readStatusByte(U32BIT s, U32BIT& o) override;
    ScopeError readStandardEventStatus(U32BIT s, U32BIT& o) override;
    ScopeError readOperationStatus(U32BIT s, U32BIT& o) override;
    ScopeError readQuestionableStatus(U32BIT s, U32BIT& o) override;
    ScopeError getInstrumentErrorCount(U32BIT s, U32BIT& o) override;
    ScopeError setRemoteState(U32BIT s, Enum_Scope_RemoteState v) override;
    ScopeError getRemoteState(U32BIT s, Enum_Scope_RemoteState& o) override;
    ScopeError setKeyLock(U32BIT s, bool v) override;
    ScopeError isKeyLocked(U32BIT s, bool& o) override;
    ScopeError setBeeper(U32BIT s, bool v) override;
    ScopeError queryErrorQueue(U32BIT s, QString& o) override;
    ScopeError writeScpi(U32BIT s, const QString& v) override;
    ScopeError queryScpi(U32BIT s, const QString& v, QString& o) override;

private:
    enum EWaveShape { SHAPE_SINE, SHAPE_SQUARE, SHAPE_RAMP, SHAPE_NOISE };

    struct S_SimDevice {
        bool                 m_bConnected;
        const S_ScopeLimits* m_pLimits;
        QString              m_strModel;
        EWaveShape           m_eShape;
        U32BIT               m_u32Timeout;

        QVector<bool>                 m_vChEnabled;
        QVector<double>               m_vVertScale;   // V/div
        QVector<double>               m_vVertOffset;  // V
        QVector<Enum_Scope_Coupling>  m_vCoupling;
        QVector<double>               m_vProbeAtten;
        QVector<Enum_Scope_BandwidthLimit> m_vBwLimit;

        double m_dTimebase;      // s/div
        double m_dTimebasePos;   // s
        unsigned m_u32MemDepth;  // points

        Enum_Scope_TriggerMode   m_eTrigMode;
        Enum_Scope_TriggerType   m_eTrigType;
        Enum_Scope_TriggerSource m_eTrigSource;
        Enum_Scope_TriggerSlope  m_eTrigSlope;
        double m_dTrigLevel;
        double m_dTrigHoldoff;

        Enum_Scope_AcqMode  m_eAcqMode;
        int    m_iAvgCount;
        Enum_Scope_AcqState m_eAcqState;

        Enum_Scope_WaveformFormat m_eWfmFormat;
        int    m_iWfmPoints;
        Enum_Scope_WaveformSource m_eWfmSource;
        int    m_iWfmSourceIndex;

        // per-channel extras (sized to the model's analog channel count)
        QVector<double>                    m_vVertPosition;
        QVector<Enum_Scope_InputImpedance> m_vImpedance;
        QVector<bool>                      m_vInvert;
        QVector<QString>                   m_vLabel;
        QVector<QString>                   m_vUnits;
        QVector<double>                    m_vDeskew;

        Enum_Scope_TimebaseMode  m_eTimebaseMode;
        U32BIT                   m_u32SegmentCount;
        Enum_Scope_Coupling      m_eTrigCoupling;

        Enum_Scope_MathOp    m_eMathOp;
        Enum_Scope_FftWindow m_eFftWindow;
        Enum_Scope_CursorType m_eCursorType;
        double m_dCursorX1, m_dCursorX2, m_dCursorY1, m_dCursorY2;

        QString m_strAwgFunction;
        double  m_dAwgFreq, m_dAwgAmpl, m_dAwgOffset;
        bool    m_bAwgOn;

        Enum_Scope_RemoteState m_eRemote;
        bool m_bKeyLocked;

        S_SimDevice() : m_bConnected(false), m_pLimits(nullptr)
            , m_eShape(SHAPE_SINE), m_u32Timeout(5000)
            , m_dTimebase(1.0e-6), m_dTimebasePos(0.0), m_u32MemDepth(1000)
            , m_eTrigMode(Enum_Scope_TriggerMode::m_enumAuto)
            , m_eTrigType(Enum_Scope_TriggerType::m_enumEdge)
            , m_eTrigSource(Enum_Scope_TriggerSource::m_enumCh1)
            , m_eTrigSlope(Enum_Scope_TriggerSlope::m_enumRising)
            , m_dTrigLevel(0.0), m_dTrigHoldoff(0.0)
            , m_eAcqMode(Enum_Scope_AcqMode::m_enumSample)
            , m_iAvgCount(16), m_eAcqState(Enum_Scope_AcqState::m_enumStopped)
            , m_eWfmFormat(Enum_Scope_WaveformFormat::m_enumWord)
            , m_iWfmPoints(1000)
            , m_eWfmSource(Enum_Scope_WaveformSource::m_enumChannel)
            , m_iWfmSourceIndex(1)
            , m_eTimebaseMode(Enum_Scope_TimebaseMode::m_enumMain)
            , m_u32SegmentCount(1)
            , m_eTrigCoupling(Enum_Scope_Coupling::m_enumDC)
            , m_eMathOp(Enum_Scope_MathOp::m_enumAdd)
            , m_eFftWindow(Enum_Scope_FftWindow::m_enumHann)
            , m_eCursorType(Enum_Scope_CursorType::m_enumOff)
            , m_dCursorX1(0.0), m_dCursorX2(0.0), m_dCursorY1(0.0), m_dCursorY2(0.0)
            , m_dAwgFreq(1000.0), m_dAwgAmpl(1.0), m_dAwgOffset(0.0), m_bAwgOn(false)
            , m_eRemote(Enum_Scope_RemoteState::m_enumRemote), m_bKeyLocked(false)
        {}
    };

    S_SimDevice* dev(U32BIT s);
    const S_SimDevice* dev(U32BIT s) const;
    bool validChannel(const S_SimDevice* d, U32BIT c) const;

    void   synthesize(const S_SimDevice* d, U32BIT c,
                      QVector<FDOUBLE>& out_time, QVector<FDOUBLE>& out_volts) const;
    double computeMeasurement(const S_SimDevice* d, U32BIT c, Enum_Scope_MeasType t, bool& out_bValid) const;

    QMap<U32BIT, S_SimDevice> m_devices;
};

#endif // SIMSCOPEPLUGIN_H
