/*============================================================================
 *  WaveformPlot.cpp - graticule + trace painting.
 *==========================================================================*/
#include "WaveformPlot.h"

#include <QPainter>
#include <QPaintEvent>
#include <algorithm>
#include <cmath>

WaveformPlot::WaveformPlot(QWidget* in_pParent) : QWidget(in_pParent), m_bHave(false)
{
    setMinimumSize(420, 260);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(10, 12, 14));
    setAutoFillBackground(true);
    setPalette(pal);
}

void WaveformPlot::setWaveform(const S_Scope_Waveform& in_sWaveform)
{
    m_vecTime = in_sWaveform.m_vecTimeSeconds;
    m_vecVolts = in_sWaveform.m_vecVolts;
    m_bHave = !m_vecVolts.isEmpty();
    update();
}

void WaveformPlot::clearWaveform()
{
    m_vecTime.clear();
    m_vecVolts.clear();
    m_bHave = false;
    update();
}

void WaveformPlot::paintEvent(QPaintEvent*)
{
    QPainter g(this);
    g.fillRect(rect(), QColor(10, 12, 14));

    const int W = width(), H = height();
    const int DIVX = 10, DIVY = 8;

    // graticule
    g.setPen(QPen(QColor(40, 60, 50), 1));
    for (int i = 1; i < DIVX; ++i)
    {
        const int x = i * W / DIVX;
        g.drawLine(x, 0, x, H);
    }
    for (int i = 1; i < DIVY; ++i)
    {
        const int y = i * H / DIVY;
        g.drawLine(0, y, W, y);
    }
    // center axes brighter
    g.setPen(QPen(QColor(70, 100, 80), 1));
    g.drawLine(W / 2, 0, W / 2, H);
    g.drawLine(0, H / 2, W, H / 2);

    if (!m_bHave || m_vecVolts.size() < 2)
    {
        g.setPen(QColor(120, 160, 140));
        g.drawText(rect(), Qt::AlignCenter, QStringLiteral("no waveform captured"));
        return;
    }

    // autoscale vertically to the data with a small margin
    double vmin = m_vecVolts[0], vmax = m_vecVolts[0];
    for (double v : m_vecVolts)
    {
        vmin = std::min(vmin, v);
        vmax = std::max(vmax, v);
    }
    double span = vmax - vmin;
    if (span < 1e-12)
    {
        span = 1.0;
    }
    vmin -= 0.1 * span;
    vmax += 0.1 * span;
    const double range = vmax - vmin;

    const int n = m_vecVolts.size();
    QPolygonF poly;
    poly.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        const double x = static_cast<double>(i) / (n - 1) * (W - 1);
        const double y = (1.0 - (m_vecVolts[i] - vmin) / range) * (H - 1);
        poly << QPointF(x, y);
    }
    g.setRenderHint(QPainter::Antialiasing, true);
    g.setPen(QPen(QColor(120, 220, 120), 1.6));
    g.drawPolyline(poly);

    // readout
    g.setPen(QColor(180, 220, 180));
    g.drawText(6, 16, QStringLiteral("Vpp %1 V   pts %2").arg(vmax - vmin - 0.2 * span, 0, 'g', 4).arg(n));
}
