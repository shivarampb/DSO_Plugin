/**
 * @file    ConnectionTab.cpp
 * @brief   Connection tab implementation.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "ConnectionTab.h"
#include "TesterCommon.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

ConnectionTab::ConnectionTab(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root = new QVBoxLayout(this);

    QFormLayout* form = new QFormLayout;
    m_pModelCombo = new QComboBox(this);
    for (const QString& m : CScopeManager::instance().getAvailablePlugins())
    {
        m_pModelCombo->addItem(m);
    }
    const int iRef = m_pModelCombo->findText(QStringLiteral("MDO34"));
    if (iRef >= 0)
    {
        m_pModelCombo->setCurrentIndex(iRef);
    }

    m_pInterfaceCombo = new QComboBox(this);
    m_pInterfaceCombo->addItems(QStringList() << "LAN" << "USB" << "GPIB" << "Mock (dev)" << "SimScope");

    m_pAddressEdit = new QLineEdit(QStringLiteral("MOCK0::MDO34::INSTR"), this);

    form->addRow(tr("Model:"), m_pModelCombo);
    form->addRow(tr("Interface:"), m_pInterfaceCombo);
    form->addRow(tr("Host / Resource:"), m_pAddressEdit);
    root->addLayout(form);

    QHBoxLayout* btns = new QHBoxLayout;
    m_pConnectBtn = new QPushButton(tr("Connect"), this);
    m_pDisconnectBtn = new QPushButton(tr("Disconnect"), this);
    tintButton(m_pConnectBtn, "#1e6b33");
    tintButton(m_pDisconnectBtn, "#6b1e1e");
    m_pDisconnectBtn->setEnabled(false);
    btns->addWidget(m_pConnectBtn);
    btns->addWidget(m_pDisconnectBtn);
    root->addLayout(btns);

    QGroupBox* idBox = new QGroupBox(tr("Instrument Identity"), this);
    QVBoxLayout* idl = new QVBoxLayout(idBox);
    m_pIdentityLabel = new QLabel(QStringLiteral("—"), idBox);
    m_pIdentityLabel->setWordWrap(true);
    idl->addWidget(m_pIdentityLabel);
    root->addWidget(idBox);
    root->addStretch(1);

    connect(m_pConnectBtn, SIGNAL(clicked()), this, SLOT(onConnect()));
    connect(m_pDisconnectBtn, SIGNAL(clicked()), this, SLOT(onDisconnect()));
    connect(m_pInterfaceCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onInterfaceChanged()));
}

QString ConnectionTab::selectedModel() const
{
    return m_pModelCombo->currentText();
}

void ConnectionTab::onInterfaceChanged()
{
    const QString iface = m_pInterfaceCombo->currentText();
    if (iface == "LAN")
    {
        m_pAddressEdit->setText("192.168.1.100");
    }
    else if (iface == "USB")
    {
        m_pAddressEdit->setText("USB0::0x0699::0x0522::C010000::INSTR");
    }
    else if (iface == "GPIB")
    {
        m_pAddressEdit->setText("7");
    }
    else if (iface == "SimScope")
    {
        m_pAddressEdit->setText("SIM::" + selectedModel());
    }
    else /* Mock (dev) */
    {
        m_pAddressEdit->setText("MOCK0::" + selectedModel() + "::INSTR");
    }
}

S_Scope_ConnectionConfig ConnectionTab::buildConfig() const
{
    S_Scope_ConnectionConfig cfg;
    const QString iface = m_pInterfaceCombo->currentText();
    const QByteArray addr = m_pAddressEdit->text().trimmed().toLatin1();
    if (iface == "LAN")
    {
        cfg.m_enumProtocol = Enum_Scope_CommunicationProtocol::TCPIP;
        qstrncpy(cfg.m_szIpAddress, addr.constData(), CONN_IP_ADDR_SIZE);
    }
    else if (iface == "USB")
    {
        cfg.m_enumProtocol = Enum_Scope_CommunicationProtocol::USB;
        cfg.setResourceString(QString::fromLatin1(addr)); // explicit USB resource
    }
    else if (iface == "GPIB")
    {
        cfg.m_enumProtocol = Enum_Scope_CommunicationProtocol::GPIB;
        cfg.m_u32GpibAddress = m_pAddressEdit->text().toUInt();
    }
    else
    { // Mock (dev) or SimScope: explicit resource string
        cfg.setResourceString(QString::fromLatin1(addr));
    }
    return cfg;
}

void ConnectionTab::onConnect()
{
    CScopeManager& mgr = CScopeManager::instance();
    const QString model = selectedModel();
    if (model.isEmpty())
    {
        emit log(tr("No model selected"));
        return;
    }

    // SimScope interface binds the SimScope plugin; otherwise the chosen model.
    const QString plugin =
        (m_pInterfaceCombo->currentText() == "SimScope") ? QStringLiteral("SimScope") : model;

    if (!mgr.instanceExists(TESTER_SCOPE))
    {
        ScopeError e = mgr.createInstance(TESTER_SCOPE, plugin);
        if (!e.isSuccess())
        {
            emit log(tr("createInstance failed: %1").arg(e.toString()));
            return;
        }
    }
    ScopeError e = mgr.connect(TESTER_SCOPE, buildConfig());
    if (!e.isSuccess())
    {
        emit log(tr("Connect failed: %1").arg(e.toString()));
        mgr.destroyInstance(TESTER_SCOPE);
        return;
    }
    QString idn;
    mgr.getIdentification(TESTER_SCOPE, idn);
    m_pIdentityLabel->setText(idn.isEmpty() ? tr("(connected)") : idn);
    m_pConnectBtn->setEnabled(false);
    m_pDisconnectBtn->setEnabled(true);
    m_pModelCombo->setEnabled(false);
    m_pInterfaceCombo->setEnabled(false);
    m_pAddressEdit->setEnabled(false);
    emit log(tr("Connected %1: %2").arg(model, idn));
    emit connected(model);
}

void ConnectionTab::onDisconnect()
{
    CScopeManager& mgr = CScopeManager::instance();
    mgr.disconnect(TESTER_SCOPE);
    mgr.destroyInstance(TESTER_SCOPE);
    m_pIdentityLabel->setText(QStringLiteral("—"));
    m_pConnectBtn->setEnabled(true);
    m_pDisconnectBtn->setEnabled(false);
    m_pModelCombo->setEnabled(true);
    m_pInterfaceCombo->setEnabled(true);
    m_pAddressEdit->setEnabled(true);
    emit log(tr("Disconnected"));
    emit disconnected();
}

bool ConnectionTab::connectTo(const QString& in_strModel, const QString& in_strResource)
{
    CScopeManager& mgr = CScopeManager::instance();
    if (!mgr.createInstance(TESTER_SCOPE, in_strModel).isSuccess())
    {
        return false;
    }
    S_Scope_ConnectionConfig cfg;
    cfg.setResourceString(in_strResource);
    if (!mgr.connect(TESTER_SCOPE, cfg).isSuccess())
    {
        mgr.destroyInstance(TESTER_SCOPE);
        return false;
    }
    emit connected(in_strModel);
    return true;
}
