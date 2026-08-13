#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QString>
#include <QTimer>

struct ImdbResult {
    int seriesId=0; bool success=false; QString error;
    double rating=0.0;
    QString imdbId, genre, director, actors, plot, country, language, awards, posterUrl, title, year;
};

struct FetchRequest { int seriesId; QString name, year; };

class ImdbFetcher : public QObject {
    Q_OBJECT
public:
    explicit ImdbFetcher(QObject *parent=nullptr);
    ~ImdbFetcher();
    void setApiKey(const QString &key);
    QString apiKey() const { return m_apiKey; }
    bool hasApiKey() const { return !m_apiKey.isEmpty(); }
    void fetchSingle(int seriesId, const QString &name, const QString &year=QString());
    void fetchBatch(const QList<FetchRequest> &requests);
    void cancelBatch();
    bool isBusy() const { return m_pending>0; }
signals:
    void resultReady(ImdbResult result);
    void batchProgress(int done, int total);
    void batchComplete();
    void errorOccurred(QString msg);
private slots:
    void onReplyFinished(QNetworkReply *reply);
    void processNextQueue();
private:
    QString buildUrl(const QString &name, const QString &year) const;
    ImdbResult parseReply(int id, const QByteArray &data);
    QNetworkAccessManager *m_nam;
    QString m_apiKey;
    QQueue<FetchRequest> m_queue;
    int m_pending=0, m_batchTotal=0, m_batchDone=0, m_concurrent=0;
    QTimer *m_rateLimitTimer;
    bool m_cancelled=false;
    static constexpr int MAX_CONCURRENT=3;
};
