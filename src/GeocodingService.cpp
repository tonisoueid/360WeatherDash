#include "GeocodingService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

namespace {
constexpr int kTimeoutMs = 8000;
constexpr int kMaxResults = 10;
}

QString GeoResult::label() const
{
    QStringList parts{name};
    if (!admin1.isEmpty() && admin1 != name)
        parts << admin1;
    if (!country.isEmpty())
        parts << country;
    return parts.join(QStringLiteral(", "));
}

GeocodingService::GeocodingService(bool offline, QObject *parent)
    : QObject(parent), m_offline(offline)
{
    m_net.setTransferTimeout(kTimeoutMs);
}

bool GeocodingService::parseResponse(const QByteArray &body, QVector<GeoResult> *out, QString *err)
{
    out->clear();
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        *err = QStringLiteral("SEARCH FAILED: UNREADABLE RESPONSE");
        return false;
    }
    const QJsonArray arr = doc.object().value(QStringLiteral("results")).toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const QJsonValue la = o.value(QStringLiteral("latitude"));
        const QJsonValue lo = o.value(QStringLiteral("longitude"));
        if (!la.isDouble() || !lo.isDouble())
            continue;                       // never invent coordinates
        GeoResult r;
        r.name = o.value(QStringLiteral("name")).toString();
        r.admin1 = o.value(QStringLiteral("admin1")).toString();
        r.country = o.value(QStringLiteral("country")).toString();
        r.countryCode = o.value(QStringLiteral("country_code")).toString();
        r.lat = la.toDouble();
        r.lon = lo.toDouble();
        if (r.name.isEmpty() || !std::isfinite(r.lat) || !std::isfinite(r.lon)
            || r.lat < -90.0 || r.lat > 90.0 || r.lon < -180.0 || r.lon > 180.0)
            continue;
        out->push_back(r);
    }
    return true;
}

void GeocodingService::search(const QString &query)
{
    const QString q = query.trimmed();
    ++m_generation;                         // retire anything still in flight
    if (m_reply) {
        QNetworkReply *old = m_reply;
        m_reply = nullptr;
        old->disconnect(this);
        old->abort();
        old->deleteLater();
    }
    if (m_offline) {
        emit searchFailed(QStringLiteral("CITY SEARCH UNAVAILABLE OFFLINE — ENTER COORDINATES MANUALLY"));
        return;
    }
    if (q.size() < 2) {
        emit searchFailed(QStringLiteral("ENTER AT LEAST 2 CHARACTERS"));
        return;
    }

    QUrl url(QStringLiteral("https://geocoding-api.open-meteo.com/v1/search"));
    QUrlQuery uq;
    uq.addQueryItem(QStringLiteral("name"), q);
    uq.addQueryItem(QStringLiteral("count"), QString::number(kMaxResults));
    uq.addQueryItem(QStringLiteral("language"), QStringLiteral("en"));
    uq.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(uq);

    QNetworkReply *reply = m_net.get(QNetworkRequest(url));
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, gen = m_generation, reply] {
        reply->deleteLater();
        if (gen != m_generation)
            return;                         // superseded by a newer query
        m_reply = nullptr;
        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::OperationCanceledError
                || reply->error() == QNetworkReply::TimeoutError)
                emit searchFailed(QStringLiteral("SEARCH TIMED OUT — CHECK CONNECTION"));
            else
                emit searchFailed(QStringLiteral("SEARCH FAILED: %1").arg(reply->errorString().toUpper()));
            return;
        }
        QVector<GeoResult> res;
        QString err;
        if (!parseResponse(reply->readAll(), &res, &err))
            emit searchFailed(err);
        else
            emit resultsReady(res);
    });
}
