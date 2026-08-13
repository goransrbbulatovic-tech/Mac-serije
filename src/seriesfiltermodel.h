#pragma once
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QString>

enum TableCol {
    COL_ID=0, COL_NAME, COL_CATEGORY, COL_YEAR, COL_SIZE,
    COL_HDD, COL_RATING, COL_GENRE, COL_WATCHED, COL_COUNT
};

class SeriesFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SeriesFilterModel(QObject *parent=nullptr);
    void setSearch(const QString &text);
    void setCategoryFilter(const QString &cat);
    void setHddFilter(const QString &hdd);
    void setMinRating(double min);
    void setMaxRating(double max);
    void setWatchedFilter(int state);
protected:
    bool filterAcceptsRow(int srcRow, const QModelIndex &srcParent) const override;
private:
    QString m_search, m_category, m_hdd;
    double m_minRating=0.0, m_maxRating=10.0;
    int m_watchedState=-1;
};
