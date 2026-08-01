/*============================================================================
 *  MeasurementCursorTab.cpp
 *==========================================================================*/
#include "MeasurementCursorTab.h"
#include "TesterCommon.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
const char* MEAS_NAMES[] = {"Vpp", "Vmax", "Vmin", "Vrms", "Vavg", "Frequency", "Period"};
const Enum_Scope_MeasType MEAS_TYPES[] = {
    Enum_Scope_MeasType::m_enumVpp,   Enum_Scope_MeasType::m_enumVmax, Enum_Scope_MeasType::m_enumVmin,
    Enum_Scope_MeasType::m_enumVrms,  Enum_Scope_MeasType::m_enumVavg, Enum_Scope_MeasType::m_enumFrequency,
    Enum_Scope_MeasType::m_enumPeriod};
const int MEAS_COUNT = 7;
} // namespace

MeasurementCursorTab::MeasurementCursorTab(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root = new QVBoxLayout(this);

    QHBoxLayout* ctl = new QHBoxLayout;
    m_pChannel = new QSpinBox(this);
    m_pChannel->setRange(1, 4);
    m_pMeasType = new QComboBox(this);
    for (int i = 0; i < MEAS_COUNT; ++i)
    {
        m_pMeasType->addItem(QString::fromLatin1(MEAS_NAMES[i]));
    }
    m_pAddBtn = new QPushButton(tr("Add / Read"), this);
    ctl->addWidget(new QLabel(tr("CH:"), this));
    ctl->addWidget(m_pChannel);
    ctl->addWidget(new QLabel(tr("Type:"), this));
    ctl->addWidget(m_pMeasType);
    ctl->addWidget(m_pAddBtn);
    ctl->addStretch(1);
    root->addLayout(ctl);

    m_pTable = new QTableWidget(0, 3, this);
    m_pTable->setHorizontalHeaderLabels(QStringList() << "Type" << "Value" << "Units");
    m_pTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_pTable, 1);

    QHBoxLayout* cur = new QHBoxLayout;
    m_pCursorType = new QComboBox(this);
    m_pCursorType->addItems(QStringList() << "Off" << "Horizontal" << "Vertical" << "Track");
    m_pCursorBtn = new QPushButton(tr("Apply Cursor"), this);
    cur->addWidget(new QLabel(tr("Cursor:"), this));
    cur->addWidget(m_pCursorType);
    cur->addWidget(m_pCursorBtn);
    cur->addStretch(1);
    root->addLayout(cur);

    connect(m_pAddBtn, SIGNAL(clicked()), this, SLOT(onAddMeasurement()));
    connect(m_pCursorBtn, SIGNAL(clicked()), this, SLOT(onApplyCursor()));
}

void MeasurementCursorTab::onAddMeasurement()
{
    const int idx = m_pMeasType->currentIndex();
    if (idx < 0 || idx >= MEAS_COUNT)
    {
        return;
    }
    S_Scope_MeasurementResult r;
    ScopeError e = CScopeManager::instance().readMeasurement(
        TESTER_SCOPE, static_cast<U32BIT>(m_pChannel->value()), MEAS_TYPES[idx], r);
    const int row = m_pTable->rowCount();
    m_pTable->insertRow(row);
    m_pTable->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(MEAS_NAMES[idx])));
    m_pTable->setItem(
        row, 1, new QTableWidgetItem(e.isSuccess() ? QString::number(r.m_dValue, 'g', 6) : e.toString()));
    m_pTable->setItem(row, 2, new QTableWidgetItem(r.m_strUnits));
    emit log(e.isSuccess() ? tr("Measured %1").arg(QString::fromLatin1(MEAS_NAMES[idx])) : e.toString());
}

void MeasurementCursorTab::onApplyCursor()
{
    CScopeManager& mgr = CScopeManager::instance();
    ScopeError e =
        mgr.setCursorType(TESTER_SCOPE, static_cast<Enum_Scope_CursorType>(m_pCursorType->currentIndex()));
    if (e.isSuccess())
    {
        e = mgr.setCursorSource(TESTER_SCOPE, static_cast<U32BIT>(m_pChannel->value()));
    }
    emit log(e.isSuccess() ? tr("Cursor applied") : e.toString());
}

void MeasurementCursorTab::setConnected(bool in_bConnected)
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
        m_pTable->setRowCount(0);
    }
}
