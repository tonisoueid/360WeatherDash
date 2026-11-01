#include "StationRegistry.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>

#include <limits>

namespace {
constexpr int kMinStations = 1;
constexpr int kMaxStations = 60;
}

StationRegistry::StationRegistry(QObject *parent)
    : QObject(parent), m_stations(stationList())
{
}

QString StationRegistry::configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/stations.json");
}

Station StationRegistry::normalized(const Station &s)
{
    Station n = s;
    n.name = s.name.trimmed().toUpper();
    n.code = s.code.trimmed().toUpper();
    n.region = s.region.trimmed().toUpper();
    return n;
}

QString StationRegistry::validateList(const QVector<Station> &list)
{
    if (list.size() < kMinStations)
        return QStringLiteral("at least %1 site required").arg(kMinStations);
    if (list.size() > kMaxStations)
        return QStringLiteral("roster capped at %1 sites").arg(kMaxStations);

    QSet<QString> seenCodes;
    for (const Station &raw : list) {
        const Station s = normalized(raw);
        if (s.name.isEmpty())
            return QStringLiteral("site with empty name");
        if (s.code.isEmpty())
            return QStringLiteral("%1 has an empty code").arg(s.name);
        if (s.lat < -90.0 || s.lat > 90.0)
            return QStringLiteral("%1 latitude out of range").arg(s.name);
        if (s.lon < -180.0 || s.lon > 180.0)
            return QStringLiteral("%1 longitude out of range").arg(s.name);
        if (seenCodes.contains(s.code))
            return QStringLiteral("duplicate code %1").arg(s.code);
        seenCodes.insert(s.code);
    }
    return QString();
}

StationRegistry::LoadResult StationRegistry::load()
{
    QFile f(configPath());
    if (!f.exists())
        return LoadResult::Defaults;
    if (!f.open(QIODevice::ReadOnly))
        return LoadResult::Corrupt;

    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    const QJsonArray arr = root.value("stations").toArray();
    if (arr.isEmpty())
        return LoadResult::Corrupt;

    QVector<Station> parsed;
    parsed.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        Station s;
        s.name = o.value("name").toString();
        s.code = o.value("code").toString();
        s.region = o.value("region").toString();
        s.lat = o.value("lat").toDouble(std::numeric_limits<double>::quiet_NaN());
        s.lon = o.value("lon").toDouble(std::numeric_limits<double>::quiet_NaN());
        parsed.push_back(s);
    }
    if (!validateList(parsed).isEmpty())
        return LoadResult::Corrupt;

    m_stations.clear();
    for (const Station &s : parsed)
        m_stations.push_back(normalized(s));
    return LoadResult::File;
}

bool StationRegistry::save() const
{
    QJsonArray arr;
    for (const Station &s : m_stations) {
        QJsonObject o;
        o.insert("name", s.name);
        o.insert("code", s.code);
        o.insert("region", s.region);
        o.insert("lat", s.lat);
        o.insert("lon", s.lon);
        arr.append(o);
    }
    QJsonObject root;
    root.insert("version", 1);
    root.insert("stations", arr);

    const QString path = configPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit();
}

bool StationRegistry::setStations(const QVector<Station> &list)
{
    if (!validateList(list).isEmpty())
        return false;
    m_stations.clear();
    m_stations.reserve(list.size());
    for (const Station &s : list)
        m_stations.push_back(normalized(s));
    if (!save())
        return false;
    emit stationsChanged();
    return true;
}

void StationRegistry::resetToDefaults()
{
    setStations(stationList());
}
