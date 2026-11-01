#include "StationCard.h"
#include "Theme.h"
#include "widgets/WeatherGlyph.h"

#include <QDateTime>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <cmath>

StationCard::StationCard(int index, QWidget *parent)
    : QWidget(parent), m_index(index)
{
    setAutoFillBackground(false);
    setMouseTracking(false);
    setCursor(Qt::PointingHandCursor);
}

void StationCard::setData(const StationData &d)
{
    m_data = d;
    m_live = d.valid();
    update();
}

void StationCard::setSelected(bool sel)
{
    m_selected = sel;
    update();
}

void StationCard::mouseReleaseEvent(QMouseEvent *e)
{
    if (rect().contains(e->pos()))
        emit clicked(m_index);
    QWidget::mouseReleaseEvent(e);
}

void StationCard::enterEvent(QEnterEvent *) { m_hover = true; update(); }
void StationCard::leaveEvent(QEvent *) { m_hover = false; update(); }

void StationCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // chassis — blue halo when selected, like the reference's focused card
    if (m_selected) {
        QColor halo = Theme::Blue; halo.setAlphaF(0.35);
        p.setPen(QPen(halo, 4));
        p.setBrush(Theme::CardBg);
        p.drawRoundedRect(rect().adjusted(2, 2, -3, -3), 10, 10);
        p.setPen(QPen(Theme::Blue, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(rect().adjusted(2, 2, -3, -3), 10, 10);
    } else {
        Theme::panelBody(p, rect(), 10, Theme::CardBg,
                         m_hover ? Theme::BorderHi : Theme::Border);
    }

    const Station &st = m_data.station;

    // ---- row 0: city name (white, headline-sized) with the site code in
    // parentheses after it | sun/moon indicator + clock
    const QFont nameFont = Theme::sans(16, true);
    p.setFont(nameFont);
    p.setPen(Theme::Text);
    p.drawText(QPoint(14, 25), st.name);
    const int nameW = QFontMetrics(nameFont).horizontalAdvance(st.name);
    p.setFont(Theme::sans(12, true));
    p.setPen(Theme::TextMuted);
    p.drawText(QPoint(14 + nameW + 7, 25),
               QStringLiteral("(%1)").arg(st.code));

    if (m_live) {
        const bool day = m_data.current.isDay > 0;
        if (day) {
            const QPointF dotCtr(width() - 76, 20);
            p.setPen(Qt::NoPen);
            p.setBrush(Theme::Amber);
            p.drawEllipse(dotCtr, 3.5, 3.5);
            p.setPen(QPen(Theme::Amber, 1));
            for (int i = 0; i < 8; ++i) {
                const double a = i * M_PI / 4.0;
                p.drawLine(QLineF(dotCtr + QPointF(cos(a) * 5.5, sin(a) * 5.5),
                                  dotCtr + QPointF(cos(a) * 7.5, sin(a) * 7.5)));
            }
        } else {
            // night reads as a crescent moon, not a plain colored dot
            GlyphPaint::draw(p, Glyph::Moon, QRectF(width() - 88, 10, 18, 20), false);
        }
        p.setFont(Theme::sans(13, true));
        p.setPen(Theme::TextSoft);
        p.drawText(QPoint(width() - 68, 25), m_data.localTimeStr());
    } else {
        p.setFont(Theme::sans(10, true, true));
        p.setPen(Theme::Red);
        p.drawText(QPoint(width() - 86, 25), QStringLiteral("OFFLINE"));
        p.setBrush(Theme::Red);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(width() - 92, 21), 3, 3);
    }

    if (!m_live) {
        p.setFont(Theme::sans(14, true, true));
        p.setPen(Theme::TextMuted);
        p.drawText(rect().adjusted(0, -20, 0, 0), Qt::AlignCenter,
                   m_data.state == StationData::State::Pending
                       ? QStringLiteral("AWAITING FIRST SCAN")
                       : QStringLiteral("LINK ERROR"));
        if (!m_data.error.isEmpty()) {
            p.setFont(Theme::sans(11));
            p.setPen(Theme::Red);
            QString msg = m_data.error;
            if (msg.size() > 46) msg = msg.left(43) + "...";
            p.drawText(rect().adjusted(0, 12, 0, 0), Qt::AlignCenter, msg);
        }
        p.setPen(QPen(Theme::Grid, 1, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(14, height() - 44, width() - 28, 26), 5, 5);
        p.setFont(Theme::sans(11));
        p.setPen(Theme::TextFaint);
        p.drawText(QRectF(14, height() - 48, width() - 28, 30), Qt::AlignCenter,
                   QStringLiteral("NO DATA — GAP SHOWN, NOT FAKED"));
        return;
    }

    const CurrentObs &c = m_data.current;
    const WxInfo wx = decodeWmo(c.weatherCode);

    // ---- temperature readout, sized no larger than the site name and
    // colored on the cold-blue → hot-red ramp
    const QFont bigFont = Theme::sans(15, true);
    QString tempStr = QStringLiteral("--");
    if (!std::isnan(c.tempC))
        tempStr = QStringLiteral("%1%2").arg(QString::number(c.tempC, 'f', 1), QChar(176));
    p.setFont(bigFont);
    p.setPen(std::isnan(c.tempC) ? Theme::TextMuted : Theme::tempColor(c.tempC));
    p.drawText(QPoint(14, 78), tempStr);

    // ---- condition chip + glyph (embedded Bootstrap Icons) share the big
    // readout's line, right-aligned to the card edge
    const QFont chipFont = Theme::sans(11, true);
    const int chipW = QFontMetrics(chipFont).horizontalAdvance(wx.text) + 20;
    Theme::chip(p, QRectF(width() - 62 - chipW, 60, chipW, 24), wx.text,
                Theme::condColor(c.weatherCode), chipFont);
    GlyphPaint::draw(p, wx.glyph, QRectF(width() - 54, 56, 40, 32), c.isDay > 0);

    // ---- segmented temperature gauge, right column below the condition
    // row (reference CPU/temp stacks)
    drawTempGauge(p, QRectF(width() - 52, 96, 13, 100), c.tempC);

    // ---- wind compass, left column
    drawCompass(p, QPointF(38, 148), 19);

    // ---- metrics: white labels, values in domain colors; the 2-column
    // block is centered horizontally, kept clear of the compass
    const QFont labelFont = Theme::sans(11);
    const QFont vFont = Theme::sans(12, true);
    const QFontMetrics vFm(vFont);
    struct Row { const char *label; QString val; QColor c; bool segBar; QColor barC; };
    const QString feels = std::isnan(c.feelsC) ? QStringLiteral("---")
        : QStringLiteral("%1°").arg(c.feelsC, 0, 'f', 0);
    const QString hum = c.humidityPct < 0 ? QStringLiteral("---")
        : QStringLiteral("%1%").arg(c.humidityPct);
    QString press = std::isnan(c.pressHpa) ? QStringLiteral("---")
        : QStringLiteral("%1 HPA").arg(qRound(c.pressHpa));
    QColor pressC = Theme::Green;
    const double pt = m_data.pressureTrend3h();
    if (!std::isnan(pt)) {
        if (pt > 1.5)      { press += QStringLiteral(" ↑"); }
        else if (pt < -1.5){ press += QStringLiteral(" ↓"); pressC = Theme::Red; }
        else               { press += QStringLiteral(" →"); pressC = Theme::TextMuted; }
    }
    const QString precip = std::isnan(c.precipMm) ? QStringLiteral("---")
        : QStringLiteral("%1 MM").arg(c.precipMm, 0, 'f', 1);
    const QString wind = std::isnan(c.windKmh) ? QStringLiteral("---")
        : QStringLiteral("%1 %2").arg(QString::number(c.windKmh, 'f', 0), compassPoint(c.windDeg));
    const QString gust = std::isnan(c.gustKmh) ? QStringLiteral("---")
        : QStringLiteral("%1 KM/H").arg(QString::number(c.gustKmh, 'f', 0));
    const QVector<Row> rows = {
        {"WIND",   wind,   Theme::BlueBright,   false, QColor()},
        {"GUST",   gust,   Theme::Blue,         false, QColor()},
        {"FEELS",  feels,  Theme::TextSoft,     false, QColor()},
        {"RH",     hum,    Theme::PurpleBright, true,  Theme::Purple},
        {"QNH",    press,  pressC,              false, QColor()},
        {"PRECIP", precip, Theme::Cyan,         false, QColor()},
    };
    // measure the block from its widest values so centering is true, and
    // never slide it left of the compass ring
    int colValW[2] = {0, 0};
    for (int i = 0; i < rows.size(); ++i) {
        int w = vFm.horizontalAdvance(rows[i].val);
        if (rows[i].segBar && c.humidityPct >= 0)
            w += 7 + 5 * 6;   // trailing humidity segment blocks
        colValW[i / 3] = qMax(colValW[i / 3], w);
    }
    const int blockW = 132 + 54 + colValW[1];   // column pitch + labels + widest value
    const int x0 = qMax((width() - blockW) / 2, 80);
    for (int i = 0; i < rows.size(); ++i) {
        const int col = i / 3;
        const int row = i % 3;
        const int x = x0 + col * 132;
        const int y = 136 + row * 20;
        p.setFont(labelFont);
        p.setPen(Theme::Text);
        p.drawText(QPoint(x, y), rows[i].label);
        p.setFont(vFont);
        p.setPen(rows[i].c);
        p.drawText(QPoint(x + 54, y), rows[i].val);
        if (rows[i].segBar && c.humidityPct >= 0) {
            // segmented blocks like the reference's stacked gauges
            const int blocks = 5, on = c.humidityPct / 20;
            int bx = x + 54 + vFm.horizontalAdvance(rows[i].val) + 7;
            for (int b = 0; b < blocks; ++b) {
                p.setPen(Qt::NoPen);
                p.setBrush((b < on) ? rows[i].barC : Theme::Grid);
                p.drawRoundedRect(QRectF(bx, y - 8, 5, 4), 1, 1);
                bx += 6;
            }
        }
    }

    // ---- sparkline with gradient area fill
    const QRectF spark(14, height() - 80, width() - 28, 50);
    drawSparkline(p, spark);

    // ---- footer: state + data age + timezone; age is minute-grained and
    // the card only repaints on the minute tick (per-second was overload)
    p.setFont(Theme::sans(11));
    const int age = m_data.ageSecs();
    const QString ageStr = (age < 0) ? QStringLiteral("--")
        : (age < 60) ? QStringLiteral("<1 MIN")
                     : QStringLiteral("%1 MIN").arg(age / 60);
    QString footer;
    QColor footC = Theme::TextMuted;
    if (m_data.state == StationData::State::Live) {
        footer = QStringLiteral("LIVE · LAST SAMPLE %1 AGO · %2").arg(ageStr)
                     .arg(m_data.tzName.isEmpty() ? QStringLiteral("UTC") : m_data.tzName);
    } else {
        footer = QStringLiteral("STALE · LAST SAMPLE %1 AGO · RETAINED").arg(ageStr);
        footC = Theme::Amber;
    }
    p.setPen(footC);
    p.drawText(QPoint(14, height() - 11), footer);
}

// Vertical segmented stack: cold (bottom, blue) → hot (top, red); filled up
// to the current temperature with the ramp gradient, unfilled segments dark.
void StationCard::drawTempGauge(QPainter &p, const QRectF &r, double tempC)
{
    const int n = 12;
    const double gap = 2.0;
    const double segH = (r.height() - gap * (n - 1)) / n;
    constexpr double lo = -15.0, hi = 40.0;

    int markerSeg = -1;
    if (!std::isnan(tempC)) {
        const double frac = qBound(0.0, (tempC - lo) / (hi - lo), 1.0);
        markerSeg = qMin(n - 1, int(frac * n));
    }

    for (int i = 0; i < n; ++i) {
        // segment 0 at the top = hottest
        const double y = r.top() + i * (segH + gap);
        const bool filled = markerSeg >= 0 && i >= (n - 1 - markerSeg);
        p.setPen(Qt::NoPen);
        if (filled) {
            QColor c = Theme::gaugeRamp(n - 1 - i, n);
            if (i == n - 1 - markerSeg)
                c = c.lighter(130);   // hot tip reads brighter
            p.setBrush(c);
        } else {
            p.setBrush(Theme::GridSoft);
        }
        p.drawRoundedRect(QRectF(r.left(), y, r.width(), segH), 1.5, 1.5);
    }

    // meaningful scale: ticks at the top, +20°, freezing and the bottom
    p.setFont(Theme::sans(11));
    const double tickTemps[4] = {hi, 20.0, 0.0, lo};
    for (double t : tickTemps) {
        const double y = qBound(r.top(), r.bottom() - (t - lo) / (hi - lo) * r.height(), r.bottom());
        p.setPen(QPen(Theme::BorderHi, 1));
        p.drawLine(QLineF(r.right() + 1, y, r.right() + 5, y));
        p.setPen(Theme::TextMuted);
        p.drawText(QRectF(r.right() + 7, y - 9, 44, 18), Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(t, 'f', 0) + QChar(176));
    }

    // current reading on the left of the stack, white with a leader tick
    if (!std::isnan(tempC)) {
        const double frac = qBound(0.0, (tempC - lo) / (hi - lo), 1.0);
        const double my = qBound(r.top(), r.bottom() - frac * r.height(), r.bottom());
        p.setPen(QPen(Theme::Text, 1));
        p.drawLine(QLineF(r.left() - 5, my, r.left() - 1, my));
        p.setFont(Theme::sans(11, true));
        p.setPen(Theme::Text);
        p.drawText(QRectF(r.left() - 46, my - 9, 40, 18), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(tempC, 'f', 0) + QChar(176));
    }
}

void StationCard::drawSparkline(QPainter &p, const QRectF &r)
{
    const HourlySeries &h = m_data.hourly;
    QVector<QPointF> pts;
    double lo = 1e9, hi = -1e9;
    const int i0 = qMax(0, h.nowIndex);
    const int i1 = qMin(h.tempC.size(), i0 + 13);
    for (int i = i0; i < i1; ++i) {
        const double t = h.tempC[i];
        if (std::isnan(t)) continue;
        lo = std::min(lo, t); hi = std::max(hi, t);
    }
    if (lo > hi) {
        p.setFont(Theme::sans(9));
        p.setPen(Theme::TextFaint);
        p.drawText(r, Qt::AlignCenter, QStringLiteral("---"));
        return;
    }
    const double span = qMax(1.0, hi - lo);

    // header band: white tag on the left, HI readout on the right, kept
    // clear of the trace so the label breathes above and below
    const QRectF band(r.left(), r.top(), r.width(), 18);
    p.fillRect(band, Theme::CardBg);
    p.setFont(Theme::sans(11, true));
    p.setPen(Theme::Text);
    p.drawText(band.adjusted(2, 0, -80, 0), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("NEXT 12 H"));

    // trace lives strictly below the band
    const QRectF plot(r.left(), r.top() + 20, r.width(), r.height() - 20);
    const int n = qMax(1, i1 - i0 - 1);
    QVector<double> ptT;   // temps parallel to pts, for the ramp gradient
    for (int i = i0; i < i1; ++i) {
        const double t = h.tempC[i];
        if (std::isnan(t)) continue;
        const double x = plot.left() + 4 + (double(i - i0) / n) * (plot.width() - 10);
        const double y = plot.bottom() - 4 - ((t - lo) / span) * (plot.height() - 12);
        pts.push_back(QPointF(x, y));
        ptT.push_back(t);
    }
    if (pts.size() < 2)
        return;

    // the trace carries the cold→hot temperature ramp — the same gradient
    // as the card's temp gauge and the trend chart's range bars — varying
    // along its length with each sample's temperature

    // gradient area under the trace (ramp hue fading out), reference-style
    QPainterPath area;
    area.moveTo(pts.first().x(), plot.bottom());
    for (const QPointF &pt : pts) area.lineTo(pt);
    area.lineTo(pts.last().x(), plot.bottom());
    area.closeSubpath();
    QLinearGradient grad(QPointF(0, plot.top()), QPointF(0, plot.bottom()));
    QColor top = Theme::tempColor((lo + hi) / 2.0); top.setAlphaF(0.38);
    QColor bot = Theme::tempColor((lo + hi) / 2.0); bot.setAlphaF(0.02);
    grad.setColorAt(0.0, top);
    grad.setColorAt(1.0, bot);
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawPath(area);

    QLinearGradient lineGrad(pts.first().x(), 0, pts.last().x(), 0);
    const double xSpan = qMax(1.0, pts.last().x() - pts.first().x());
    for (int i = 0; i < pts.size(); ++i)
        lineGrad.setColorAt(qBound(0.0, (pts[i].x() - pts.first().x()) / xSpan, 1.0),
                            Theme::tempColor(ptT[i]));
    QPen linePen(QBrush(lineGrad), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(linePen);
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(pts[0]);
    for (int i = 1; i < pts.size(); ++i) path.lineTo(pts[i]);
    p.drawPath(path);

    // endpoint marker in the ramp color of the final sample
    p.setBrush(Theme::tempColor(ptT.last()));
    p.setPen(Qt::NoPen);
    p.drawEllipse(pts.last(), 2.2, 2.2);

    // explicit hi/lo readouts with words, not bare numbers; LO sits directly
    // under HI, both right-aligned to the same edge
    p.setFont(Theme::sans(11, true));
    auto chipText = [&](const QRectF &box, const QString &txt, Qt::Alignment a) {
        p.fillRect(box, Theme::CardBg);
        p.setPen(Theme::TextSoft);
        p.drawText(box, a | Qt::AlignVCenter, txt);
    };
    const QFontMetrics chipFm(p.font());
    const QString hiTxt = QStringLiteral("HI %1%2").arg(QString::number(hi, 'f', 0), QChar(176));
    const QString loTxt = QStringLiteral("LO %1%2").arg(QString::number(lo, 'f', 0), QChar(176));
    chipText(QRectF(band.right() - chipFm.horizontalAdvance(hiTxt) - 10, band.top() + 1,
                    chipFm.horizontalAdvance(hiTxt) + 8, 16), hiTxt, Qt::AlignRight);
    chipText(QRectF(band.right() - chipFm.horizontalAdvance(loTxt) - 10, band.top() + 20,
                    chipFm.horizontalAdvance(loTxt) + 8, 16), loTxt, Qt::AlignRight);
}

void StationCard::drawCompass(QPainter &p, const QPointF &ctr, double radius)
{
    p.setPen(QPen(Theme::Border, 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(ctr, radius, radius);

    p.setFont(Theme::sans(10));
    const char *card[4] = {"N", "E", "S", "W"};
    for (int i = 0; i < 4; ++i) {
        const double a = i * M_PI / 2.0 - M_PI / 2.0;   // N up, E right, S down, W left
        const QPointF d(cos(a), sin(a));
        p.setPen(QPen(Theme::BorderHi, 1));
        p.drawLine(QLineF(ctr + d * (radius - 4), ctr + d * radius));
        p.setPen(Theme::TextFaint);
        p.drawText(ctr + d * (radius + 6) + QPointF(-4, 3), QString::fromLatin1(card[i]));
    }

    const CurrentObs &c = m_data.current;
    if (c.windDeg < 0 || std::isnan(c.windKmh)) {
        p.setPen(Theme::TextFaint);
        p.drawText(QRectF(ctr.x() - radius + 2, ctr.y() - 7, 2 * radius - 4, 14),
                   Qt::AlignCenter, QStringLiteral("--"));
        return;
    }
    const double a = (c.windDeg - 90) * M_PI / 180.0;
    const QPointF d(cos(a), sin(a));
    QPen arrow(Theme::Blue, 2); arrow.setCapStyle(Qt::RoundCap);
    p.setPen(arrow);
    p.drawLine(QLineF(ctr + d * (radius - 3), ctr - d * (radius - 3)));
    p.setBrush(Theme::BlueBright);
    p.setPen(Qt::NoPen);
    QPolygonF head;
    head << ctr - d * (radius - 1)
         << ctr - d * (radius - 6) + QPointF(-d.y(), d.x()) * 3
         << ctr - d * (radius - 6) + QPointF(d.y(), -d.x()) * 3;
    p.drawPolygon(head);
    // speed readout in the ring center, backed by a card-colored chip so the
    // needle never crosses the digits
    p.setFont(Theme::sans(10, true));
    const QString spd = QString::number(c.windKmh, 'f', 0);
    const int spdW = QFontMetrics(p.font()).horizontalAdvance(spd);
    p.fillRect(QRectF(ctr.x() - spdW / 2 - 4, ctr.y() - 8, spdW + 8, 15), Theme::CardBg);
    p.setPen(Theme::Text);
    p.drawText(QRectF(ctr.x() - radius, ctr.y() - 8, 2 * radius, 15),
               Qt::AlignHCenter | Qt::AlignVCenter, spd);
}
