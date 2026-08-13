#include "mainwindow.h"
#include "seriesdialog.h"
#include "importdialog.h"
#include "settingsdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QStandardItem>
#include <QHeaderView>
#include <QApplication>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QBuffer>
#include <QMenu>
#include <QInputDialog>
#include <QRegularExpression>
#include <QUrl>
#include <QDebug>

// ─────────────────────────────────────────────────────────────
//  Load data & table refresh
// ─────────────────────────────────────────────────────────────
void MainWindow::loadAllData() {
    refreshTable(m_db->getAllSeries());
    populateHddCombo();
    updateStats();
}

void MainWindow::refreshTable(const QList<SeriesData> &data) {
    m_model->removeRows(0, m_model->rowCount());

    for (const SeriesData &s : data) {
        QList<QStandardItem*> row;

        auto makeItem = [](const QString &text, const QVariant &userData = QVariant()) {
            auto *it = new QStandardItem(text);
            it->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            if (userData.isValid()) it->setData(userData, Qt::UserRole);
            return it;
        };

        // ID (hidden)
        auto *idItem = makeItem(QString::number(s.id), s.id);
        row.append(idItem);

        // Name
        auto *nameItem = makeItem(s.name);
        nameItem->setData(s.id, Qt::UserRole);
        row.append(nameItem);

        // Category badge
        QString catText = (s.category == "DOMACE") ? "🇷🇸 DOM" : "🌍 STR";
        auto *catItem = makeItem(catText, s.category);
        catItem->setTextAlignment(Qt::AlignCenter);
        if (s.category == "DOMACE")
            catItem->setForeground(QColor("#2ed573"));
        else
            catItem->setForeground(QColor("#6c63ff"));
        row.append(catItem);

        // Year
        row.append(makeItem(s.year));

        // Size
        row.append(makeItem(s.size));

        // HDD
        row.append(makeItem(s.hardDisk));

        // IMDB Rating
        QString ratingText;
        QColor  ratingColor;
        if (s.imdbRating > 0) {
            ratingText  = QString("%1").arg(s.imdbRating, 0, 'f', 1);
            ratingColor = s.imdbRating >= 7.5 ? QColor("#f5c518")
                        : s.imdbRating >= 6.0 ? QColor("#ffa502")
                                               : QColor("#ff4757");
        } else {
            ratingText  = "—";
            ratingColor = QColor("#444466");
        }
        auto *ratingItem = makeItem(ratingText, s.imdbRating);
        ratingItem->setForeground(ratingColor);
        ratingItem->setTextAlignment(Qt::AlignCenter);
        if (s.imdbRating >= 7.5) {
            QFont f; f.setBold(true);
            ratingItem->setFont(f);
        }
        row.append(ratingItem);

        // Genre
        row.append(makeItem(s.genre));

        // Watched
        QString watchedText = s.watched ? "✅" : "";
        auto *watchedItem = makeItem(watchedText, s.watched);
        watchedItem->setTextAlignment(Qt::AlignCenter);
        row.append(watchedItem);

        m_model->appendRow(row);
    }

    updateStats();
    statusBar()->showMessage(
        QString("Učitano %1 serija  ·  Vidljivo: %2")
            .arg(data.size())
            .arg(m_filter->rowCount()),
        5000
    );
}

void MainWindow::populateHddCombo() {
    QString current = m_hddFilter->currentData().toString();
    m_hddFilter->blockSignals(true);
    m_hddFilter->clear();
    m_hddFilter->addItem("💾  Svi diskovi", "");
    for (const QString &hdd : m_db->allHardDisks())
        m_hddFilter->addItem("🖴  " + hdd, hdd);
    int idx = m_hddFilter->findData(current);
    if (idx >= 0) m_hddFilter->setCurrentIndex(idx);
    m_hddFilter->blockSignals(false);
}

