#include "database.h"
#include <QSqlRecord>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>

Database::Database(QObject *parent) : QObject(parent) {}
Database::~Database() { close(); }

bool Database::open(const QString &path) {
    m_db = QSqlDatabase::addDatabase("QSQLITE","series_db");
    m_db.setDatabaseName(path);
    if (!m_db.open()) { m_lastError=m_db.lastError().text(); return false; }
    return createTables();
}
void Database::close() { if(m_db.isOpen()) m_db.close(); }
bool Database::isOpen() const { return m_db.isOpen(); }

bool Database::createTables() {
    QSqlQuery q(m_db);
    bool ok=q.exec(R"(CREATE TABLE IF NOT EXISTS series (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL, size TEXT DEFAULT '', year TEXT DEFAULT '',
        hard_disk TEXT DEFAULT '', category TEXT DEFAULT 'STRANE',
        imdb_rating REAL DEFAULT 0, imdb_id TEXT DEFAULT '',
        genre TEXT DEFAULT '', director TEXT DEFAULT '',
        actors TEXT DEFAULT '', plot TEXT DEFAULT '',
        country TEXT DEFAULT '', language TEXT DEFAULT '',
        awards TEXT DEFAULT '', poster_url TEXT DEFAULT '',
        watched INTEGER DEFAULT 0,
        created_at TEXT DEFAULT (datetime('now')),
        updated_at TEXT DEFAULT (datetime('now'))
    ))");
    if(!ok){m_lastError=q.lastError().text();return false;}
    q.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_series_name ON series(name COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_series_category ON series(category)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_series_hdd ON series(hard_disk)");
    return true;
}

SeriesData Database::rowToSeries(QSqlQuery &q) const {
    SeriesData s;
    s.id=q.value("id").toInt(); s.name=q.value("name").toString();
    s.size=q.value("size").toString(); s.year=q.value("year").toString();
    s.hardDisk=q.value("hard_disk").toString(); s.category=q.value("category").toString();
    s.imdbRating=q.value("imdb_rating").toDouble(); s.imdbId=q.value("imdb_id").toString();
    s.genre=q.value("genre").toString(); s.director=q.value("director").toString();
    s.actors=q.value("actors").toString(); s.plot=q.value("plot").toString();
    s.country=q.value("country").toString(); s.language=q.value("language").toString();
    s.awards=q.value("awards").toString(); s.posterUrl=q.value("poster_url").toString();
    s.watched=q.value("watched").toBool(); s.createdAt=q.value("created_at").toString();
    s.updatedAt=q.value("updated_at").toString();
    return s;
}

int Database::addSeries(const SeriesData &s) {
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT OR IGNORE INTO series
        (name,size,year,hard_disk,category,imdb_rating,imdb_id,genre,director,
         actors,plot,country,language,awards,poster_url,watched)
        VALUES(:name,:size,:year,:hdd,:cat,:rating,:imdbid,:genre,:dir,
               :actors,:plot,:country,:lang,:awards,:poster,:watched))");
    q.bindValue(":name",s.name); q.bindValue(":size",s.size);
    q.bindValue(":year",s.year); q.bindValue(":hdd",s.hardDisk);
    q.bindValue(":cat",s.category); q.bindValue(":rating",s.imdbRating);
    q.bindValue(":imdbid",s.imdbId); q.bindValue(":genre",s.genre);
    q.bindValue(":dir",s.director); q.bindValue(":actors",s.actors);
    q.bindValue(":plot",s.plot); q.bindValue(":country",s.country);
    q.bindValue(":lang",s.language); q.bindValue(":awards",s.awards);
    q.bindValue(":poster",s.posterUrl); q.bindValue(":watched",s.watched?1:0);
    if(!q.exec()){m_lastError=q.lastError().text();return -1;}
    return q.lastInsertId().toInt();
}

bool Database::updateSeries(const SeriesData &s) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE series SET name=:name,size=:size,year=:year,hard_disk=:hdd,"
              "category=:cat,watched=:watched,updated_at=datetime('now') WHERE id=:id");
    q.bindValue(":name",s.name); q.bindValue(":size",s.size);
    q.bindValue(":year",s.year); q.bindValue(":hdd",s.hardDisk);
    q.bindValue(":cat",s.category); q.bindValue(":watched",s.watched?1:0);
    q.bindValue(":id",s.id);
    if(!q.exec()){m_lastError=q.lastError().text();return false;}
    return true;
}

bool Database::deleteSeries(int id) {
    QSqlQuery q(m_db); q.prepare("DELETE FROM series WHERE id=:id");
    q.bindValue(":id",id);
    if(!q.exec()){m_lastError=q.lastError().text();return false;}
    return true;
}

QList<SeriesData> Database::getAllSeries() const {
    QList<SeriesData> list; QSqlQuery q(m_db);
    q.exec("SELECT * FROM series ORDER BY name COLLATE NOCASE");
    while(q.next()) list.append(rowToSeries(q));
    return list;
}

