#include "catalog/ThumbnailCache.h"

#include "core/EditSettings.h"
#include "core/Pipeline.h"

#include <QDir>
#include <QFile>
#include <QPointer>
#include <QRunnable>
#include <algorithm>

namespace svitlo {

ThumbnailCache::ThumbnailCache(const QString& cacheDir, QObject* parent)
    : QObject(parent), m_dir(cacheDir) {
    QDir().mkpath(m_dir);
    // ~480px RGB thumbnails are ~0.7 MB each; keep a few hundred in memory.
    m_memory.setMaxCost(400);
    // Decoding is heavy; two parallel jobs keep the UI responsive.
    m_pool.setMaxThreadCount(2);
    qRegisterMetaType<svitlo::ImageInfo>();
}

ThumbnailCache::~ThumbnailCache() {
    m_pool.clear();
    m_pool.waitForDone();
}

QString ThumbnailCache::diskPath(qint64 id, qint64 stamp) const {
    return QDir(m_dir).filePath(QStringLiteral("%1_%2.jpg").arg(id).arg(stamp));
}

QImage ThumbnailCache::thumbnail(const PhotoRecord& rec) {
    if (const Entry* e = m_memory.object(rec.id)) {
        if (e->stamp == rec.editStamp)
            return e->image;
    }

    const auto key = qMakePair(rec.id, rec.editStamp);
    if (m_pending.contains(key))
        return {};
    m_pending.insert(key);

    const QString cached = diskPath(rec.id, rec.editStamp);
    const qint64 id = rec.id, stamp = rec.editStamp;
    const QString path = rec.path, settingsJson = rec.settingsJson;
    const bool needInfo = !rec.size.isValid() || rec.size.isEmpty();
    QPointer<ThumbnailCache> self(this);

    m_pool.start([self, id, stamp, path, settingsJson, cached, needInfo] {
        QImage img;
        ImageInfo info;
        bool haveInfo = false;

        if (QFile::exists(cached))
            img.load(cached, "JPEG");

        if (img.isNull()) {
            const EditSettings s = EditSettings::fromJsonString(settingsJson);
            if (s.isIdentity()) {
                img = ImageLoader::loadThumbnail(path, kThumbSize, &info);
                haveInfo = img.isNull() ? false : true;
            } else {
                const ImageBuffer buf =
                    ImageLoader::load(path, ImageLoader::Quality::Preview, kThumbSize * 2, nullptr, &info);
                haveInfo = !buf.isNull();
                if (!buf.isNull()) {
                    Pipeline::Options opt;
                    opt.isRaw = ImageLoader::isRawFile(path);
                    opt.resolutionScale = info.size.isValid() && info.size.width() > 0
                                              ? double(std::max(buf.width, buf.height)) /
                                                    std::max(info.size.width(), info.size.height())
                                              : 1.0;
                    img = Pipeline::render(buf, s, opt);
                }
            }
            if (!img.isNull()) {
                if (std::max(img.width(), img.height()) > kThumbSize)
                    img = img.scaled(kThumbSize, kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                img.save(cached, "JPEG", 85);
            }
        } else if (needInfo) {
            // Cached image but unknown metadata (e.g. catalog rebuilt).
            ImageLoader::loadThumbnail(path, 64, &info);
            haveInfo = true;
        }

        if (!self)
            return;
        QMetaObject::invokeMethod(self.data(), [self, id, stamp, img, info, haveInfo, needInfo] {
            if (!self)
                return;
            self->deliver(id, stamp, img);
            if (haveInfo && needInfo)
                emit self->infoAvailable(id, info);
        }, Qt::QueuedConnection);
    });
    return {};
}

void ThumbnailCache::deliver(qint64 id, qint64 stamp, const QImage& image) {
    m_pending.remove(qMakePair(id, stamp));
    // Remove stale disk copies of older edits.
    for (qint64 s = std::max<qint64>(0, stamp - 3); s < stamp; ++s)
        QFile::remove(diskPath(id, s));

    // A null image is cached too, so a file that fails to decode is not
    // retried on every repaint of the grid.
    m_memory.insert(id, new Entry{image, stamp}, 1);
    emit thumbnailReady(id);
}

void ThumbnailCache::store(const PhotoRecord& rec, const QImage& image) {
    if (image.isNull())
        return;
    QImage img = image;
    if (std::max(img.width(), img.height()) > kThumbSize)
        img = img.scaled(kThumbSize, kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    img.save(diskPath(rec.id, rec.editStamp), "JPEG", 85);
    m_memory.insert(rec.id, new Entry{img, rec.editStamp}, 1);
    emit thumbnailReady(rec.id);
}

} // namespace svitlo
