#include "ui/MainWindow.h"

#include "catalog/Catalog.h"
#include "catalog/ThumbnailCache.h"
#include "core/Exporter.h"
#include "ui/DevelopView.h"
#include "ui/ExportDialog.h"
#include "ui/LicensesDialog.h"
#include "ui/SupportDialog.h"
#include "ui/UpdateChecker.h"
#include "ui/Language.h"
#include "ui/LibraryView.h"
#include "ui/PhotoModel.h"
#include "ui/Theme.h"

#include <QAction>
#include <QActionGroup>
#include <QProcess>
#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QLabel>
#include <QListView>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QDir>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentMap>

#include <algorithm>

namespace svitlo {

MainWindow::MainWindow(Catalog* catalog, ThumbnailCache* thumbs, QWidget* parent)
    : QMainWindow(parent), m_catalog(catalog), m_thumbs(thumbs) {
    setWindowTitle("Svitlo");
    setUnifiedTitleAndToolBarOnMac(true);

    m_model = new PhotoModel(m_catalog, m_thumbs, this);
    m_updates = new UpdateChecker(this);
    m_library = new LibraryView(m_model);
    m_develop = new DevelopView(m_catalog, m_thumbs, m_model);

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_library);
    m_stack->addWidget(m_develop);
    setCentralWidget(m_stack);

    // --- Module bar ---------------------------------------------------------------
    auto* top = new QToolBar;
    top->setMovable(false);
    top->setFloatable(false);
    auto* brand = new QLabel(QStringLiteral("Svitlo"));
    brand->setObjectName("brand"); // styled in Theme.cpp
    brand->setContentsMargins(10, 0, 10, 0);
    top->addWidget(brand);
    auto* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    top->addWidget(spacer);

    m_libraryTab = new QToolButton;
    m_libraryTab->setText(tr("Library"));
    m_developTab = new QToolButton;
    m_developTab->setText(tr("Develop"));
    auto* group = new QButtonGroup(this);
    for (QToolButton* b : {m_libraryTab, m_developTab}) {
        b->setObjectName("moduleTab");
        b->setCheckable(true);
        group->addButton(b);
        top->addWidget(b);
    }
    addToolBar(Qt::TopToolBarArea, top);

    connect(m_libraryTab, &QToolButton::clicked, this, [this] { showModule(Module::Library); });
    connect(m_developTab, &QToolButton::clicked, this, [this] { showModule(Module::Develop); });

    connect(m_library, &LibraryView::openInDevelop, this, [this](qint64 id) {
        m_develop->openPhoto(id);
        showModule(Module::Develop);
    });
    connect(m_library, &LibraryView::importRequested, this, &MainWindow::importFolder);
    connect(m_library, &LibraryView::removeFolderRequested, this, &MainWindow::removeFolder);
    connect(m_develop, &DevelopView::photoOpened, m_library, &LibraryView::selectId);
    connect(m_develop, &DevelopView::exportRequested, this, &MainWindow::exportPhotos);
    connect(m_develop, &DevelopView::copySettingsRequested, this, &MainWindow::copySettings);
    connect(m_develop, &DevelopView::pasteSettingsRequested, this, &MainWindow::pasteSettings);

    buildMenus();
    statusBar()->showMessage(ImageLoader::rawSupportCompiled()
                                 ? tr("Ready")
                                 : tr("RAW support is not available in this build (LibRaw missing)"));

    QSettings s;
    if (!restoreGeometry(s.value("window/geometry").toByteArray()))
        resize(1440, 900);
    showModule(Module::Library);
}

