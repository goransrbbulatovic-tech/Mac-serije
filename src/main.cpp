#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QStyleFactory>
#include <QTimer>
#include <QDebug>
#include "mainwindow.h"
#include "licensemanager.h"
#include "licensedialog.h"

int main(int argc, char *argv[]) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication app(argc, argv);
    app.setApplicationName("AcmigoIndexer");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("AcmigoIndexer");
    app.setStyle(QStyleFactory::create("Fusion"));

    QFile styleFile(":/resources/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
    }
    app.setWindowIcon(QIcon(":/resources/acmigo.ico"));

    if (!LicenseManager::instance().isActivated()) {
        LicenseDialog licDlg(nullptr, true);
        QObject::connect(&licDlg, &QDialog::rejected, &app, &QApplication::quit);
        if (licDlg.exec() != QDialog::Accepted) return 0;
        if (!LicenseManager::instance().isActivated()) return 0;
    }

    MainWindow w;
    w.show();
    return app.exec();
}
