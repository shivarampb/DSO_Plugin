/**
 * @file    ScopeError.h
 * @brief   Error/result type and device-status aggregate for the Scope framework.
 * @details Declares Enum_Scope_ErrorCode (the result codes returned by every
 *          framework/plugin operation), the decoded device-status flag set, the
 *          S_Scope_DeviceErrorStatus snapshot filled by readErrorStatus(), and
 *          the ScopeError value class. Mirrors the ELoad framework's ELoadError.
 *
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023 — the exported types (ScopeError,
 *          S_Scope_DeviceErrorStatus) carry SCOPECORE_EXPORT so a plugin that
 *          links the core resolves them on every platform. Scoped enums are used
 *          throughout (no unscoped enumerators leak into the global namespace).
 */
#ifndef SCOPEERROR_H
#define SCOPEERROR_H

#include <QString>
#include <QStringList>
#include <QFlags>
#include <QDebug>

#include "ScopeTypes.h"

/**
 * @brief Result codes returned through the ScopeError object.
 * @details Banded like the ELoad framework: framework/plugin 1000s,
 *          communication 2000s, parameter/usage 3000s, instrument-reported
 *          4000s, and a catch-all UNKNOWN_ERROR.
 */
enum class Enum_Scope_ErrorCode
{
    SUCCESS = 0,

    /* framework / plugin */
    PLUGIN_NOT_FOUND        = 1000,
    PLUGIN_LOAD_FAILED      = 1001,
    PLUGIN_VERSION_MISMATCH = 1002,
    INVALID_SCOPE_NUMBER    = 1003,
    ALREADY_CONNECTED       = 1004,
    NOT_CONNECTED           = 1005,
    INVALID_CHANNEL         = 1006,

    /* communication */
    CONNECTION_FAILED       = 2000,
    DISCONNECTION_FAILED    = 2001,
    COMMUNICATION_TIMEOUT   = 2002,
    COMMUNICATION_ERROR     = 2003,
    INVALID_RESPONSE        = 2004,

    /* parameter / usage */
    PARAMETER_OUT_OF_RANGE  = 3000,
    INVALID_PARAMETER       = 3001,
    NOT_SUPPORTED           = 3002,

    /* instrument-reported */
    INSTRUMENT_ERROR        = 4000,
    TRIGGER_TIMEOUT         = 4001,
    ACQUISITION_ERROR       = 4002,
    OVERLOAD                = 4003,
    CAL_ERROR               = 4004,

    UNKNOWN_ERROR           = 9999
};

/**
 * @brief Decoded device-status flags (IEEE-488.2 status + trigger/acq state).
 */
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

/**
 * @brief Aggregate error/status snapshot filled by readErrorStatus().
 * @details Holds the decoded status flags, the current trigger/acquisition
 *          state and the raw IEEE-488.2 register values (*ESR?, STAT:QUES?,
 *          STAT:OPER?). Exported so plugins can construct it across the ABI.
 */
struct SCOPECORE_EXPORT S_Scope_DeviceErrorStatus
{
    Scope_DeviceStatus       m_statusFlags;
    Enum_Scope_TriggerState  m_EnumTriggerState;
    Enum_Scope_AcqState      m_EnumAcqState;
    QString                  m_StrErrorMessage;
    S32BIT                   m_iStandardEventStatus;   /**< *ESR?      */
    S32BIT                   m_iQuestionableStatus;    /**< STAT:QUES? */
    S32BIT                   m_iOperationStatus;       /**< STAT:OPER? */

    /**
     * @brief Construct a cleared status snapshot (no error, stopped, unknown trigger).
     * @pre   None.
     */
    S_Scope_DeviceErrorStatus()
        : m_statusFlags(Enum_Scope_DeviceStatusFlag::NoError)
        , m_EnumTriggerState(Enum_Scope_TriggerState::m_enumUnknown)
        , m_EnumAcqState(Enum_Scope_AcqState::m_enumStopped)
        , m_iStandardEventStatus(0)
        , m_iQuestionableStatus(0)
        , m_iOperationStatus(0)
    {
        // Members are fully initialised in the list; nothing to do in the body.
    }

    /**
     * @brief  Test whether any status flag is set.
     * @return true if at least one flag beyond NoError is asserted.
     * @pre    None.
     */
    bool hasError() const
    {
        // A non-zero flag word means at least one condition is asserted.
        return static_cast<int>(m_statusFlags) != 0;
    }

    /**
     * @brief  Render the snapshot (trigger state, flags, message) as text.
     * @return A single-line human-readable summary.
     * @pre    None.
     */
    QString toString() const;
};

/**
 * @brief  The value returned by every framework/plugin operation.
 * @details Carries an Enum_Scope_ErrorCode plus a descriptive string. Exported
 *          so a model plugin that links the core can construct and return it.
 */
class SCOPECORE_EXPORT ScopeError
{
public:
    /**
     * @brief Construct a success result (SUCCESS, "Success").
     * @pre   None.
     */
    ScopeError();

    /**
     * @brief Construct a result from a code and (optional) description.
     * @param[in] in_eCode         The result code.
     * @param[in] in_strDescription Human-readable detail; if empty, a default
     *                              string for the code is used.
     * @pre   None.
     */
    ScopeError(Enum_Scope_ErrorCode in_eCode, const QString& in_strDescription = QString());

    /**
     * @brief  Get the result code.
     * @return The Enum_Scope_ErrorCode this result carries.
     * @pre    None.
     */
    Enum_Scope_ErrorCode code() const { return m_eCode; }

    /**
     * @brief  Get the descriptive text.
     * @return The description string.
     * @pre    None.
     */
    QString description() const { return m_strDescription; }

    /**
     * @brief  Render the result as "[code] description".
     * @return The formatted string.
     * @pre    None.
     */
    QString toString() const;

    /**
     * @brief  Test whether the result is SUCCESS.
     * @return true if the code is SUCCESS.
     * @pre    None.
     */
    bool isSuccess() const { return m_eCode == Enum_Scope_ErrorCode::SUCCESS; }

    /**
     * @brief  Map a result code to its default description.
     * @param[in] in_eCode  The code to describe.
     * @return A static English description for the code.
     * @pre    None.
     */
    static QString errorCodeToString(Enum_Scope_ErrorCode in_eCode);

    /**
     * @brief  Join the asserted device-status flags into text.
     * @param[in] in_status  The decoded status flag set.
     * @return A comma-separated list of asserted flags, or "No error".
     * @pre    None.
     */
    static QString deviceStatusToString(Scope_DeviceStatus in_status);

    /**
     * @brief  Map a trigger state enum to its short label.
     * @param[in] in_eState  The trigger state.
     * @return A short label (ARMED/READY/TRIGGERED/AUTO/STOPPED/UNKNOWN).
     * @pre    None.
     */
    static QString triggerStateToString(Enum_Scope_TriggerState in_eState);

private:
    Enum_Scope_ErrorCode m_eCode;          /**< the result code        */
    QString              m_strDescription; /**< human-readable detail  */
};

#endif // SCOPEERROR_H
