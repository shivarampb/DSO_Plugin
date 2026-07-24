/*============================================================================
 *  ScopeTesterWindow.cpp - implementation of the tabbed tester window.
 *
 *  Calls into CScopeManager directly; against SimScope / MockVisa every call
 *  returns immediately, so the UI stays responsive without a worker thread.
 *  (On real hardware, long calls would be moved to a per-scope worker.)
 *==========================================================================*/
#include "ScopeTesterWindow.h"
#include "WaveformPlot.h"
#include "TesterCommon.h"

#include "ScopeManager.h"

#include <QtWidgets>

namespace {
const char* MEAS_NAMES[] = { "Vpp", "Vmax", "Vmin", "Vrms", "Vavg", "Frequency", "Period" };
const Enum_Scope_MeasType MEAS_TYPES[] = {
    Enum_Scope_MeasType::m_enumVpp, Enum_Scope_MeasType::m_enumVmax, Enum_Scope_MeasType::m_enumVmin,
    Enum_Scope_MeasType::m_enumVrms, Enum_Scope_MeasType::m_enumVavg,
    Enum_Scope_MeasType::m_enumFrequency, Enum_Scope_MeasType::m_enumPeriod
};
const int MEAS_COUNT = 7;
} // namespace

ScopeTesterWindow::ScopeTesterWindow(const QString& in_strPluginDir, QWidget* in_pParent)
    : QMainWindow(in_pParent)
    , m_mgr(CScopeManager::instance())
    , m_strPluginDir(in_strPluginDir)
    , m_bConnected(false)
    , m_pStreamTimer(nullptr)
{
    m_mgr.loadPlugins(m_strPluginDir);

    setWindowTitle(QStringLiteral("Scope Plugin Test"));
    resize(880, 620);

    m_pTabs = new QTabWidget(this);
    m_pTabs->addTab(buildConnectionTab(), QStringLiteral("Connection"));
    m_pTabs->addTab(buildVerticalTab(), QStringLiteral("Vertical"));
    m_pTabs->addTab(buildHorizTrigTab(), QStringLiteral("Horizontal && Trigger"));
    m_pTabs->addTab(buildAcquisitionTab(), QStringLiteral("Acquisition"));
    m_pTabs->addTab(buildWaveformTab(), QStringLiteral("Waveform"));
    m_pTabs->addTab(buildMeasurementTab(), QStringLiteral("Measurements && Cursors"));
    m_pTabs->addTab(buildSaveTab(), QStringLiteral("Save / Screenshot"));
    setCentralWidget(m_pTabs);

    statusBar()->showMessage(QStringLiteral("Disconnected"));
    setConnectedState(false);
}

/*----------------------------------------------------------------------------
 * Connection tab
 *--------------------------------------------------------------------------*/
QWidget* ScopeTesterWindow::buildConnectionTab()
{
    QWidget* w = new QWidget;
    QFormLayout* form = new QFormLayout(w);

    m_pModelCombo = new QComboBox;
    const QStringList plugins = m_mgr.getAvailablePlugins();
    m_pModelCombo->addItems(plugins.isEmpty() ? QStringList(QStringLiteral("MDO34")) : plugins);

    m_pIfaceCombo = new QComboBox;
    m_pIfaceCombo->addItems(QStringList() << QStringLiteral("Mock (dev)") << QStringLiteral("SimScope")
                            << QStringLiteral("LAN") << QStringLiteral("USB") << QStringLiteral("GPIB"));

    m_pAddress = new QLineEdit(QStringLiteral("MOCK0::MDO34::INSTR"));

    m_pConnectBtn = new QPushButton(QStringLiteral("Connect"));
    m_pDisconnectBtn = new QPushButton(QStringLiteral("Disconnect"));
    tintButton(m_pConnectBtn, QColor(40, 160, 70));
    tintButton(m_pDisconnectBtn, QColor(180, 60, 60));
    connect(m_pConnectBtn, &QPushButton::clicked, this, &ScopeTesterWindow::onConnectClicked);
    connect(m_pDisconnectBtn, &QPushButton::clicked, this, &ScopeTesterWindow::onDisconnectClicked);

    m_pIdnLabel = new QLabel(QStringLiteral("-"));
    m_pIdnLabel->setWordWrap(true);

    QHBoxLayout* btns = new QHBoxLayout;
    btns->addWidget(m_pConnectBtn);
    btns->addWidget(m_pDisconnectBtn);
    btns->addStretch();

    form->addRow(QStringLiteral("Model:"), m_pModelCombo);
    form->addRow(QStringLiteral("Interface:"), m_pIfaceCombo);
    form->addRow(QStringLiteral("Address / resource:"), m_pAddress);
    form->addRow(btns);
    form->addRow(QStringLiteral("Identity (*IDN?):"), m_pIdnLabel);
    return w;
}

