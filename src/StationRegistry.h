#pragma once

#include "StationData.h"

#include <QObject>
#include <QString>
#include <QVector>

// ----------------------------------------------------------------------------
// The administrable watched-site roster. Seeded from the built-in defaults in
// StationData.h, then overridden by a JSON file in the user's config
// directory. The admin dialog commits edits through setStations(), which
// normalizes, validates, persists, and announces the new list.
// ----------------------------------------------------------------------------
class StationRegistry : public QObject
{
    Q_OBJECT
public:
    explicit StationRegistry(QObject *parent = nullptr);

    enum class LoadResult { Defaults, File, Corrupt };
    LoadResult load();                                  // apply saved overrides at startup

    bool setStations(const QVector<Station> &list);     // validate + save + emit
    void resetToDefaults();                             // restore built-in roster (persists)

    const QVector<Station> &stations() const { return m_stations; }

    static QString configPath();
    static Station normalized(const Station &s);        // trim + upper-case identity fields
    static QString validateList(const QVector<Station> &list);   // "" when ok

signals:
    void stationsChanged();

private:
    bool save() const;

    QVector<Station> m_stations;   // starts as stationList() defaults
};
