#include "catalog/Catalog.h"
#include "catalog/ThumbnailCache.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("Svitlo");
    QApplication::setApplicationName("Svitlo");
    QApplication::setApplicationVersion(SVITLO_VERSION);

    svitlo::theme::apply(app);

    // macOS: ~/Library/Application Support/Svitlo/Svitlo and ~/Library/Caches/Svitlo/Svitlo
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    svitlo::Catalog catalog;
    QString error;
    if (!catalog.open(QDir(dataDir).filePath("catalog.sqlite"), &error)) {
        QMessageBox::critical(nullptr, "Svitlo", QObject::tr("Cannot open the catalog:\n%1").arg(error));
        return 1;
    }
    svitlo::ThumbnailCache thumbs(QDir(cacheDir).filePath("thumbnails"));

    svitlo::MainWindow window(&catalog, &thumbs);
    window.show();
    return app.exec();
}
