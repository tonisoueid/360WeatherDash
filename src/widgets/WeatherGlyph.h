#pragma once

#include "../Theme.h"
#include "StationData.h"

#include <QPainter>
#include <QPainterPath>

#include <cmath>
#include <cstdlib>

// ----------------------------------------------------------------------------
// Weather condition glyphs — Bootstrap Icons v1.11.3, © 2019-2024 The
// Bootstrap Authors (MIT license, https://icons.getbootstrap.com). The SVG
// path data below is embedded verbatim from the upstream icon files, so the
// artwork is byte-identical to the published open-source set rather than a
// hand-drawn approximation. A small deterministic parser converts it to a
// QPainterPath, filled with the theme's per-condition accent color.
// ----------------------------------------------------------------------------
namespace GlyphPaint {

// pick the accent color per glyph when the caller doesn't override
inline QColor baseColor(Glyph g)
{
    switch (g) {
    case Glyph::Sun:       return Theme::Amber;
    case Glyph::Moon:      return Theme::Blue;
    case Glyph::CloudSun:  return Theme::Amber;
    case Glyph::CloudMoon: return Theme::Blue;
    case Glyph::Rain:
    case Glyph::Drizzle:   return Theme::Blue;
    case Glyph::Snow:      return Theme::Cyan;
    case Glyph::Sleet:     return Theme::Cyan;
    case Glyph::Thunder:   return Theme::Purple;
    case Glyph::Fog:       return Theme::TextMuted;
    default:               return Theme::TextFaint;
    }
}

// Verbatim 'd' attributes from bootstrap-icons@1.11.3 (16×16 viewBox):
// sun-fill, moon-fill, cloud-sun-fill, cloud-moon-fill, cloud-fill,
// cloud-fog2-fill, cloud-drizzle-fill, cloud-rain-fill, cloud-snow-fill,
// cloud-sleet-fill, cloud-lightning-rain-fill.
inline const char *pathData(Glyph g)
{
    switch (g) {
    case Glyph::Sun: return R"BI(M8 12a4 4 0 1 0 0-8 4 4 0 0 0 0 8M8 0a.5.5 0 0 1 .5.5v2a.5.5 0 0 1-1 0v-2A.5.5 0 0 1 8 0m0 13a.5.5 0 0 1 .5.5v2a.5.5 0 0 1-1 0v-2A.5.5 0 0 1 8 13m8-5a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1 0-1h2a.5.5 0 0 1 .5.5M3 8a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1 0-1h2A.5.5 0 0 1 3 8m10.657-5.657a.5.5 0 0 1 0 .707l-1.414 1.415a.5.5 0 1 1-.707-.708l1.414-1.414a.5.5 0 0 1 .707 0m-9.193 9.193a.5.5 0 0 1 0 .707L3.05 13.657a.5.5 0 0 1-.707-.707l1.414-1.414a.5.5 0 0 1 .707 0m9.193 2.121a.5.5 0 0 1-.707 0l-1.414-1.414a.5.5 0 0 1 .707-.707l1.414 1.414a.5.5 0 0 1 0 .707M4.464 4.465a.5.5 0 0 1-.707 0L2.343 3.05a.5.5 0 1 1 .707-.707l1.414 1.414a.5.5 0 0 1 0 .708)BI";
    case Glyph::Moon: return R"BI(M6 .278a.77.77 0 0 1 .08.858 7.2 7.2 0 0 0-.878 3.46c0 4.021 3.278 7.277 7.318 7.277q.792-.001 1.533-.16a.79.79 0 0 1 .81.316.73.73 0 0 1-.031.893A8.35 8.35 0 0 1 8.344 16C3.734 16 0 12.286 0 7.71 0 4.266 2.114 1.312 5.124.06A.75.75 0 0 1 6 .278)BI";
    case Glyph::CloudSun: return R"BI(M11.473 11a4.5 4.5 0 0 0-8.72-.99A3 3 0 0 0 3 16h8.5a2.5 2.5 0 0 0 0-5z M10.5 1.5a.5.5 0 0 0-1 0v1a.5.5 0 0 0 1 0zm3.743 1.964a.5.5 0 1 0-.707-.707l-.708.707a.5.5 0 0 0 .708.708zm-7.779-.707a.5.5 0 0 0-.707.707l.707.708a.5.5 0 1 0 .708-.708zm1.734 3.374a2 2 0 1 1 3.296 2.198q.3.423.516.898a3 3 0 1 0-4.84-3.225q.529.017 1.028.129m4.484 4.074c.6.215 1.125.59 1.522 1.072a.5.5 0 0 0 .039-.742l-.707-.707a.5.5 0 0 0-.854.377M14.5 6.5a.5.5 0 0 0 0 1h1a.5.5 0 0 0 0-1z)BI";
    case Glyph::CloudMoon: return R"BI(M11.473 11a4.5 4.5 0 0 0-8.72-.99A3 3 0 0 0 3 16h8.5a2.5 2.5 0 0 0 0-5z M11.286 1.778a.5.5 0 0 0-.565-.755 4.595 4.595 0 0 0-3.18 5.003 5.5 5.5 0 0 1 1.055.209A3.6 3.6 0 0 1 9.83 2.617a4.593 4.593 0 0 0 4.31 5.744 3.58 3.58 0 0 1-2.241.634q.244.477.394 1a4.59 4.59 0 0 0 3.624-2.04.5.5 0 0 0-.565-.755 3.593 3.593 0 0 1-4.065-5.422z)BI";
    case Glyph::Cloud: return R"BI(M4.406 3.342A5.53 5.53 0 0 1 8 2c2.69 0 4.923 2 5.166 4.579C14.758 6.804 16 8.137 16 9.773 16 11.569 14.502 13 12.687 13H3.781C1.708 13 0 11.366 0 9.318c0-1.763 1.266-3.223 2.942-3.593.143-.863.698-1.723 1.464-2.383)BI";
    case Glyph::Fog: return R"BI(M8.5 3a5 5 0 0 1 4.905 4.027A3 3 0 0 1 13 13h-1.5a.5.5 0 0 0 0-1H1.05a3.5 3.5 0 0 1-.713-1H9.5a.5.5 0 0 0 0-1H.035a3.5 3.5 0 0 1 0-1H7.5a.5.5 0 0 0 0-1H.337a3.5 3.5 0 0 1 3.57-1.977A5 5 0 0 1 8.5 3)BI";
    case Glyph::Drizzle: return R"BI(M4.158 12.025a.5.5 0 0 1 .316.633l-.5 1.5a.5.5 0 0 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.317m6 0a.5.5 0 0 1 .316.633l-.5 1.5a.5.5 0 0 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.317m-3.5 1.5a.5.5 0 0 1 .316.633l-.5 1.5a.5.5 0 0 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.317m6 0a.5.5 0 0 1 .316.633l-.5 1.5a.5.5 0 1 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.317m.747-8.498a5.001 5.001 0 0 0-9.499-1.004A3.5 3.5 0 1 0 3.5 11H13a3 3 0 0 0 .405-5.973)BI";
    case Glyph::Rain: return R"BI(M4.158 12.025a.5.5 0 0 1 .316.633l-.5 1.5a.5.5 0 1 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.317m3 0a.5.5 0 0 1 .316.633l-1 3a.5.5 0 1 1-.948-.316l1-3a.5.5 0 0 1 .632-.317m3 0a.5.5 0 0 1 .316.633l-.5 1.5a.5.5 0 1 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.317m3 0a.5.5 0 0 1 .316.633l-1 3a.5.5 0 1 1-.948-.316l1-3a.5.5 0 0 1 .632-.317m.247-6.998a5.001 5.001 0 0 0-9.499-1.004A3.5 3.5 0 1 0 3.5 11H13a3 3 0 0 0 .405-5.973)BI";
    case Glyph::Snow: return R"BI(M2.625 11.5a.25.25 0 0 1 .25.25v.57l.501-.287a.25.25 0 0 1 .248.434l-.495.283.495.283a.25.25 0 0 1-.248.434l-.501-.286v.569a.25.25 0 1 1-.5 0v-.57l-.501.287a.25.25 0 0 1-.248-.434l.495-.283-.495-.283a.25.25 0 0 1 .248-.434l.501.286v-.569a.25.25 0 0 1 .25-.25m2.75 2a.25.25 0 0 1 .25.25v.57l.5-.287a.25.25 0 0 1 .249.434l-.495.283.495.283a.25.25 0 0 1-.248.434l-.501-.286v.569a.25.25 0 1 1-.5 0v-.57l-.501.287a.25.25 0 0 1-.248-.434l.495-.283-.495-.283a.25.25 0 0 1 .248-.434l.501.286v-.569a.25.25 0 0 1 .25-.25m5.5 0a.25.25 0 0 1 .25.25v.57l.5-.287a.25.25 0 0 1 .249.434l-.495.283.495.283a.25.25 0 0 1-.248.434l-.501-.286v.569a.25.25 0 0 1-.5 0v-.57l-.501.287a.25.25 0 0 1-.248-.434l.495-.283-.495-.283a.25.25 0 0 1 .248-.434l.501.286v-.569a.25.25 0 0 1 .25-.25m-2.75-2a.25.25 0 0 1 .25.25v.57l.5-.287a.25.25 0 0 1 .249.434l-.495.283.495.283a.25.25 0 0 1-.248.434l-.501-.286v.569a.25.25 0 1 1-.5 0v-.57l-.501.287a.25.25 0 0 1-.248-.434l.495-.283-.495-.283a.25.25 0 0 1 .248-.434l.501.286v-.569a.25.25 0 0 1 .25-.25m5.5 0a.25.25 0 0 1 .25.25v.57l.5-.287a.25.25 0 0 1 .249.434l-.495.283.495.283a.25.25 0 0 1-.248.434l-.501-.286v.569a.25.25 0 0 1-.5 0v-.57l-.501.287a.25.25 0 1 1-.248-.434l.495-.283-.495-.283a.25.25 0 0 1 .248-.434l.501.286v-.569a.25.25 0 0 1 .25-.25m-.22-7.223a5.001 5.001 0 0 0-9.499-1.004A3.5 3.5 0 1 0 3.5 10.25H13a3 3 0 0 0 .405-5.973)BI";
    case Glyph::Sleet: return R"BI(M2.375 13.5a.25.25 0 0 1 .25.25v.57l.501-.287a.25.25 0 0 1 .248.434l-.495.283.495.283a.25.25 0 1 1-.248.434l-.501-.286v.569a.25.25 0 1 1-.5 0v-.57l-.501.287a.25.25 0 1 1-.248-.434l.495-.283-.495-.283a.25.25 0 1 1 .248-.434l.501.286v-.569a.25.25 0 0 1 .25-.25m1.849-2.447a.5.5 0 0 1 .223.67l-.5 1a.5.5 0 0 1-.894-.447l.5-1a.5.5 0 0 1 .67-.223zM6.375 13.5a.25.25 0 0 1 .25.25v.57l.5-.287a.25.25 0 0 1 .249.434l-.495.283.495.283a.25.25 0 1 1-.248.434l-.501-.286v.569a.25.25 0 1 1-.5 0v-.57l-.501.287a.25.25 0 1 1-.248-.434l.495-.283-.495-.283a.25.25 0 1 1 .248-.434l.501.286v-.569a.25.25 0 0 1 .25-.25m1.849-2.447a.5.5 0 0 1 .223.67l-.5 1a.5.5 0 0 1-.894-.447l.5-1a.5.5 0 0 1 .67-.223zm2.151 2.447a.25.25 0 0 1 .25.25v.57l.5-.287a.25.25 0 0 1 .249.434l-.495.283.495.283a.25.25 0 1 1-.248.434l-.501-.286v.569a.25.25 0 0 1-.5 0v-.57l-.501.287a.25.25 0 1 1-.248-.434l.495-.283-.495-.283a.25.25 0 1 1 .248-.434l.501.286v-.569a.25.25 0 0 1 .25-.25m1.849-2.447a.5.5 0 0 1 .223.67l-.5 1a.5.5 0 1 1-.894-.447l.5-1a.5.5 0 0 1 .67-.223zm1.181-7.026a5.001 5.001 0 0 0-9.499-1.004A3.5 3.5 0 1 0 3.5 10H13a3 3 0 0 0 .405-5.973)BI";
    case Glyph::Thunder: return R"BI(M2.658 11.026a.5.5 0 0 1 .316.632l-.5 1.5a.5.5 0 1 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.316m9.5 0a.5.5 0 0 1 .316.632l-.5 1.5a.5.5 0 0 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.316m-7.5 1.5a.5.5 0 0 1 .316.632l-.5 1.5a.5.5 0 1 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.316m9.5 0a.5.5 0 0 1 .316.632l-.5 1.5a.5.5 0 0 1-.948-.316l.5-1.5a.5.5 0 0 1 .632-.316m-7.105-1.25A.5.5 0 0 1 7.5 11h1a.5.5 0 0 1 .474.658l-.28.842H9.5a.5.5 0 0 1 .39.812l-2 2.5a.5.5 0 0 1-.875-.433L7.36 14H6.5a.5.5 0 0 1-.447-.724zm6.352-7.249a5.001 5.001 0 0 0-9.499-1.004A3.5 3.5 0 1 0 3.5 10H13a3 3 0 0 0 .405-5.973)BI";
    default: return nullptr;
    }
}

// -- SVG path 'd' → QPainterPath ---------------------------------------------
// Supports the command subset the embedded icons use — M m L l H h V v C c
// Q q A a Z z — including implicit parameter-list repetition per the SVG
// spec. Deterministic: same input always yields the same geometry.

namespace SvgPathDetail {

// SVG elliptical arc, endpoint parameterization (W3C F.6.5), approximated
// with ≤90° cubic segments whose control points ride the arc derivative.
inline void arcTo(QPainterPath &path, const QPointF &from, const QPointF &to,
                  double rx, double ry, double xRotDeg, bool largeArc, bool sweep)
{
    if (rx <= 0 || ry <= 0 || from == to) {
        path.lineTo(to);
        return;
    }
    const double phi = qDegreesToRadians(xRotDeg);
    const double cosP = std::cos(phi), sinP = std::sin(phi);
    const double hx = (from.x() - to.x()) / 2.0, hy = (from.y() - to.y()) / 2.0;
    const double x1p =  cosP * hx + sinP * hy;
    const double y1p = -sinP * hx + cosP * hy;

    const double lam = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
    if (lam > 1.0) {                       // endpoints too far: grow radii
        const double s = std::sqrt(lam);
        rx *= s;
        ry *= s;
    }

    const double num = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p;
    const double den = rx * rx * y1p * y1p + ry * ry * x1p * x1p;
    double co = std::sqrt(qMax(0.0, num) / (den > 0 ? den : 1.0));
    if (largeArc == sweep)
        co = -co;
    const double cxp =  co * rx * y1p / ry;
    const double cyp = -co * ry * x1p / rx;
    const double cx = cosP * cxp - sinP * cyp + (from.x() + to.x()) / 2.0;
    const double cy = sinP * cxp + cosP * cyp + (from.y() + to.y()) / 2.0;

    auto angleBetween = [](double ux, double uy, double vx, double vy) {
        const double dot = (ux * vx + uy * vy)
            / (std::sqrt(ux * ux + uy * uy) * std::sqrt(vx * vx + vy * vy) + 1e-12);
        const double a = std::acos(qBound(-1.0, dot, 1.0));
        return (ux * vy - uy * vx < 0) ? -a : a;
    };

    double t1 = angleBetween(1.0, 0.0, (x1p - cxp) / rx, (y1p - cyp) / ry);
    double dt = angleBetween((x1p - cxp) / rx, (y1p - cyp) / ry,
                             (-x1p - cxp) / rx, (-y1p - cyp) / ry);
    if (!sweep && dt > 0) dt -= 2 * M_PI;
    if (sweep && dt < 0) dt += 2 * M_PI;

    const int segs = qMax(1, int(std::ceil(std::abs(dt) / (M_PI / 2))));
    const double step = dt / segs;
    QPointF prev = from;
    for (int i = 0; i < segs; ++i) {
        const double ta = t1 + step * i;
        const double tb = ta + step;
        const QPointF end(cx + rx * cosP * std::cos(tb) - ry * sinP * std::sin(tb),
                          cy + rx * sinP * std::cos(tb) + ry * cosP * std::sin(tb));
        const QPointF d1(-rx * cosP * std::sin(ta) - ry * sinP * std::cos(ta),
                         -rx * sinP * std::sin(ta) + ry * cosP * std::cos(ta));
        const QPointF d2(-rx * cosP * std::sin(tb) - ry * sinP * std::cos(tb),
                         -rx * sinP * std::sin(tb) + ry * cosP * std::cos(tb));
        path.cubicTo(prev + d1 * (step / 3.0), end - d2 * (step / 3.0), end);
        prev = end;
    }
}

} // namespace SvgPathDetail

inline QPainterPath parse(const char *s)
{
    QPainterPath path;
    if (!s)
        return path;
    const char *c = s;
    auto skipSep = [&] {
        while (*c == ' ' || *c == ',' || *c == '\t' || *c == '\r' || *c == '\n')
            ++c;
    };
    auto num = [&](double &out) {
        skipSep();
        char *end = nullptr;
        out = std::strtod(c, &end);
        if (end == c)
            return false;
        c = end;
        return true;
    };
    auto flag = [&](bool &out) {
        skipSep();
        if (*c != '0' && *c != '1')
            return false;
        out = (*c == '1');
        ++c;
        return true;
    };

    char cmd = 0;
    double a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0, a6 = 0;
    bool f1 = false, f2 = false;
    QPointF cur(0, 0), sub(0, 0);
    for (;;) {
        skipSep();
        if (*c == '\0')
            break;
        if ((*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z'))
            cmd = *c++;
        switch (cmd) {
        case 'M':
            if (!num(a1) || !num(a2)) return path;
            cur = sub = QPointF(a1, a2);
            path.moveTo(cur);
            cmd = 'L';               // implicit repeats after M are lineto
            break;
        case 'm':
            if (!num(a1) || !num(a2)) return path;
            cur += QPointF(a1, a2);
            sub = cur;
            path.moveTo(cur);
            cmd = 'l';
            break;
        case 'L':
            if (!num(a1) || !num(a2)) return path;
            cur = QPointF(a1, a2);
            path.lineTo(cur);
            break;
        case 'l':
            if (!num(a1) || !num(a2)) return path;
            cur += QPointF(a1, a2);
            path.lineTo(cur);
            break;
        case 'H':
            if (!num(a1)) return path;
            cur.setX(a1);
            path.lineTo(cur);
            break;
        case 'h':
            if (!num(a1)) return path;
            cur.rx() += a1;
            path.lineTo(cur);
            break;
        case 'V':
            if (!num(a1)) return path;
            cur.setY(a1);
            path.lineTo(cur);
            break;
        case 'v':
            if (!num(a1)) return path;
            cur.ry() += a1;
            path.lineTo(cur);
            break;
        case 'C':
            if (!num(a1) || !num(a2) || !num(a3) || !num(a4) || !num(a5) || !num(a6)) return path;
            path.cubicTo(QPointF(a1, a2), QPointF(a3, a4), QPointF(a5, a6));
            cur = QPointF(a5, a6);
            break;
        case 'c':
            if (!num(a1) || !num(a2) || !num(a3) || !num(a4) || !num(a5) || !num(a6)) return path;
            path.cubicTo(cur + QPointF(a1, a2), cur + QPointF(a3, a4), cur + QPointF(a5, a6));
            cur += QPointF(a5, a6);
            break;
        case 'Q':
            if (!num(a1) || !num(a2) || !num(a3) || !num(a4)) return path;
            path.quadTo(QPointF(a1, a2), QPointF(a3, a4));
            cur = QPointF(a3, a4);
            break;
        case 'q':
            if (!num(a1) || !num(a2) || !num(a3) || !num(a4)) return path;
            path.quadTo(cur + QPointF(a1, a2), cur + QPointF(a3, a4));
            cur += QPointF(a3, a4);
            break;
        case 'A':
            if (!num(a1) || !num(a2) || !num(a3) || !flag(f1) || !flag(f2) || !num(a4) || !num(a5)) return path;
            SvgPathDetail::arcTo(path, cur, QPointF(a4, a5), a1, a2, a3, f1, f2);
            cur = QPointF(a4, a5);
            break;
        case 'a':
            if (!num(a1) || !num(a2) || !num(a3) || !flag(f1) || !flag(f2) || !num(a4) || !num(a5)) return path;
            SvgPathDetail::arcTo(path, cur, cur + QPointF(a4, a5), a1, a2, a3, f1, f2);
            cur += QPointF(a4, a5);
            break;
        case 'Z':
        case 'z':
            path.closeSubpath();
            cur = sub;
            break;
        default:
            return path;             // unknown command: keep what parsed
        }
    }
    return path;
}

// Parsed glyph geometry, cached after first use (upstream 16×16 viewBox).
inline QPainterPath glyphPath(Glyph g)
{
    struct Cache {
        QVector<QPainterPath> paths;
        Cache() : paths(int(Glyph::Thunder) + 1)
        {
            for (int i = 0; i < paths.size(); ++i)
                paths[i] = parse(pathData(Glyph(i)));
        }
    };
    static Cache cache;
    return cache.paths[int(g)];
}

// Draw the glyph centered in r as the largest square that fits, filled with
// the condition color. isDay is accepted for API compatibility; day vs night
// is already encoded in the Sun/Moon/CloudSun/CloudMoon glyph choice.
inline void draw(QPainter &p, Glyph g, const QRectF &r, bool isDay)
{
    Q_UNUSED(isDay);
    if (g == Glyph::None) {
        // explicit no-data placeholder, matching the app's gap convention
        QPen pen(Theme::TextFaint, 1.2, Qt::DashLine);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(2, 2, -2, -2));
        return;
    }
    const QPainterPath gp = glyphPath(g);
    const qreal side = qMin(r.width(), r.height());
    p.save();
    p.translate(r.center().x() - side / 2.0, r.center().y() - side / 2.0);
    p.scale(side / 16.0, side / 16.0);
    p.fillPath(gp, baseColor(g));
    p.restore();
}

} // namespace GlyphPaint
