/**
 * @file    VerticalTab.cpp
 * @brief   Vertical tab implementation.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "VerticalTab.h"
#include "TesterCommon.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

/**
 * @brief  Build the vertical tab: per-channel scale, offset, coupling and
 *         probe-attenuation controls with an Apply button.
 * @param[in] parent  Parent widget, or nullptr for a top-level widget.
 * @pre    None.
 */
VerticalTab::VerticalTab(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    QGroupBox* box = new QGroupBox(tr("Vertical (per channel)"), this);
    QFormLayout* f = new QFormLayout(box);

    m_pChannel = new QSpinBox(box);
    m_pChannel->setRange(1, 4);
    m_pScale = new QDoubleSpinBox(box);
    m_pScale->setDecimals(4);
    m_pScale->setRange(0.001, 10.0);
    m_pScale->setValue(0.5);
    m_pScale->setSuffix(tr(" V/div"));
    m_pOffset = new QDoubleSpinBox(box);
    m_pOffset->setDecimals(4);
    m_pOffset->setRange(-100.0, 100.0);
    m_pOffset->setSuffix(tr(" V"));
    m_pCoupling = new QComboBox(box);
    m_pCoupling->addItems(QStringList() << "DC" << "AC" << "GND");
    m_pProbe = new QComboBox(box);
    // common probe-attenuation ratios (the plugin validates the exact set)
    static const double kProbe[] = {0.1, 1.0, 10.0, 20.0, 100.0, 1000.0};
    for (double a : kProbe)
    {
        m_pProbe->addItem(QStringLiteral("%1x").arg(a), a);
    }
    m_pProbe->setCurrentText(QStringLiteral("10x"));

    f->addRow(tr("Channel:"), m_pChannel);
    f->addRow(tr("Scale:"), m_pScale);
    f->addRow(tr("Offset:"), m_pOffset);
    f->addRow(tr("Coupling:"), m_pCoupling);
    f->addRow(tr("Probe:"), m_pProbe);
    root->addWidget(box);

    m_pApplyBtn = new QPushButton(tr("Apply"), this);
    root->addWidget(m_pApplyBtn);
    root->addStretch(1);

    connect(m_pChannel, SIGNAL(valueChanged(int)), this, SLOT(onChannelChanged(int)));
    connect(m_pApplyBtn, SIGNAL(clicked()), this, SLOT(onApply()));
}

/**
 * @brief  Populate the scale/offset controls (and scale range) for the current
 *         channel by reading the connected instrument.
 * @pre    A scope is connected on TESTER_SCOPE.
 */
void VerticalTab::loadFromInstrument()
{
    CScopeManager& mgr = CScopeManager::instance();
    const U32BIT ch = static_cast<U32BIT>(m_pChannel->value());
    S_Scope_ParameterRange r;
    if (mgr.getParameterRange(TESTER_SCOPE, ch, Enum_Scope_ParamId::m_enumVerticalScale, r).isSuccess())
    {
        m_pScale->setRange(r.m_dMin, r.m_dMax);
    }
    FDOUBLE v = 0.0;
    if (mgr.getVerticalScale(TESTER_SCOPE, ch, v).isSuccess())
    {
        m_pScale->setValue(v);
    }
    if (mgr.getVerticalOffset(TESTER_SCOPE, ch, v).isSuccess())
    {
        m_pOffset->setValue(v);
    }
}

/**
 * @brief  Slot: reload the controls when the channel selection changes.
 * @pre    None (reloads only while the tab is enabled/connected).
 */
void VerticalTab::onChannelChanged(int)
{
    if (isEnabled())
    {
        loadFromInstrument();
    }
}

/**
 * @brief  Slot: apply scale, offset, coupling and probe attenuation to the
 *         selected channel, logging the first error if any.
 * @pre    A scope is connected on TESTER_SCOPE.
 */
void VerticalTab::onApply()
{
    CScopeManager& mgr = CScopeManager::instance();
    const U32BIT ch = static_cast<U32BIT>(m_pChannel->value());
    mgr.enableChannel(TESTER_SCOPE, ch, true);
    ScopeError e = mgr.setVerticalScale(TESTER_SCOPE, ch, m_pScale->value());
    if (e.isSuccess())
    {
        e = mgr.setVerticalOffset(TESTER_SCOPE, ch, m_pOffset->value());
    }
    const Enum_Scope_Coupling cp = (m_pCoupling->currentIndex() == 1)   ? Enum_Scope_Coupling::m_enumAC
                                   : (m_pCoupling->currentIndex() == 2) ? Enum_Scope_Coupling::m_enumGND
                                                                        : Enum_Scope_Coupling::m_enumDC;
    if (e.isSuccess())
    {
        e = mgr.setCoupling(TESTER_SCOPE, ch, cp);
    }
    if (e.isSuccess())
    {
        e = mgr.setProbeAttenuation(TESTER_SCOPE, ch, m_pProbe->currentData().toDouble());
    }
    emit log(e.isSuccess() ? tr("Vertical applied to CH%1").arg(ch) : e.toString());
}

/**
 * @brief  Enable/disable the tab; on connect, clamp the channel range to the
 *         instrument's channel count and load current values.
 * @param[in] in_bConnected  true when a scope is connected.
 * @pre    When true, a scope is connected on TESTER_SCOPE.
 */
void VerticalTab::setConnected(bool in_bConnected)
{
    setEnabled(in_bConnected);
    if (in_bConnected)
    {
        S_Scope_Capabilities caps = CScopeManager::instance().getCapabilities(TESTER_SCOPE);
        if (caps.m_u32NumberOfChannels >= 1)
        {
            m_pChannel->setRange(1, static_cast<int>(caps.m_u32NumberOfChannels));
        }
        loadFromInstrument();
    }
}
