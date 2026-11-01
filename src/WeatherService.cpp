#include "WeatherService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <utility>

WeatherService::WeatherService(const QVector<Station> &stations, QObject *parent)
    : QObject(parent)
{
    m_data.reserve(stations.size());
    for (const Station &s : stations)
        m_data.push_back(StationData{s, {}, {}, {}, 0, {}, {}, StationData::State::Pending, {}, {}});

    m_net.setTransferTimeout(9000);
    connect(&m_scanTimer, &QTimer::timeout, this, &WeatherService::advanceScan);
}

void WeatherService::setStations(const QVector<Station> &stations)
{
    ++m_generation;                    // retire replies still in flight for the old roster
    m_scanTimer.stop();
    m_scanQueueRemaining = 0;
    m_scanCursor = 0;

    QVector<StationData> next;
    next.reserve(stations.size());
    for (const Station &s : stations) {
        StationData d;
        d.station = s;
        // Carry the last good sample across an edit when site identity
        // (code) and coordinates are unchanged.
        for (const StationData &old : std::as_const(m_data)) {
            if (old.station.code == s.code && old.station.lat == s.lat
                && old.station.lon == s.lon) {
                d = old;
                break;
            }
        }
        next.push_back(d);
    }
    m_data = next;
    emit stationsReconfigured();
    if (m_started)
        rescan();                      // pick up new / relocated sites right away
}

void WeatherService::start()
{
    m_started = true;
    rescan();
}

void WeatherService::rescan()
{
    m_scanCursor = 0;
    m_scanQueueRemaining = stationCount();
    m_scanDone = 0;
    m_scanTotal = stationCount();
    m_scanTimer.start(kStaggerMs);
    emit scanProgress(m_scanDone, m_scanTotal);
}

void WeatherService::advanceScan()
{
    if (m_scanCursor >= stationCount()) {
        m_scanTimer.stop();
        return;
    }
    pollStation(m_scanCursor++);
}

void WeatherService::pollStation(int idx)
{
    if (idx < 0 || idx >= stationCount())
        return;

    const Station &st = m_data[idx].station;
    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("latitude"), QString::number(st.lat, 'f', 2));
    q.addQueryItem(QStringLiteral("longitude"), QString::number(st.lon, 'f', 2));
    q.addQueryItem(QStringLiteral("current"),
                   QStringLiteral("temperature_2m,relative_humidity_2m,apparent_temperature,"
                                  "is_day,precipitation,weather_code,cloud_cover,pressure_msl,"
                                  "wind_speed_10m,wind_direction_10m,wind_gusts_10m"));
    q.addQueryItem(QStringLiteral("hourly"),
                   QStringLiteral("temperature_2m,precipitation_probability,pressure_msl"));
    q.addQueryItem(QStringLiteral("daily"),
                   QStringLiteral("weather_code,temperature_2m_max,temperature_2m_min,"
                                  "sunrise,sunset,precipitation_probability_max,wind_speed_10m_max"));
    q.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    q.addQueryItem(QStringLiteral("past_days"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("forecast_days"), QStringLiteral("3"));
    url.setQuery(q);

    QNetworkReply *reply = m_net.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, idx, gen = m_generation, reply] {
        reply->deleteLater();
        // The roster was swapped while this reply was in flight: the index
        // no longer names the station that was polled, so drop the sample.
        if (gen != m_generation || idx < 0 || idx >= m_data.size())
            return;
        StationData &d = m_data[idx];
        if (reply->error() != QNetworkReply::NoError) {
            d.error = reply->errorString();
            // Keep the last good sample; just flag it stale.
            if (d.state == StationData::State::Live)
                d.state = StationData::State::Stale;
            else if (d.state == StationData::State::Pending)
                d.state = StationData::State::Error;
            if (m_linkOnline) {
                m_linkOnline = false;
                emit linkStatusChanged(false);
            }
        } else {
            parseResponse(idx, reply->readAll());
            if (!m_linkOnline) {
                m_linkOnline = true;
                emit linkStatusChanged(true);
            }
        }

        emit stationUpdated(idx);

        if (m_scanQueueRemaining > 0) {
            --m_scanQueueRemaining;
            ++m_scanDone;
            if (m_scanQueueRemaining == 0)
                m_lastSync = QDateTime::currentDateTimeUtc();
            emit scanProgress(m_scanDone, m_scanTotal);
        }
    });
}

