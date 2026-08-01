/**
 * @file    ScopeTypes.h
 * @brief   Shared value types (enums, POD structs) for the Scope framework.
 * @details Mirrors the ELoad framework's ELoadTypes.h, adapted for
 *          oscilloscopes. Included by the core, every model plugin and the
 *          application. Defines the primitive house typedefs, the
 *          SCOPECORE_EXPORT decoration, the domain enumerations and the value
 *          structs (connection config, plugin info, capabilities, parameter
 *          range, waveform preamble/data, measurement result).
 *
 * @author  Scope framework
 * @date    2026
 * @note    Naming — every structure is S_Scope_<Name> and every enumeration
 *          Enum_Scope_<Name> so the framework never collides with the sibling
 *          ELoad / PowerSupply frameworks in the same application.
 * @note    MISRA C++:2023 — fixed-width house typedefs only (no `long`); scoped
 *          enums; POD structs fully initialise their members in the constructor.
 */
#ifndef SCOPETYPES_H
#define SCOPETYPES_H

#include <QtGlobal>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QList>
#include <cstring>

/*----------------------------------------------------------------------------
 * Import/export decoration for the public core types (ScopeCore library).
 * Defined here (the lowest-level header) so ScopeError, S_Scope_DeviceErrorStatus
 * and CScopeManager all share one definition: a model plugin that links the
 * core imports these symbols on every platform (MSVC and MinGW alike), so
 * constructing a ScopeError in a plugin never becomes an unresolved symbol.
 *--------------------------------------------------------------------------*/
#if defined(SCOPECORE_LIBRARY)
#define SCOPECORE_EXPORT Q_DECL_EXPORT
#else
#define SCOPECORE_EXPORT Q_DECL_IMPORT
#endif

/*----------------------------------------------------------------------------
 * Primitive typedefs (house style, matching the ELoad / PowerSupply frameworks).
 * Fixed-width only - never `long` - for a stable C-ABI across MinGW/MSVC/GCC.
 *--------------------------------------------------------------------------*/
typedef unsigned int U32BIT;
typedef int S32BIT;
typedef unsigned short U16BIT;
typedef short S16BIT;
typedef unsigned char U8BIT;
typedef char S8BIT;
typedef double FDOUBLE;
typedef float FFLOAT;

/*----------------------------------------------------------------------------
 * Fixed field sizes for the POD plugin-info / connection structs.
 *--------------------------------------------------------------------------*/
#define PLUGIN_INFO_NAME_SIZE 32
#define PLUGIN_INFO_VERSION_SIZE 16
#define PLUGIN_INFO_MANUFACTURER_SIZE 32
#define PLUGIN_INFO_MODEL_NAME_SIZE 32
#define PLUGIN_INFO_SERIES_SIZE 32
#define PLUGIN_INFO_DESCRIPTION_SIZE 128
#define PLUGIN_INFO_MIN_CORE_VERSION 16
#define PLUGIN_INFO_MAX_CORE_VERSION 16

#define CONN_RES_STR_SIZE 128  // main VISA resource string
#define CONN_PORT_NAME_SIZE 16 // COM port or /dev/tty name
#define CONN_IP_ADDR_SIZE 16   // IPv4 address
#define CONN_USB_ID_SIZE 8     // Vendor / Product ID
#define CONN_USB_SN_SIZE 64    // USB serial number

/*----------------------------------------------------------------------------
 * Communication protocol for a connection.
 *--------------------------------------------------------------------------*/
enum class Enum_Scope_CommunicationProtocol
{
    RS232,
    USB,
    GPIB,
    ETHERNET,
    LXI,
    TCPIP,
    VXI11,
    HiSLIP
};

/*----------------------------------------------------------------------------
 * Domain enums (oscilloscope semantics).  Enumerator values keep short names.
 *--------------------------------------------------------------------------*/
enum class Enum_Scope_Coupling
{
    m_enumDC,
    m_enumAC,
    m_enumGND
};
enum class Enum_Scope_BandwidthLimit
{
    m_enumFull,
    m_enum20MHz,
    m_enum200MHz,
    m_enum250MHz
};
enum class Enum_Scope_InputImpedance
{
    m_enum1M,
    m_enum50
};

