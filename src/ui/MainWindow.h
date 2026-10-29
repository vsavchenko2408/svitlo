#pragma once

#include "core/EditSettings.h"

#include <QMainWindow>

class QStackedWidget;
class QToolButton;

namespace svitlo {

class Catalog;
class ThumbnailCache;
class PhotoModel;
class LibraryView;
class DevelopView;
class UpdateChecker;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Catalog* catalog, ThumbnailCache* thumbs, QWidget* parent = nullptr);

public:
    // Starts the automatic update check (if enabled); call after show().
    void startUpdateCheck();

protected:
    void closeEvent(QCloseEvent* e) override;

private:
    enum class Module { Library, Develop };

    void buildMenus();
    void showModule(Module m);
    bool inDevelop() const;
    QVector<qint64> targetIds() const; // selection in library, current photo in develop

    void importFolder();
    void exportPhotos();
    void copySettings();
    void pasteSettings();
    void setRating(int rating);
    void setFlag(int flag);
    void rotateSelection(int delta);
    void removeSelection();
    void changeLanguage(const QString& code);

    Catalog* m_catalog;
    ThumbnailCache* m_thumbs;
    PhotoModel* m_model;
    QStackedWidget* m_stack;
    LibraryView* m_library;
    DevelopView* m_develop;
    UpdateChecker* m_updates;
    QToolButton* m_libraryTab;
    QToolButton* m_developTab;

    EditSettings m_clipboard;
    bool m_hasClipboard = false;
};

} // namespace svitlo
