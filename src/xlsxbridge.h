#pragma once
#include <QObject>
#include <QList>
#include <QString>
#include "database.h"

class XlsxBridge : public QObject {
    Q_OBJECT
public:
    explicit XlsxBridge(QObject *parent=nullptr);
    QList<SeriesData> importFromXlsx(const QString &filePath, QString &error);
    bool exportToXlsx(const QString &filePath, const QList<SeriesData> &data, QString &error);
    static QString pythonExecutable();
private:
    bool runPython(const QString &script, const QStringList &args, QString &output, QString &error);
};