QList<SeriesData> Database::searchSeries(const QString &term, const QString &category,
    const QString &hardDisk, double minRating, double maxRating) const {
    QList<SeriesData> list;
    QString sql="SELECT * FROM series WHERE 1=1";
    if(!term.isEmpty()) sql+=" AND (name LIKE :term OR genre LIKE :term2 OR year LIKE :term3 OR hard_disk LIKE :term4)";
    if(!category.isEmpty()) sql+=" AND category=:cat";
    if(!hardDisk.isEmpty()) sql+=" AND hard_disk LIKE :hdd";
    if(minRating>0||maxRating<10) sql+=" AND imdb_rating BETWEEN :minr AND :maxr";
    sql+=" ORDER BY name COLLATE NOCASE";
    QSqlQuery q(m_db); q.prepare(sql);
    if(!term.isEmpty()){QString like="%"+term+"%";q.bindValue(":term",like);q.bindValue(":term2",like);q.bindValue(":term3",like);q.bindValue(":term4",like);}
    if(!category.isEmpty()) q.bindValue(":cat",category);
    if(!hardDisk.isEmpty()) q.bindValue(":hdd","%"+hardDisk+"%");
    if(minRating>0||maxRating<10){q.bindValue(":minr",minRating);q.bindValue(":maxr",maxRating);}
    q.exec(); while(q.next()) list.append(rowToSeries(q));
    return list;
}

SeriesData Database::getSeriesById(int id) const {
    QSqlQuery q(m_db); q.prepare("SELECT * FROM series WHERE id=:id");
    q.bindValue(":id",id); q.exec();
    if(q.next()) return rowToSeries(q); return SeriesData{};
}

SeriesData Database::getSeriesByName(const QString &name) const {
    QSqlQuery q(m_db); q.prepare("SELECT * FROM series WHERE name=:name COLLATE NOCASE");
    q.bindValue(":name",name); q.exec();
    if(q.next()) return rowToSeries(q); return SeriesData{};
}

bool Database::updateImdb(int id, double rating, const QString &imdbId,
    const QString &genre, const QString &director, const QString &actors,
    const QString &plot, const QString &country, const QString &language,
    const QString &awards, const QString &posterUrl) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE series SET imdb_rating=:r,imdb_id=:imdbid,genre=:genre,"
              "director=:dir,actors=:actors,plot=:plot,country=:country,"
              "language=:lang,awards=:awards,poster_url=:poster,updated_at=datetime('now') WHERE id=:id");
    q.bindValue(":r",rating); q.bindValue(":imdbid",imdbId); q.bindValue(":genre",genre);
    q.bindValue(":dir",director); q.bindValue(":actors",actors); q.bindValue(":plot",plot);
    q.bindValue(":country",country); q.bindValue(":lang",language);
    q.bindValue(":awards",awards); q.bindValue(":poster",posterUrl); q.bindValue(":id",id);
    if(!q.exec()){m_lastError=q.lastError().text();return false;}
    return true;
}

int Database::importSeries(const QList<SeriesData> &list, bool updateExisting) {
    int count=0; m_db.transaction();
    for(const SeriesData &s:list){
        SeriesData existing=getSeriesByName(s.name);
        if(existing.id==0){if(addSeries(s)>0)count++;}
        else if(updateExisting){
            SeriesData updated=existing;
            if(!s.size.isEmpty()&&s.size!=existing.size) updated.size=s.size;
            if(!s.year.isEmpty()&&s.year!=existing.year) updated.year=s.year;
            if(!s.hardDisk.isEmpty()&&s.hardDisk!=existing.hardDisk) updated.hardDisk=s.hardDisk;
            if(s.category!=existing.category) updated.category=s.category;
            if(updateSeries(updated)) count++;
        }
    }
    m_db.commit(); return count;
}

int Database::countAll() const {
    QSqlQuery q(m_db); q.exec("SELECT COUNT(*) FROM series");
    return q.next()?q.value(0).toInt():0;
}

int Database::countByCategory(const QString &cat) const {
    QSqlQuery q(m_db); q.prepare("SELECT COUNT(*) FROM series WHERE category=:cat");
    q.bindValue(":cat",cat); q.exec(); return q.next()?q.value(0).toInt():0;
}

double Database::totalSizeGB() const {
    QSqlQuery q(m_db); q.exec("SELECT size FROM series");
    double total=0.0;
    while(q.next()){
        QString sz=q.value(0).toString().toLower().trimmed();
        if(sz.isEmpty()) continue;
        QString numStr=sz; numStr.remove(QRegularExpression("[^0-9.]"));
        bool ok; double val=numStr.toDouble(&ok);
        if(!ok) continue;
        if(sz.contains("tb")) val*=1024.0;
        else if(sz.contains("mb")) val/=1024.0;
        total+=val;
    }
    return total;
}

QStringList Database::allHardDisks() const {
    QStringList list; QSqlQuery q(m_db);
    q.exec("SELECT DISTINCT hard_disk FROM series WHERE hard_disk!='' ORDER BY hard_disk");
    while(q.next()) list.append(q.value(0).toString());
    return list;
}
