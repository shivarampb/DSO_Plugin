/*============================================================================
 *  WaveformPlot.h - a custom QWidget that paints a captured waveform
 *  (volts vs. time) over an oscilloscope-style graticule. The signature
 *  scope view of the GuiTester.
 *==========================================================================*/
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
    bool             m_bHave;
};

#endif // WAVEFORMPLOT_H