void MainWindow::updateStats() {
    m_statTotal ->setText(QString::number(m_db->countAll()));
    m_statDomace->setText(QString::number(m_db->countByCategory("DOMACE")));
    m_statStrane->setText(QString::number(m_db->countByCategory("STRANE")));

    double gb = m_db->totalSizeGB();
    m_statSize->setText(gb >= 1000 ?
        QString("%1 TB").arg(gb/1024.0, 0, 'f', 1) :
        QString("%1 GB").arg(gb, 0, 'f', 0));

    // Count rated
    int rated = 0;
    for (int r = 0; r < m_model->rowCount(); r++) {
        if (m_model->item(r, COL_RATING) &&
            m_model->item(r, COL_RATING)->data(Qt::UserRole).toDouble() > 0)
            rated++;
    }
    m_statRated->setText(QString::number(rated));
}

// ─────────────────────────────────────────────────────────────
//  CRUD Actions
// ─────────────────────────────────────────────────────────────
void MainWindow::onAddSeries() {
    SeriesDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    SeriesData s = dlg.getData();
    int id = m_db->addSeries(s);
    if (id < 0) {
        QMessageBox::warning(this, "Greška", "Nije moguće dodati seriju:\n" + m_db->lastError());
        return;
    }
    loadAllData();
    statusBar()->showMessage("✅  Serija dodana: " + s.name, 4000);
}

void MainWindow::onEditSeries() {
    SeriesData s = getSelectedSeries();
    if (s.id == 0) { QMessageBox::information(this, "Info", "Odaberite seriju."); return; }
    SeriesDialog dlg(this, s);
    if (dlg.exec() != QDialog::Accepted) return;
    SeriesData updated = dlg.getData();
    if (!m_db->updateSeries(updated)) {
        QMessageBox::warning(this, "Greška", "Greška pri čuvanju:\n" + m_db->lastError());
        return;
    }
    loadAllData();
    statusBar()->showMessage("✅  Sačuvano: " + updated.name, 4000);
}