enum class Enum_Scope_TriggerMode
{
    m_enumAuto,
    m_enumNormal,
    m_enumSingle
};
enum class Enum_Scope_TriggerType
{
    m_enumEdge,
    m_enumPulse,
    m_enumWidth,
    m_enumVideo,
    m_enumPattern,
    m_enumRunt,
    m_enumSlope,
    m_enumSetupHold,
    m_enumSerial
};
enum class Enum_Scope_TriggerSlope
{
    m_enumRising,
    m_enumFalling,
    m_enumEither
};
enum class Enum_Scope_TriggerSource
{
    m_enumCh1,
    m_enumCh2,
    m_enumCh3,
    m_enumCh4,
    m_enumExt,
    m_enumLine,
    m_enumDigital
};
enum class Enum_Scope_TriggerState
{
    m_enumArmed,
    m_enumReady,
    m_enumTriggered,
    m_enumAuto,
    m_enumStopped,
    m_enumUnknown
};

enum class Enum_Scope_AcqMode
{
    m_enumSample,
    m_enumPeakDetect,
    m_enumAverage,
    m_enumHiRes,
    m_enumEnvelope
};
enum class Enum_Scope_AcqState
{
    m_enumRunning,
    m_enumStopped,
    m_enumComplete
};

enum class Enum_Scope_WaveformFormat
{
    m_enumByte,
    m_enumWord,
    m_enumAscii
};
enum class Enum_Scope_WaveformSource
{
    m_enumChannel,
    m_enumMath,
    m_enumRef,
    m_enumDigital
};

enum class Enum_Scope_MeasType
{
    m_enumVpp,
    m_enumVmax,
    m_enumVmin,
    m_enumVrms,
    m_enumVavg,
    m_enumFrequency,
    m_enumPeriod,
    m_enumRiseTime,
    m_enumFallTime,
    m_enumPosWidth,
    m_enumNegWidth,
    m_enumDutyCycle,
    m_enumOvershoot,
    m_enumPhase,
    m_enumDelay,
    m_enumUnknown
};

enum class Enum_Scope_MathOp
{
    m_enumAdd,
    m_enumSub,
    m_enumMult,
    m_enumDiv,
    m_enumFFT
};
enum class Enum_Scope_FftWindow
{
    m_enumRect,
    m_enumHann,
    m_enumHamming,
    m_enumBlackman,
    m_enumFlattop
};
enum class Enum_Scope_CursorType
{
    m_enumOff,
    m_enumHorizontal,
    m_enumVertical,
    m_enumTrack
};
enum class Enum_Scope_TimebaseMode
{
    m_enumMain,
    m_enumZoom,
    m_enumRoll,
    m_enumXY
};
enum class Enum_Scope_RemoteState
{
    m_enumLocal,
    m_enumRemote,
    m_enumRWLock
};
enum class Enum_Scope_ImageFormat
{
    m_enumPng,
    m_enumBmp
};

/*----------------------------------------------------------------------------
 * S_Scope_ConnectionConfig - how to reach one instrument (protocol + parameters).
 * POD-ish; provides toVisaResourceString() (see VisaHelper.h).
 *--------------------------------------------------------------------------*/
struct S_Scope_ConnectionConfig
{
    Enum_Scope_CommunicationProtocol m_enumProtocol;

    S8BIT m_szResourceString[CONN_RES_STR_SIZE]; // explicit VISA resource

    S8BIT m_szPortName[CONN_PORT_NAME_SIZE];
    U32BIT m_u32BaudRate;

    S8BIT m_szIpAddress[CONN_IP_ADDR_SIZE];
    U32BIT m_u32Port;

    U32BIT m_u32GpibAddress;
    U32BIT m_u32GpibBoard;

    S8BIT m_szUsbVendorId[CONN_USB_ID_SIZE];
    S8BIT m_szUsbProductId[CONN_USB_ID_SIZE];
    S8BIT m_szUsbSerialNumber[CONN_USB_SN_SIZE];

    U32BIT m_u32Timeout; // communication timeout [ms]

    S_Scope_ConnectionConfig()
        : m_enumProtocol(Enum_Scope_CommunicationProtocol::USB), m_u32BaudRate(115200), m_u32Port(5025),
          m_u32GpibAddress(0), m_u32GpibBoard(0), m_u32Timeout(5000)
    {
        memset(m_szResourceString, 0, sizeof(m_szResourceString));
        memset(m_szPortName, 0, sizeof(m_szPortName));
        memset(m_szIpAddress, 0, sizeof(m_szIpAddress));
        memset(m_szUsbVendorId, 0, sizeof(m_szUsbVendorId));
        memset(m_szUsbProductId, 0, sizeof(m_szUsbProductId));
        memset(m_szUsbSerialNumber, 0, sizeof(m_szUsbSerialNumber));
    }

