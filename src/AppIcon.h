#pragma once

#include "Theme.h"
#include "widgets/WeatherGlyph.h"

#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QTransform>

// ----------------------------------------------------------------------------
// Application icon, painted (not a bitmap asset, matching the rest of the
// console): a dark rounded tile carrying the three things the app is —
//   a globe (worldwide stations) · an amber sun-behind-cloud mark (weather)
//   · the segmented cold→hot temperature ring (the signature gauge ramp)
// All artwork geometry is verbatim Bootstrap Icons v1.11.3 path data: the
// globe below and Glyph::CloudSun via GlyphPaint, in the app palette.
// ----------------------------------------------------------------------------
namespace AppIcon {

// verbatim 'd' attribute of bootstrap-icons@1.11.3 globe.svg (16×16 viewBox)
inline const char *kGlobeData =
    "M0 8a8 8 0 1 1 16 0A8 8 0 0 1 0 8m7.5-6.923c-.67.204-1.335.82-1.887 1.855A8 8 0 0 0 "
    "5.145 4H7.5zM4.09 4a9.3 9.3 0 0 1 .64-1.539 7 7 0 0 1 .597-.933A7.03 7.03 0 0 0 "
    "2.255 4zm-.582 3.5c.03-.877.138-1.718.312-2.5H1.674a7 7 0 0 0-.656 2.5zM4.847 5a12.5 "
    "12.5 0 0 0-.338 2.5H7.5V5zM8.5 5v2.5h2.99a12.5 12.5 0 0 0-.337-2.5zM4.51 8.5a12.5 12.5 "
    "0 0 0 .337 2.5H7.5V8.5zm3.99 0V11h2.653c.187-.765.306-1.608.338-2.5zM5.145 12q.208.58.468 "
    "1.068c.552 1.035 1.218 1.65 1.887 1.855V12zm.182 2.472a7 7 0 0 1-.597-.933A9.3 9.3 0 0 1 "
    "4.09 12H2.255a7 7 0 0 0 3.072 2.472M3.82 11a13.7 13.7 0 0 1-.312-2.5h-2.49c.062.89.291 "
    "1.733.656 2.5zm6.853 3.472A7 7 0 0 0 13.745 12H11.91a9.3 9.3 0 0 1-.64 1.539 7 7 0 0 1-"
    ".597.933M8.5 12v2.923c.67-.204 1.335-.82 1.887-1.855q.26-.487.468-1.068zm3.68-1h2.146c.365-"
    ".767.594-1.61.656-2.5h-2.49a13.7 13.7 0 0 1-.312 2.5m2.802-3.5a7 7 0 0 0-.656-2.5H12.18c."
    "174.782.282 1.623.312 2.5zM11.27 2.461c.247.464.462.98.64 1.539h1.835a7 7 0 0 0-3.072-2.472"
    "c.218.284.418.598.597.933M10.855 4a8 8 0 0 0-.468-1.068C9.835 1.897 9.17 1.282 8.5 1.077V4z";

inline const QPainterPath &globePath()
{
    static const QPainterPath path = GlyphPaint::parse(kGlobeData);
    return path;
}

// Fill a path authored in the upstream 16×16 viewBox into a square of the
// given side, centered on the given point.
inline void fillGlyph(QPainter &p, const QPainterPath &path16,
                      const QPointF &center, double side, const QColor &color)
{
    QTransform t;
    t.translate(center.x() - side / 2.0, center.y() - side / 2.0);
    t.scale(side / 16.0, side / 16.0);
    p.fillPath(t.map(path16), color);
}

inline void paint(QPainter &p, int size)
{
    const double S = size;
    const QPointF c(S / 2.0, S / 2.0);

    // chassis: rounded dark tile, same family as the panel cards
    p.setPen(QPen(Theme::BorderHi, qMax(1.0, S / 128.0)));
    p.setBrush(Theme::PanelBg);
    p.drawRoundedRect(QRectF(0.5, 0.5, S - 1.0, S - 1.0), S * 0.225, S * 0.225);

    // segmented temperature ring — cold→hot like the station gauges, with a
    // gap at the bottom where the weather mark sits (coarser at tiny sizes
    // so the segments stay distinct)
    const int segs = S < 48 ? 16 : 24;
    const double spanEach = 300.0 / segs;
    const double r = S * 0.400;
    for (int i = 0; i < segs; ++i) {
        QPen pen(Theme::gaugeRamp(i, segs), qMax(1.2, S * 0.052));
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.drawArc(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r),
                  int((300.0 - i * spanEach - 2.5) * 16),
                  int(-(spanEach - 5.0) * 16));
    }

    // globe (worldwide stations) in the wind/world blue
    fillGlyph(p, globePath(), QPointF(S * 0.455, S * 0.425), S * 0.46, Theme::BlueBright);

    // sun behind a cloud (weather) in the condition amber, as on the cards
    fillGlyph(p, GlyphPaint::glyphPath(Glyph::CloudSun),
              QPointF(S * 0.630, S * 0.660), S * 0.38, Theme::Amber);
}

inline QPixmap pixmap(int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    paint(p, size);
    return pm;
}

inline QIcon icon()
{
    QIcon ic;
    for (int s : {16, 24, 32, 48, 64, 128, 256, 512})
        ic.addPixmap(pixmap(s));
    return ic;
}

} // namespace AppIcon
