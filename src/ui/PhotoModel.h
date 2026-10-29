#pragma once

#include "catalog/PhotoRecord.h"

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

namespace svitlo {

class Catalog;
class ThumbnailCache;

// Filtered, sorted view of the catalog shared by the grid and the filmstrip.
class PhotoModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        RecordRole,
        ThumbnailRole,
        RatingRole,
        FlagRole,
        EditedRole,
    };

    enum class FlagFilter { All, PickedOnly, HideRejected, RejectedOnly };
    enum class SortOrder { CaptureTime, FileName, ImportTime };

    PhotoModel(Catalog* catalog, ThumbnailCache* thumbs, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void reload();

    void setFolder(const QString& folder); // empty = all photographs
    void setMinRating(int rating);
    void setFlagFilter(FlagFilter f);
    void setSearchText(const QString& text);
    void setSortOrder(SortOrder order);

    QString folder() const { return m_folder; }

    const PhotoRecord* recordAt(int row) const;
    int rowOf(qint64 id) const;

    // Distinct folders with photo counts, sorted by path.
    QVector<QPair<QString, int>> folders() const;

private:
    void refilter();
    void onPhotoChanged(qint64 id);
    void onThumbnailReady(qint64 id);
    bool accepts(const PhotoRecord& r) const;

    Catalog* m_catalog;
    ThumbnailCache* m_thumbs;
    QVector<PhotoRecord> m_all;
    QVector<int> m_visible; // indices into m_all
    QHash<qint64, int> m_rowById;

    QString m_folder;
    int m_minRating = 0;
    FlagFilter m_flagFilter = FlagFilter::HideRejected;
    QString m_search;
    SortOrder m_sort = SortOrder::CaptureTime;
};

} // namespace svitlo
