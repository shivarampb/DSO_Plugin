/**
 * @file    ScopeTesterWindow.cpp
 * @brief   Main window implementation - tab composition, status, smoke/screenshot.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "ScopeTesterWindow.h"
#include "TesterCommon.h"

#include "ConnectionTab.h"
#include "VerticalTab.h"
#include "HorizontalTriggerTab.h"
#include "AcquisitionTab.h"
#include "WaveformTab.h"
#include "MeasurementCursorTab.h"
#include "SaveTab.h"

#include <QApplication>
#include <QLabel>
#include <QStatusBar>
#include <QTabWidget>

#include "ScopeManager.h"

ScopeTesterWindow::ScopeTesterWindow(const QString& in_strPluginDir, QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Scope Plugin Test"));
    CScopeManager::instance().loadPlugins(in_strPluginDir);

    m_pTabs = new QTabWidget(this);
    m_pConnection = new ConnectionTab(this);
    m_pVertical = new VerticalTab(this);
    m_pHorizontal = new HorizontalTriggerTab(this);
    m_pAcquisition = new AcquisitionTab(this);
    m_pWaveform = new WaveformTab(this);
    m_pMeasurement = new MeasurementCursorTab(this);
    m_pSave = new SaveTab(this);

    m_pTabs->addTab(m_pConnection, tr("Connection"));
    m_pTabs->addTab(m_pVertical, tr("Vertical"));
    m_pTabs->addTab(m_pHorizontal, tr("Horizontal && Trigger"));
    m_pTabs->addTab(m_pAcquisition, tr("Acquisition"));
    m_pTabs->addTab(m_pWaveform, tr("Waveform"));
    m_pTabs->addTab(m_pMeasurement, tr("Measurements && Cursors"));
    m_pTabs->addTab(m_pSave, tr("Save / Screenshot"));
    setCentralWidget(m_pTabs);

    m_pStatus = new QLabel(tr("Disconnected"), this);
    statusBar()->addWidget(m_pStatus);

    connect(m_pConnection, SIGNAL(connected(QString)), this, SLOT(onConnected(QString)));
    connect(m_pConnection, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(m_pConnection, SIGNAL(log(QString)), this, SLOT(onLog(QString)));
    connect(m_pVertical, SIGNAL(log(QString)), this, SLOT(onLog(QString)));
    connect(m_pHorizontal, SIGNAL(log(QString)), this, SLOT(onLog(QString)));
    connect(m_pAcquisition, SIGNAL(log(QString)), this, SLOT(onLog(QString)));
    connect(m_pWaveform, SIGNAL(log(QString)), this, SLOT(onLog(QString)));
    connect(m_pMeasurement, SIGNAL(log(QString)), this, SLOT(onLog(QString)));
    connect(m_pSave, SIGNAL(log(QString)), this, SLOT(onLog(QString)));

    setOperationTabsEnabled(false);
    resize(880, 680);
}

void ScopeTesterWindow::setOperationTabsEnabled(bool in_bEnabled)
{
    m_pVertical->setConnected(in_bEnabled);
    m_pHorizontal->setConnected(in_bEnabled);
    m_pAcquisition->setConnected(in_bEnabled);
    m_pWaveform->setConnected(in_bEnabled);
    m_pMeasurement->setConnected(in_bEnabled);
    m_pSave->setConnected(in_bEnabled);
}

void ScopeTesterWindow::onConnected(const QString& in_strModel)
{
    setOperationTabsEnabled(true);
    m_pStatus->setText(tr("Connected — %1").arg(in_strModel));
    m_pTabs->setCurrentWidget(m_pVertical);
}

void ScopeTesterWindow::onDisconnected()
{
    setOperationTabsEnabled(false);
    m_pStatus->setText(tr("Disconnected"));
    m_pTabs->setCurrentWidget(m_pConnection);
}

void ScopeTesterWindow::onLog(const QString& in_strText)
{
    statusBar()->showMessage(in_strText, 4000);
}

int ScopeTesterWindow::runSmokeTest()
{
    CScopeManager& mgr = CScopeManager::instance();
    if (!mgr.getAvailablePlugins().contains(QStringLiteral("MDO34")))
    {
        return 1;
    }
    if (!m_pConnection->connectTo(QStringLiteral("MDO34"), QStringLiteral("MOCK0::MDO34::INSTR")))
    {
        return 2;
    }
    mgr.enableChannel(TESTER_SCOPE, TESTER_CHANNEL, true);
    mgr.setVerticalScale(TESTER_SCOPE, TESTER_CHANNEL, 0.5);
    mgr.setTimebaseScale(TESTER_SCOPE, 1.0e-6);
    mgr.single(TESTER_SCOPE);
    const bool ok = m_pWaveform->pollOnce();
    mgr.disconnect(TESTER_SCOPE);
    mgr.destroyInstance(TESTER_SCOPE);
    return ok ? 0 : 3;
}

int ScopeTesterWindow::screenshotTo(const QString& in_strPath)
{
    CScopeManager& mgr = CScopeManager::instance();
    if (!mgr.getAvailablePlugins().contains(QStringLiteral("MDO34")))
    {
        return 1;
    }
    if (!m_pConnection->connectTo(QStringLiteral("MDO34"), QStringLiteral("MOCK0::MDO34::INSTR")))
    {
        return 2;
    }
    onConnected(QStringLiteral("MDO34"));
    mgr.enableChannel(TESTER_SCOPE, TESTER_CHANNEL, true);
    mgr.setVerticalScale(TESTER_SCOPE, TESTER_CHANNEL, 0.5);
    mgr.setTimebaseScale(TESTER_SCOPE, 1.0e-6);
    mgr.single(TESTER_SCOPE);
    m_pWaveform->pollOnce();
    m_pTabs->setCurrentWidget(m_pWaveform);
    show();
    QApplication::processEvents();
    const bool ok = grab().save(in_strPath);
    mgr.disconnect(TESTER_SCOPE);
    mgr.destroyInstance(TESTER_SCOPE);
    return ok ? 0 : 3;
}