void MainWindow::buildMenus() {
    auto add = [this](QMenu* menu, const QString& text, const QKeySequence& key, auto&& fn) {
        QAction* a = menu->addAction(text);
        if (!key.isEmpty())
            a->setShortcut(key);
        connect(a, &QAction::triggered, this, fn);
        return a;
    };

    QMenu* file = menuBar()->addMenu(tr("&File"));
    add(file, tr("Import Folder…"), QKeySequence(QStringLiteral("Ctrl+Shift+I")), [this] { importFolder(); });
    QAction* exportAction =
        add(file, tr("Export…"), QKeySequence(QStringLiteral("Ctrl+Shift+E")), [this] { exportPhotos(); });
    file->addSeparator();
    add(file, tr("Remove Folder from Catalog…"), QKeySequence(), [this] {
        const QString folder = m_library->currentFolder();
        if (folder.isEmpty() || inDevelop())
            statusBar()->showMessage(tr("Select a folder in the Library's folder list first"), 5000);
        else
            removeFolder(folder);
    });
    file->addSeparator();
    // Explicit roles: on macOS Qt otherwise guesses from the (English) text,
    // which fails once the menu is translated.
    add(file, tr("Quit"), QKeySequence::Quit, [this] { close(); })->setMenuRole(QAction::QuitRole);

    QMenu* edit = menuBar()->addMenu(tr("&Edit"));
    add(edit, tr("Undo"), QKeySequence::Undo, [this] { if (inDevelop()) m_develop->undo(); });
    add(edit, tr("Redo"), QKeySequence::Redo, [this] { if (inDevelop()) m_develop->redo(); });
    edit->addSeparator();
    add(edit, tr("Copy Settings"), QKeySequence(QStringLiteral("Ctrl+Shift+C")), [this] { copySettings(); });
    add(edit, tr("Paste Settings"), QKeySequence(QStringLiteral("Ctrl+Shift+V")), [this] { pasteSettings(); });
    add(edit, tr("Reset Settings"), QKeySequence(QStringLiteral("Ctrl+Shift+R")), [this] {
        if (inDevelop()) {
            m_develop->resetAll();
        } else {
            for (qint64 id : targetIds()) {
                EditSettings s;
                s.orientationQuarterTurns = EditSettings::fromJsonString(m_catalog->photo(id).settingsJson)
                                                .orientationQuarterTurns;
                m_catalog->setSettings(id, s);
            }
        }
    });
    edit->addSeparator();
    add(edit, tr("Select All"), QKeySequence::SelectAll, [this] {
        if (!inDevelop()) m_library->grid()->selectAll();
    });

    QMenu* photo = menuBar()->addMenu(tr("&Photo"));
    for (int r = 0; r <= 5; ++r)
        add(photo, r == 0 ? tr("Clear Rating") : tr("Rate %1").arg(r), QKeySequence(QString::number(r)),
            [this, r] { setRating(r); });
    photo->addSeparator();
    add(photo, tr("Pick"), QKeySequence("P"), [this] { setFlag(1); });
    add(photo, tr("Reject"), QKeySequence("X"), [this] { setFlag(-1); });
    add(photo, tr("Unflag"), QKeySequence("U"), [this] { setFlag(0); });
    photo->addSeparator();
    add(photo, tr("Rotate Left"), QKeySequence(QStringLiteral("Ctrl+[")), [this] { rotateSelection(-1); });
    add(photo, tr("Rotate Right"), QKeySequence(QStringLiteral("Ctrl+]")), [this] { rotateSelection(1); });
    photo->addSeparator();
    QAction* removeAction =
        add(photo, tr("Remove from Catalog…"), QKeySequence(Qt::Key_Backspace), [this] { removeSelection(); });

    // Right-click menu of the thumbnail grid.
    QListView* grid = m_library->grid();
    auto* openAction = new QAction(tr("Open in Develop"), grid);
    connect(openAction, &QAction::triggered, this, [this] {
        if (const qint64 id = m_library->currentId()) {
            m_develop->openPhoto(id);
            showModule(Module::Develop);
        }
    });
    auto* separator = new QAction(grid);
    separator->setSeparator(true);
    grid->setContextMenuPolicy(Qt::ActionsContextMenu);
    grid->addActions({openAction, exportAction, separator, removeAction});

    QMenu* view = menuBar()->addMenu(tr("&View"));
    add(view, tr("Library"), QKeySequence("G"), [this] { showModule(Module::Library); });
    add(view, tr("Develop"), QKeySequence("D"), [this] {
        const qint64 id = m_library->currentId();
        if (id) m_develop->openPhoto(id);
        showModule(Module::Develop);
    });
    view->addSeparator();
    add(view, tr("Before / After"), QKeySequence("\\"), [this] { if (inDevelop()) m_develop->toggleBefore(); });
    add(view, tr("Crop Tool"), QKeySequence("C"), [this] { if (inDevelop()) m_develop->toggleCropMode(); });
    add(view, tr("Toggle Zoom"), QKeySequence("Z"), [this] { if (inDevelop()) m_develop->toggleZoom(); });
    add(view, tr("Next Photo"), QKeySequence(Qt::Key_Right), [this] { if (inDevelop()) m_develop->step(1); });
    add(view, tr("Previous Photo"), QKeySequence(Qt::Key_Left), [this] { if (inDevelop()) m_develop->step(-1); });
    view->addSeparator();

    // Language: "System" follows the OS; changing it needs a restart.
    QMenu* langMenu = view->addMenu(tr("Language"));
    auto* langGroup = new QActionGroup(this);
    const QString saved = language::savedChoice();
    auto addLang = [&](const QString& code, const QString& name) {
        QAction* a = langMenu->addAction(name);
        a->setCheckable(true);
        a->setChecked(code == saved);
        a->setMenuRole(QAction::NoRole);
        langGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, code] { changeLanguage(code); });
    };
    addLang(QString(), tr("System Default"));
    langMenu->addSeparator();
    for (const language::Info& l : language::supported())
        addLang(l.code, l.nativeName);

    // Appearance: switches immediately, "System" follows the OS live.
    QMenu* themeMenu = view->addMenu(tr("Appearance"));
    auto* themeGroup = new QActionGroup(this);
    auto addTheme = [&](theme::Mode mode, const QString& name) {
        QAction* a = themeMenu->addAction(name);
        a->setCheckable(true);
        a->setChecked(mode == theme::savedMode());
        a->setMenuRole(QAction::NoRole);
        themeGroup->addAction(a);
        connect(a, &QAction::triggered, this, [mode] {
            theme::saveMode(mode);
            theme::apply(*qApp);
        });
    };
    addTheme(theme::Mode::System, tr("Match System"));
    themeMenu->addSeparator();
    addTheme(theme::Mode::Dark, tr("Dark"));
    addTheme(theme::Mode::Light, tr("Light"));

    QMenu* help = menuBar()->addMenu(tr("&Help"));
    add(help, tr("Support Svitlo…"), QKeySequence(), [this] {
        SupportDialog(SupportDialog::Mode::Manual, this).exec();
    });
    add(help, tr("Licenses…"), QKeySequence(), [this] { LicensesDialog(this).exec(); });
    help->addSeparator();
    add(help, tr("Check for Updates…"), QKeySequence(), [this] { m_updates->checkNow(); })
        ->setMenuRole(QAction::NoRole);
    QAction* autoCheck = help->addAction(tr("Check for Updates at Startup"));
    autoCheck->setCheckable(true);
    autoCheck->setChecked(UpdateChecker::checkAtStartupEnabled());
    autoCheck->setMenuRole(QAction::NoRole);
    connect(autoCheck, &QAction::toggled, this, [](bool on) { UpdateChecker::setCheckAtStartupEnabled(on); });
    help->addSeparator();
    QAction* about = add(help, tr("About Svitlo"), QKeySequence(), [this] {
        QMessageBox::about(this, tr("About Svitlo"),
                           tr("<h3>Svitlo %1</h3>"
                              "<p>A free, open-source photo library and non-destructive editor.</p>"
                              "<p>Built with Qt %2%3.</p>")
                               .arg(QApplication::applicationVersion(), QT_VERSION_STR,
                                    ImageLoader::rawSupportCompiled() ? tr(" and LibRaw") : QString()));
    });
    about->setMenuRole(QAction::AboutRole);
}

