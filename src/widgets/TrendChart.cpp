#include "TrendChart.h"
#include "Theme.h"
#include "WeatherGlyph.h"

#include <QPainter>

#include <algorithm>
#include <cmath>

TrendChart::TrendChart(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(false);
    setMinimumHeight(420);
}

void TrendChart::setData(const StationData &d)
{
    m_data = d;
    m_hasData = d.valid() && d.hourly.timeIso.size() > 1;
    update();
}

double TrendChart::xForIndex(int i) const
{
    return m_plot.left() + (double(i - m_i0) / double(qMax(1, m_i1 - m_i0 - 1))) * m_plot.width();
}

double TrendChart::yForTemp(double t) const
{
    const double span = qMax(1.0, m_yMax - m_yMin);
    return m_plot.bottom() - ((t - m_yMin) / span) * m_plot.height();
}

void TrendChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (!m_hasData) {
        p.setFont(Theme::sans(14, false, true));
        p.setPen(Theme::TextFaint);
        p.drawText(rect(), Qt::AlignCenter,
                   QStringLiteral("NO TRACE — SELECT A LIVE STATION"));
        return;
    }

    // ---- plot geometry: trace on top (stretch), daily strip below
    const int padL = 50, padR = 36, padT = 36, padB = 22;
    const int stripGap = 34, stripH = 106;
    m_plot = QRectF(rect().left() + padL, rect().top() + padT,
                    rect().width() - padL - padR,
                    rect().height() - padT - padB - stripGap - stripH);
    m_strip = QRectF(m_plot.left(), m_plot.bottom() + stripGap, m_plot.width(), stripH);

    // ---- locate the station-local calendar day in the daily feed, then the
    // YESTERDAY..TOMORROW window around it (the feed already includes one
    // past day, so slot names must follow dates, never array order)
    m_todayIdx = -1;
    const QString todayIso = m_data.current.timeIso.left(10);
    for (int i = 0; i < m_data.daily.size(); ++i) {
        if (m_data.daily[i].dateIso.left(10) == todayIso) {
            m_todayIdx = i;
            break;
        }
    }
    m_dayWin0 = m_dayWin1 = -1;
    if (m_todayIdx >= 0) {
        m_dayWin0 = qMax(0, m_todayIdx - 1);
        m_dayWin1 = qMin(m_data.daily.size(), m_todayIdx + 2);
    }

    const HourlySeries &h = m_data.hourly;
    m_i0 = qMax(0, h.nowIndex - 12);
    m_i1 = qMin(h.timeIso.size(), h.nowIndex + 36);
    if (m_i1 - m_i0 < 2)
        return;

    // ---- vertical range from the visible window, padded
    m_yMin = 1e9; m_yMax = -1e9;
    for (int i = m_i0; i < m_i1; ++i) {
        if (!std::isnan(h.tempC[i])) {
            m_yMin = std::min(m_yMin, h.tempC[i]);
            m_yMax = std::max(m_yMax, h.tempC[i]);
        }
    }
    if (m_yMin > m_yMax) return;
    m_yMin = std::floor(m_yMin) - 2.0;
    m_yMax = std::ceil(m_yMax) + 2.0;

    drawDayZones(p);
    drawTrace(p);
    drawDailyStrip(p);
}

// one contrasty domain hue per day column: yesterday blue, today amber,
// tomorrow violet
static QColor zoneHue(int k)
{
    static const QColor hues[3] = {Theme::Blue, Theme::Amber, Theme::Purple};
    return hues[qBound(0, k, 2)];
}

// Each strip day casts a translucent rectangle in its hue from the top of
// the trace area down through its strip cell, so the three days read as
// distinct bands over the graph. Drawn before the trace so it stays on top.
void TrendChart::drawDayZones(QPainter &p)
{
    if (m_dayWin0 < 0 || m_dayWin1 <= m_dayWin0)
        return;
    const int n = m_dayWin1 - m_dayWin0;
    const double cellW = m_strip.width() / n;
    for (int k = 0; k < n; ++k) {
        const QColor hue = zoneHue(k);
        const double x = m_strip.left() + k * cellW;
        // one continuous shape per day: graph top → bottom of the strip cell
        QColor tint = hue; tint.setAlphaF(0.08);
        p.setPen(Qt::NoPen);
        p.setBrush(tint);
        p.drawRect(QRectF(x, m_plot.top(), cellW, m_strip.bottom() - m_plot.top()));
        QColor stripTint = hue; stripTint.setAlphaF(0.05);
        p.setBrush(stripTint);
        p.drawRect(QRectF(x, m_strip.top(), cellW, m_strip.height()));
        if (k > 0) {
            QColor ln = hue; ln.setAlphaF(0.40);
            p.setPen(QPen(ln, 1));
            p.setBrush(Qt::NoBrush);
            p.drawLine(QLineF(x, m_plot.top(), x, m_strip.bottom()));
        }
    }
}

