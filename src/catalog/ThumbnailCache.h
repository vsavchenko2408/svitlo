#pragma once

#include "catalog/PhotoRecord.h"
#include "core/ImageLoader.h"

#include <QCache>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QSet>
#include <QThreadPool>

namespace svitlo {

// Asynchronous thumbnail provider with a memory cache and an on-disk cache.
// Thumbnails of edited photos are rendered through the develop pipeline so
// the grid always reflects the current edit.
class ThumbnailCache : public QObject {
    Q_OBJECT
public:
    static constexpr int kThumbSize = 480; // long edge in pixels

    explicit ThumbnailCache(const QString& cacheDir, QObject* parent = nullptr);
    ~ThumbnailCache() override;

    // Returns the cached image, or a null image after queueing generation.
    QImage thumbnail(const PhotoRecord& rec);

    // Replace the thumbnail directly (e.g. from the develop preview).
    void store(const PhotoRecord& rec, const QImage& image);

signals:
    void thumbnailReady(qint64 id);
    void infoAvailable(qint64 id, const svitlo::ImageInfo& info);

private:
    struct Entry {
        QImage image;
        qint64 stamp = -1;
    };

    QString diskPath(qint64 id, qint64 stamp) const;
    void deliver(qint64 id, qint64 stamp, const QImage& image);

    QString m_dir;
    QCache<qint64, Entry> m_memory;
    QSet<QPair<qint64, qint64>> m_pending; // (id, stamp)
    QThreadPool m_pool;
};

} // namespace svitlo

Q_DECLARE_METATYPE(svitlo::ImageInfo)
