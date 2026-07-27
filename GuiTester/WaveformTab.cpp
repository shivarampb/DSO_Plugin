/*============================================================================
 *  WaveformTab.cpp
 *==========================================================================*/
#include "WaveformTab.h"
#include "WaveformPlot.h"
#include "TesterCommon.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

WaveformTab::WaveformTab(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root = new QVBoxLayout(this);

    QHBoxLayout* ctl = new QHBoxLayout;
    m_pChannel = new QSpinBox(this); m_pChannel->setRange(1, 4);
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

bool WaveformTab::pollOnce()
{
    CScopeManager& mgr = CScopeManager::instance();
    const U32BIT ch = static_cast<U32BIT>(m_pChannel->value());
    mgr.enableChannel(TESTER_SCOPE, ch, true);
    mgr.single(TESTER_SCOPE);
    S_Scope_Waveform wfm;
    const bool ok = mgr.readWaveform(TESTER_SCOPE, ch, wfm).isSuccess() && wfm.pointCount() > 1;
    if (ok) m_pPlot->setWaveform(wfm);
    return ok;
}

void WaveformTab::onFetch()
{
    if (!pollOnce()) emit log(tr("Waveform fetch failed"));
}

void WaveformTab::onTick() { pollOnce(); }

void WaveformTab::onStreamToggled(bool checked)
{
    m_pStreamBtn->setText(checked ? tr("Stop stream") : tr("Start stream"));
    if (checked) m_pTimer->start(); else m_pTimer->stop();
}

void WaveformTab::setConnected(bool in_bConnected)
{
    setEnabled(in_bConnected);
    if (in_bConnected) {
        S_Scope_Capabilities caps = CScopeManager::instance().getCapabilities(TESTER_SCOPE);
        if (caps.m_u32NumberOfChannels >= 1)
            m_pChannel->setRange(1, static_cast<int>(caps.m_u32NumberOfChannels));
    } else {
        m_pTimer->stop();
        m_pStreamBtn->blockSignals(true);
        m_pStreamBtn->setChecked(false);
        m_pStreamBtn->setText(tr("Start stream"));
        m_pStreamBtn->blockSignals(false);
        m_pPlot->clearWaveform();
    }
}