void MainWindow::changeLanguage(const QString& code) {
    if (code == language::savedChoice())
        return;
    language::saveChoice(code);

    QMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Language"));
    box.setText(tr("The new language will be used after Svitlo restarts."));
    QPushButton* restart = box.addButton(tr("Restart Now"), QMessageBox::AcceptRole);
    box.addButton(tr("Later"), QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == restart) {
        m_develop->flush();
        QProcess::startDetached(QApplication::applicationFilePath(), {});
        close();
    }
}

void MainWindow::startUpdateCheck() { m_updates->checkOnStartup(); }

bool MainWindow::inDevelop() const { return m_stack->currentWidget() == m_develop; }

void MainWindow::showModule(Module m) {
    if (m == Module::Develop) {
        if (!m_develop->currentPhotoId()) {
            qint64 id = m_library->currentId();
            if (!id && m_model->rowCount() > 0)
                id = m_model->recordAt(0)->id;
            if (!id) {
                statusBar()->showMessage(tr("Import some photos first"), 4000);
                m_libraryTab->setChecked(true);
                return;
            }
            m_develop->openPhoto(id);
        }
        m_stack->setCurrentWidget(m_develop);
        m_developTab->setChecked(true);
    } else {
        m_develop->flush();
        m_stack->setCurrentWidget(m_library);
        m_libraryTab->setChecked(true);
        if (m_develop->currentPhotoId())
            m_library->selectId(m_develop->currentPhotoId());
    }
}

