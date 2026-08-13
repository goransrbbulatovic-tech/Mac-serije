#include "mainwindow.h"
#include "seriesdialog.h"
#include "importdialog.h"
#include "settingsdialog.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QPushButton>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItem>
#include <QHeaderView>
#include <QSplitter>
#include <QScrollArea>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QFont>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>

// ─────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ─────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Acmigo Indexer - Serija v2.0");
    setMinimumSize(1200, 720);
    resize(1440, 860);

    m_db      = new Database(this);
    m_fetcher = new ImdbFetcher(this);
    m_xlsx    = new XlsxBridge(this);
    m_posterNam = new QNetworkAccessManager(this);
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);

    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbPath);
    if (!m_db->open(dbPath + "/series.db")) {
        QMessageBox::critical(this, "Greška", "Nije moguće otvoriti bazu podataka:\n" + m_db->lastError());
    }

    // Load saved API key
    QString apiKey = SettingsDialog::loadApiKey();
    m_fetcher->setApiKey(apiKey);
    QTimer::singleShot(2000, this, [this, apiKey](){
        if (apiKey.isEmpty()) {
            statusBar()->showMessage(
                "⚠  API ključ nije postavljen — idite na ⚙ Podešavanja", 10000);
        } else {
            statusBar()->showMessage(
                QString("✅  API ključ učitan (%1 znakova)  — Spreman").arg(apiKey.length()), 5000);
        }
    });

    setupUi();
    setupMenu();
    setupStatusBar();
    loadAllData();

    connect(m_fetcher, &ImdbFetcher::resultReady,     this, &MainWindow::onImdbResult);
    connect(m_fetcher, &ImdbFetcher::batchProgress,   this, &MainWindow::onImdbBatchProgress);
    connect(m_fetcher, &ImdbFetcher::batchComplete,   this, &MainWindow::onImdbBatchComplete);
    connect(m_fetcher, &ImdbFetcher::errorOccurred,   this, [this](const QString &e){
        statusBar()->showMessage("IMDB greška: " + e, 5000);
    });
    connect(m_searchTimer, &QTimer::timeout, this, &MainWindow::doSearch);
}

MainWindow::~MainWindow() {
    m_db->close();
}

// ─────────────────────────────────────────────────────────────
//  UI Setup
// ─────────────────────────────────────────────────────────────
void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    mainLayout->addWidget(createHeader());
    mainLayout->addWidget(createToolbar());
    mainLayout->addWidget(createFilterBar());

    // Main splitter: table | detail panel
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(3);
    splitter->setStyleSheet("QSplitter::handle { background: #2a2a40; }");

    // Table area
    auto *tableWidget = new QWidget;
    auto *tableLayout = new QVBoxLayout(tableWidget);
    tableLayout->setContentsMargins(0,0,0,0);
    tableLayout->setSpacing(0);

    m_model  = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"ID","NAZIV SERIJE","KAT.","GODINA","VELIČINA","HARD DISK","IMDB","ŽANR","GLEDANO"});

    m_filter = new SeriesFilterModel(this);
    m_filter->setSourceModel(m_model);
    m_filter->setDynamicSortFilter(true);

    m_table = new QTableView;
    m_table->setModel(m_filter);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(32);
    m_table->setShowGrid(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    // Column widths
    m_table->setColumnHidden(0, true); // hide ID
    m_table->horizontalHeader()->resizeSection(1, 280);
    m_table->horizontalHeader()->resizeSection(2, 80);
    m_table->horizontalHeader()->resizeSection(3, 110);
    m_table->horizontalHeader()->resizeSection(4, 85);
    m_table->horizontalHeader()->resizeSection(5, 130);
    m_table->horizontalHeader()->resizeSection(6, 70);
    m_table->horizontalHeader()->resizeSection(7, 160);
    m_table->horizontalHeader()->resizeSection(8, 75);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->sortByColumn(1, Qt::AscendingOrder);

    tableLayout->addWidget(m_table);
    tableLayout->addWidget(createStatBar());

    // IMDB progress bar
    auto *progressWidget = new QWidget;
    progressWidget->setStyleSheet("background:#0a0a18; padding:0 8px;");
    progressWidget->setFixedHeight(36);
    auto *pLayout = new QHBoxLayout(progressWidget);
    pLayout->setContentsMargins(8,4,8,4);
    m_imdbStatus   = new QLabel("Spreman");
    m_imdbStatus->setStyleSheet("color:#666699; font-size:11px;");
    m_imdbProgress = new QProgressBar;
    m_imdbProgress->setVisible(false);
    m_imdbProgress->setFixedHeight(10);
    m_imdbProgress->setTextVisible(false);
    pLayout->addWidget(m_imdbStatus);
    pLayout->addWidget(m_imdbProgress, 1);
    tableLayout->addWidget(progressWidget);

    splitter->addWidget(tableWidget);
    splitter->addWidget(createDetailPanel());
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({1000, 380});

    mainLayout->addWidget(splitter, 1);

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);
    connect(m_table, &QTableView::doubleClicked,
            this, &MainWindow::onTableDoubleClicked);
    connect(m_table, &QTableView::customContextMenuRequested,
            [this](const QPoint &pos){
                Q_UNUSED(pos);
                if (getSelectedId() > 0) onEditSeries();
            });
}

