#include "importdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>

ImportDialog::ImportDialog(QWidget *parent, const QList<SeriesData> &pending, bool isUpdate)
    : QDialog(parent), m_pending(pending), m_isUpdate(isUpdate) {
    setWindowTitle(isUpdate?"Uvezi Izmjene (Update)":"Uvezi Serije iz Excel-a");
    setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(560); setMinimumHeight(500);
    setupUi();
}

bool ImportDialog::shouldUpdate() const { return m_addAndUpdate&&m_addAndUpdate->isChecked(); }

void ImportDialog::setupUi() {
    auto *mainLayout=new QVBoxLayout(this);
    mainLayout->setSpacing(0); mainLayout->setContentsMargins(0,0,0,0);

    auto *header=new QWidget; header->setObjectName("dialogHeader");
    auto *hlay=new QHBoxLayout(header);
    auto *icon=new QLabel(m_isUpdate?"UPDATE - UVOZ IZMJENA":"UVOZ IZ EXCEL FAJLA");
    icon->setStyleSheet("color:#ffffff;font-size:14px;font-weight:bold;letter-spacing:2px;");
    hlay->addWidget(icon); mainLayout->addWidget(header);

    auto *content=new QWidget; content->setStyleSheet("background:#12121f;padding:16px 20px;");
    auto *cLayout=new QVBoxLayout(content); cLayout->setSpacing(12);

    m_summaryLabel=new QLabel(QString("Pronadeno <b style='color:#6c63ff'>%1 serija</b> u Excel fajlu").arg(m_pending.size()));
    m_summaryLabel->setStyleSheet("color:#e0e0f0;font-size:13px;padding:8px;");
    m_summaryLabel->setTextFormat(Qt::RichText); cLayout->addWidget(m_summaryLabel);

    auto *modeGroup=new QGroupBox("NACIN UVOZA"); auto *modeLayout=new QVBoxLayout(modeGroup);
    m_addOnly=new QRadioButton("  Dodaj samo NOVE serije (postojece ostaju nepromijenjene)");
    m_addOnly->setChecked(true); m_addOnly->setStyleSheet("color:#e0e0f0;font-size:12px;padding:4px;");
    m_addAndUpdate=new QRadioButton("  Dodaj nove + AZURIRAJ postojece (velicina, HDD, godina)");
    m_addAndUpdate->setStyleSheet("color:#e0e0f0;font-size:12px;padding:4px;");
    if(m_isUpdate) m_addAndUpdate->setChecked(true);
    modeLayout->addWidget(m_addOnly); modeLayout->addWidget(m_addAndUpdate);
    cLayout->addWidget(modeGroup);

    auto *previewLabel=new QLabel("PREGLED SERIJA IZ FAJLA:");
    previewLabel->setObjectName("labelTitle"); cLayout->addWidget(previewLabel);

    m_previewList=new QListWidget; m_previewList->setMaximumHeight(220);
    m_previewList->setStyleSheet("QListWidget{background:#1a1a2e;border:1px solid #2a2a45;border-radius:6px;}"
        "QListWidget::item{padding:5px 10px;border-bottom:1px solid #1e1e35;}"
        "QListWidget::item:alternate{background:#141428;}"
        "QListWidget::item:hover{background:#1e1e40;}");
    m_previewList->setAlternatingRowColors(true);

    int domCount=0, strCount=0;
    for(const auto &s:m_pending){
        QString hdd=s.hardDisk.isEmpty()?"":QString(" - %1").arg(s.hardDisk);
        m_previewList->addItem(QString("[%1] %2 (%3)%4")
            .arg(s.category=="DOMACE"?"DOM":"STR",s.name,s.year,hdd));
        if(s.category=="DOMACE") domCount++; else strCount++;
    }
    auto *statsLbl=new QLabel(QString("  Domace: <b>%1</b>   Strane: <b>%2</b>").arg(domCount).arg(strCount));
    statsLbl->setStyleSheet("color:#9999cc;font-size:12px;");
    statsLbl->setTextFormat(Qt::RichText);
    cLayout->addWidget(statsLbl); cLayout->addWidget(m_previewList);
    mainLayout->addWidget(content,1);

    auto *btnWidget=new QWidget;
    btnWidget->setStyleSheet("background:#0d0d1a;border-top:1px solid #2a2a45;padding:10px 16px;");
    auto *btnLayout=new QHBoxLayout(btnWidget);
    auto *cancelBtn=new QPushButton("Otkazi"); cancelBtn->setMinimumHeight(36);
    auto *importBtn=new QPushButton("Uvezi");
    importBtn->setObjectName("btnPrimary"); importBtn->setMinimumHeight(36); importBtn->setMinimumWidth(120);
    btnLayout->addStretch(); btnLayout->addWidget(cancelBtn); btnLayout->addWidget(importBtn);
    mainLayout->addWidget(btnWidget);
    connect(cancelBtn,&QPushButton::clicked,this,&QDialog::reject);
    connect(importBtn,&QPushButton::clicked,this,&QDialog::accept);
}