/*----------------------------------------------------------------------------
 * Vertical tab
 *--------------------------------------------------------------------------*/
QWidget* ScopeTesterWindow::buildVerticalTab()
{
    QWidget* w = new QWidget;
    QFormLayout* form = new QFormLayout(w);
    m_pVertChannel = new QSpinBox; m_pVertChannel->setRange(1, 4);
    m_pVdiv = new QDoubleSpinBox; m_pVdiv->setDecimals(4); m_pVdiv->setRange(0.001, 10.0); m_pVdiv->setValue(0.1);
    m_pOffset = new QDoubleSpinBox; m_pOffset->setDecimals(4); m_pOffset->setRange(-100.0, 100.0);
    m_pCoupling = new QComboBox; m_pCoupling->addItems(QStringList() << "DC" << "AC" << "GND");
    QPushButton* apply = new QPushButton(QStringLiteral("Apply"));
    connect(apply, &QPushButton::clicked, this, &ScopeTesterWindow::onApplyVertical);
    form->addRow(QStringLiteral("Channel:"), m_pVertChannel);
    form->addRow(QStringLiteral("Scale (V/div):"), m_pVdiv);
    form->addRow(QStringLiteral("Offset (V):"), m_pOffset);
    form->addRow(QStringLiteral("Coupling:"), m_pCoupling);
    form->addRow(apply);
    return w;
}

/*----------------------------------------------------------------------------
 * Horizontal & Trigger tab
 *--------------------------------------------------------------------------*/
QWidget* ScopeTesterWindow::buildHorizTrigTab()
{
    QWidget* w = new QWidget;
    QFormLayout* form = new QFormLayout(w);
    m_pTimebase = new QDoubleSpinBox; m_pTimebase->setDecimals(12);
    m_pTimebase->setRange(1e-10, 1000.0); m_pTimebase->setValue(1e-6);
    m_pTrigSource = new QComboBox; m_pTrigSource->addItems(QStringList() << "CH1" << "CH2" << "CH3" << "CH4" << "EXT");
    m_pTrigSlope = new QComboBox; m_pTrigSlope->addItems(QStringList() << "Rising" << "Falling" << "Either");
    m_pTrigLevel = new QDoubleSpinBox; m_pTrigLevel->setDecimals(4); m_pTrigLevel->setRange(-100.0, 100.0);
    QPushButton* apply = new QPushButton(QStringLiteral("Apply"));
    QPushButton* force = new QPushButton(QStringLiteral("Force Trigger"));
    connect(apply, &QPushButton::clicked, this, &ScopeTesterWindow::onApplyHorizontalTrigger);
    connect(force, &QPushButton::clicked, this, &ScopeTesterWindow::onForceTrigger);
    m_pTrigState = new QLabel(QStringLiteral("-"));
    form->addRow(QStringLiteral("Timebase (s/div):"), m_pTimebase);
    form->addRow(QStringLiteral("Trigger source:"), m_pTrigSource);
    form->addRow(QStringLiteral("Trigger slope:"), m_pTrigSlope);
    form->addRow(QStringLiteral("Trigger level (V):"), m_pTrigLevel);
    QHBoxLayout* h = new QHBoxLayout; h->addWidget(apply); h->addWidget(force); h->addStretch();
    form->addRow(h);
    form->addRow(QStringLiteral("Trigger state:"), m_pTrigState);
    return w;
}

/*----------------------------------------------------------------------------
 * Acquisition tab
 *--------------------------------------------------------------------------*/