QWidget* MainWindow::createHeader() {
    auto *w = new QWidget;
    w->setObjectName("headerWidget");
    w->setFixedHeight(72);
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(20, 8, 20, 8);

    // App icon + title
    auto *titleCol = new QVBoxLayout;
    auto *titleLbl = new QLabel("🎬  ACMIGO INDEXER — SERIJA");
    titleLbl->setObjectName("appTitle");
    auto *subLbl = new QLabel("VAŠA KOLEKCIJA SERIJA — KATALOG & IMDB");
    subLbl->setObjectName("appSubtitle");
    titleCol->addWidget(titleLbl);
    titleCol->addWidget(subLbl);
    l->addLayout(titleCol);
    l->addStretch();

    // Quick stat chips in header
    m_statTotal  = new QLabel("0");
    m_statDomace = new QLabel("0");
    m_statStrane = new QLabel("0");
    m_statSize   = new QLabel("0 GB");
    m_statRated  = new QLabel("0");

    auto addChip = [&](const QString &icon, QLabel *val, const QString &sub) {
        auto *chip = new QWidget;
        chip->setObjectName("statCard");
        auto *cl = new QVBoxLayout(chip);
        cl->setSpacing(2);
        cl->setContentsMargins(12,8,12,8);
        auto *row = new QHBoxLayout;
        auto *ic = new QLabel(icon); ic->setStyleSheet("font-size:16px;");
        val->setObjectName("statValue");
        val->setStyleSheet("color:#6c63ff; font-size:18px; font-weight:bold;");
        row->addWidget(ic); row->addWidget(val);
        auto *sl = new QLabel(sub); sl->setObjectName("statLabel");
        cl->addLayout(row); cl->addWidget(sl);
        l->addWidget(chip);
    };

    addChip("📋", m_statTotal,  "Ukupno");
    addChip("🇷🇸", m_statDomace, "Domaće");
    addChip("🌍", m_statStrane, "Strane");
    addChip("💾", m_statSize,   "Ukupno GB");
    addChip("⭐", m_statRated,  "Sa ocjenom");

    return w;
}

