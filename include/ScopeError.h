/*============================================================================
 *  ScopeError.h
 *
 *  Error/result type for the Scope framework. Mirrors ELoadError.h: a returned
 *  ScopeError object carrying an Enum_Scope_ErrorCode plus a descriptive
 *  string, the device status flags and the S_Scope_DeviceErrorStatus aggregate.
 *
 *  \author  Scope framework
 *==========================================================================*/
#ifndef SCOPEERROR_H
#define SCOPEERROR_H

#include <QString>
#include <QStringList>
#include <QFlags>
#include <QDebug>

#include "ScopeTypes.h"

/*----------------------------------------------------------------------------
 * Enum_Scope_ErrorCode - result codes returned through the ScopeError object.
 * Same code bands as ELoad: framework 1000s, comms 2000s, param/usage 3000s,
 * instrument-reported 4000s, UNKNOWN 9999.
 *--------------------------------------------------------------------------*/
enum class Enum_Scope_ErrorCode
{
    SUCCESS = 0,

    // framework / plugin
    PLUGIN_NOT_FOUND        = 1000,
    PLUGIN_LOAD_FAILED      = 1001,
    PLUGIN_VERSION_MISMATCH = 1002,
    INVALID_SCOPE_NUMBER    = 1003,
    ALREADY_CONNECTED       = 1004,
    NOT_CONNECTED           = 1005,
    INVALID_CHANNEL         = 1006,

    // communication
    CONNECTION_FAILED       = 2000,
    DISCONNECTION_FAILED    = 2001,
    COMMUNICATION_TIMEOUT   = 2002,
    COMMUNICATION_ERROR     = 2003,
    INVALID_RESPONSE        = 2004,

    // parameter / usage
    PARAMETER_OUT_OF_RANGE  = 3000,
    INVALID_PARAMETER       = 3001,
    NOT_SUPPORTED           = 3002,

    // instrument-reported
    INSTRUMENT_ERROR        = 4000,
    TRIGGER_TIMEOUT         = 4001,
    ACQUISITION_ERROR       = 4002,
    OVERLOAD                = 4003,
    CAL_ERROR               = 4004,

    UNKNOWN_ERROR           = 9999
};

/*----------------------------------------------------------------------------
 * Decoded device status flags (from IEEE-488.2 status + trigger/acq state).
 *--------------------------------------------------------------------------*/
enum class Enum_Scope_DeviceStatusFlag
{
    NoError          = 0x0000,
    Triggered        = 0x0001,
    Running          = 0x0002,
    Stopped          = 0x0004,
    WaitTrigger      = 0x0008,
    Overload         = 0x0010,
    ClippedPositive  = 0x0020,
    ClippedNegative  = 0x0040,
    CalRequired      = 0x0080,
    ErrorQueue       = 0x0100,
    ConnectionError  = 0x0200,
    AutoTrigger      = 0x0400
};
Q_DECLARE_FLAGS(Scope_DeviceStatus, Enum_Scope_DeviceStatusFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(Scope_DeviceStatus)

/*----------------------------------------------------------------------------
 * Aggregate error/status snapshot filled by readErrorStatus().
 *--------------------------------------------------------------------------*/
struct SCOPECORE_EXPORT S_Scope_DeviceErrorStatus
{
    Scope_DeviceStatus       m_statusFlags;
    Enum_Scope_TriggerState  m_EnumTriggerState;
    Enum_Scope_AcqState      m_EnumAcqState;
    QString            m_StrErrorMessage;
    S32BIT             m_iStandardEventStatus;   // *ESR?
    S32BIT             m_iQuestionableStatus;    // STAT:QUES?
    S32BIT             m_iOperationStatus;       // STAT:OPER?

    S_Scope_DeviceErrorStatus()
        : m_statusFlags(Enum_Scope_DeviceStatusFlag::NoError)
        , m_EnumTriggerState(Enum_Scope_TriggerState::m_enumUnknown)
        , m_EnumAcqState(Enum_Scope_AcqState::m_enumStopped)
        , m_iStandardEventStatus(0)
        , m_iQuestionableStatus(0)
        , m_iOperationStatus(0)
    {}

    bool hasError() const
    {
        return static_cast<int>(m_statusFlags) != 0;
    }

    QString toString() const;
};

/*----------------------------------------------------------------------------
 * ScopeError - the value returned by every framework/plugin operation.
 *--------------------------------------------------------------------------*/
class SCOPECORE_EXPORT ScopeError
{
public:
    ScopeError();
    ScopeError(Enum_Scope_ErrorCode in_eCode, const QString& in_strDescription = QString());

    Enum_Scope_ErrorCode code() const { return m_eCode; }
    QString description() const { return m_strDescription; }
    QString toString() const;
    bool isSuccess() const { return m_eCode == Enum_Scope_ErrorCode::SUCCESS; }

    static QString errorCodeToString(Enum_Scope_ErrorCode in_eCode);
    static QString deviceStatusToString(Scope_DeviceStatus in_status);
    static QString triggerStateToString(Enum_Scope_TriggerState in_eState);

private:
    Enum_Scope_ErrorCode m_eCode;
    QString     m_strDescription;
};

#endif // SCOPEERROR_H