QWidget* ScopeTesterWindow::buildAcquisitionTab()
{
    QWidget* w = new QWidget;
    QFormLayout* form = new QFormLayout(w);
    m_pAcqMode = new QComboBox;
    m_pAcqMode->addItems(QStringList() << "Sample" << "PeakDetect" << "Average" << "HiRes" << "Envelope");
    m_pAvgCount = new QSpinBox; m_pAvgCount->setRange(1, 100000); m_pAvgCount->setValue(16);
    QPushButton* runBtn = new QPushButton(QStringLiteral("Run"));
    QPushButton* stopBtn = new QPushButton(QStringLiteral("Stop"));
    QPushButton* singleBtn = new QPushButton(QStringLiteral("Single"));
    tintButton(runBtn, QColor(40, 160, 70));
    tintButton(stopBtn, QColor(180, 60, 60));
    connect(runBtn, &QPushButton::clicked, this, &ScopeTesterWindow::onRun);
    connect(stopBtn, &QPushButton::clicked, this, &ScopeTesterWindow::onStop);
    connect(singleBtn, &QPushButton::clicked, this, &ScopeTesterWindow::onSingle);
    m_pSampleRate = new QLabel(QStringLiteral("-"));
    form->addRow(QStringLiteral("Acquisition mode:"), m_pAcqMode);
    form->addRow(QStringLiteral("Average count:"), m_pAvgCount);
    QHBoxLayout* h = new QHBoxLayout; h->addWidget(runBtn); h->addWidget(stopBtn); h->addWidget(singleBtn); h->addStretch();
    form->addRow(h);
    form->addRow(QStringLiteral("Sample rate / mem:"), m_pSampleRate);
    return w;
}

/*----------------------------------------------------------------------------
 * Waveform tab (the live plot)
 *--------------------------------------------------------------------------*/
QWidget* ScopeTesterWindow::buildWaveformTab()
{
    QWidget* w = new QWidget;
    QVBoxLayout* v = new QVBoxLayout(w);
    QHBoxLayout* ctrl = new QHBoxLayout;
    m_pWfmChannel = new QSpinBox; m_pWfmChannel->setRange(1, 4);
    QPushButton* fetch = new QPushButton(QStringLiteral("Fetch"));
    m_pStreamBtn = new QPushButton(QStringLiteral("Start stream"));
    m_pStreamBtn->setCheckable(true);
    connect(fetch, &QPushButton::clicked, this, &ScopeTesterWindow::onFetchWaveform);
    connect(m_pStreamBtn, &QPushButton::toggled, this, &ScopeTesterWindow::onStreamToggled);
    ctrl->addWidget(new QLabel(QStringLiteral("Source CH:")));
    ctrl->addWidget(m_pWfmChannel);
    ctrl->addWidget(fetch);
    ctrl->addWidget(m_pStreamBtn);
    ctrl->addStretch();
    m_pPlot = new WaveformPlot;
    v->addLayout(ctrl);
    v->addWidget(m_pPlot, 1);

    m_pStreamTimer = new QTimer(this);
    m_pStreamTimer->setInterval(200);
    connect(m_pStreamTimer, &QTimer::timeout, this, &ScopeTesterWindow::onFetchWaveform);
    return w;
}

/*----------------------------------------------------------------------------
 * Measurements & Cursors tab
 *--------------------------------------------------------------------------*/
QWidget* ScopeTesterWindow::buildMeasurementTab()
{
    QWidget* w = new QWidget;
    QVBoxLayout* v = new QVBoxLayout(w);
    QHBoxLayout* ctrl = new QHBoxLayout;
    m_pMeasType = new QComboBox;
    for (int i = 0; i < MEAS_COUNT; ++i) m_pMeasType->addItem(QString::fromLatin1(MEAS_NAMES[i]));
    QPushButton* add = new QPushButton(QStringLiteral("Add / Read"));
    connect(add, &QPushButton::clicked, this, &ScopeTesterWindow::onAddMeasurement);
    ctrl->addWidget(new QLabel(QStringLiteral("Type:")));
    ctrl->addWidget(m_pMeasType);
    ctrl->addWidget(add);
    ctrl->addStretch();
    m_pMeasTable = new QTableWidget(0, 3);
    m_pMeasTable->setHorizontalHeaderLabels(QStringList() << "Type" << "Value" << "Units");
    m_pMeasTable->horizontalHeader()->setStretchLastSection(true);
    v->addLayout(ctrl);
    v->addWidget(m_pMeasTable, 1);
    return w;
}

/*----------------------------------------------------------------------------
 * Save / Screenshot tab
 *--------------------------------------------------------------------------*/