QWidget* MainWindow::createToolbar() {
    auto *w = new QWidget;
    w->setStyleSheet("background:#16162a; border-bottom:1px solid #2a2a40;");
    w->setFixedHeight(52);
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(12,6,12,6);
    l->setSpacing(6);

    auto makeBtn = [&](const QString &text, const QString &objName,
                        const QString &tip, auto slot) {
        auto *b = new QPushButton(text);
        b->setObjectName(objName);
        b->setToolTip(tip);
        b->setMinimumHeight(36);
        b->setMinimumWidth(110);
        connect(b, &QPushButton::clicked, this, slot);
        l->addWidget(b);
        return b;
    };

    makeBtn("➕  Dodaj",    "btnPrimary",  "Dodaj novu seriju ručno",          &MainWindow::onAddSeries);
    makeBtn("✏  Uredi",    "",            "Uredi odabranu seriju",             &MainWindow::onEditSeries);
    makeBtn("🗑  Obriši",  "btnDanger",   "Obriši odabranu seriju",            &MainWindow::onDeleteSeries);
    makeBtn("👁  Gledano", "btnSuccess",  "Označi kao gledano/negledano",      &MainWindow::onToggleWatched);

    auto *sep1 = new QFrame; sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet("color:#2a2a45;"); l->addWidget(sep1);

    makeBtn("📂  Uvezi",   "",            "Uvezi serije iz Excel (.xlsx) fajla", &MainWindow::onImportXlsx);
    makeBtn("🔄  Update",  "btnWarning",  "Ažuriraj izmjene iz Excel fajla",     &MainWindow::onUpdateXlsx);
    makeBtn("📤  Izvezi",  "btnSuccess",  "Izvezi kolekciju u Excel (.xlsx)",    &MainWindow::onExportXlsx);

    auto *sep2 = new QFrame; sep2->setFrameShape(QFrame::VLine);
    sep2->setStyleSheet("color:#2a2a45;"); l->addWidget(sep2);

    makeBtn("⭐  IMDB",     "btnImdb",     "Preuzmi IMDB ocjenu za odabranu seriju", &MainWindow::onFetchImdb);
    makeBtn("⭐  IMDB Sve", "btnImdb",     "Preuzmi IMDB ocjene za sve serije (batch)", &MainWindow::onFetchAllImdb);

    l->addStretch();

    auto *settingsBtn = new QPushButton("⚙  Podešavanja");
    settingsBtn->setMinimumHeight(36);
    settingsBtn->setToolTip("API ključ i podešavanja");
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::onSettings);
    l->addWidget(settingsBtn);

    return w;
}

QWidget* MainWindow::createFilterBar() {
    auto *w = new QWidget;
    w->setStyleSheet("background:#13132a; border-bottom:2px solid #1e1e38; padding:0;");
    w->setFixedHeight(52);
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(12,6,12,6);
    l->setSpacing(10);

    // Search
    auto *searchFrame = new QFrame;
    searchFrame->setObjectName("searchFrame");
    searchFrame->setFixedHeight(36);
    auto *sl = new QHBoxLayout(searchFrame);
    sl->setContentsMargins(4,0,8,0);
    sl->setSpacing(4);
    auto *searchIcon = new QLabel("🔍");
    searchIcon->setStyleSheet("font-size:14px; padding:4px;");
    m_searchInput = new QLineEdit;
    m_searchInput->setObjectName("searchInput");
    m_searchInput->setPlaceholderText("Pretraga po nazivu, žanru, godini, HDD...");
    m_searchInput->setFixedHeight(32);
    sl->addWidget(searchIcon);
    sl->addWidget(m_searchInput, 1);
    auto *clearBtn = new QPushButton("✕");
    clearBtn->setFixedSize(24,24);
    clearBtn->setStyleSheet("QPushButton{background:transparent;color:#666699;border:none;font-size:12px;}"
                             "QPushButton:hover{color:#ff4757;}");
    connect(clearBtn, &QPushButton::clicked, m_searchInput, &QLineEdit::clear);
    sl->addWidget(clearBtn);
    l->addWidget(searchFrame, 2);

    // Category filter
    m_catFilter = new QComboBox;
    m_catFilter->setFixedHeight(36);
    m_catFilter->setMinimumWidth(140);
    m_catFilter->addItem("🎬  Sve serije",   "");
    m_catFilter->addItem("🌍  Strane",       "STRANE");
    m_catFilter->addItem("🇷🇸  Domaće",      "DOMACE");
    l->addWidget(m_catFilter);

    // HDD filter
    m_hddFilter = new QComboBox;
    m_hddFilter->setFixedHeight(36);
    m_hddFilter->setMinimumWidth(155);
    m_hddFilter->addItem("💾  Svi diskovi", "");
    l->addWidget(m_hddFilter);

    // Watched filter
    m_watchFilter = new QComboBox;
    m_watchFilter->setFixedHeight(36);
    m_watchFilter->setMinimumWidth(130);
    m_watchFilter->addItem("👁  Sve",         "-1");
    m_watchFilter->addItem("✅  Gledano",      "1");
    m_watchFilter->addItem("⬜  Negledano",    "0");
    l->addWidget(m_watchFilter);

    // Rating range
    auto *ratingWidget = new QWidget;
    ratingWidget->setStyleSheet("background:transparent;");
    auto *rl = new QHBoxLayout(ratingWidget);
    rl->setContentsMargins(4,0,4,0);
    rl->setSpacing(4);
    auto *rIcon = new QLabel("⭐"); rIcon->setStyleSheet("font-size:12px;");
    m_minRatingSlider = new QSlider(Qt::Horizontal);
    m_minRatingSlider->setRange(0, 100);
    m_minRatingSlider->setValue(0);
    m_minRatingSlider->setFixedWidth(80);
    m_maxRatingSlider = new QSlider(Qt::Horizontal);
    m_maxRatingSlider->setRange(0, 100);
    m_maxRatingSlider->setValue(100);
    m_maxRatingSlider->setFixedWidth(80);
    m_ratingRangeLabel = new QLabel("0.0–10.0");
    m_ratingRangeLabel->setStyleSheet("color:#9999cc; font-size:11px; min-width:65px;");
    rl->addWidget(rIcon);
    rl->addWidget(m_minRatingSlider);
    rl->addWidget(m_maxRatingSlider);
    rl->addWidget(m_ratingRangeLabel);
    l->addWidget(ratingWidget);

    l->addStretch();

    // Connections
    connect(m_searchInput,    &QLineEdit::textChanged,
            this, &MainWindow::onSearchChanged);
    connect(m_catFilter,      QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCategoryChanged);
    connect(m_hddFilter,      QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onHddFilterChanged);
    connect(m_watchFilter,    QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onWatchedFilterChanged);
    connect(m_minRatingSlider,&QSlider::valueChanged, this, &MainWindow::onRatingFilterChanged);
    connect(m_maxRatingSlider,&QSlider::valueChanged, this, &MainWindow::onRatingFilterChanged);

    return w;
}

