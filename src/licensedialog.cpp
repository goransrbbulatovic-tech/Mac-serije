#include "licensedialog.h"
#include "licensemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include <QTimer>

LicenseDialog::LicenseDialog(QWidget *parent, bool firstRun) : QDialog(parent) {
    setWindowTitle("Acmigo Indexer — Aktivacija");
    setWindowFlags(Qt::Dialog|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
    setFixedWidth(520); setModal(true);
    setupUi(firstRun);
}

void LicenseDialog::setupUi(bool firstRun) {
    auto *mainLayout=new QVBoxLayout(this);
    mainLayout->setSpacing(0); mainLayout->setContentsMargins(0,0,0,0);

    auto *header=new QWidget; header->setObjectName("dialogHeader"); header->setFixedHeight(90);
    auto *hl=new QVBoxLayout(header); hl->setContentsMargins(24,16,24,16);
    auto *titleRow=new QHBoxLayout;
    auto *iconLbl=new QLabel("[A]");
    iconLbl->setStyleSheet("font-size:28px;");
    auto *titleLbl=new QLabel("ACMIGO INDEXER");
    titleLbl->setStyleSheet("color:#ffffff;font-size:20px;font-weight:bold;letter-spacing:3px;");
    auto *subLbl=new QLabel("Aktivacija softvera");
    subLbl->setStyleSheet("color:#9999cc;font-size:11px;letter-spacing:2px;");
    titleRow->addWidget(iconLbl); titleRow->addSpacing(10);
    auto *titCol=new QVBoxLayout; titCol->addWidget(titleLbl); titCol->addWidget(subLbl);
    titleRow->addLayout(titCol); titleRow->addStretch();
    hl->addLayout(titleRow); mainLayout->addWidget(header);

    auto *content=new QWidget; content->setStyleSheet("background:#12121f;padding:0;");
    auto *cl=new QVBoxLayout(content); cl->setContentsMargins(24,20,24,20); cl->setSpacing(16);

    if(firstRun){
        auto *infoBox=new QWidget;
        infoBox->setStyleSheet("background:#1a1a2e;border:1px solid #2a2a45;border-radius:8px;padding:14px;");
        auto *ibl=new QVBoxLayout(infoBox);
        auto *infoTitle=new QLabel("i  Za pokretanje potrebna je licenca");
        infoTitle->setStyleSheet("color:#6c63ff;font-size:12px;font-weight:bold;");
        auto *infoText=new QLabel("Unesite serijski broj koji ste dobili uz softver.\nFormat: XXXXX-XXXXX-XXXXX-XXXXX");
        infoText->setStyleSheet("color:#9999cc;font-size:12px;"); infoText->setWordWrap(true);
        ibl->addWidget(infoTitle); ibl->addWidget(infoText); cl->addWidget(infoBox);
    } else {
        QString saved=LicenseManager::instance().savedSerial();
        auto *statusBox=new QWidget;
        statusBox->setStyleSheet("background:#1a2e1a;border:1px solid #2ed573;border-radius:8px;padding:14px;");
        auto *sbl=new QHBoxLayout(statusBox);
        auto *checkLbl=new QLabel("OK"); checkLbl->setStyleSheet("font-size:20px; color:#2ed573;");
        auto *stxt=new QLabel(QString("Softver je aktiviran\n%1").arg(saved));
        stxt->setStyleSheet("color:#2ed573;font-size:12px;");
        sbl->addWidget(checkLbl); sbl->addWidget(stxt,1); cl->addWidget(statusBox);
    }

    auto *serialLabel=new QLabel("SERIJSKI BROJ:");
    serialLabel->setStyleSheet("color:#9999cc;font-size:11px;font-weight:bold;letter-spacing:1px;");
    cl->addWidget(serialLabel);

    m_serialInput=new QLineEdit;
    m_serialInput->setPlaceholderText("XXXXX-XXXXX-XXXXX-XXXXX");
    m_serialInput->setMinimumHeight(44); m_serialInput->setMaxLength(23);
    m_serialInput->setStyleSheet("QLineEdit{font-size:16px;letter-spacing:2px;font-family:'Courier New',monospace;}");
    cl->addWidget(m_serialInput);

    m_statusLabel=new QLabel(" ");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size:12px;padding:4px;");
    cl->addWidget(m_statusLabel);
    mainLayout->addWidget(content,1);

    auto *btnWidget=new QWidget;
    btnWidget->setStyleSheet("background:#0d0d1a;border-top:1px solid #2a2a45;padding:12px 20px;");
    auto *bl=new QHBoxLayout(btnWidget);

    if(!firstRun){
        auto *exitBtn=new QPushButton("Zatvori"); exitBtn->setMinimumHeight(38);
        bl->addWidget(exitBtn);
        connect(exitBtn,&QPushButton::clicked,this,&QDialog::accept);
    } else {
        auto *quitBtn=new QPushButton("Izlaz"); quitBtn->setMinimumHeight(38);
        connect(quitBtn,&QPushButton::clicked,qApp,&QApplication::quit);
        bl->addWidget(quitBtn);
    }
    bl->addStretch();
    m_activateBtn=new QPushButton("Aktiviraj");
    m_activateBtn->setObjectName("btnPrimary");
    m_activateBtn->setMinimumHeight(38); m_activateBtn->setMinimumWidth(150);
    m_activateBtn->setEnabled(false); bl->addWidget(m_activateBtn);
    mainLayout->addWidget(btnWidget);

    connect(m_activateBtn,&QPushButton::clicked,this,&LicenseDialog::onActivate);
    connect(m_serialInput,&QLineEdit::textChanged,this,&LicenseDialog::onSerialChanged);
    connect(m_serialInput,&QLineEdit::textEdited,[this](const QString &text){
        QString clean;
        for(QChar c:text) if(c.isLetterOrNumber()) clean+=c.toUpper();
        clean=clean.left(20);
        QString formatted;
        for(int i=0;i<clean.length();i++){if(i>0&&i%5==0)formatted+="-";formatted+=clean[i];}
        m_serialInput->blockSignals(true);
        m_serialInput->setText(formatted);
        m_serialInput->setCursorPosition(formatted.length());
        m_serialInput->blockSignals(false);
    });
}

void LicenseDialog::onSerialChanged(const QString &text) {
    QString stripped=LicenseManager::stripSerial(text);
    bool complete=stripped.length()==20;
    m_activateBtn->setEnabled(complete);
    if(complete){
        bool valid=LicenseManager::validateSerial(stripped);
        if(valid){m_statusLabel->setText("Serijski broj je ispravan");
            m_statusLabel->setStyleSheet("color:#2ed573;font-size:12px;padding:4px;");}
        else{m_statusLabel->setText("Neispravan serijski broj");
            m_statusLabel->setStyleSheet("color:#ff4757;font-size:12px;padding:4px;");}
    } else {
        m_statusLabel->setText(QString("Uneseno %1/20 znakova").arg(stripped.length()));
        m_statusLabel->setStyleSheet("color:#666699;font-size:11px;padding:4px;");
    }
}

void LicenseDialog::onActivate() {
    auto status=LicenseManager::instance().activate(m_serialInput->text());
    switch(status){
    case LicenseManager::Activated:
        m_statusLabel->setText("Aktivacija uspjesna! Dobrodosli!");
        m_statusLabel->setStyleSheet("color:#2ed573;font-size:13px;font-weight:bold;padding:4px;");
        m_activateBtn->setEnabled(false); m_serialInput->setEnabled(false);
        QTimer::singleShot(1500,this,&QDialog::accept);
        break;
    case LicenseManager::InvalidSerial:
        m_statusLabel->setText("Pogresan serijski broj. Pokusajte ponovo.");
        m_statusLabel->setStyleSheet("color:#ff4757;font-size:12px;padding:4px;");
        m_serialInput->selectAll();
        break;
    default: break;
    }
}