QWidget* ScopeTesterWindow::buildSaveTab()
{
    QWidget* w = new QWidget;
    QVBoxLayout* v = new QVBoxLayout(w);
    QHBoxLayout* ctrl = new QHBoxLayout;
    m_pSetupSlot = new QSpinBox; m_pSetupSlot->setRange(0, 9);
    QPushButton* saveBtn = new QPushButton(QStringLiteral("Save setup"));
    QPushButton* recallBtn = new QPushButton(QStringLiteral("Recall setup"));
    QPushButton* shotBtn = new QPushButton(QStringLiteral("Capture Screenshot"));
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        if (m_bConnected) m_mgr.saveSetup(TESTER_SCOPE, static_cast<U32BIT>(m_pSetupSlot->value())); });
    connect(recallBtn, &QPushButton::clicked, this, [this]() {
        if (m_bConnected) m_mgr.recallSetup(TESTER_SCOPE, static_cast<U32BIT>(m_pSetupSlot->value())); });
    connect(shotBtn, &QPushButton::clicked, this, &ScopeTesterWindow::onCaptureScreenshot);
    ctrl->addWidget(new QLabel(QStringLiteral("Slot:")));
    ctrl->addWidget(m_pSetupSlot);
    ctrl->addWidget(saveBtn);
    ctrl->addWidget(recallBtn);
    ctrl->addWidget(shotBtn);
    ctrl->addStretch();
    m_pShotLabel = new QLabel(QStringLiteral("no screenshot"));
    m_pShotLabel->setAlignment(Qt::AlignCenter);
    m_pShotLabel->setMinimumHeight(200);
    v->addLayout(ctrl);
    v->addWidget(m_pShotLabel, 1);
    return w;
}

/*----------------------------------------------------------------------------
 * Enable/disable operation tabs based on connection state.
 *--------------------------------------------------------------------------*/
void ScopeTesterWindow::setConnectedState(bool in_bConnected)
{
    m_bConnected = in_bConnected;
    for (int i = 1; i < m_pTabs->count(); ++i)
        m_pTabs->setTabEnabled(i, in_bConnected);
    m_pConnectBtn->setEnabled(!in_bConnected);
    m_pDisconnectBtn->setEnabled(in_bConnected);
    statusBar()->showMessage(in_bConnected ? QStringLiteral("Connected") : QStringLiteral("Disconnected"));
}

void ScopeTesterWindow::refreshVerticalRanges()
{
    if (!m_bConnected) return;
    S_Scope_ParameterRange r;
    if (m_mgr.getParameterRange(TESTER_SCOPE, TESTER_CHANNEL,
                                Enum_Scope_ParamId::m_enumVerticalScale, r).isSuccess()) {
        m_pVdiv->setRange(r.m_dMin, r.m_dMax);
    }
    if (m_mgr.getParameterRange(TESTER_SCOPE, TESTER_CHANNEL,
                                Enum_Scope_ParamId::m_enumTimebaseScale, r).isSuccess()) {
        m_pTimebase->setRange(r.m_dMin, r.m_dMax);
    }
    S_Scope_Capabilities caps = m_mgr.getCapabilities(TESTER_SCOPE);
    if (caps.m_u32NumberOfChannels >= 1) {
        m_pVertChannel->setRange(1, static_cast<int>(caps.m_u32NumberOfChannels));
        m_pWfmChannel->setRange(1, static_cast<int>(caps.m_u32NumberOfChannels));
    }
}

/*----------------------------------------------------------------------------
 * Slots
 *--------------------------------------------------------------------------*/
