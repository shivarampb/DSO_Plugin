/*============================================================================
 *  ScopeError.cpp - implementation of the ScopeError result type.
 *==========================================================================*/
#include "ScopeError.h"

ScopeError::ScopeError()
    : m_eCode(Enum_Scope_ErrorCode::SUCCESS)
    , m_strDescription(QStringLiteral("Success"))
{
}

ScopeError::ScopeError(Enum_Scope_ErrorCode in_eCode, const QString& in_strDescription)
    : m_eCode(in_eCode)
    , m_strDescription(in_strDescription)
{
    if (m_strDescription.isEmpty())
    {
        m_strDescription = errorCodeToString(in_eCode);
    }
}

QString ScopeError::toString() const
{
    return QStringLiteral("[%1] %2").arg(static_cast<int>(m_eCode)).arg(m_strDescription);
}

QString ScopeError::errorCodeToString(Enum_Scope_ErrorCode in_eCode)
{
    switch (in_eCode)
    {
    case Enum_Scope_ErrorCode::SUCCESS:                 return QStringLiteral("Success");
    case Enum_Scope_ErrorCode::PLUGIN_NOT_FOUND:        return QStringLiteral("Plugin not found");
    case Enum_Scope_ErrorCode::PLUGIN_LOAD_FAILED:      return QStringLiteral("Plugin load failed");
    case Enum_Scope_ErrorCode::PLUGIN_VERSION_MISMATCH: return QStringLiteral("Plugin version mismatch");
    case Enum_Scope_ErrorCode::INVALID_SCOPE_NUMBER:    return QStringLiteral("Invalid scope number");
    case Enum_Scope_ErrorCode::ALREADY_CONNECTED:       return QStringLiteral("Already connected");
    case Enum_Scope_ErrorCode::NOT_CONNECTED:           return QStringLiteral("Not connected");
    case Enum_Scope_ErrorCode::INVALID_CHANNEL:         return QStringLiteral("Invalid channel");
    case Enum_Scope_ErrorCode::CONNECTION_FAILED:       return QStringLiteral("Connection failed");
    case Enum_Scope_ErrorCode::DISCONNECTION_FAILED:    return QStringLiteral("Disconnection failed");
    case Enum_Scope_ErrorCode::COMMUNICATION_TIMEOUT:   return QStringLiteral("Communication timeout");
    case Enum_Scope_ErrorCode::COMMUNICATION_ERROR:     return QStringLiteral("Communication error");
    case Enum_Scope_ErrorCode::INVALID_RESPONSE:        return QStringLiteral("Invalid response");
    case Enum_Scope_ErrorCode::PARAMETER_OUT_OF_RANGE:  return QStringLiteral("Parameter out of range");
    case Enum_Scope_ErrorCode::INVALID_PARAMETER:       return QStringLiteral("Invalid parameter");
    case Enum_Scope_ErrorCode::NOT_SUPPORTED:           return QStringLiteral("Operation not supported");
    case Enum_Scope_ErrorCode::INSTRUMENT_ERROR:        return QStringLiteral("Instrument error");
    case Enum_Scope_ErrorCode::TRIGGER_TIMEOUT:         return QStringLiteral("Trigger timeout");
    case Enum_Scope_ErrorCode::ACQUISITION_ERROR:       return QStringLiteral("Acquisition error");
    case Enum_Scope_ErrorCode::OVERLOAD:                return QStringLiteral("Input overload");
    case Enum_Scope_ErrorCode::CAL_ERROR:               return QStringLiteral("Calibration error");
    case Enum_Scope_ErrorCode::UNKNOWN_ERROR:           return QStringLiteral("Unknown error");
    default:                                            return QStringLiteral("Undefined error");
    }
}

QString ScopeError::deviceStatusToString(Scope_DeviceStatus in_status)
{
    QStringList lst;
    if (in_status & Enum_Scope_DeviceStatusFlag::Triggered)       lst << QStringLiteral("Triggered");
    if (in_status & Enum_Scope_DeviceStatusFlag::Running)         lst << QStringLiteral("Running");
    if (in_status & Enum_Scope_DeviceStatusFlag::Stopped)         lst << QStringLiteral("Stopped");
    if (in_status & Enum_Scope_DeviceStatusFlag::WaitTrigger)     lst << QStringLiteral("Wait trigger");
    if (in_status & Enum_Scope_DeviceStatusFlag::Overload)        lst << QStringLiteral("Overload");
    if (in_status & Enum_Scope_DeviceStatusFlag::ClippedPositive) lst << QStringLiteral("Clipped +");
    if (in_status & Enum_Scope_DeviceStatusFlag::ClippedNegative) lst << QStringLiteral("Clipped -");
    if (in_status & Enum_Scope_DeviceStatusFlag::CalRequired)     lst << QStringLiteral("Cal required");
    if (in_status & Enum_Scope_DeviceStatusFlag::ErrorQueue)      lst << QStringLiteral("Error queue");
    if (in_status & Enum_Scope_DeviceStatusFlag::ConnectionError) lst << QStringLiteral("Connection error");
    if (in_status & Enum_Scope_DeviceStatusFlag::AutoTrigger)     lst << QStringLiteral("Auto trigger");
    return lst.isEmpty() ? QStringLiteral("No error") : lst.join(QStringLiteral(", "));
}

QString ScopeError::triggerStateToString(Enum_Scope_TriggerState in_eState)
{
    switch (in_eState)
    {
    case Enum_Scope_TriggerState::m_enumArmed:     return QStringLiteral("ARMED");
    case Enum_Scope_TriggerState::m_enumReady:     return QStringLiteral("READY");
    case Enum_Scope_TriggerState::m_enumTriggered: return QStringLiteral("TRIGGERED");
    case Enum_Scope_TriggerState::m_enumAuto:      return QStringLiteral("AUTO");
    case Enum_Scope_TriggerState::m_enumStopped:   return QStringLiteral("STOPPED");
    default:                                       return QStringLiteral("UNKNOWN");
    }
}

QString S_Scope_DeviceErrorStatus::toString() const
{
    QStringList lst;
    lst << QStringLiteral("Trigger: %1").arg(ScopeError::triggerStateToString(m_EnumTriggerState));
    lst << QStringLiteral("Status: %1").arg(ScopeError::deviceStatusToString(m_statusFlags));
    if (!m_StrErrorMessage.isEmpty())
    {
        lst << QStringLiteral("Message: %1").arg(m_StrErrorMessage);
    }
    return lst.join(QStringLiteral(" | "));
}