    // Convenience: build from an explicit VISA resource string.
    void setResourceString(const QString& in_strResource)
    {
        const QByteArray aby = in_strResource.toLatin1();
        qstrncpy(m_szResourceString, aby.constData(), CONN_RES_STR_SIZE);
    }

    QString toVisaResourceString() const; // implemented in VisaHelper.h
};

/*----------------------------------------------------------------------------
 * S_Scope_PluginInfo - static plugin metadata reported by getPluginInfo().
 *--------------------------------------------------------------------------*/
struct S_Scope_PluginInfo
{
    S8BIT m_szName[PLUGIN_INFO_NAME_SIZE];
    S8BIT m_szVersion[PLUGIN_INFO_VERSION_SIZE];
    S8BIT m_szManufacturer[PLUGIN_INFO_MANUFACTURER_SIZE];
    S8BIT m_szModelName[PLUGIN_INFO_MODEL_NAME_SIZE];
    S8BIT m_szSeries[PLUGIN_INFO_SERIES_SIZE];
    S8BIT m_szDescription[PLUGIN_INFO_DESCRIPTION_SIZE];
    S8BIT m_szMinCoreVersion[PLUGIN_INFO_MIN_CORE_VERSION];
    S8BIT m_szMaxCoreVersion[PLUGIN_INFO_MAX_CORE_VERSION];
    QStringList m_StrlstSupportedProtocols;
    QStringList m_StrlstSupportedModes;

    S_Scope_PluginInfo()
    {
        memset(m_szName, 0, sizeof(m_szName));
        memset(m_szVersion, 0, sizeof(m_szVersion));
        memset(m_szManufacturer, 0, sizeof(m_szManufacturer));
        memset(m_szModelName, 0, sizeof(m_szModelName));
        memset(m_szSeries, 0, sizeof(m_szSeries));
        memset(m_szDescription, 0, sizeof(m_szDescription));
        memset(m_szMinCoreVersion, 0, sizeof(m_szMinCoreVersion));
        memset(m_szMaxCoreVersion, 0, sizeof(m_szMaxCoreVersion));
    }

    bool isCompatible(const QString& in_strCoreVersion) const
    {
        const QString strMin = QString::fromLatin1(m_szMinCoreVersion);
        const QString strMax = QString::fromLatin1(m_szMaxCoreVersion);
        return (in_strCoreVersion >= strMin && in_strCoreVersion <= strMax);
    }
};

/*----------------------------------------------------------------------------
 * Per-channel capabilities.
 *--------------------------------------------------------------------------*/
struct S_Scope_ChannelCapabilities
{
    U32BIT m_u32ChannelNumber;
    FDOUBLE m_dBandwidthHz;
    FDOUBLE m_dMaxVerticalScale; // V/div max
    FDOUBLE m_dMinVerticalScale; // V/div min
    bool m_bHas50Ohm;
    bool m_bHasBandwidthLimit;

    S_Scope_ChannelCapabilities()
        : m_u32ChannelNumber(1), m_dBandwidthHz(0.0), m_dMaxVerticalScale(0.0), m_dMinVerticalScale(0.0),
          m_bHas50Ohm(false), m_bHasBandwidthLimit(false)
    {
    }
};

/*----------------------------------------------------------------------------
 * Whole-instrument capabilities reported by getCapabilities().
 *--------------------------------------------------------------------------*/
struct S_Scope_Capabilities
{
    U32BIT m_u32NumberOfChannels; // analog channels
    U32BIT m_u32NumberOfDigital;  // digital / MSO channels (0 = none)
    FDOUBLE m_dBandwidthHz;
    FDOUBLE m_dMaxSampleRate;    // Sa/s
    U32BIT m_u32MaxMemoryDepth;  // points
    FDOUBLE m_dMinTimebaseScale; // s/div min
    FDOUBLE m_dMaxTimebaseScale; // s/div max
    bool m_bHasDigital;
    bool m_bHasAWG;
    bool m_bHasFFT;
    bool m_bHasSerialDecode;
    bool m_bHasSegmented;
    QList<S_Scope_ChannelCapabilities> m_QlistChannels;

    S_Scope_Capabilities()
        : m_u32NumberOfChannels(1), m_u32NumberOfDigital(0), m_dBandwidthHz(0.0), m_dMaxSampleRate(0.0),
          m_u32MaxMemoryDepth(0), m_dMinTimebaseScale(0.0), m_dMaxTimebaseScale(0.0), m_bHasDigital(false),
          m_bHasAWG(false), m_bHasFFT(false), m_bHasSerialDecode(false), m_bHasSegmented(false)
    {
    }

