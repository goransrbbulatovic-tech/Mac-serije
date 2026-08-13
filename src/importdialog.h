#pragma once
#include <QDialog>
#include <QLabel>
#include <QRadioButton>
#include <QListWidget>
#include "database.h"

class ImportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImportDialog(QWidget *parent, const QList<SeriesData> &pending, bool isUpdate=false);
    bool shouldUpdate() const;
private:
    void setupUi();
    QList<SeriesData> m_pending;
    bool m_isUpdate;
    QRadioButton *m_addOnly, *m_addAndUpdate;
    QLabel *m_summaryLabel;
    QListWidget *m_previewList;
};
