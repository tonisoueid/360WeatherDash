#pragma once

#include "StationData.h"

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVector>

// ----------------------------------------------------------------------------
// Polls the Open-Meteo feed site-by-site on a staggered scan cycle, like a
// NOC polling its telemetry stations in round-robin. One in-flight request
// per site; failures degrade the site to Error/Stale without touching the
// last good sample. The roster is hot-swappable: setStations() rebuilds the
// polling list while preserving the last good sample of any site whose code
// and coordinates are unchanged, and drops replies that were in flight for
// a superseded roster.
// ----------------------------------------------------------------------------
class WeatherService : public QObject
{
    Q_OBJECT
public:
    explicit WeatherService(const QVector<Station> &stations, QObject *parent = nullptr);

    void start();                        // kick off the first scan
    void rescan();                       // full sweep now
    void pollStation(int idx);           // single-site poll
    void setStations(const QVector<Station> &stations);   // hot roster swap

    int stationCount() const { return int(m_data.size()); }
    const StationData &data(int idx) const { return m_data[idx]; }
    const QVector<StationData> &all() const { return m_data; }

    int scanDone() const { return m_scanDone; }
    int scanTotal() const { return m_scanTotal; }
    bool scanActive() const { return m_scanTimer.isActive() || m_scanQueueRemaining > 0; }
    QDateTime lastFullSyncUtc() const { return m_lastSync; }

    static constexpr int kRefreshSecs = 900;   // scheduled sweep every 15 min (feed also updates ~15 min)
    static constexpr int kStaggerMs = 450;     // cadence between site polls

signals:
    void stationUpdated(int idx);
    void scanProgress(int done, int total);
    void linkStatusChanged(bool online);
    void stationsReconfigured();         // roster swapped; indexes are new

private:
    void advanceScan();
    void parseResponse(int idx, const QByteArray &body);

    QVector<StationData> m_data;
    QNetworkAccessManager m_net;
    QTimer m_scanTimer;                 // walks the staggered sweep
    int m_scanCursor = 0;
    int m_scanQueueRemaining = 0;
    int m_scanDone = 0;
    int m_scanTotal = 0;
    QDateTime m_lastSync;
    bool m_linkOnline = true;
    bool m_started = false;
    quint32 m_generation = 0;           // bumped on roster swap; retires in-flight replies
};