    S_Scope_ChannelCapabilities getChannelCapabilities(U32BIT in_u32Channel) const
    {
        for (const S_Scope_ChannelCapabilities& ch : m_QlistChannels)
        {
            if (ch.m_u32ChannelNumber == in_u32Channel)
            {
                return ch;
            }
        }
        S_Scope_ChannelCapabilities sDef;
        sDef.m_u32ChannelNumber = in_u32Channel;
        sDef.m_dBandwidthHz = m_dBandwidthHz;
        return sDef;
    }
};

/*----------------------------------------------------------------------------
 * Numeric range descriptor for a parameter (auto-ranging UIs).
 *--------------------------------------------------------------------------*/
struct S_Scope_ParameterRange
{
    FDOUBLE m_dMin;
    FDOUBLE m_dMax;
    FDOUBLE m_dResolution;
    QList<FDOUBLE> m_QlistDiscreteValues; // non-empty for discrete params

    S_Scope_ParameterRange() : m_dMin(0.0), m_dMax(0.0), m_dResolution(0.0)
    {
    }
};

/*----------------------------------------------------------------------------
 * Parameter identifiers for getParameterRange().
 *--------------------------------------------------------------------------*/
enum class Enum_Scope_ParamId
{
    m_enumVerticalScale,
    m_enumVerticalOffset,
    m_enumVerticalPosition,
    m_enumTimebaseScale,
    m_enumTimebasePosition,
    m_enumTriggerLevel,
    m_enumTriggerHoldoff,
    m_enumProbeAttenuation,
    m_enumSampleRate,
    m_enumMemoryDepth,
    m_enumAverageCount,
    m_enumWaveformPoints,
    m_enumAwgFrequency,
    m_enumAwgAmplitude,
    m_enumMeasureThreshold
};

/*----------------------------------------------------------------------------
 * Waveform preamble parsed from :WAV:PRE? (IVI / IEEE style).
 *--------------------------------------------------------------------------*/
struct S_Scope_WaveformPreamble
{
    S32BIT m_iFormat; // 0 = BYTE, 1 = WORD, 4 = ASCII
    S32BIT m_iType;   // 0 = NORMAL, 1 = PEAK, 2 = AVERAGE
    U32BIT m_u32Points;
    U32BIT m_u32Count;
    FDOUBLE m_dXIncrement; // s per point
    FDOUBLE m_dXOrigin;    // s of first point
    FDOUBLE m_dXReference; // reference point index
    FDOUBLE m_dYIncrement; // V per code
    FDOUBLE m_dYOrigin;    // V at reference
    FDOUBLE m_dYReference; // code of reference

    S_Scope_WaveformPreamble()
        : m_iFormat(0), m_iType(0), m_u32Points(0), m_u32Count(1), m_dXIncrement(0.0), m_dXOrigin(0.0),
          m_dXReference(0.0), m_dYIncrement(0.0), m_dYOrigin(0.0), m_dYReference(0.0)
    {
    }
};

/*----------------------------------------------------------------------------
 * A captured, scaled waveform: real volts vs. real time.
 *--------------------------------------------------------------------------*/
struct S_Scope_Waveform
{
    S_Scope_WaveformPreamble m_sPreamble;
    QVector<FDOUBLE> m_vecTimeSeconds;
    QVector<FDOUBLE> m_vecVolts;
    U32BIT m_u32SourceChannel;

    S_Scope_Waveform() : m_u32SourceChannel(1)
    {
    }

    U32BIT pointCount() const
    {
        return static_cast<U32BIT>(m_vecVolts.size());
    }
};

/*----------------------------------------------------------------------------
 * One automatic-measurement result (+ optional statistics).
 *--------------------------------------------------------------------------*/
struct S_Scope_MeasurementResult
{
    Enum_Scope_MeasType m_eType;
    FDOUBLE m_dValue;
    bool m_bValid;
    QString m_strUnits;
    FDOUBLE m_dMin;
    FDOUBLE m_dMax;
    FDOUBLE m_dMean;
    FDOUBLE m_dStdDev;

    S_Scope_MeasurementResult()
        : m_eType(Enum_Scope_MeasType::m_enumUnknown), m_dValue(0.0), m_bValid(false), m_dMin(0.0),
          m_dMax(0.0), m_dMean(0.0), m_dStdDev(0.0)
    {
    }
};

#endif // SCOPETYPES_H
