#include "seriesfiltermodel.h"
#include <QStandardItem>

SeriesFilterModel::SeriesFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void SeriesFilterModel::setSearch(const QString &text){m_search=text;invalidateFilter();}
void SeriesFilterModel::setCategoryFilter(const QString &cat){m_category=cat;invalidateFilter();}
void SeriesFilterModel::setHddFilter(const QString &hdd){m_hdd=hdd;invalidateFilter();}
void SeriesFilterModel::setMinRating(double min){m_minRating=min;invalidateFilter();}
void SeriesFilterModel::setMaxRating(double max){m_maxRating=max;invalidateFilter();}
void SeriesFilterModel::setWatchedFilter(int state){m_watchedState=state;invalidateFilter();}

bool SeriesFilterModel::filterAcceptsRow(int srcRow, const QModelIndex &srcParent) const {
    auto *src=sourceModel(); if(!src) return true;
    auto idx=[&](int col){return src->index(srcRow,col,srcParent);};
    if(!m_category.isEmpty()){
        QString cat=src->data(idx(COL_CATEGORY),Qt::UserRole).toString();
        if(cat.isEmpty()) cat=src->data(idx(COL_CATEGORY)).toString();
        if(!cat.contains(m_category,Qt::CaseInsensitive)) return false;
    }
    if(!m_hdd.isEmpty()){
        if(!src->data(idx(COL_HDD)).toString().contains(m_hdd,Qt::CaseInsensitive)) return false;
    }
    double rating=src->data(idx(COL_RATING),Qt::UserRole).toDouble();
    if(m_minRating>0||m_maxRating<10){if(rating<m_minRating||rating>m_maxRating) return false;}
    if(m_watchedState>=0){
        bool watched=src->data(idx(COL_WATCHED),Qt::UserRole).toBool();
        if((m_watchedState==1)!=watched) return false;
    }
    if(!m_search.isEmpty()){
        bool found=false;
        for(int col:{COL_NAME,COL_GENRE,COL_YEAR,COL_HDD,COL_CATEGORY})
            if(src->data(idx(col)).toString().contains(m_search,Qt::CaseInsensitive)){found=true;break;}
        if(!found) return false;
    }
    return true;
}
