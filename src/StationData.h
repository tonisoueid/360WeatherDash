#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>
#include <cmath>

// ----------------------------------------------------------------------------
// Data model. "Measured, never invented": every field starts invalid (NaN/-1)
// and the UI renders an explicit gap ("---", dashed baseline) until a real
// sample arrives from the feed.
// ----------------------------------------------------------------------------

struct Station {
    QString name;     // "TOKYO"
    QString code;     // ICAO-style site id, "RJTT"
    QString region;   // "ASIA"
    double lat = 0.0;
    double lon = 0.0;
};

inline bool operator==(const Station &a, const Station &b)
{
    return a.name == b.name && a.code == b.code && a.region == b.region
        && a.lat == b.lat && a.lon == b.lon;
}

struct CurrentObs {
    QString timeIso;                  // station-local ISO, e.g. 2026-10-24T22:30
    double tempC   = std::nan("");
    double feelsC  = std::nan("");
    double precipMm = std::nan("");
    double cloudPct = std::nan("");
    double pressHpa = std::nan("");
    double windKmh  = std::nan("");
    double gustKmh  = std::nan("");
    int humidityPct = -1;
    int windDeg     = -1;
    int weatherCode = -1;
    int isDay       = -1;
};

struct HourlySeries {
    QVector<QString> timeIso;         // station-local ISO hours
    QVector<double>  tempC;
    QVector<double>  precipProb;
    QVector<double>  pressHpa;
    int nowIndex = -1;                // first hourly slot >= current obs time
};

struct DailyEntry {
    QString dateIso;                  // YYYY-MM-DD
    QString sunriseIso, sunsetIso;    // station-local HH:MM
    int    weatherCode = -1;
    double tMaxC = std::nan("");
    double tMinC = std::nan("");
    double windMaxKmh = std::nan("");
    int    precipProbMax = -1;
};

struct StationData {
    Station station;
    CurrentObs current;
    HourlySeries hourly;
    QVector<DailyEntry> daily;
    qint64 utcOffsetSec = 0;
    QString tzName;                    // "Asia/Tokyo"
    QString tzAbbr;                    // "GMT+9"

    enum class State { Pending, Live, Stale, Error };
    State state = State::Pending;
    QString error;
    QDateTime lastSyncUtc;             // when this station last answered

    bool valid() const { return state == State::Live || state == State::Stale; }

    // Station-local wall clock right now (from UTC instant + feed offset).
    QTime localTimeNow() const
    {
        const QDateTime local = QDateTime::currentDateTimeUtc().addSecs(utcOffsetSec);
        return local.time();
    }

    QString localTimeStr() const { return localTimeNow().toString(QStringLiteral("hh:mm")); }

    // Seconds since the site last answered the poll.
    int ageSecs() const
    { return lastSyncUtc.isValid() ? int(lastSyncUtc.secsTo(QDateTime::currentDateTimeUtc())) : -1; }

    // Surface-pressure trend over the trailing 3 h of hourly history, hPa.
    double pressureTrend3h() const
    {
        const int i = hourly.nowIndex;
        if (i < 3 || i >= hourly.pressHpa.size())
            return std::nan("");
        const double a = hourly.pressHpa[i - 3], b = hourly.pressHpa[i];
        if (std::isnan(a) || std::isnan(b))
            return std::nan("");
        return b - a;
    }
};

// ----------------------------------------------------------------------------
// WMO 4677 weather-code table.
// ----------------------------------------------------------------------------
enum class Glyph { None, Sun, Moon, CloudSun, CloudMoon, Cloud, Fog, Drizzle, Rain, Snow, Sleet, Thunder };

struct WxInfo { const char *text; Glyph glyph; };

