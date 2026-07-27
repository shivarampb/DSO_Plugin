/*============================================================================
 *  AcquisitionTab.cpp
 *==========================================================================*/
#include "AcquisitionTab.h"
#include "TesterCommon.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

AcquisitionTab::AcquisitionTab(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root = new QVBoxLayout(this);

    QGroupBox* box = new QGroupBox(tr("Acquisition"), this);
    QFormLayout* f = new QFormLayout(box);
    m_pMode = new QComboBox(box);
    m_pMode->addItems(QStringList() << "Sample" << "PeakDetect" << "Average" << "HiRes" << "Envelope");
    m_pAvgCount = new QSpinBox(box); m_pAvgCount->setRange(1, 1000000); m_pAvgCount->setValue(16);
    f->addRow(tr("Mode:"), m_pMode);
    f->addRow(tr("Average count:"), m_pAvgCount);
    root->addWidget(box);

    m_pApplyBtn = new QPushButton(tr("Apply"), this);
    root->addWidget(m_pApplyBtn);

    QHBoxLayout* btns = new QHBoxLayout;
    m_pRunBtn = new QPushButton(tr("Run"), this);
    m_pStopBtn = new QPushButton(tr("Stop"), this);
    m_pSingleBtn = new QPushButton(tr("Single"), this);
    tintButton(m_pRunBtn, "#1e6b33");
    tintButton(m_pStopBtn, "#6b1e1e");
    btns->addWidget(m_pRunBtn);
    btns->addWidget(m_pStopBtn);
    btns->addWidget(m_pSingleBtn);
    root->addLayout(btns);

    m_pReadout = new QLabel(tr("Sample rate: —"), this);
    root->addWidget(m_pReadout);
    root->addStretch(1);

    connect(m_pApplyBtn, SIGNAL(clicked()), this, SLOT(onApply()));
    connect(m_pRunBtn, SIGNAL(clicked()), this, SLOT(onRun()));
    connect(m_pStopBtn, SIGNAL(clicked()), this, SLOT(onStop()));
    connect(m_pSingleBtn, SIGNAL(clicked()), this, SLOT(onSingle()));
}

void AcquisitionTab::refreshReadout()
{
    FDOUBLE sr = 0.0;
    if (CScopeManager::instance().getSampleRate(TESTER_SCOPE, sr).isSuccess())
        m_pReadout->setText(tr("Sample rate: %1 Sa/s").arg(sr, 0, 'g', 4));
}

void AcquisitionTab::onApply()
{
    CScopeManager& mgr = CScopeManager::instance();
    ScopeError e = mgr.setAcqMode(TESTER_SCOPE, static_cast<Enum_Scope_AcqMode>(m_pMode->currentIndex()));
    if (e.isSuccess()) e = mgr.setAverageCount(TESTER_SCOPE, static_cast<U32BIT>(m_pAvgCount->value()));
    refreshReadout();
    emit log(e.isSuccess() ? tr("Acquisition applied") : e.toString());
}

void AcquisitionTab::onRun()    { ScopeError e = CScopeManager::instance().run(TESTER_SCOPE);    refreshReadout(); emit log(e.isSuccess() ? tr("Running")   : e.toString()); }
void AcquisitionTab::onStop()   { ScopeError e = CScopeManager::instance().stop(TESTER_SCOPE);   emit log(e.isSuccess() ? tr("Stopped")   : e.toString()); }
void AcquisitionTab::onSingle() { ScopeError e = CScopeManager::instance().single(TESTER_SCOPE); refreshReadout(); emit log(e.isSuccess() ? tr("Single shot") : e.toString()); }

void AcquisitionTab::setConnected(bool in_bConnected)
{
    setEnabled(in_bConnected);
    if (in_bConnected) refreshReadout();
    else m_pReadout->setText(tr("Sample rate: —"));
}
