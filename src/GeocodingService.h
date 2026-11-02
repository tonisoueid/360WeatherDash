#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QVector>

#include <cmath>

class QNetworkReply;

// One geocoder hit. Coordinates start NaN (missing-data convention); a hit
// without both coordinates is dropped at parse time, never defaulted.
struct GeoResult
{
    QString name;
    QString admin1;
    QString country;
    QString countryCode;
    double lat = std::nan("");
    double lon = std::nan("");

    // "Name, Admin1, Country" for the disambiguation list.
    QString label() const;
};

// ----------------------------------------------------------------------------
// City-name lookup against the keyless Open-Meteo geocoding API. One query in
// flight at a time: a new search() aborts the previous one and a generation
// counter retires any reply that still slips through (same pattern as
// WeatherService / StationRegistry). In offline mode no request is ever made.
// ----------------------------------------------------------------------------
class GeocodingService : public QObject
{
    Q_OBJECT
public:
    explicit GeocodingService(bool offline, QObject *parent = nullptr);

    bool offline() const { return m_offline; }

    // Parses a geocoding response body. Returns false (and sets *err) when the
    // payload is not valid JSON; a payload without "results" is valid + empty.
    static bool parseResponse(const QByteArray &body, QVector<GeoResult> *out, QString *err);

public slots:
    void search(const QString &query);

signals:
    void resultsReady(const QVector<GeoResult> &results);   // may be empty
    void searchFailed(const QString &message);

private:
    QNetworkAccessManager m_net;
    QNetworkReply *m_reply = nullptr;
    quint64 m_generation = 0;
    bool m_offline = false;
};