QWidget* MainWindow::createDetailPanel() {
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setMinimumWidth(320);
    scroll->setMaximumWidth(440);
    scroll->setStyleSheet("QScrollArea{border:none; background:#0e0e1c;}"
                          "QScrollArea > QWidget > QWidget{background:#0e0e1c;}");

    auto *panel = new QWidget;
    panel->setStyleSheet("background:#0e0e1c;");
    auto *l = new QVBoxLayout(panel);
    l->setContentsMargins(14,14,14,14);
    l->setSpacing(10);
    l->setAlignment(Qt::AlignTop);

    // Poster
    m_posterLabel = new QLabel;
    m_posterLabel->setObjectName("posterLabel");
    m_posterLabel->setAlignment(Qt::AlignCenter);
    m_posterLabel->setFixedHeight(200);
    m_posterLabel->setText("🎬");
    m_posterLabel->setStyleSheet("#posterLabel{background:#1a1a2e; border:2px solid #2a2a45; "
                                  "border-radius:8px; color:#444466; font-size:48px;}");
    l->addWidget(m_posterLabel);

    // Name
    m_detailName = new QLabel("—");
    m_detailName->setWordWrap(true);
    m_detailName->setAlignment(Qt::AlignCenter);
    m_detailName->setStyleSheet("color:#ffffff; font-size:16px; font-weight:bold; padding:4px;");
    l->addWidget(m_detailName);

    // Category + Watched badge
    auto *badgeRow = new QHBoxLayout;
    m_detailCategory = new QLabel;
    m_detailCategory->setAlignment(Qt::AlignCenter);
    m_detailWatched = new QLabel;
    m_detailWatched->setAlignment(Qt::AlignCenter);
    badgeRow->addStretch();
    badgeRow->addWidget(m_detailCategory);
    badgeRow->addWidget(m_detailWatched);
    badgeRow->addStretch();
    l->addLayout(badgeRow);

    // Rating
    m_detailRating = new QLabel;
    m_detailRating->setAlignment(Qt::AlignCenter);
    m_detailRating->setStyleSheet("color:#f5c518; font-size:22px; font-weight:bold; padding:4px;");
    l->addWidget(m_detailRating);

    // Separator
    auto addSep = [&]() {
        auto *sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color:#2a2a40;");
        l->addWidget(sep);
    };

    addSep();

    // Info rows
    auto addInfoRow = [&](const QString &icon, const QString &label, QLabel *&valLbl) {
        auto *row = new QWidget;
        row->setStyleSheet("background:#1a1a2e; border-radius:6px; padding:4px 8px;");
        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(8,4,8,4);
        rl->setSpacing(8);
        auto *ic = new QLabel(icon + "  " + label);
        ic->setStyleSheet("color:#666699; font-size:11px; font-weight:bold; letter-spacing:0.5px;");
        ic->setFixedWidth(90);
        valLbl = new QLabel("—");
        valLbl->setStyleSheet("color:#e0e0f0; font-size:12px;");
        valLbl->setWordWrap(true);
        rl->addWidget(ic);
        rl->addWidget(valLbl, 1);
        l->addWidget(row);
    };

    addInfoRow("📅","GODINA",    m_detailYear);
    addInfoRow("💾","VELIČINA",  m_detailSize);
    addInfoRow("🖴","HARD DISK", m_detailHdd);
    addInfoRow("🎭","ŽANR",      m_detailGenre);
    addInfoRow("🎬","REŽISER",   m_detailDirector);
    addInfoRow("🌍","ZEMLJA",    m_detailCountry);

    addSep();

    // Actors
    m_detailActors = new QLabel("—");
    m_detailActors->setWordWrap(true);
    m_detailActors->setStyleSheet("color:#9999cc; font-size:11px; font-style:italic; padding:4px;");
    l->addWidget(m_detailActors);

    // Plot
    auto *plotLabel = new QLabel("OPIS:");
    plotLabel->setObjectName("labelTitle");
    l->addWidget(plotLabel);

    m_detailPlot = new QTextBrowser;
    m_detailPlot->setMinimumHeight(90);
    m_detailPlot->setMaximumHeight(130);
    m_detailPlot->setStyleSheet("color:#aaaacc; font-size:11px; background:#151525; "
                                 "border:1px solid #2a2a40; border-radius:6px;");
    m_detailPlot->setOpenExternalLinks(true);
    l->addWidget(m_detailPlot);

    l->addStretch();
    scroll->setWidget(panel);
    return scroll;
}

