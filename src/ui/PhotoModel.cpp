#include "ui/PhotoModel.h"

#include "catalog/Catalog.h"
#include "catalog/ThumbnailCache.h"

#include <QMap>

#include <algorithm>

namespace svitlo {

PhotoModel::PhotoModel(Catalog* catalog, ThumbnailCache* thumbs, QObject* parent)
    : QAbstractListModel(parent), m_catalog(catalog), m_thumbs(thumbs) {
    connect(m_catalog, &Catalog::photosAdded, this, &PhotoModel::reload);
    connect(m_catalog, &Catalog::photosRemoved, this, &PhotoModel::reload);
    connect(m_catalog, &Catalog::photoChanged, this, &PhotoModel::onPhotoChanged);
    connect(m_thumbs, &ThumbnailCache::thumbnailReady, this, &PhotoModel::onThumbnailReady);
    connect(m_thumbs, &ThumbnailCache::infoAvailable, this, [this](qint64 id, const ImageInfo& info) {
        m_catalog->updateInfo(id, info);
        for (PhotoRecord& r : m_all)
            if (r.id == id) {
                r.size = info.size;
                r.camera = info.camera;
                if (info.captureTime.isValid())
                    r.captureTime = info.captureTime;
            }
    });
    reload();
}

int PhotoModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_visible.size();
}

const PhotoRecord* PhotoModel::recordAt(int row) const {
    if (row < 0 || row >= m_visible.size())
        return nullptr;
    return &m_all[m_visible[row]];
}

int PhotoModel::rowOf(qint64 id) const { return m_rowById.value(id, -1); }

QVariant PhotoModel::data(const QModelIndex& index, int role) const {
    const PhotoRecord* r = recordAt(index.row());
    if (!r)
        return {};
    switch (role) {
    case Qt::DisplayRole: return r->fileName;
    case Qt::ToolTipRole: {
        QString tip = r->fileName;
        if (r->size.isValid() && !r->size.isEmpty())
            tip += QStringLiteral("\n%1 × %2").arg(r->size.width()).arg(r->size.height());
        if (!r->camera.trimmed().isEmpty())
            tip += '\n' + r->camera;
        return tip;
    }
    case IdRole: return r->id;
    case RecordRole: return QVariant::fromValue(*r);
    case ThumbnailRole: return m_thumbs->thumbnail(*r);
    case RatingRole: return r->rating;
    case FlagRole: return int(r->flag);
    case EditedRole: return r->isEdited();
    }
    return {};
}

void PhotoModel::reload() {
    beginResetModel();
    m_all = m_catalog->allPhotos();
    endResetModel();
    refilter();
}

bool PhotoModel::accepts(const PhotoRecord& r) const {
    if (!m_folder.isEmpty() && r.folder != m_folder)
        return false;
    if (r.rating < m_minRating)
        return false;
    switch (m_flagFilter) {
    case FlagFilter::All: break;
    case FlagFilter::PickedOnly: if (r.flag != Flag::Picked) return false; break;
    case FlagFilter::HideRejected: if (r.flag == Flag::Rejected) return false; break;
    case FlagFilter::RejectedOnly: if (r.flag != Flag::Rejected) return false; break;
    }
    if (!m_search.isEmpty() && !r.fileName.contains(m_search, Qt::CaseInsensitive) &&
        !r.camera.contains(m_search, Qt::CaseInsensitive))
        return false;
    return true;
}

void PhotoModel::refilter() {
    beginResetModel();
    m_visible.clear();
    for (int i = 0; i < m_all.size(); ++i)
        if (accepts(m_all[i]))
            m_visible.append(i);

    auto key = [this](int i) -> const PhotoRecord& { return m_all[i]; };
    std::stable_sort(m_visible.begin(), m_visible.end(), [&](int a, int b) {
        const PhotoRecord& x = key(a);
        const PhotoRecord& y = key(b);
        switch (m_sort) {
        case SortOrder::FileName:
            return QString::compare(x.fileName, y.fileName, Qt::CaseInsensitive) < 0;
        case SortOrder::ImportTime:
            return x.importedAt != y.importedAt ? x.importedAt < y.importedAt : x.fileName < y.fileName;
        case SortOrder::CaptureTime:
        default:
            return x.captureTime != y.captureTime ? x.captureTime < y.captureTime : x.fileName < y.fileName;
        }
    });

    m_rowById.clear();
    for (int row = 0; row < m_visible.size(); ++row)
        m_rowById.insert(m_all[m_visible[row]].id, row);
    endResetModel();
}

void PhotoModel::setFolder(const QString& folder) {
    if (folder == m_folder) return;
    m_folder = folder;
    refilter();
}

void PhotoModel::setMinRating(int rating) {
    if (rating == m_minRating) return;
    m_minRating = rating;
    refilter();
}

void PhotoModel::setFlagFilter(FlagFilter f) {
    if (f == m_flagFilter) return;
    m_flagFilter = f;
    refilter();
}

void PhotoModel::setSearchText(const QString& text) {
    const QString t = text.trimmed();
    if (t == m_search) return;
    m_search = t;
    refilter();
}

void PhotoModel::setSortOrder(SortOrder order) {
    if (order == m_sort) return;
    m_sort = order;
    refilter();
}

QVector<QPair<QString, int>> PhotoModel::folders() const {
    QMap<QString, int> counts;
    for (const PhotoRecord& r : m_all)
        ++counts[r.folder];
    QVector<QPair<QString, int>> out;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        out.append({it.key(), it.value()});
    return out;
}

void PhotoModel::onPhotoChanged(qint64 id) {
    const PhotoRecord fresh = m_catalog->photo(id);
    for (PhotoRecord& r : m_all) {
        if (r.id != id)
            continue;
        // A rating/flag change may no longer match the current filter, but
        // making the photo you just rated vanish is jarring: it stays
        // visible until the filter itself changes.
        r = fresh;
        const int row = rowOf(id);
        if (row >= 0)
            emit dataChanged(index(row), index(row));
        return;
    }
}

void PhotoModel::onThumbnailReady(qint64 id) {
    const int row = rowOf(id);
    if (row >= 0)
        emit dataChanged(index(row), index(row), {ThumbnailRole});
}

} // namespace svitlo
