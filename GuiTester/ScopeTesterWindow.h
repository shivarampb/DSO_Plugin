/**
 * @file    ScopeTesterWindow.h
 * @brief   Main window declaration - composes the function-group tabs.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#ifndef SCOPETESTERWINDOW_H
#define SCOPETESTERWINDOW_H

#include <QMainWindow>

class QTabWidget;
class QLabel;
class ConnectionTab;
class VerticalTab;
class HorizontalTriggerTab;
class AcquisitionTab;
class WaveformTab;
class MeasurementCursorTab;
class SaveTab;

class ScopeTesterWindow : public QMainWindow
{
    Q_OBJECT
  public:
    explicit ScopeTesterWindow(const QString& in_strPluginDir, QWidget* parent = nullptr);

    int runSmokeTest();                          // offscreen CI self-test; 0 on success
    int screenshotTo(const QString& in_strPath); // connect + render to an image

  private slots:
    void onConnected(const QString& in_strModel);
    void onDisconnected();
    void onLog(const QString& in_strText);

  private:
    void setOperationTabsEnabled(bool in_bEnabled);

    QTabWidget* m_pTabs;
    QLabel* m_pStatus;
    ConnectionTab* m_pConnection;
    VerticalTab* m_pVertical;
    HorizontalTriggerTab* m_pHorizontal;
    AcquisitionTab* m_pAcquisition;
    WaveformTab* m_pWaveform;
    MeasurementCursorTab* m_pMeasurement;
    SaveTab* m_pSave;
};

#endif // SCOPETESTERWINDOW_H