void ScopeTesterWindow::onConnectClicked()
{
    const QString model = m_pModelCombo->currentText();
    const QString iface = m_pIfaceCombo->currentText();
    QString pluginName = model;
    QString resource;
    if (iface == QStringLiteral("SimScope") || model == QStringLiteral("SimScope")) {
        pluginName = QStringLiteral("SimScope");
        const QString m = (model == QStringLiteral("SimScope")) ? QStringLiteral("MDO34") : model;
        resource = QStringLiteral("SIM::%1").arg(m);
    } else if (iface == QStringLiteral("Mock (dev)")) {
        resource = QStringLiteral("MOCK0::%1::INSTR").arg(model);
    } else if (iface == QStringLiteral("LAN")) {
        resource = QStringLiteral("TCPIP0::%1::INSTR").arg(m_pAddress->text());
    } else if (iface == QStringLiteral("GPIB")) {
        resource = QStringLiteral("GPIB0::%1::INSTR").arg(m_pAddress->text());
    } else {
        resource = m_pAddress->text();
    }

    if (m_mgr.instanceExists(TESTER_SCOPE)) m_mgr.destroyInstance(TESTER_SCOPE);
    ScopeError e = m_mgr.createInstance(TESTER_SCOPE, pluginName);
    if (!e.isSuccess()) { QMessageBox::warning(this, QStringLiteral("Connect"), e.toString()); return; }
    S_Scope_ConnectionConfig cfg; cfg.setResourceString(resource);
    e = m_mgr.connect(TESTER_SCOPE, cfg);
    if (!e.isSuccess()) {
        QMessageBox::warning(this, QStringLiteral("Connect"), e.toString());
        m_mgr.destroyInstance(TESTER_SCOPE);
        return;
    }
    QString idn; m_mgr.getIdentification(TESTER_SCOPE, idn);
    m_pIdnLabel->setText(idn);
    setConnectedState(true);
    refreshVerticalRanges();
}

void ScopeTesterWindow::onDisconnectClicked()
{
    if (m_pStreamBtn->isChecked()) m_pStreamBtn->setChecked(false);
    m_mgr.disconnect(TESTER_SCOPE);
    m_mgr.destroyInstance(TESTER_SCOPE);
    m_pIdnLabel->setText(QStringLiteral("-"));
    m_pPlot->clearWaveform();
    setConnectedState(false);
}

void ScopeTesterWindow::onApplyVertical()
{
    if (!m_bConnected) return;
    const U32BIT ch = static_cast<U32BIT>(m_pVertChannel->value());
    m_mgr.enableChannel(TESTER_SCOPE, ch, true);
    m_mgr.setVerticalScale(TESTER_SCOPE, ch, m_pVdiv->value());
    m_mgr.setVerticalOffset(TESTER_SCOPE, ch, m_pOffset->value());
    const Enum_Scope_Coupling cp = (m_pCoupling->currentIndex() == 1) ? Enum_Scope_Coupling::m_enumAC
                                 : (m_pCoupling->currentIndex() == 2) ? Enum_Scope_Coupling::m_enumGND
                                 : Enum_Scope_Coupling::m_enumDC;
    ScopeError e = m_mgr.setCoupling(TESTER_SCOPE, ch, cp);
    statusBar()->showMessage(e.isSuccess() ? QStringLiteral("Vertical applied") : e.toString(), 3000);
}

void ScopeTesterWindow::onApplyHorizontalTrigger()
{
    if (!m_bConnected) return;
    m_mgr.setTimebaseScale(TESTER_SCOPE, m_pTimebase->value());
    const Enum_Scope_TriggerSource src = static_cast<Enum_Scope_TriggerSource>(m_pTrigSource->currentIndex());
    m_mgr.setTriggerSource(TESTER_SCOPE, src);
    const Enum_Scope_TriggerSlope slp = static_cast<Enum_Scope_TriggerSlope>(m_pTrigSlope->currentIndex());
    m_mgr.setTriggerSlope(TESTER_SCOPE, slp);
    m_mgr.setTriggerLevel(TESTER_SCOPE, TESTER_CHANNEL, m_pTrigLevel->value());
    Enum_Scope_TriggerState st = Enum_Scope_TriggerState::m_enumUnknown;
    m_mgr.getTriggerState(TESTER_SCOPE, st);
    m_pTrigState->setText(ScopeError::triggerStateToString(st));
}

void ScopeTesterWindow::onForceTrigger() { if (m_bConnected) m_mgr.forceTrigger(TESTER_SCOPE); }
void ScopeTesterWindow::onRun()          { if (m_bConnected) m_mgr.run(TESTER_SCOPE); }
void ScopeTesterWindow::onStop()         { if (m_bConnected) m_mgr.stop(TESTER_SCOPE); }
void ScopeTesterWindow::onSingle()       { if (m_bConnected) m_mgr.single(TESTER_SCOPE); }

