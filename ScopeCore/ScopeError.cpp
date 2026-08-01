/**
 * @file    ScopeError.cpp
 * @brief   Implementation of the ScopeError result type and status rendering.
 * @details Provides the constructors, the code/status/trigger-state text
 *          mappings and S_Scope_DeviceErrorStatus::toString().
 *
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023 — every switch has a default; string mappings are pure
 *          (no side effects); Allman braces with a leading sequence comment.
 */
#include "ScopeError.h"

/**
 * @brief Construct a success result.
 * @pre   None.
 * @post  isSuccess() == true.
 */
ScopeError::ScopeError() : m_eCode(Enum_Scope_ErrorCode::SUCCESS), m_strDescription(QStringLiteral("Success"))
{
    // Members initialised in the list; the default result is SUCCESS.
}

/**
 * @brief Construct a result from a code and optional description.
 * @pre   None.
 * @note  Parameters are documented on the declaration in ScopeError.h.
 */
ScopeError::ScopeError(Enum_Scope_ErrorCode in_eCode, const QString& in_strDescription)
    : m_eCode(in_eCode), m_strDescription(in_strDescription)
{
    if (m_strDescription.isEmpty())
    {
        // No caller detail supplied: fall back to the code's default string.
        m_strDescription = errorCodeToString(in_eCode);
    }
}

/**
 * @brief  Render the result as "[code] description".
 * @return The formatted string.
 * @pre    None.
 */
QString ScopeError::toString() const
{
    // Compose the numeric code and the description into one line.
    return QStringLiteral("[%1] %2").arg(static_cast<int>(m_eCode)).arg(m_strDescription);
}

/**
 * @brief  Map a result code to its default English description.
 * @return A static description; "Undefined error" for an unknown code.
 * @pre    None.
 * @note   Parameters are documented on the declaration in ScopeError.h.
 */
QString ScopeError::errorCodeToString(Enum_Scope_ErrorCode in_eCode)
{
    switch (in_eCode)
    {
    /* One case per code; the default guards against an out-of-range value. */
    case Enum_Scope_ErrorCode::SUCCESS:
        return QStringLiteral("Success");
    case Enum_Scope_ErrorCode::PLUGIN_NOT_FOUND:
        return QStringLiteral("Plugin not found");
    case Enum_Scope_ErrorCode::PLUGIN_LOAD_FAILED:
        return QStringLiteral("Plugin load failed");
    case Enum_Scope_ErrorCode::PLUGIN_VERSION_MISMATCH:
        return QStringLiteral("Plugin version mismatch");
    case Enum_Scope_ErrorCode::INVALID_SCOPE_NUMBER:
        return QStringLiteral("Invalid scope number");
    case Enum_Scope_ErrorCode::ALREADY_CONNECTED:
        return QStringLiteral("Already connected");
    case Enum_Scope_ErrorCode::NOT_CONNECTED:
        return QStringLiteral("Not connected");
    case Enum_Scope_ErrorCode::INVALID_CHANNEL:
        return QStringLiteral("Invalid channel");
    case Enum_Scope_ErrorCode::CONNECTION_FAILED:
        return QStringLiteral("Connection failed");
    case Enum_Scope_ErrorCode::DISCONNECTION_FAILED:
        return QStringLiteral("Disconnection failed");
    case Enum_Scope_ErrorCode::COMMUNICATION_TIMEOUT:
        return QStringLiteral("Communication timeout");
    case Enum_Scope_ErrorCode::COMMUNICATION_ERROR:
        return QStringLiteral("Communication error");
    case Enum_Scope_ErrorCode::INVALID_RESPONSE:
        return QStringLiteral("Invalid response");
    case Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE:
        return QStringLiteral("Parameter out of range");
    case Enum_Scope_ErrorCode::INVALID_PARAMETER:
        return QStringLiteral("Invalid parameter");
    case Enum_Scope_ErrorCode::NOT_SUPPORTED:
        return QStringLiteral("Operation not supported");
    case Enum_Scope_ErrorCode::INSTRUMENT_ERROR:
        return QStringLiteral("Instrument error");
    case Enum_Scope_ErrorCode::TRIGGER_TIMEOUT:
        return QStringLiteral("Trigger timeout");
    case Enum_Scope_ErrorCode::ACQUISITION_ERROR:
        return QStringLiteral("Acquisition error");
    case Enum_Scope_ErrorCode::OVERLOAD:
        return QStringLiteral("Input overload");
    case Enum_Scope_ErrorCode::CAL_ERROR:
        return QStringLiteral("Calibration error");
    case Enum_Scope_ErrorCode::UNKNOWN_ERROR:
        return QStringLiteral("Unknown error");
    default:
        return QStringLiteral("Undefined error");
    }
}