QVector<qint64> MainWindow::targetIds() const {
    if (inDevelop())
        return m_develop->currentPhotoId() ? QVector<qint64>{m_develop->currentPhotoId()} : QVector<qint64>{};
    return m_library->selectedIds();
}

void MainWindow::importFolder() {
    QSettings s;
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Import folder"),
                                                          s.value("import/lastDir").toString());
    if (dir.isEmpty())
        return;
    s.setValue("import/lastDir", dir);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const int added = m_catalog->importFolder(dir, true);
    QApplication::restoreOverrideCursor();
    statusBar()->showMessage(added ? tr("Imported %n photo(s)", "", added)
                                   : tr("No new photos found in %1").arg(dir),
                             6000);
    showModule(Module::Library);
}

void MainWindow::exportPhotos() {
    if (inDevelop())
        m_develop->flush();
    const QVector<qint64> ids = targetIds();
    if (ids.isEmpty()) {
        statusBar()->showMessage(tr("Select photos to export"), 4000);
        return;
    }
    ExportDialog dialog(ids.size(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const ExportOptions opt = dialog.options();

    struct Job {
        QString path;
        EditSettings settings;
        QString target;
    };
    // Output names are chosen up front: photos are exported in parallel, and
    // two photos called e.g. DSC_0001 must not end up in the same file.
    QVector<Job> jobs;
    QSet<QString> reserved;
    for (qint64 id : ids) {
        const PhotoRecord r = m_catalog->photo(id);
        jobs.append({r.path, EditSettings::fromJsonString(r.settingsJson),
                     Exporter::reserveTargetPath(r.path, opt, &reserved)});
    }

    auto* progress = new QProgressDialog(tr("Exporting…"), tr("Cancel"), 0, jobs.size(), this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setAttribute(Qt::WA_DeleteOnClose);

    auto* watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcherBase::progressValueChanged, progress, &QProgressDialog::setValue);
    connect(progress, &QProgressDialog::canceled, watcher, &QFutureWatcherBase::cancel);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, progress, opt, total = jobs.size()] {
        const QList<QString> results = watcher->future().results();
        progress->close();
        watcher->deleteLater();
        int ok = 0;
        QStringList errors;
        for (const QString& r : results)
            r.startsWith("ERR:") ? void(errors << r.mid(4)) : void(++ok);

        QMessageBox box(this);
        box.setWindowTitle(tr("Export finished"));
        box.setText(tr("Exported %1 of %2 photo(s).").arg(ok).arg(total));
        if (!errors.isEmpty())
            box.setDetailedText(errors.join('\n'));
        QPushButton* show = box.addButton(tr("Show Folder"), QMessageBox::ActionRole);
        box.addButton(QMessageBox::Ok);
        box.exec();
        if (box.clickedButton() == show)
            QDesktopServices::openUrl(QUrl::fromLocalFile(opt.outputDir));
    });

    // Several photos at once. Each one already uses all cores for the
    // adjustments, but decoding a RAW file and writing the JPEG run on one
    // core, so exporting 2-3 photos side by side is much faster. More would
    // mostly cost memory: a full-size RAW needs several hundred MB.
    auto* pool = new QThreadPool(watcher);
    pool->setMaxThreadCount(std::clamp(QThread::idealThreadCount() / 2, 1, 3));
    watcher->setFuture(QtConcurrent::mapped(pool, jobs, [opt](const Job& job) {
        QString written, error;
        if (Exporter::exportPhoto(job.path, job.settings, opt, &written, &error, job.target))
            return written;
        return QStringLiteral("ERR:") + error;
    }));
}

