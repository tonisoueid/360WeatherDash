#pragma once

#include "../StationData.h"

#include <QWidget>

// Oscilloscope-style trace: trailing 12 h of temperature (dim), next 36 h
// (bright phosphor with glow), precipitation probability as blue histogram
// bars from the baseline, graticule every 6 h, and a "NOW" cursor line.
// Beneath it a 3-day strip — YESTERDAY / TODAY / TOMORROW with calendar
// dates — each day casting a contrasty translucent zone up through the
// trace area to the top of the graph.
class TrendChart : public QWidget
{
    Q_OBJECT
public:
    explicit TrendChart(QWidget *parent = nullptr);
    void setData(const StationData &d);

protected:
    void paintEvent(QPaintEvent *) override;
    QSize minimumSizeHint() const override { return {420, 420}; }

private:
    StationData m_data;
    bool m_hasData = false;
    QRectF m_plot;        // trace area, cached at paint time
    QRectF m_strip;       // daily strip area beneath the trace
    double m_yMin = 0, m_yMax = 1;
    int m_i0 = 0, m_i1 = 0;   // visible hourly index window [i0, i1)
    int m_todayIdx = -1;      // daily[] slot whose date == station-local today
    int m_dayWin0 = -1, m_dayWin1 = -1;  // daily[] window: yesterday..tomorrow

    void drawTrace(QPainter &p);
    void drawDayZones(QPainter &p);
    void drawDailyStrip(QPainter &p);
    double xForIndex(int i) const;
    double yForTemp(double t) const;
};