void ScopeTesterWindow::onFetchWaveform()
{
    if (!m_bConnected) return;
    const U32BIT ch = static_cast<U32BIT>(m_pWfmChannel->value());
    m_mgr.enableChannel(TESTER_SCOPE, ch, true);
    m_mgr.single(TESTER_SCOPE);
    S_Scope_Waveform wfm;
    ScopeError e = m_mgr.readWaveform(TESTER_SCOPE, ch, wfm);
    if (e.isSuccess()) {
        m_pPlot->setWaveform(wfm);
        FDOUBLE sr = 0; m_mgr.getSampleRate(TESTER_SCOPE, sr);
        m_pSampleRate->setText(QStringLiteral("%1 Sa/s, %2 pts").arg(sr, 0, 'g', 4).arg(wfm.pointCount()));
    } else {
        statusBar()->showMessage(e.toString(), 3000);
    }
}

void ScopeTesterWindow::onStreamToggled(bool in_bOn)
{
    m_pStreamBtn->setText(in_bOn ? QStringLiteral("Stop stream") : QStringLiteral("Start stream"));
    if (in_bOn) m_pStreamTimer->start(); else m_pStreamTimer->stop();
}

void ScopeTesterWindow::onAddMeasurement()
{
    if (!m_bConnected) return;
    const int idx = m_pMeasType->currentIndex();
    if (idx < 0 || idx >= MEAS_COUNT) return;
    S_Scope_MeasurementResult r;
    ScopeError e = m_mgr.readMeasurement(TESTER_SCOPE, static_cast<U32BIT>(m_pWfmChannel->value()),
                                         MEAS_TYPES[idx], r);
    const int row = m_pMeasTable->rowCount();
    m_pMeasTable->insertRow(row);
    m_pMeasTable->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(MEAS_NAMES[idx])));
    m_pMeasTable->setItem(row, 1, new QTableWidgetItem(
        e.isSuccess() ? QString::number(r.m_dValue, 'g', 6) : e.toString()));
    m_pMeasTable->setItem(row, 2, new QTableWidgetItem(r.m_strUnits));
}

void ScopeTesterWindow::onCaptureScreenshot()
{
    if (!m_bConnected) return;
    QByteArray png;
    ScopeError e = m_mgr.captureScreenshot(TESTER_SCOPE, Enum_Scope_ImageFormat::m_enumPng, png);
    QImage img;
    if (e.isSuccess() && img.loadFromData(png)) {
        m_pShotLabel->setPixmap(QPixmap::fromImage(img).scaled(
            m_pShotLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // model does not implement instrument screenshot: grab the app view
        m_pShotLabel->setPixmap(grab().scaled(m_pShotLabel->size(), Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
        m_pShotLabel->setText(QString());
    }
}

/*----------------------------------------------------------------------------
 * Headless CI helpers
 *--------------------------------------------------------------------------*/
int ScopeTesterWindow::runSmokeTest()
{
    // Open a SimScope (no VISA needed), fetch one waveform, verify it decoded.
    if (m_mgr.instanceExists(TESTER_SCOPE)) m_mgr.destroyInstance(TESTER_SCOPE);
    if (!m_mgr.createInstance(TESTER_SCOPE, QStringLiteral("SimScope")).isSuccess()) {
        std::fprintf(stderr, "smoke: cannot create SimScope instance\n");
        return 2;
    }
    S_Scope_ConnectionConfig cfg; cfg.setResourceString(QStringLiteral("SIM::MDO34"));
    if (!m_mgr.connect(TESTER_SCOPE, cfg).isSuccess()) {
        std::fprintf(stderr, "smoke: connect failed\n");
        return 3;
    }
    setConnectedState(true);
    refreshVerticalRanges();
    m_mgr.enableChannel(TESTER_SCOPE, 1, true);
    m_mgr.single(TESTER_SCOPE);
    S_Scope_Waveform wfm;
    if (!m_mgr.readWaveform(TESTER_SCOPE, 1, wfm).isSuccess() || wfm.pointCount() < 2) {
        std::fprintf(stderr, "smoke: waveform read failed\n");
        return 4;
    }
    m_pPlot->setWaveform(wfm);
    m_mgr.disconnect(TESTER_SCOPE);
    m_mgr.destroyInstance(TESTER_SCOPE);
    return 0;
}

bool ScopeTesterWindow::screenshotTo(const QString& in_strPath)
{
    return grab().save(in_strPath);
}