inline WxInfo decodeWmo(int code)
{
    switch (code) {
    case 0:  return {"CLEAR SKY",            Glyph::Sun};
    case 1:  return {"MAINLY CLEAR",         Glyph::CloudSun};
    case 2:  return {"PARTLY CLOUDY",        Glyph::CloudSun};
    case 3:  return {"OVERCAST",             Glyph::Cloud};
    case 45: return {"FOG",                  Glyph::Fog};
    case 48: return {"RIME FOG",             Glyph::Fog};
    case 51: return {"LIGHT DRIZZLE",        Glyph::Drizzle};
    case 53: return {"DRIZZLE",              Glyph::Drizzle};
    case 55: return {"DENSE DRIZZLE",        Glyph::Drizzle};
    case 56: return {"FREEZING DRIZZLE",     Glyph::Sleet};
    case 57: return {"FREEZING DRIZZLE",     Glyph::Sleet};
    case 61: return {"LIGHT RAIN",           Glyph::Rain};
    case 63: return {"RAIN",                 Glyph::Rain};
    case 65: return {"HEAVY RAIN",           Glyph::Rain};
    case 66: return {"FREEZING RAIN",        Glyph::Sleet};
    case 67: return {"FREEZING RAIN",        Glyph::Sleet};
    case 71: return {"LIGHT SNOW",           Glyph::Snow};
    case 73: return {"SNOW",                 Glyph::Snow};
    case 75: return {"HEAVY SNOW",           Glyph::Snow};
    case 77: return {"SNOW GRAINS",          Glyph::Snow};
    case 80: return {"RAIN SHOWERS",         Glyph::Rain};
    case 81: return {"RAIN SHOWERS",         Glyph::Rain};
    case 82: return {"VIOLENT SHOWERS",      Glyph::Rain};
    case 85: return {"SNOW SHOWERS",         Glyph::Snow};
    case 86: return {"SNOW SHOWERS",         Glyph::Snow};
    case 95: return {"THUNDERSTORM",         Glyph::Thunder};
    case 96: return {"THUNDER + HAIL",       Glyph::Thunder};
    case 99: return {"SEVERE THUNDER + HAIL",Glyph::Thunder};
    default: return {"NO SIGNAL",            Glyph::None};
    }
}

// 16-point compass abbreviation from degrees.
inline QString compassPoint(int deg)
{
    static const char *pts[16] = {"N","NNE","NE","ENE","E","ESE","SE","SSE",
                                  "S","SSW","SW","WSW","W","WNW","NW","NNW"};
    if (deg < 0) return QStringLiteral("---");
    return QString::fromLatin1(pts[((deg % 360) + 360) % 360 / 23]);
}

// ----------------------------------------------------------------------------
// Built-in default roster — global coverage, one screen. The live roster is
// administrable at runtime (see StationRegistry); it is seeded from this list
// and overridden by the saved JSON config.
// ----------------------------------------------------------------------------
inline const QVector<Station> &stationList()
{
    static const QVector<Station> stations = {
        {"REYKJAVIK", "BIRK", "N. EUROPE",   64.15, -21.94},
        {"LONDON",    "EGLL", "W. EUROPE",   51.51,  -0.13},
        {"PARIS",     "LFPG", "W. EUROPE",   48.86,   2.35},
        {"NEW YORK",  "KNYC", "N. AMERICA",  40.71, -74.01},
        {"LOS ANGELES","KLAX","N. AMERICA",  33.94,-118.41},
        {"SAO PAULO", "SBGR", "S. AMERICA", -23.55, -46.63},
        {"CAPE TOWN", "FACT", "AFRICA",     -33.92,  18.56},
        {"DUBAI",     "OMDB", "MIDDLE EAST", 25.25,  55.36},
        {"DELHI",     "VIDP", "ASIA",        28.57,  77.10},
        {"SINGAPORE", "WSSS", "SE ASIA",      1.36, 103.99},
        {"TOKYO",     "RJTT", "ASIA",        35.55, 139.78},
        {"SYDNEY",    "YSSY", "OCEANIA",    -33.87, 151.21},
    };
    return stations;
}

