#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QList>
#include <QVariantMap>

struct SeriesData {
    int id = 0;
    QString name, size, year, hardDisk, category;
    double imdbRating = 0.0;
    QString imdbId, genre, director, actors, plot, country, language, awards, posterUrl;
    bool watched = false;
    QString createdAt, updatedAt;
};

class Database : public QObject {
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr);
    ~Database();
    bool open(const QString &path);
    void close();
    bool isOpen() const;
    int  addSeries(const SeriesData &s);
    bool updateSeries(const SeriesData &s);
    bool deleteSeries(int id);
    QList<SeriesData> getAllSeries() const;
    QList<SeriesData> searchSeries(const QString &term,
        const QString &category=QString(), const QString &hardDisk=QString(),
        double minRating=0.0, double maxRating=10.0) const;
    SeriesData getSeriesById(int id) const;
    SeriesData getSeriesByName(const QString &name) const;
    bool updateImdb(int id, double rating, const QString &imdbId,
        const QString &genre, const QString &director, const QString &actors,
        const QString &plot, const QString &country, const QString &language,
        const QString &awards, const QString &posterUrl);
    int importSeries(const QList<SeriesData> &list, bool updateExisting=false);
    int countAll() const;
    int countByCategory(const QString &cat) const;
    double totalSizeGB() const;
    QStringList allHardDisks() const;
    QString lastError() const { return m_lastError; }
private:
    bool createTables();
    SeriesData rowToSeries(QSqlQuery &q) const;
    mutable QString m_lastError;
    QSqlDatabase m_db;
};
