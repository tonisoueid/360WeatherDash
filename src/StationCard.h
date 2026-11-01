#pragma once

#include "StationData.h"

#include <QWidget>

// One watched site: city name with its site code, day/night indicator and
// local clock, temperature readout, condition glyph, wind compass, metric
// rows (feels/humidity/pressure/precip), 12-hour temperature sparkline, and
// a data-age/status footer. Repaints on data updates and the minute tick.
// Pending sites render an explicit dashed placeholder — no invented values.
class StationCard : public QWidget
{
    Q_OBJECT
public:
    explicit StationCard(int index, QWidget *parent = nullptr);

    void setData(const StationData &d);
    void setSelected(bool sel);
    int index() const { return m_index; }

signals:
    void clicked(int index);

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
    QSize minimumSizeHint() const override { return {384, 288}; }

private:
    int m_index;
    StationData m_data;
    bool m_live = false;
    bool m_selected = false;
    bool m_hover = false;

    void drawSparkline(QPainter &p, const QRectF &r);
    void drawCompass(QPainter &p, const QPointF &ctr, double radius);
    void drawTempGauge(QPainter &p, const QRectF &r, double tempC);
};
