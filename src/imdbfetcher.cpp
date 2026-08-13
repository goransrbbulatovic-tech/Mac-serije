#include "imdbfetcher.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>
#include <QRegularExpression>

ImdbFetcher::ImdbFetcher(QObject *parent) : QObject(parent) {
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam,&QNetworkAccessManager::finished,this,&ImdbFetcher::onReplyFinished);
    m_rateLimitTimer = new QTimer(this);
    m_rateLimitTimer->setSingleShot(false);
    m_rateLimitTimer->setInterval(350);
    connect(m_rateLimitTimer,&QTimer::timeout,this,&ImdbFetcher::processNextQueue);
}
ImdbFetcher::~ImdbFetcher() {}

void ImdbFetcher::setApiKey(const QString &key) { m_apiKey=key.trimmed(); }

QString ImdbFetcher::buildUrl(const QString &name, const QString &year) const {
    // Use HTTP (not HTTPS) to avoid needing OpenSSL DLLs on Windows
    QUrl url("http://www.omdbapi.com/");
    QUrlQuery q;
    q.addQueryItem("apikey",m_apiKey);
    q.addQueryItem("t",name);
    q.addQueryItem("type","series");
    if(!year.isEmpty()){
        QString y=year; y.remove(QRegularExpression("[^0-9]"));
        if(y.length()>=4) q.addQueryItem("y",y.left(4));
    }
    q.addQueryItem("plot","short");
    url.setQuery(q);
    return url.toString();
}

void ImdbFetcher::fetchSingle(int seriesId, const QString &name, const QString &year) {
    if(m_apiKey.isEmpty()){emit errorOccurred("API kljuc nije postavljen");return;}
    QNetworkRequest req{QUrl(buildUrl(name,year))};
    req.setHeader(QNetworkRequest::UserAgentHeader,"AcmigoIndexer/2.0");
    auto *reply=m_nam->get(req);
    reply->setProperty("seriesId",seriesId);
    m_pending++;
}

void ImdbFetcher::fetchBatch(const QList<FetchRequest> &requests) {
    if(m_apiKey.isEmpty()){emit errorOccurred("API kljuc nije postavljen");return;}
    m_cancelled=false; m_queue.clear();
    for(const auto &r:requests) m_queue.enqueue(r);
    m_batchTotal=requests.size(); m_batchDone=0; m_pending=0; m_concurrent=0;
    m_rateLimitTimer->start();
    processNextQueue();
}

void ImdbFetcher::cancelBatch() {
    m_cancelled=true; m_queue.clear(); m_rateLimitTimer->stop();
}

void ImdbFetcher::processNextQueue() {
    while(!m_queue.isEmpty()&&m_concurrent<MAX_CONCURRENT&&!m_cancelled){
        auto req=m_queue.dequeue();
        QNetworkRequest netReq{QUrl(buildUrl(req.name,req.year))};
        netReq.setHeader(QNetworkRequest::UserAgentHeader,"AcmigoIndexer/2.0");
        auto *reply=m_nam->get(netReq);
        reply->setProperty("seriesId",req.seriesId);
        m_pending++; m_concurrent++;
    }
    if(m_queue.isEmpty()&&m_concurrent==0){
        m_rateLimitTimer->stop();
        if(m_batchTotal>0) emit batchComplete();
    }
}

void ImdbFetcher::onReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();
    m_pending=qMax(0,m_pending-1);
    m_concurrent=qMax(0,m_concurrent-1);
    int seriesId=reply->property("seriesId").toInt();
    ImdbResult result;
    result.seriesId=seriesId;
    if(reply->error()!=QNetworkReply::NoError){
        result.success=false;
        result.error=reply->errorString();
    } else {
        result=parseReply(seriesId,reply->readAll());
    }
    emit resultReady(result);
    m_batchDone++;
    if(m_batchTotal>0) emit batchProgress(m_batchDone,m_batchTotal);
    if(!m_cancelled) processNextQueue();
}

ImdbResult ImdbFetcher::parseReply(int id, const QByteArray &data) {
    ImdbResult r; r.seriesId=id;
    QJsonParseError pe;
    auto doc=QJsonDocument::fromJson(data,&pe);
    if(pe.error!=QJsonParseError::NoError){r.error="JSON parse error";return r;}
    QJsonObject obj=doc.object();
    if(obj["Response"].toString()!="True"){r.error=obj["Error"].toString();return r;}
    r.success=true;
    r.title=obj["Title"].toString(); r.year=obj["Year"].toString();
    r.genre=obj["Genre"].toString(); r.director=obj["Director"].toString();
    r.actors=obj["Actors"].toString(); r.plot=obj["Plot"].toString();
    r.country=obj["Country"].toString(); r.language=obj["Language"].toString();
    r.awards=obj["Awards"].toString(); r.posterUrl=obj["Poster"].toString();
    r.imdbId=obj["imdbID"].toString();
    bool ok; double v=obj["imdbRating"].toString().toDouble(&ok);
    r.rating=ok?v:0.0;
    for(const auto &rv:obj["Ratings"].toArray()){
        QJsonObject ro=rv.toObject();
        if(ro["Source"].toString()=="Internet Movie Database"){
            QString val=ro["Value"].toString().split("/").first();
            double d=val.toDouble(&ok); if(ok&&d>0) r.rating=d;
        }
    }
    return r;
}