static double getD(const QJsonObject &o, const char *k) { return o.value(k).toDouble(std::nan("")); }
static int    getI(const QJsonObject &o, const char *k) { return o.value(k).toInt(-1); }

void WeatherService::parseResponse(int idx, const QByteArray &body)
{
    StationData &d = m_data[idx];
    const QJsonObject root = QJsonDocument::fromJson(body).object();
    if (root.isEmpty()) { d.state = StationData::State::Error; d.error = "EMPTY FRAME"; return; }

    d.utcOffsetSec = root.value("utc_offset_seconds").toInt();
    d.tzName = root.value("timezone").toString();
    d.tzAbbr = root.value("timezone_abbreviation").toString();

    const QJsonObject cur = root.value("current").toObject();
    if (!cur.isEmpty()) {
        CurrentObs c;
        c.timeIso    = cur.value("time").toString();
        c.tempC      = getD(cur, "temperature_2m");
        c.feelsC     = getD(cur, "apparent_temperature");
        c.precipMm   = getD(cur, "precipitation");
        c.cloudPct   = getD(cur, "cloud_cover");
        c.pressHpa   = getD(cur, "pressure_msl");
        c.windKmh    = getD(cur, "wind_speed_10m");
        c.gustKmh    = getD(cur, "wind_gusts_10m");
        c.humidityPct= getI(cur, "relative_humidity_2m");
        c.windDeg    = getI(cur, "wind_direction_10m");
        c.weatherCode= getI(cur, "weather_code");
        c.isDay      = getI(cur, "is_day");
        d.current = c;
    }

    const QJsonObject hr = root.value("hourly").toObject();
    const QJsonArray hTime = hr.value("time").toArray();
    if (!hTime.isEmpty()) {
        const QJsonArray hTemp  = hr.value("temperature_2m").toArray();
        const QJsonArray hProb  = hr.value("precipitation_probability").toArray();
        const QJsonArray hPress = hr.value("pressure_msl").toArray();
        HourlySeries s;
        s.tempC.reserve(hTime.size());
        for (int i = 0; i < hTime.size(); ++i) {
            s.timeIso.push_back(hTime.at(i).toString());
            s.tempC.push_back(hTemp.at(i).toDouble(std::nan("")));
            s.precipProb.push_back(hProb.at(i).toDouble(std::nan("")));
            s.pressHpa.push_back(hPress.at(i).toDouble(std::nan("")));
        }
        // First hourly slot at/after the current observation time (ISO strings
        // share the station-local timezone, so lexicographic compare is safe).
        s.nowIndex = 0;
        for (int i = 0; i < s.timeIso.size(); ++i) {
            if (s.timeIso[i] >= d.current.timeIso) { s.nowIndex = i; break; }
        }
        d.hourly = s;
    }

    const QJsonObject dy = root.value("daily").toObject();
    const QJsonArray dTime = dy.value("time").toArray();
    if (!dTime.isEmpty()) {
        const QJsonArray dCode = dy.value("weather_code").toArray();
        const QJsonArray dMax  = dy.value("temperature_2m_max").toArray();
        const QJsonArray dMin  = dy.value("temperature_2m_min").toArray();
        const QJsonArray dRise = dy.value("sunrise").toArray();
        const QJsonArray dSet  = dy.value("sunset").toArray();
        const QJsonArray dProb = dy.value("precipitation_probability_max").toArray();
        const QJsonArray dWind = dy.value("wind_speed_10m_max").toArray();
        d.daily.clear();
        d.daily.reserve(dTime.size());
        for (int i = 0; i < dTime.size(); ++i) {
            DailyEntry e;
            e.dateIso = dTime.at(i).toString();
            e.weatherCode = dCode.at(i).toInt(-1);
            e.tMaxC = dMax.at(i).toDouble(std::nan(""));
            e.tMinC = dMin.at(i).toDouble(std::nan(""));
            e.sunriseIso = dRise.at(i).toString();
            e.sunsetIso  = dSet.at(i).toString();
            e.precipProbMax = dProb.at(i).toInt(-1);
            e.windMaxKmh = dWind.at(i).toDouble(std::nan(""));
            d.daily.push_back(e);
        }
    }

    d.error.clear();
    d.lastSyncUtc = QDateTime::currentDateTimeUtc();
    d.state = StationData::State::Live;
}