/**
 * @brief  Join the asserted device-status flags into readable text.
 * @return A comma-separated list of asserted flags, or "No error".
 * @pre    None.
 * @note   Parameters are documented on the declaration in ScopeError.h.
 */
QString ScopeError::deviceStatusToString(Scope_DeviceStatus in_status)
{
    QStringList lst;

    /* Append the label for each asserted flag, most significant first. */
    if (in_status & Enum_Scope_DeviceStatusFlag::Triggered)
    {
        lst << QStringLiteral("Triggered");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::Running)
    {
        lst << QStringLiteral("Running");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::Stopped)
    {
        lst << QStringLiteral("Stopped");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::WaitTrigger)
    {
        lst << QStringLiteral("Wait trigger");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::Overload)
    {
        lst << QStringLiteral("Overload");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::ClippedPositive)
    {
        lst << QStringLiteral("Clipped +");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::ClippedNegative)
    {
        lst << QStringLiteral("Clipped -");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::CalRequired)
    {
        lst << QStringLiteral("Cal required");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::ErrorQueue)
    {
        lst << QStringLiteral("Error queue");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::ConnectionError)
    {
        lst << QStringLiteral("Connection error");
    }
    if (in_status & Enum_Scope_DeviceStatusFlag::AutoTrigger)
    {
        lst << QStringLiteral("Auto trigger");
    }

    // An empty list means nothing is asserted.
    return lst.isEmpty() ? QStringLiteral("No error") : lst.join(QStringLiteral(", "));
}

/**
 * @brief  Map a trigger state to its short label.
 * @return A short label; "UNKNOWN" for an unrecognised value.
 * @pre    None.
 * @note   Parameters are documented on the declaration in ScopeError.h.
 */
QString ScopeError::triggerStateToString(Enum_Scope_TriggerState in_eState)
{
    switch (in_eState)
    {
    /* One label per state; default covers m_enumUnknown and any new value. */
    case Enum_Scope_TriggerState::m_enumArmed:
        return QStringLiteral("ARMED");
    case Enum_Scope_TriggerState::m_enumReady:
        return QStringLiteral("READY");
    case Enum_Scope_TriggerState::m_enumTriggered:
        return QStringLiteral("TRIGGERED");
    case Enum_Scope_TriggerState::m_enumAuto:
        return QStringLiteral("AUTO");
    case Enum_Scope_TriggerState::m_enumStopped:
        return QStringLiteral("STOPPED");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

/**
 * @brief  Render the status snapshot (trigger state, flags, optional message).
 * @return A single-line "Trigger: … | Status: … | Message: …" summary.
 * @pre    None.
 */
QString S_Scope_DeviceErrorStatus::toString() const
{
    QStringList lst;

    /* Always show the trigger state and decoded flags; append a message if any. */
    lst << QStringLiteral("Trigger: %1").arg(ScopeError::triggerStateToString(m_EnumTriggerState));
    lst << QStringLiteral("Status: %1").arg(ScopeError::deviceStatusToString(m_statusFlags));
    if (!m_StrErrorMessage.isEmpty())
    {
        // Only include the message field when the instrument reported text.
        lst << QStringLiteral("Message: %1").arg(m_StrErrorMessage);
    }

    return lst.join(QStringLiteral(" | "));
}