// ----------------------------------------------------------------------------
// Watch rules — evaluated on every sample. NOC semantics: a rule that fires
// logs an event; a rule that stops firing logs a "nominal" clearing event.
// ----------------------------------------------------------------------------
struct Alert {
    enum class Sev { Info, Warn, Crit };
    Sev sev = Sev::Info;
    QString key;      // stable rule id, used for raise/clear diffing
    QString message;  // human text, no station prefix
};

inline const char *sevLabel(Alert::Sev s)
{
    switch (s) { case Alert::Sev::Crit: return "CRIT"; case Alert::Sev::Warn: return "WARN";
                 default: return "INFO"; }
}

inline bool operator==(const Alert &a, const Alert &b) { return a.key == b.key; }

inline QVector<Alert> evaluateAlerts(const StationData &d)
{
    QVector<Alert> out;
    const auto add = [&](Alert::Sev s, QString key, QString msg) { out.push_back({s, key, msg}); };
    const CurrentObs &c = d.current;
    const WxInfo wx = decodeWmo(c.weatherCode);

    if (c.weatherCode >= 95)
        add(Alert::Sev::Crit, "STORM", QStringLiteral("THUNDERSTORM ACTIVITY OBSERVED"));
    if (!std::isnan(c.gustKmh)) {
        if (c.gustKmh >= 90)      add(Alert::Sev::Crit, "GUST", QStringLiteral("EXTREME GUSTS %1 KM/H").arg(qRound(c.gustKmh)));
        else if (c.gustKmh >= 60) add(Alert::Sev::Warn, "GUST", QStringLiteral("GUSTS %1 KM/H").arg(qRound(c.gustKmh)));
    }
    if (!std::isnan(c.windKmh) && c.windKmh >= 50)
        add(c.windKmh >= 75 ? Alert::Sev::Crit : Alert::Sev::Warn,
            "WIND", QStringLiteral("SUSTAINED WIND %1 KM/H").arg(qRound(c.windKmh)));
    if (!std::isnan(c.precipMm) && c.precipMm > 1.0)
        add(c.precipMm > 4.0 ? Alert::Sev::Crit : Alert::Sev::Warn,
            "PRECIP", QStringLiteral("PRECIP RATE %1 MM/H").arg(c.precipMm, 0, 'f', 1));
    if (c.weatherCode >= 71 && c.weatherCode <= 77)
        add(Alert::Sev::Warn, "SNOW", QStringLiteral("SNOWFALL IN PROGRESS"));
    if (!std::isnan(c.tempC)) {
        if (c.tempC >= 40)      add(Alert::Sev::Crit, "HEAT", QStringLiteral("EXTREME HEAT %1°C").arg(c.tempC, 0, 'f', 0));
        else if (c.tempC >= 35) add(Alert::Sev::Warn, "HEAT", QStringLiteral("HIGH TEMP %1°C").arg(c.tempC, 0, 'f', 0));
        if (c.tempC <= -20)     add(Alert::Sev::Crit, "COLD", QStringLiteral("EXTREME COLD %1°C").arg(c.tempC, 0, 'f', 0));
        else if (c.tempC <= -10)add(Alert::Sev::Warn, "COLD", QStringLiteral("SEVERE COLD %1°C").arg(c.tempC, 0, 'f', 0));
    }
    if (!std::isnan(c.pressHpa)) {
        if (c.pressHpa <= 980)
            add(Alert::Sev::Warn, "PRESSURE", QStringLiteral("LOW PRESSURE %1 HPA").arg(qRound(c.pressHpa)));
        const double trend = d.pressureTrend3h();
        if (!std::isnan(trend) && trend <= -4.0)
            add(Alert::Sev::Warn, "PTREND", QStringLiteral("PRESSURE FALLING %1 HPA/3H").arg(trend, 0, 'f', 1));
    }
    Q_UNUSED(wx);
    return out;
}
