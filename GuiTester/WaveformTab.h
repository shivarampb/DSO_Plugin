/**
 * @file    WaveformTab.h
 * @brief   Waveform tab - live plot with fetch/stream control.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#ifndef WAVEFORMTAB_H
#define WAVEFORMTAB_H

#include <QWidget>
#include "ScopeManager.h"

class QSpinBox;
class QPushButton;
class QTimer;
class WaveformPlot;

class WaveformTab : public QWidget
{
    Q_OBJECT
  public:
    explicit WaveformTab(QWidget* parent = nullptr);
    void setConnected(bool in_bConnected);
    bool pollOnce(); // used by the smoke test

  signals:
    void log(const QString& in_strText);

  private slots:
    void onFetch();
    void onStreamToggled(bool checked);
    void onTick();

  private:
    QSpinBox* m_pChannel;
    QPushButton* m_pFetchBtn;
    QPushButton* m_pStreamBtn;
    WaveformPlot* m_pPlot;
    QTimer* m_pTimer;
};

#endif // WAVEFORMTAB_H
