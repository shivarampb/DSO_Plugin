/**
 * @file    WaveformTab.cpp
 * @brief   Waveform tab implementation.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "WaveformTab.h"
#include "WaveformPlot.h"
#include "TesterCommon.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

/**
 * @brief  Build the waveform tab: channel selector, Fetch and Stream controls,
 *         and the embedded plot with its polling timer.
 * @param[in] parent  Parent widget, or nullptr for a top-level widget.
 * @pre    None.
 */
WaveformTab::WaveformTab(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root = new QVBoxLayout(this);

    QHBoxLayout* ctl = new QHBoxLayout;
    m_pChannel = new QSpinBox(this);
    m_pChannel->setRange(1, 4);
    m_pFetchBtn = new QPushButton(tr("Fetch"), this);
    m_pStreamBtn = new QPushButton(tr("Start stream"), this);
    m_pStreamBtn->setCheckable(true);
    ctl->addWidget(new QLabel(tr("Source CH:"), this));
    ctl->addWidget(m_pChannel);
    ctl->addWidget(m_pFetchBtn);
    ctl->addWidget(m_pStreamBtn);
    ctl->addStretch(1);
    root->addLayout(ctl);

    m_pPlot = new WaveformPlot(this);
    root->addWidget(m_pPlot, 1);

    m_pTimer = new QTimer(this);
    m_pTimer->setInterval(200);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTick()));
    connect(m_pFetchBtn, SIGNAL(clicked()), this, SLOT(onFetch()));
    connect(m_pStreamBtn, SIGNAL(toggled(bool)), this, SLOT(onStreamToggled(bool)));
}

/**
 * @brief  Capture one waveform on the selected channel and display it.
 * @return true if a waveform with more than one point was read and plotted.
 * @pre    A scope is connected on TESTER_SCOPE.
 */
bool WaveformTab::pollOnce()
{
    CScopeManager& mgr = CScopeManager::instance();
    const U32BIT ch = static_cast<U32BIT>(m_pChannel->value());
    mgr.enableChannel(TESTER_SCOPE, ch, true);
    mgr.single(TESTER_SCOPE);
    S_Scope_Waveform wfm;
    const bool ok = mgr.readWaveform(TESTER_SCOPE, ch, wfm).isSuccess() && wfm.pointCount() > 1;
    if (ok)
    {
        m_pPlot->setWaveform(wfm);
    }
    return ok;
}

/**
 * @brief  Slot: fetch a single waveform, logging on failure.
 * @pre    A scope is connected on TESTER_SCOPE.
 */
void WaveformTab::onFetch()
{
    if (!pollOnce())
    {
        emit log(tr("Waveform fetch failed"));
    }
}

/**
 * @brief  Slot: timer tick during streaming; captures one waveform.
 * @pre    A scope is connected on TESTER_SCOPE.
 */
void WaveformTab::onTick()
{
    pollOnce();
}

/**
 * @brief  Slot: start or stop the streaming timer and update the button label.
 * @param[in] checked  true to start streaming, false to stop.
 * @pre    A scope is connected on TESTER_SCOPE when starting.
 */
void WaveformTab::onStreamToggled(bool checked)
{
    m_pStreamBtn->setText(checked ? tr("Stop stream") : tr("Start stream"));
    if (checked)
    {
        m_pTimer->start();
    }
    else
    {
        m_pTimer->stop();
    }
}

/**
 * @brief  Enable/disable the tab; on connect clamp the channel range, on
 *         disconnect stop streaming and clear the plot.
 * @param[in] in_bConnected  true when a scope is connected.
 * @pre    When true, a scope is connected on TESTER_SCOPE.
 */
void WaveformTab::setConnected(bool in_bConnected)
{
    setEnabled(in_bConnected);
    if (in_bConnected)
    {
        S_Scope_Capabilities caps = CScopeManager::instance().getCapabilities(TESTER_SCOPE);
        if (caps.m_u32NumberOfChannels >= 1)
        {
            m_pChannel->setRange(1, static_cast<int>(caps.m_u32NumberOfChannels));
        }
    }
    else
    {
        m_pTimer->stop();
        m_pStreamBtn->blockSignals(true);
        m_pStreamBtn->setChecked(false);
        m_pStreamBtn->setText(tr("Start stream"));
        m_pStreamBtn->blockSignals(false);
        m_pPlot->clearWaveform();
    }
}
