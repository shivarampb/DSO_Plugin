/*============================================================================
 *  ScopeTesterWindow.h - the "Scope Plugin Test" main window (dark, tabbed).
 *
 *  A QMainWindow with a QTabWidget: Connection, Vertical, Horizontal & Trigger,
 *  Acquisition, Waveform (live plot), Measurements & Cursors, Save/Screenshot.
 *  Operation tabs stay disabled until a scope is connected. Provides headless
 *  --smoke / --screenshot paths for CI.
 *==========================================================================*/
#ifndef SCOPETESTERWINDOW_H
#define SCOPETESTERWINDOW_H

#include <QMainWindow>

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QTableWidget;
class QTimer;
class QLabel;
class WaveformPlot;
class CScopeManager;

class ScopeTesterWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ScopeTesterWindow(const QString& in_strPluginDir, QWidget* in_pParent = nullptr);

    // Headless CI helpers.
    int  runSmokeTest();
    bool screenshotTo(const QString& in_strPath);

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onApplyVertical();
    void onApplyHorizontalTrigger();
    void onForceTrigger();
    void onRun();
    void onStop();
    void onSingle();
    void onFetchWaveform();
    void onStreamToggled(bool in_bOn);
    void onAddMeasurement();
    void onCaptureScreenshot();

private:
    QWidget* buildConnectionTab();
    QWidget* buildVerticalTab();
    QWidget* buildHorizTrigTab();
    QWidget* buildAcquisitionTab();
    QWidget* buildWaveformTab();
    QWidget* buildMeasurementTab();
    QWidget* buildSaveTab();

    void setConnectedState(bool in_bConnected);
    void refreshVerticalRanges();

    CScopeManager& m_mgr;
    QString        m_strPluginDir;
    bool           m_bConnected;

    QTabWidget*  m_pTabs;
    // connection
    QComboBox*   m_pModelCombo;
    QComboBox*   m_pIfaceCombo;
    QLineEdit*   m_pAddress;
    QPushButton* m_pConnectBtn;
    QPushButton* m_pDisconnectBtn;
    QLabel*      m_pIdnLabel;
    // vertical
    QSpinBox*       m_pVertChannel;
    QDoubleSpinBox* m_pVdiv;
    QDoubleSpinBox* m_pOffset;
    QComboBox*      m_pCoupling;
    // horizontal / trigger
    QDoubleSpinBox* m_pTimebase;
    QComboBox*      m_pTrigSource;
    QComboBox*      m_pTrigSlope;
    QDoubleSpinBox* m_pTrigLevel;
    QLabel*         m_pTrigState;
    // acquisition
    QComboBox*      m_pAcqMode;
    QSpinBox*       m_pAvgCount;
    QLabel*         m_pSampleRate;
    // waveform
    QSpinBox*       m_pWfmChannel;
    WaveformPlot*   m_pPlot;
    QTimer*         m_pStreamTimer;
    QPushButton*    m_pStreamBtn;
    // measurements
    QComboBox*      m_pMeasType;
    QTableWidget*   m_pMeasTable;
    // save / screenshot
    QSpinBox*       m_pSetupSlot;
    QLabel*         m_pShotLabel;
};

#endif // SCOPETESTERWINDOW_H