QWidget* MainWindow::createStatBar() {
    auto *w = new QWidget;
    w->setStyleSheet("background:#0a0a18; border-top:1px solid #1e1e38;");
    w->setFixedHeight(28);
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(12,2,12,2);
    l->setSpacing(20);

    auto addStat = [&](const QString &text) {
        auto *lbl = new QLabel(text);
        lbl->setStyleSheet("color:#555566; font-size:11px;");
        l->addWidget(lbl);
        return lbl;
    };

    addStat("Prikazano:");
    // These will be updated dynamically via status bar
    l->addStretch();
    return w;
}

void MainWindow::setupMenu() {
    auto *menuBar = this->menuBar();

    auto *fileMenu = menuBar->addMenu("&Fajl");
    fileMenu->addAction("📂  Uvezi Excel",  this, &MainWindow::onImportXlsx, QKeySequence("Ctrl+O"));
    fileMenu->addAction("🔄  Update Excel", this, &MainWindow::onUpdateXlsx, QKeySequence("Ctrl+U"));
    fileMenu->addAction("📤  Izvezi Excel", this, &MainWindow::onExportXlsx, QKeySequence("Ctrl+E"));
    fileMenu->addSeparator();
    fileMenu->addAction("⚙  Podešavanja",  this, &MainWindow::onSettings,   QKeySequence("Ctrl+,"));
    fileMenu->addSeparator();
    fileMenu->addAction("❌  Izlaz", qApp, &QApplication::quit, QKeySequence("Alt+F4"));

    auto *editMenu = menuBar->addMenu("&Uredi");
    editMenu->addAction("➕  Dodaj seriju",  this, &MainWindow::onAddSeries,    QKeySequence("Ctrl+N"));
    editMenu->addAction("✏  Uredi seriju",  this, &MainWindow::onEditSeries,   QKeySequence("F2"));
    editMenu->addAction("🗑  Obriši seriju", this, &MainWindow::onDeleteSeries, QKeySequence("Delete"));
    editMenu->addSeparator();
    editMenu->addAction("👁  Toggle Gledano", this, &MainWindow::onToggleWatched, QKeySequence("Space"));

    auto *imdbMenu = menuBar->addMenu("&IMDB");
    imdbMenu->addAction("⭐  Preuzmi IMDB (odabrana)", this, &MainWindow::onFetchImdb,    QKeySequence("Ctrl+I"));
    imdbMenu->addAction("⭐  Preuzmi IMDB (sve)",      this, &MainWindow::onFetchAllImdb, QKeySequence("Ctrl+Shift+I"));
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("Spreman — Dobrodošli u Acmigo Indexer - Serija v2.0");
    statusBar()->setStyleSheet("background:#0a0a18; color:#666699; font-size:11px;");
}
