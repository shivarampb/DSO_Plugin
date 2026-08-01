/**
 * @file    VisaHelper.h
 * @brief   Inline S_Scope_ConnectionConfig::toVisaResourceString() builder.
 * @details Builds a VISA resource string from the structured connection
 *          parameters (protocol + address fields), or returns an explicit
 *          resource string when one was set. Mirrors the ELoad VisaHelper.h.
 *
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023 — the switch over the protocol has a default; Allman
 *          braces with a leading sequence comment.
 */
#ifndef VISAHELPER_H
#define VISAHELPER_H

#include <QString>
#include "ScopeTypes.h"

/**
 * @brief  Build a VISA resource string from this connection configuration.
 * @return An explicit resource string if one was set; otherwise a string
 *         synthesized from the protocol-specific fields (ASRL/GPIB/USB/TCPIP).
 * @pre    None. The relevant fields for @c m_enumProtocol should be populated;
 *         an unset field yields the protocol's conventional default form.
 */
inline QString S_Scope_ConnectionConfig::toVisaResourceString() const
{
    const QString strPrimary = QString::fromLocal8Bit(m_szResourceString);
    const QString strPortName = QString::fromLocal8Bit(m_szPortName);
    const QString strIpAddress = QString::fromLocal8Bit(m_szIpAddress);
    const QString strVendorId = QString::fromLocal8Bit(m_szUsbVendorId);
    const QString strProductId = QString::fromLocal8Bit(m_szUsbProductId);
    const QString strSerial = QString::fromLocal8Bit(m_szUsbSerialNumber);

    // An explicit resource string always wins.
    if (!strPrimary.isEmpty())
    {
        return strPrimary;
    }

    QString strResource;
    switch (m_enumProtocol)
    {
    case Enum_Scope_CommunicationProtocol::RS232:
        if (strPortName.toUpper().startsWith(QStringLiteral("COM")))
        {
            strResource = QStringLiteral("ASRL%1::INSTR").arg(strPortName.mid(3));
        }
        else
        {
            strResource = QStringLiteral("ASRL%1::INSTR").arg(strPortName);
        }
        break;

    case Enum_Scope_CommunicationProtocol::GPIB:
        strResource = QStringLiteral("GPIB%1::%2::INSTR").arg(m_u32GpibBoard).arg(m_u32GpibAddress);
        break;

    case Enum_Scope_CommunicationProtocol::USB:
        if (!strVendorId.isEmpty() && !strProductId.isEmpty())
        {
            if (!strSerial.isEmpty())
            {
                strResource =
                    QStringLiteral("USB0::%1::%2::%3::INSTR").arg(strVendorId, strProductId, strSerial);
            }
            else
            {
                strResource = QStringLiteral("USB0::%1::%2::INSTR").arg(strVendorId, strProductId);
            }
        }
        else
        {
            strResource = QStringLiteral("USB0::INSTR");
        }
        break;

    case Enum_Scope_CommunicationProtocol::TCPIP:
    case Enum_Scope_CommunicationProtocol::ETHERNET:
    case Enum_Scope_CommunicationProtocol::LXI:
        if (m_u32Port != 5025 && m_u32Port != 0)
        {
            strResource = QStringLiteral("TCPIP0::%1::%2::SOCKET").arg(strIpAddress).arg(m_u32Port);
        }
        else
        {
            strResource = QStringLiteral("TCPIP0::%1::INSTR").arg(strIpAddress);
        }
        break;

    case Enum_Scope_CommunicationProtocol::VXI11:
        strResource = QStringLiteral("TCPIP0::%1::inst0::INSTR").arg(strIpAddress);
        break;

    case Enum_Scope_CommunicationProtocol::HiSLIP:
        strResource = QStringLiteral("TCPIP0::%1::hislip0::INSTR").arg(strIpAddress);
        break;

    default:
        strResource = QStringLiteral("INSTR");
        break;
    }
    return strResource;
}

#endif // VISAHELPER_H