void TrendChart::drawTrace(QPainter &p)
{
    const HourlySeries &h = m_data.hourly;

    // graticule: solid hairlines like the reference chart, labels muted
    p.setFont(Theme::sans(12));
    for (int k = 0; k <= 5; ++k) {
        const double t = m_yMin + (m_yMax - m_yMin) * k / 5.0;
        const double y = yForTemp(t);
        p.setPen(QPen(Theme::Grid, 1));
        p.drawLine(QLineF(m_plot.left(), y, m_plot.right(), y));
        p.setPen(Theme::TextMuted);
        p.drawText(QRectF(m_plot.left() - 46, y - 8, 42, 16),
                   Qt::AlignRight | Qt::AlignVCenter, QString::number(qRound(t)) + QChar(176));
    }
    for (int i = m_i0; i < m_i1; ++i) {
        const QString &iso = h.timeIso[i];
        const int hh = iso.mid(11, 2).toInt();
        if (hh % 6 != 0)
            continue;
        const double x = xForIndex(i);
        p.setPen(QPen(Theme::Grid, 1));
        p.drawLine(QLineF(x, m_plot.top(), x, m_plot.bottom()));
        p.setPen(Theme::TextMuted);
        p.drawText(QRectF(x - 24, m_plot.bottom() + 5, 48, 16),
                   Qt::AlignHCenter | Qt::AlignTop,
                   QStringLiteral("%1H").arg(hh, 2, 10, QChar('0')));
    }

    // precipitation probability histogram — blue, the domain accent
    const double barW = m_plot.width() / double(qMax(1, m_i1 - m_i0 - 1)) * 0.62;
    for (int i = m_i0; i < m_i1; ++i) {
        const double prob = h.precipProb.value(i, std::nan(""));
        if (std::isnan(prob) || prob <= 0)
            continue;
        const double bh = prob / 100.0 * m_plot.height() * 0.42;
        QRectF bar(xForIndex(i) - barW / 2, m_plot.bottom() - bh, barW, bh);
        QColor fill = Theme::Blue; fill.setAlphaF(0.32);
        p.fillRect(bar, fill);
        p.setPen(QPen(Theme::BlueBright, 1));
        p.drawLine(QLineF(bar.left(), bar.top(), bar.right(), bar.top()));
    }

    // 50 % probability reference line with its own scale label
    {
        const double refY = m_plot.bottom() - 0.5 * 0.42 * m_plot.height();
        QColor rc = Theme::Blue; rc.setAlphaF(0.45);
        QPen rp(rc, 1, Qt::DashLine);
        p.setPen(rp);
        p.drawLine(QLineF(m_plot.left(), refY, m_plot.right(), refY));
        p.setFont(Theme::sans(11));
        p.setPen(Theme::Blue);
        p.drawText(QRectF(m_plot.right() + 4, refY - 9, 34, 18),
                   Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("50%"));
    }

    // temperature trace with soft area fill; history gray, forecast orange
    auto buildPath = [&](int from, int to) {
        QPainterPath path;
        bool started = false;
        for (int i = from; i < to; ++i) {
            const double t = h.tempC.value(i, std::nan(""));
            if (std::isnan(t)) { started = false; continue; }
            const QPointF pt(xForIndex(i), yForTemp(t));
            if (!started) { path.moveTo(pt); started = true; }
            else path.lineTo(pt);
        }
        return path;
    };

    // area under the forecast section, tinted with the window's mean-temp
    // ramp hue so it follows the trace
    {
        QPainterPath line = buildPath(qMax(m_i0, h.nowIndex), m_i1);
        if (!line.isEmpty()) {
            double sum = 0; int cnt = 0;
            for (int i = qMax(m_i0, h.nowIndex); i < m_i1; ++i) {
                const double t = h.tempC.value(i, std::nan(""));
                if (!std::isnan(t)) { sum += t; ++cnt; }
            }
            QPainterPath area(line);
            area.lineTo(line.currentPosition().x(), m_plot.bottom());
            area.lineTo(xForIndex(qMax(m_i0, h.nowIndex)), m_plot.bottom());
            area.closeSubpath();
            QLinearGradient grad(QPointF(0, m_plot.top()), QPointF(0, m_plot.bottom()));
            QColor top = (cnt ? Theme::tempColor(sum / cnt) : Theme::Orange); top.setAlphaF(0.22);
            QColor bot = top; bot.setAlphaF(0.0);
            grad.setColorAt(0.0, top);
            grad.setColorAt(1.0, bot);
            p.setPen(Qt::NoPen);
            p.setBrush(grad);
            p.drawPath(area);
        }
    }

    auto strokePath = [&](int from, int to, const QPen &pen) {
        QPainterPath path = buildPath(from, to);
        if (path.isEmpty()) return;
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    };
    QPen histPen(Theme::TextFaint, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    strokePath(m_i0, qMin(m_i1, h.nowIndex + 1), histPen);

    // forecast trace carries the cold→hot temperature ramp — the same
    // gradient encoding as the cards' sparklines and temp gauges — varying
    // along its length with each sample's temperature
    {
        const int f0 = qMax(m_i0, h.nowIndex);
        double x0 = 0, x1 = 0; bool any = false;
        for (int i = f0; i < m_i1; ++i) {
            const double t = h.tempC.value(i, std::nan(""));
            if (std::isnan(t)) continue;
            const double x = xForIndex(i);
            if (!any) { x0 = x; any = true; }
            x1 = x;
        }
        if (any) {
            QLinearGradient ramp(x0, 0, x1, 0);
            const double span = qMax(1.0, x1 - x0);
            for (int i = f0; i < m_i1; ++i) {
                const double t = h.tempC.value(i, std::nan(""));
                if (std::isnan(t)) continue;
                ramp.setColorAt(qBound(0.0, (xForIndex(i) - x0) / span, 1.0),
                                Theme::tempColor(t));
            }
            strokePath(f0, m_i1, QPen(QBrush(ramp), 2.2, Qt::SolidLine,
                                      Qt::RoundCap, Qt::RoundJoin));
        }
    }

    // sample dots on the forecast section
    p.setPen(Qt::NoPen);
    p.setBrush(Theme::Text);
    for (int i = h.nowIndex; i < m_i1; i += 6) {
        const double t = h.tempC.value(i, std::nan(""));
        if (std::isnan(t)) continue;
        p.drawEllipse(QPointF(xForIndex(i), yForTemp(t)), 2.2, 2.2);
    }

    // NOW cursor: dashed white line + chip-backed readout
    if (h.nowIndex >= m_i0 && h.nowIndex < m_i1) {
        const double x = xForIndex(h.nowIndex);
        QColor c = Theme::Text; c.setAlphaF(0.45);
        p.setPen(QPen(c, 1, Qt::DashLine));
        p.drawLine(QLineF(x, m_plot.top() - 6, x, m_plot.bottom()));
        const double t = h.tempC.value(h.nowIndex, std::nan(""));
        if (!std::isnan(t)) {
            p.setPen(Qt::NoPen);
            p.setBrush(Theme::tempColor(t));
            p.drawEllipse(QPointF(x, yForTemp(t)), 3.2, 3.2);
        }
        p.setFont(Theme::sans(11, true));
        const QString nowCap = QStringLiteral("NOW  ") + QString::number(t, 'f', 0) + QChar(176);
        const int capW = QFontMetrics(p.font()).horizontalAdvance(nowCap);
        const qreal capX = qBound(m_plot.left(), x + 5, m_plot.right() - capW - 8);
        p.fillRect(QRectF(capX, m_plot.top() - 7, capW + 8, 22), Theme::BgDeep);
        p.setPen(Theme::TextMuted);
        p.drawText(QPointF(capX + 4, m_plot.top() + 10), QStringLiteral("NOW"));
        p.setPen(std::isnan(t) ? Theme::TextMuted : Theme::tempColor(t));
        p.drawText(QPointF(capX + 4 + QFontMetrics(p.font()).horizontalAdvance(QStringLiteral("NOW  ")),
                           m_plot.top() + 10),
                   QString::number(t, 'f', 0) + QChar(176));
    }

    // min/max annotations on the visible window
    int iMin = -1, iMax = -1;
    for (int i = m_i0; i < m_i1; ++i) {
        const double t = h.tempC.value(i, std::nan(""));
        if (std::isnan(t)) continue;
        if (iMin < 0 || t < h.tempC[iMin]) iMin = i;
        if (iMax < 0 || t > h.tempC[iMax]) iMax = i;
    }
    p.setFont(Theme::sans(11, true));
    auto annotate = [&](int i, bool above, const QString &tag, const QColor &c) {
        const QPointF pt(xForIndex(i), yForTemp(h.tempC[i]));
        p.setPen(c);
        p.drawText(pt + QPointF(5, above ? -8 : 13),
                   QStringLiteral("%1 %2%3").arg(tag,
                       QString::number(h.tempC[i], 'f', 0), QChar(176)));
    };
    if (iMax >= 0) annotate(iMax, true, QStringLiteral("MAX"), Theme::Orange);
    if (iMin >= 0 && iMin != iMax) annotate(iMin, false, QStringLiteral("MIN"), Theme::Blue);

    // legend: the temp swatch carries the same ramp so the encoding is
    // self-describing; precip keeps its blue domain accent
    p.setFont(Theme::sans(11));
    const qreal lx = m_plot.left();
    QLinearGradient lg(lx, 0, lx + 22, 0);
    lg.setColorAt(0.0, Theme::tempColor(-10));
    lg.setColorAt(1.0, Theme::tempColor(35));
    QPen lgPen(QBrush(lg), 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(lgPen);
    p.drawLine(QPointF(lx + 1, 13), QPointF(lx + 23, 13));
    p.setPen(Theme::OrangeBright);
    p.drawText(QPointF(lx + 28, 17), QStringLiteral("TEMP °C"));
    p.setPen(Theme::Blue);
    p.drawText(QPointF(lx + 104, 17), QStringLiteral("▮ PRECIP PROB %"));
}

void TrendChart::drawDailyStrip(QPainter &p)
{
    if (m_dayWin0 < 0 || m_dayWin1 <= m_dayWin0)
        return;
    const QVector<DailyEntry> &days = m_data.daily;

    // shared min/max for the range bars across the strip
    double lo = 1e9, hi = -1e9;
    for (int i = m_dayWin0; i < m_dayWin1; ++i) {
        if (!std::isnan(days[i].tMinC)) lo = std::min(lo, days[i].tMinC);
        if (!std::isnan(days[i].tMaxC)) hi = std::max(hi, days[i].tMaxC);
    }
    if (lo > hi)
        return;
    const double span = qMax(1.0, hi - lo);

    const int n = m_dayWin1 - m_dayWin0;
    const double cellW = m_strip.width() / n;

    for (int k = 0; k < n; ++k) {
        const int i = m_dayWin0 + k;
        const DailyEntry &e = days[i];
        const double x = m_strip.left() + k * cellW;
        const double top = m_strip.top();
        const QDate d = QDate::fromString(e.dateIso.left(10), "yyyy-MM-dd");

        // day label with its calendar date, e.g. YESTERDAY (24-10), in the
        // same hue as the day's zone so the column reads as one unit
        const char *name = (i == m_todayIdx) ? "TODAY"
                          : (i < m_todayIdx) ? "YESTERDAY" : "TOMORROW";
        const QString label = QStringLiteral("%1 (%2)")
                                  .arg(QString::fromLatin1(name),
                                       d.isValid() ? d.toString("dd-MM")
                                                   : QStringLiteral("--"));
        p.setFont(Theme::sans(13, true));
        p.setPen(zoneHue(k));
        p.drawText(QPointF(x + 8, top + 16), label);

        // condition glyph (embedded open-source icon set)
        const WxInfo wx = decodeWmo(e.weatherCode);
        GlyphPaint::draw(p, wx.glyph, QRectF(x + 8, top + 24, 34, 32), true);

        // hi / lo, stacked and larger
        p.setFont(Theme::sans(14, true));
        p.setPen(Theme::Text);
        p.drawText(QPointF(x + 50, top + 38),
                   QString("%1%2").arg(QString::number(e.tMaxC, 'f', 0), QChar(176)));
        p.setPen(Theme::TextMuted);
        p.drawText(QPointF(x + 50, top + 57),
                   QString("%1%2").arg(QString::number(e.tMinC, 'f', 0), QChar(176)));

        // vertical min—max range bar with cold→hot gradient
        const QRectF barBox(x + cellW - 26, top + 22, 10, 46);
        const double yLo = barBox.bottom() - ((e.tMinC - lo) / span) * barBox.height();
        const double yHi = barBox.bottom() - ((e.tMaxC - lo) / span) * barBox.height();
        QLinearGradient barGrad(QPointF(0, yHi), QPointF(0, yLo));
        barGrad.setColorAt(0.0, Theme::tempColor(e.tMaxC));
        barGrad.setColorAt(1.0, Theme::tempColor(e.tMinC));
        p.setPen(Qt::NoPen);
        p.setBrush(barGrad);
        p.drawRoundedRect(QRectF(barBox.left(), yHi, barBox.width(), qMax(3.0, yLo - yHi)), 2, 2);

        // rain probability + max wind, on their own roomy row
        p.setFont(Theme::sans(12, true));
        if (e.precipProbMax >= 0) {
            p.setPen(Theme::Blue);
            p.drawText(QPointF(x + 8, top + 82),
                       QStringLiteral("R %1%").arg(e.precipProbMax));
        }
        if (!std::isnan(e.windMaxKmh)) {
            p.setPen(Theme::TextMuted);
            p.drawText(QPointF(x + 68, top + 82),
                       QStringLiteral("W %1").arg(qRound(e.windMaxKmh)));
        }

        // sunrise / sunset
        p.setPen(Theme::TextMuted);
        p.drawText(QPointF(x + 8, top + 101),
                   QStringLiteral("↑%1 ↓%2")
                       .arg(e.sunriseIso.right(5), e.sunsetIso.right(5)));
    }
}