void MainWindow::copySettings() {
    const QVector<qint64> ids = targetIds();
    if (ids.isEmpty())
        return;
    m_clipboard = inDevelop() ? m_develop->settings()
                              : EditSettings::fromJsonString(m_catalog->photo(ids.first()).settingsJson);
    m_hasClipboard = true;
    statusBar()->showMessage(tr("Settings copied"), 3000);
}

void MainWindow::pasteSettings() {
    if (!m_hasClipboard)
        return;
    if (inDevelop()) {
        m_develop->pasteSettings(m_clipboard);
        return;
    }
    const QVector<qint64> ids = targetIds();
    for (qint64 id : ids) {
        const EditSettings current = EditSettings::fromJsonString(m_catalog->photo(id).settingsJson);
        EditSettings s = m_clipboard;
        // Geometry is specific to each photo and is never pasted.
        s.crop = current.crop;
        s.rotation = current.rotation;
        s.orientationQuarterTurns = current.orientationQuarterTurns;
        m_catalog->setSettings(id, s);
    }
    statusBar()->showMessage(tr("Settings pasted to %n photo(s)", "", ids.size()), 3000);
}

void MainWindow::setRating(int rating) {
    m_catalog->setRating(targetIds(), rating);
}

void MainWindow::setFlag(int flag) {
    m_catalog->setFlag(targetIds(), Flag(flag));
}

void MainWindow::rotateSelection(int delta) {
    if (inDevelop()) {
        m_develop->rotate(delta);
        return;
    }
    for (qint64 id : targetIds()) {
        EditSettings s = EditSettings::fromJsonString(m_catalog->photo(id).settingsJson);
        s.orientationQuarterTurns = ((s.orientationQuarterTurns + delta) % 4 + 4) % 4;
        s.crop = QRectF(0, 0, 1, 1);
        m_catalog->setSettings(id, s);
    }
}

void MainWindow::removeSelection() {
    if (inDevelop())
        return;
    const QVector<qint64> ids = targetIds();
    if (ids.isEmpty())
        return;
    const auto answer = QMessageBox::question(
        this, tr("Remove from catalog"),
        tr("Remove %n photo(s) from the catalog?\nThe files on disk are not deleted.", "", ids.size()));
    if (answer == QMessageBox::Yes) {
        m_catalog->remove(ids);
        forgetRemoved(ids);
    }
}

void MainWindow::removeFolder(const QString& folder) {
    const QVector<qint64> ids = m_catalog->photosInFolder(folder);
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Remove folder from catalog"));
    box.setText(tr("Remove “%1” and its %n photo(s) from the catalog?", "", ids.size())
                    .arg(QDir(folder).dirName()));
    box.setInformativeText(tr("The files on disk are not deleted. Your edits are kept in the "
                              ".svitlo.json files next to the photos, so importing the folder "
                              "again brings them back."));
    QPushButton* remove = box.addButton(tr("Remove"), QMessageBox::DestructiveRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() != remove)
        return;

    m_develop->flush(); // a pending edit is saved to its sidecar first
    if (m_model->folder() == folder)
        m_model->setFolder(QString()); // show "All Photographs" afterwards
    m_catalog->remove(ids);
    forgetRemoved(ids);
    statusBar()->showMessage(tr("Removed %n photo(s) from the catalog", "", ids.size()), 5000);
}

void MainWindow::forgetRemoved(const QVector<qint64>& ids) {
    if (ids.contains(m_develop->currentPhotoId()))
        m_develop->closePhoto();
    m_thumbs->forget(ids);
}

void MainWindow::closeEvent(QCloseEvent* e) {
    m_develop->flush();
    QSettings().setValue("window/geometry", saveGeometry());
    QMainWindow::closeEvent(e);
}

} // namespace svitlo
