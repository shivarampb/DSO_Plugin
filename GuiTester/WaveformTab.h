/*============================================================================
 *  WaveformTab.h - live volts-vs-time plot with a fetch / stream control.
 *==========================================================================*/
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
    bool pollOnce();                          // used by the smoke test

signals:
    void log(const QString& in_strText);

private slots:
    void onFetch();
    void onStreamToggled(bool checked);
    void onTick();

private:
    QSpinBox*     m_pChannel;
    QPushButton*  m_pFetchBtn;
    QPushButton*  m_pStreamBtn;
    WaveformPlot* m_pPlot;
    QTimer*       m_pTimer;
};

#endif // WAVEFORMTAB_H
