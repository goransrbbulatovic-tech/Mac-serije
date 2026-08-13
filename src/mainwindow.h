#pragma once
#include <QMainWindow>
#include <QTableView>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QTabWidget>
#include <QSplitter>
#include <QTextBrowser>
#include <QSlider>
#include <QCheckBox>
#include <QTimer>

#include "database.h"
#include "imdbfetcher.h"
#include "xlsxbridge.h"
#include "seriesfiltermodel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Actions
    void onAddSeries();
    void onEditSeries();
    void onDeleteSeries();
    void onToggleWatched();

    // Import / Export
    void onImportXlsx();
    void onUpdateXlsx();
    void onExportXlsx();

    // IMDB
    void onFetchImdb();
    void onFetchAllImdb();
    void onImdbResult(ImdbResult result);
    void onImdbBatchProgress(int done, int total);
    void onImdbBatchComplete();

    // Search & Filter
    void onSearchChanged(const QString &text);
    void onCategoryChanged(int idx);
    void onHddFilterChanged(int idx);
    void onRatingFilterChanged();
    void onWatchedFilterChanged(int idx);

    // Table interaction
    void onTableDoubleClicked(const QModelIndex &idx);
    void onSelectionChanged();

    // Settings
    void onSettings();

    // Timer-based search debounce
    void doSearch();

private:
    // UI setup
    void setupUi();
    QWidget* createHeader();
    QWidget* createToolbar();
    QWidget* createFilterBar();
    QWidget* createDetailPanel();
    QWidget* createStatBar();
    void     setupMenu();
    void     setupStatusBar();

    // Data
    void loadAllData();
    void refreshTable(const QList<SeriesData> &data);
    void populateHddCombo();
    void updateStats();
    void updateDetailPanel(const SeriesData &s);
    void clearDetailPanel();

    SeriesData getSelectedSeries() const;
    int        getSelectedId() const;

    // Table model
    QStandardItemModel  *m_model  = nullptr;
    SeriesFilterModel   *m_filter = nullptr;
    QTableView          *m_table  = nullptr;

    // Detail panel widgets
    QLabel      *m_detailName     = nullptr;
    QLabel      *m_detailCategory = nullptr;
    QLabel      *m_detailYear     = nullptr;
    QLabel      *m_detailSize     = nullptr;
    QLabel      *m_detailHdd      = nullptr;
    QLabel      *m_detailRating   = nullptr;
    QLabel      *m_detailGenre    = nullptr;
    QLabel      *m_detailDirector = nullptr;
    QTextBrowser*m_detailPlot     = nullptr;
    QLabel      *m_detailActors   = nullptr;
    QLabel      *m_detailCountry  = nullptr;
    QLabel      *m_posterLabel    = nullptr;
    QLabel      *m_detailWatched  = nullptr;

    // Filter widgets
    QLineEdit   *m_searchInput = nullptr;
    QComboBox   *m_catFilter   = nullptr;
    QComboBox   *m_hddFilter   = nullptr;
    QComboBox   *m_watchFilter = nullptr;
    QSlider     *m_minRatingSlider = nullptr;
    QSlider     *m_maxRatingSlider = nullptr;
    QLabel      *m_ratingRangeLabel = nullptr;

    // Stat labels
    QLabel      *m_statTotal    = nullptr;
    QLabel      *m_statDomace   = nullptr;
    QLabel      *m_statStrane   = nullptr;
    QLabel      *m_statSize     = nullptr;
    QLabel      *m_statRated    = nullptr;

    // Progress
    QProgressBar *m_imdbProgress = nullptr;
    QLabel       *m_imdbStatus   = nullptr;

    // Core objects
    Database     *m_db      = nullptr;
    ImdbFetcher  *m_fetcher = nullptr;
    XlsxBridge   *m_xlsx    = nullptr;

    // Search debounce
    QTimer *m_searchTimer = nullptr;

    // Poster downloader
    QNetworkAccessManager *m_posterNam = nullptr;
    int m_currentDetailId = -1;

    // IMDB batch counters
    int m_imdbSuccessCount = 0;
    int m_imdbFailCount    = 0;
};
