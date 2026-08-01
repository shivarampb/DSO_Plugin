/**
 * @file    WaveformPlot.h
 * @brief   Custom QWidget that paints a captured waveform over a graticule.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#ifndef WAVEFORMPLOT_H
#define WAVEFORMPLOT_H

#include <QWidget>
#include <QVector>

#include "ScopeTypes.h"

class WaveformPlot : public QWidget
{
    Q_OBJECT
  public:
    explicit WaveformPlot(QWidget* in_pParent = nullptr);

    void setWaveform(const S_Scope_Waveform& in_sWaveform);
    void clearWaveform();

  protected:
    void paintEvent(QPaintEvent* in_pEvent) override;

  private:
    QVector<FDOUBLE> m_vecTime;
    QVector<FDOUBLE> m_vecVolts;
    bool m_bHave;
};

#endif // WAVEFORMPLOT_H