void MainWindow::onDeleteSeries() {
    SeriesData s = getSelectedSeries();
    if (s.id == 0) return;
    auto res = QMessageBox::question(this, "Potvrda brisanja",
        QString("Jeste li sigurni da želite obrisati:\n\n\"%1\"?").arg(s.name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (res != QMessageBox::Yes) return;
    m_db->deleteSeries(s.id);
    loadAllData();
    clearDetailPanel();
    statusBar()->showMessage("🗑  Obrisano: " + s.name, 4000);
}

void MainWindow::onToggleWatched() {
    SeriesData s = getSelectedSeries();
    if (s.id == 0) return;
    s.watched = !s.watched;
    m_db->updateSeries(s);
    loadAllData();
    statusBar()->showMessage(
        s.watched ? "✅  Označeno kao gledano: " + s.name
                  : "⬜  Označeno kao negledano: " + s.name,
        3000
    );
}

// ─────────────────────────────────────────────────────────────
//  Import / Export
// ─────────────────────────────────────────────────────────────
void MainWindow::onImportXlsx() {
    QString path = QFileDialog::getOpenFileName(
        this, "Odaberi Excel fajl", QDir::homePath(),
        "Excel fajlovi (*.xlsx *.xls);;Svi fajlovi (*)"
    );
    if (path.isEmpty()) return;

    statusBar()->showMessage("⏳  Učitavam Excel fajl...");
    qApp->processEvents();

    QString error;
    QList<SeriesData> imported = m_xlsx->importFromXlsx(path, error);

    if (!error.isEmpty()) {
        QMessageBox::critical(this, "Greška pri uvozu", error);
        return;
    }
    if (imported.isEmpty()) {
        QMessageBox::information(this, "Info", "Nema podataka u fajlu.");
        return;
    }

    ImportDialog dlg(this, imported, false);
    if (dlg.exec() != QDialog::Accepted) return;

    int count = m_db->importSeries(imported, dlg.shouldUpdate());
    loadAllData();
    QMessageBox::information(this, "Uvoz završen",
        QString("✅  Uvezeno %1 novih serija.\n(Ukupno u bazi: %2)")
            .arg(count).arg(m_db->countAll()));
    statusBar()->showMessage(QString("✅  Uvezeno %1 serija").arg(count), 5000);
}

void MainWindow::onUpdateXlsx() {
    QString path = QFileDialog::getOpenFileName(
        this, "Odaberi Excel fajl za Update", QDir::homePath(),
        "Excel fajlovi (*.xlsx *.xls);;Svi fajlovi (*)"
    );
    if (path.isEmpty()) return;

    statusBar()->showMessage("⏳  Učitavam Excel fajl...");
    qApp->processEvents();

    QString error;
    QList<SeriesData> imported = m_xlsx->importFromXlsx(path, error);

    if (!error.isEmpty()) {
        QMessageBox::critical(this, "Greška pri uvozu", error);
        return;
    }

    ImportDialog dlg(this, imported, true);
    if (dlg.exec() != QDialog::Accepted) return;

    int count = m_db->importSeries(imported, true);
    loadAllData();
    QMessageBox::information(this, "Update završen",
        QString("🔄  Ažurirano/dodano %1 serija.\n(Ukupno u bazi: %2)")
            .arg(count).arg(m_db->countAll()));
    statusBar()->showMessage(QString("🔄  Update završen: %1 izmjena").arg(count), 5000);
}

void MainWindow::onExportXlsx() {
    QString path = QFileDialog::getSaveFileName(
        this, "Sačuvaj Excel fajl", QDir::homePath() + "/AcmigoIndexer_Export.xlsx",
        "Excel fajlovi (*.xlsx)"
    );
    if (path.isEmpty()) return;
    if (!path.endsWith(".xlsx", Qt::CaseInsensitive)) path += ".xlsx";

    statusBar()->showMessage("⏳  Izvozim u Excel...");
    qApp->processEvents();

    QList<SeriesData> all = m_db->getAllSeries();
    QString error;
    if (!m_xlsx->exportToXlsx(path, all, error)) {
        QMessageBox::critical(this, "Greška pri izvozu", error);
        return;
    }
    QMessageBox::information(this, "Izvoz završen",
        QString("✅  Izvezeno %1 serija u:\n%2").arg(all.size()).arg(path));
    statusBar()->showMessage("✅  Excel izvoz završen: " + path, 5000);
}

// ─────────────────────────────────────────────────────────────
//  IMDB
// ─────────────────────────────────────────────────────────────
void MainWindow::onFetchImdb() {
    if (!m_fetcher->hasApiKey()) {
        onSettings();
        if (!m_fetcher->hasApiKey()) return;
    }
    SeriesData s = getSelectedSeries();
    if (s.id == 0) { QMessageBox::information(this,"Info","Odaberite seriju."); return; }

    statusBar()->showMessage("⏳  Preuzimam IMDB ocjenu za: " + s.name);
    m_fetcher->fetchSingle(s.id, s.name, s.year);
}

void MainWindow::onFetchAllImdb() {
    if (!m_fetcher->hasApiKey()) {
        QMessageBox::warning(this, "API Ključ",
            "API ključ nije postavljen!\n\n"
            "Idite na ⚙ Podešavanja i unesite vaš OMDb API ključ.\n"
            "Besplatan ključ: https://www.omdbapi.com/apikey.aspx");
        onSettings();
        if (!m_fetcher->hasApiKey()) return;
    }
    // Show current API key status
    statusBar()->showMessage("API ključ: " + m_fetcher->apiKey().left(4) + "****  — Preuzimam...", 3000);

    auto all = m_db->getAllSeries();
    QList<FetchRequest> reqs;
    for (const auto &s : all) {
        if (s.imdbRating <= 0)   // Only fetch those without rating
            reqs.append({s.id, s.name, s.year});
    }

    if (reqs.isEmpty()) {
        auto res = QMessageBox::question(this, "IMDB Batch",
            "Sve serije već imaju ocjene. Osvježiti sve?",
            QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
        if (res == QMessageBox::No) return;
        for (const auto &s : all) reqs.append({s.id, s.name, s.year});
    }

    auto res = QMessageBox::question(this, "IMDB Batch Preuzimanje",
        QString("Preuzeti IMDB ocjene za %1 serija?\n\n"
                "Ovo može potrajati nekoliko minuta.").arg(reqs.size()),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if (res != QMessageBox::Yes) return;

    m_imdbProgress->setVisible(true);
    m_imdbProgress->setRange(0, reqs.size());
    m_imdbProgress->setValue(0);
    m_imdbStatus->setText(QString("⏳  Preuzimam IMDB ocjene (0/%1)...").arg(reqs.size()));

    m_imdbSuccessCount = 0;
    m_imdbFailCount = 0;
    m_fetcher->fetchBatch(reqs);
}

void MainWindow::onImdbResult(ImdbResult result) {
    if (!result.success) {
        m_imdbFailCount++;
        // Show first error so user knows what's happening
        if (m_imdbFailCount <= 3) {
            statusBar()->showMessage(
                QString("IMDB greška (%1): %2").arg(m_imdbFailCount).arg(result.error), 5000);
        }
        return;
    }

    // Save to database
    m_db->updateImdb(result.seriesId, result.rating, result.imdbId,
                     result.genre, result.director, result.actors,
                     result.plot, result.country, result.language,
                     result.awards, result.posterUrl);

    m_imdbSuccessCount++;

    // Update table row directly in model (fast, no full reload needed)
    for (int r = 0; r < m_model->rowCount(); r++) {
        // Check COL_NAME UserRole (stores series ID)
        auto *nameItem = m_model->item(r, COL_NAME);
        if (!nameItem) continue;
        if (nameItem->data(Qt::UserRole).toInt() != result.seriesId) continue;

        // Update rating cell
        auto *ratingItem = m_model->item(r, COL_RATING);
        if (ratingItem) {
            if (result.rating > 0) {
                ratingItem->setText(QString("%1").arg(result.rating, 0, 'f', 1));
                ratingItem->setData(result.rating, Qt::UserRole);
                QColor c = result.rating >= 7.5 ? QColor("#f5c518")
                         : result.rating >= 6.0 ? QColor("#ffa502")
                                                 : QColor("#ff4757");
                ratingItem->setForeground(c);
                if (result.rating >= 7.5) { QFont f; f.setBold(true); ratingItem->setFont(f); }
            } else {
                ratingItem->setText("N/A");
                ratingItem->setData(0.0, Qt::UserRole);
                ratingItem->setForeground(QColor("#444466"));
            }
        }
        // Update genre cell
        if (m_model->item(r, COL_GENRE) && !result.genre.isEmpty())
            m_model->item(r, COL_GENRE)->setText(result.genre);
        break;
    }

    // Refresh detail panel if this series is selected
    if (m_currentDetailId == result.seriesId)
        updateDetailPanel(m_db->getSeriesById(result.seriesId));

    updateStats();
    if (result.rating > 0)
        statusBar()->showMessage(
            QString("⭐  %1 — %2/10").arg(result.title).arg(result.rating, 0,'f',1), 3000);
}

void MainWindow::onImdbBatchProgress(int done, int total) {
    m_imdbProgress->setValue(done);
    m_imdbStatus->setText(QString("⏳  Preuzimam IMDB ocjene (%1/%2)...").arg(done).arg(total));
}

void MainWindow::onImdbBatchComplete() {
    m_imdbProgress->setVisible(false);
    loadAllData();

    QString msg;
    if (m_imdbSuccessCount == 0 && m_imdbFailCount > 0) {
        msg = QString(
            "⚠️  Nije pronađena ni jedna ocjena!\n\n"
            "Mogući razlozi:\n"
            "• API ključ nije ispravan ili je istekao\n"
            "• Nema internet konekcije\n"
            "• OMDb server nedostupan\n\n"
            "Provjerite API ključ u ⚙ Podešavanjima.\n"
            "Pogledajte statusnu traku za detalje greške.");
        m_imdbStatus->setText("❌  Greška — provjerite API ključ");
    } else {
        msg = QString("✅  Pronađeno: %1\n❌  Nije pronađeno: %2\n\n"
                      "Domaće serije često nisu na OMDb bazi.")
            .arg(m_imdbSuccessCount).arg(m_imdbFailCount);
        m_imdbStatus->setText(QString("✅  %1 pronađeno, %2 nije pronađeno")
            .arg(m_imdbSuccessCount).arg(m_imdbFailCount));
    }

    QMessageBox::information(this, "IMDB Preuzimanje Završeno", msg);
    m_imdbSuccessCount = 0;
    m_imdbFailCount = 0;
}

// ─────────────────────────────────────────────────────────────
//  Search & Filter
// ─────────────────────────────────────────────────────────────
void MainWindow::onSearchChanged(const QString &) {
    m_searchTimer->start();
}

void MainWindow::doSearch() {
    m_filter->setSearch(m_searchInput->text().trimmed());
    statusBar()->showMessage(
        QString("Pronađeno: %1 serija").arg(m_filter->rowCount()), 3000);
}

void MainWindow::onCategoryChanged(int) {
    m_filter->setCategoryFilter(m_catFilter->currentData().toString());
}

void MainWindow::onHddFilterChanged(int) {
    m_filter->setHddFilter(m_hddFilter->currentData().toString());
}

void MainWindow::onRatingFilterChanged() {
    double minR = m_minRatingSlider->value() / 10.0;
    double maxR = m_maxRatingSlider->value() / 10.0;
    if (minR > maxR) {
        m_minRatingSlider->blockSignals(true);
        m_minRatingSlider->setValue(m_maxRatingSlider->value());
        m_minRatingSlider->blockSignals(false);
        minR = maxR;
    }
    m_ratingRangeLabel->setText(QString("%1–%2").arg(minR,0,'f',1).arg(maxR,0,'f',1));
    m_filter->setMinRating(minR);
    m_filter->setMaxRating(maxR);
}

void MainWindow::onWatchedFilterChanged(int) {
    int state = m_watchFilter->currentData().toInt();
    m_filter->setWatchedFilter(state);
}

// ─────────────────────────────────────────────────────────────
//  Table interaction
// ─────────────────────────────────────────────────────────────
void MainWindow::onTableDoubleClicked(const QModelIndex &) {
    onEditSeries();
}

void MainWindow::onSelectionChanged() {
    SeriesData s = getSelectedSeries();
    if (s.id == 0) { clearDetailPanel(); return; }
    m_currentDetailId = s.id;
    updateDetailPanel(s);
}

void MainWindow::updateDetailPanel(const SeriesData &s) {
    m_detailName->setText(s.name);

    if (s.category == "DOMACE") {
        m_detailCategory->setText("🇷🇸  DOMAĆA SERIJA");
        m_detailCategory->setStyleSheet("background:#1a3a1a; color:#2ed573; border:1px solid #2ed573;"
                                         "border-radius:4px; padding:3px 10px; font-size:11px; font-weight:bold;");
    } else {
        m_detailCategory->setText("🌍  STRANA SERIJA");
        m_detailCategory->setStyleSheet("background:#1a1a3a; color:#6c63ff; border:1px solid #6c63ff;"
                                         "border-radius:4px; padding:3px 10px; font-size:11px; font-weight:bold;");
    }

    if (s.watched) {
        m_detailWatched->setText("✅  GLEDANO");
        m_detailWatched->setStyleSheet("background:#1a2a1a; color:#2ed573; border:1px solid #2ed573;"
                                        "border-radius:4px; padding:3px 8px; font-size:11px;");
        m_detailWatched->setVisible(true);
    } else {
        m_detailWatched->setVisible(false);
    }

    if (s.imdbRating > 0) {
        m_detailRating->setText(QString("⭐  %1 / 10").arg(s.imdbRating, 0, 'f', 1));
        QString color = s.imdbRating >= 7.5 ? "#f5c518"
                      : s.imdbRating >= 6.0 ? "#ffa502" : "#ff4757";
        m_detailRating->setStyleSheet(QString("color:%1; font-size:22px; font-weight:bold; padding:4px;").arg(color));
    } else {
        m_detailRating->setText("─  Nema ocjene");
        m_detailRating->setStyleSheet("color:#444466; font-size:14px; padding:4px;");
    }

    m_detailYear->setText(s.year.isEmpty() ? "—" : s.year);
    m_detailSize->setText(s.size.isEmpty() ? "—" : s.size);
    m_detailHdd ->setText(s.hardDisk.isEmpty() ? "—" : s.hardDisk);
    m_detailGenre->setText(s.genre.isEmpty() ? "—" : s.genre);
    m_detailDirector->setText(s.director.isEmpty() ? "—" : s.director);
    m_detailCountry->setText(s.country.isEmpty() ? "—" : s.country);

    m_detailActors->setText(s.actors.isEmpty() ? "" :
        "👥  " + s.actors.split(",").first().trimmed() + " i dr.");

    m_detailPlot->setPlainText(s.plot.isEmpty() ? "Nema opisa. Klikni ⭐ IMDB za preuzimanje." : s.plot);

    // Load poster
    if (!s.posterUrl.isEmpty() && s.posterUrl != "N/A") {
        QNetworkRequest req(QUrl(s.posterUrl));
        auto *reply = m_posterNam->get(req);
        connect(reply, &QNetworkReply::finished, [this, reply]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pix;
                pix.loadFromData(reply->readAll());
                if (!pix.isNull()) {
                    m_posterLabel->setPixmap(
                        pix.scaled(m_posterLabel->width(), m_posterLabel->height(),
                                   Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
        });
    } else {
        m_posterLabel->setPixmap(QPixmap());
        m_posterLabel->setText("🎬");
    }
}

void MainWindow::clearDetailPanel() {
    m_currentDetailId = -1;
    m_detailName->setText("─  Odaberite seriju");
    m_detailCategory->setText("");
    m_detailWatched->setVisible(false);
    m_detailRating->setText("");
    m_detailYear->setText("—");
    m_detailSize->setText("—");
    m_detailHdd->setText("—");
    m_detailGenre->setText("—");
    m_detailDirector->setText("—");
    m_detailCountry->setText("—");
    m_detailActors->clear();
    m_detailPlot->clear();
    m_posterLabel->setPixmap(QPixmap());
    m_posterLabel->setText("🎬");
}

// ─────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────
SeriesData MainWindow::getSelectedSeries() const {
    auto sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) return SeriesData{};
    QModelIndex srcIdx = m_filter->mapToSource(sel.first());
    int id = m_model->item(srcIdx.row(), COL_NAME)
                     ->data(Qt::UserRole).toInt();
    return m_db->getSeriesById(id);
}

int MainWindow::getSelectedId() const {
    return getSelectedSeries().id;
}

void MainWindow::onSettings() {
    SettingsDialog dlg(this, m_fetcher->apiKey());
    if (dlg.exec() != QDialog::Accepted) return;
    QString key = dlg.apiKey();
    m_fetcher->setApiKey(key);
    SettingsDialog::saveApiKey(key);
    statusBar()->showMessage("✅  API ključ sačuvan", 3000);
}
