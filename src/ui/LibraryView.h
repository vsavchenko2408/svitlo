#pragma once

#include <QWidget>

class QListView;
class QListWidget;
class QLineEdit;
class QComboBox;
class QSlider;

namespace svitlo {

class PhotoModel;
class PhotoDelegate;

// Library module: folder list, filter bar and the thumbnail grid.
class LibraryView : public QWidget {
    Q_OBJECT
public:
    LibraryView(PhotoModel* model, QWidget* parent = nullptr);

    QListView* grid() const { return m_grid; }
    QVector<qint64> selectedIds() const;
    qint64 currentId() const;
    void selectId(qint64 id);
    void refreshFolders();
    // Folder selected in the folder list; empty for "All Photographs".
    QString currentFolder() const;

signals:
    void openInDevelop(qint64 id);
    void importRequested();
    void removeFolderRequested(const QString& folder);

private:
    PhotoModel* m_model;
    PhotoDelegate* m_delegate;
    QListView* m_grid;
    QListWidget* m_folders;
    QLineEdit* m_search;
    QComboBox* m_ratingFilter;
    QComboBox* m_flagFilter;
    QComboBox* m_sort;
    QSlider* m_size;
    QWidget* m_emptyState;
};

} // namespace svitlo
