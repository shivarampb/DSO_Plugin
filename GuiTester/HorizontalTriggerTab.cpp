/*============================================================================
 *  HorizontalTriggerTab.cpp
 *==========================================================================*/
#include "HorizontalTriggerTab.h"
#include "TesterCommon.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

HorizontalTriggerTab::HorizontalTriggerTab(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root = new QVBoxLayout(this);

    QGroupBox* hbox = new QGroupBox(tr("Horizontal"), this);
    QFormLayout* hf = new QFormLayout(hbox);
    m_pTimebase = new QDoubleSpinBox(hbox);
    m_pTimebase->setDecimals(12); m_pTimebase->setRange(1e-10, 1000.0); m_pTimebase->setValue(1e-6);
    m_pTimebase->setSuffix(tr(" s/div"));
    hf->addRow(tr("Timebase:"), m_pTimebase);
    root->addWidget(hbox);

    QGroupBox* tbox = new QGroupBox(tr("Edge Trigger"), this);
    QFormLayout* tf = new QFormLayout(tbox);
    m_pTrigSource = new QComboBox(tbox); m_pTrigSource->addItems(QStringList() << "CH1" << "CH2" << "CH3" << "CH4" << "EXT");
    m_pTrigSlope = new QComboBox(tbox);  m_pTrigSlope->addItems(QStringList() << "Rising" << "Falling" << "Either");
    m_pTrigMode = new QComboBox(tbox);   m_pTrigMode->addItems(QStringList() << "Auto" << "Normal");
    m_pTrigLevel = new QDoubleSpinBox(tbox); m_pTrigLevel->setDecimals(4); m_pTrigLevel->setRange(-100.0, 100.0); m_pTrigLevel->setSuffix(tr(" V"));
    tf->addRow(tr("Source:"), m_pTrigSource);
    tf->addRow(tr("Slope:"), m_pTrigSlope);
    tf->addRow(tr("Mode:"), m_pTrigMode);
    tf->addRow(tr("Level:"), m_pTrigLevel);
    root->addWidget(tbox);

    QHBoxLayout* btns = new QHBoxLayout;
    m_pApplyBtn = new QPushButton(tr("Apply"), this);
    m_pForceBtn = new QPushButton(tr("Force Trigger"), this);
    btns->addWidget(m_pApplyBtn);
    btns->addWidget(m_pForceBtn);
    btns->addStretch(1);
    root->addLayout(btns);

    m_pStateLabel = new QLabel(tr("Trigger state: —"), this);
    root->addWidget(m_pStateLabel);
    root->addStretch(1);

    connect(m_pApplyBtn, SIGNAL(clicked()), this, SLOT(onApply()));
    connect(m_pForceBtn, SIGNAL(clicked()), this, SLOT(onForce()));
}

void HorizontalTriggerTab::onApply()
{
    CScopeManager& mgr = CScopeManager::instance();
    ScopeError e = mgr.setTimebaseScale(TESTER_SCOPE, m_pTimebase->value());
    if (e.isSuccess()) e = mgr.setTriggerSource(TESTER_SCOPE, static_cast<Enum_Scope_TriggerSource>(m_pTrigSource->currentIndex()));
    if (e.isSuccess()) e = mgr.setTriggerSlope(TESTER_SCOPE, static_cast<Enum_Scope_TriggerSlope>(m_pTrigSlope->currentIndex()));
    if (e.isSuccess()) {
        const Enum_Scope_TriggerMode m = (m_pTrigMode->currentIndex() == 1)
            ? Enum_Scope_TriggerMode::m_enumNormal : Enum_Scope_TriggerMode::m_enumAuto;
        e = mgr.setTriggerMode(TESTER_SCOPE, m);
    }
    if (e.isSuccess()) e = mgr.setTriggerLevel(TESTER_SCOPE, TESTER_CHANNEL, m_pTrigLevel->value());
    Enum_Scope_TriggerState st = Enum_Scope_TriggerState::m_enumUnknown;
    mgr.getTriggerState(TESTER_SCOPE, st);
    m_pStateLabel->setText(tr("Trigger state: %1").arg(ScopeError::triggerStateToString(st)));
    emit log(e.isSuccess() ? tr("Horizontal/Trigger applied") : e.toString());
}

void HorizontalTriggerTab::onForce()
{
    ScopeError e = CScopeManager::instance().forceTrigger(TESTER_SCOPE);
    emit log(e.isSuccess() ? tr("Trigger forced") : e.toString());
}

void HorizontalTriggerTab::setConnected(bool in_bConnected)
{
    setEnabled(in_bConnected);
    if (in_bConnected) {
        S_Scope_ParameterRange r;
        if (CScopeManager::instance().getParameterRange(
                TESTER_SCOPE, TESTER_CHANNEL, Enum_Scope_ParamId::m_enumTimebaseScale, r).isSuccess())
            m_pTimebase->setRange(r.m_dMin, r.m_dMax);
    }
}
