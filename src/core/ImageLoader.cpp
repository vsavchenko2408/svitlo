#include "core/ImageLoader.h"

#include "core/ColorMath.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QTransform>
#include <QtConcurrent/QtConcurrentMap>

#include <array>
#include <memory>
#include <numeric>

#ifdef SVITLO_HAVE_LIBRAW
#include <libraw/libraw.h>
#endif

namespace svitlo {

namespace {

// RAW files decoded in linear light with no auto-brightening come out darker
// than the camera's own JPEG, which applies a tone curve and some gain. A
// small default gain gives a more familiar starting point; the user's
// exposure slider is applied on top of it.
constexpr float kRawBaselineEv = 0.5f;

const QStringList& rawExtensions() {
    static const QStringList ext = {
        "3fr", "arw", "cr2", "cr3", "crw", "dcr", "dng", "erf", "iiq", "k25", "kdc",
        "mef", "mos", "mrw", "nef", "nrw", "orf", "pef", "raf", "raw", "rw2", "rwl",
        "sr2", "srf", "srw", "x3f"};
    return ext;
}

const std::array<float, 256>& srgb8ToLinearTable() {
    static const std::array<float, 256> table = [] {
        std::array<float, 256> t{};
        for (int i = 0; i < 256; ++i)
            t[i] = color::srgbToLinear(i / 255.0f);
        return t;
    }();
    return table;
}

QImage applyLibRawFlip(const QImage& img, int flip) {
    // LibRaw: 0 = none, 3 = 180°, 5 = 90° CCW, 6 = 90° CW.
    QTransform t;
    switch (flip) {
    case 3: t.rotate(180); break;
    case 5: t.rotate(-90); break;
    case 6: t.rotate(90); break;
    default: return img;
    }
    return img.transformed(t);
}

} // namespace

ImageBuffer downscaleToFit(const ImageBuffer& src, int maxLongEdge) {
    if (src.isNull() || maxLongEdge <= 0 || std::max(src.width, src.height) <= maxLongEdge)
        return src;

    const double scale = double(maxLongEdge) / std::max(src.width, src.height);
    const int dw = std::max(1, int(src.width * scale + 0.5));
    const int dh = std::max(1, int(src.height * scale + 0.5));
    ImageBuffer dst(dw, dh);

    const double fx = double(src.width) / dw;
    const double fy = double(src.height) / dh;

    std::vector<int> rows(dh);
    std::iota(rows.begin(), rows.end(), 0);
    QtConcurrent::blockingMap(rows, [&](int y) {
        const int sy0 = int(y * fy);
        const int sy1 = std::max(sy0 + 1, std::min(src.height, int((y + 1) * fy)));
        float* out = dst.row(y);
        for (int x = 0; x < dw; ++x) {
            const int sx0 = int(x * fx);
            const int sx1 = std::max(sx0 + 1, std::min(src.width, int((x + 1) * fx)));
            float acc[3] = {0, 0, 0};
            for (int sy = sy0; sy < sy1; ++sy) {
                const float* p = src.at(sx0, sy);
                for (int sx = sx0; sx < sx1; ++sx, p += 3) {
                    acc[0] += p[0];
                    acc[1] += p[1];
                    acc[2] += p[2];
                }
            }
            const float inv = 1.0f / float((sy1 - sy0) * (sx1 - sx0));
            out[x * 3 + 0] = acc[0] * inv;
            out[x * 3 + 1] = acc[1] * inv;
            out[x * 3 + 2] = acc[2] * inv;
        }
    });
    return dst;
}

QStringList ImageLoader::supportedExtensions() {
    QStringList ext = {"jpg", "jpeg", "png", "tif", "tiff", "webp", "bmp"};
    // Only advertise formats this Qt build can actually read.
    const auto readable = QImageReader::supportedImageFormats();
    ext.erase(std::remove_if(ext.begin(), ext.end(),
                             [&](const QString& e) { return !readable.contains(e.toLatin1()); }),
              ext.end());
    if (rawSupportCompiled())
        ext += rawExtensions();
    return ext;
}

bool ImageLoader::isSupported(const QString& path) {
    static const QStringList ext = supportedExtensions();
    return ext.contains(QFileInfo(path).suffix().toLower());
}

bool ImageLoader::isRawFile(const QString& path) {
    return rawExtensions().contains(QFileInfo(path).suffix().toLower());
}

bool ImageLoader::rawSupportCompiled() {
#ifdef SVITLO_HAVE_LIBRAW
    return true;
#else
    return false;
#endif
}

ImageBuffer ImageLoader::fromQImage(const QImage& input) {
    if (input.isNull())
        return {};
    const QImage img = input.convertToFormat(QImage::Format_RGB888);
    ImageBuffer buf(img.width(), img.height());
    const auto& lut = srgb8ToLinearTable();

    std::vector<int> rows(img.height());
    std::iota(rows.begin(), rows.end(), 0);
    QtConcurrent::blockingMap(rows, [&](int y) {
        const uchar* in = img.constScanLine(y);
        float* out = buf.row(y);
        for (int i = 0; i < img.width() * 3; ++i)
            out[i] = lut[in[i]];
    });
    return buf;
}

#ifdef SVITLO_HAVE_LIBRAW
namespace {

ImageBuffer loadRaw(const QString& path, bool halfSize, QString* error, ImageInfo* info) {
    auto proc = std::make_unique<LibRaw>();
    auto& p = proc->imgdata.params;
    p.output_bps = 16;
    p.gamm[0] = 1.0;      // linear output — tone mapping happens in our pipeline
    p.gamm[1] = 1.0;
    p.no_auto_bright = 1;
    p.use_camera_wb = 1;
    p.output_color = 1;   // sRGB primaries
    p.half_size = halfSize ? 1 : 0;
    p.user_qual = halfSize ? 0 : 3; // bilinear for previews, AHD for full quality

    const QByteArray encoded = QFile::encodeName(path);
    int rc = proc->open_file(encoded.constData());
    if (rc == LIBRAW_SUCCESS) rc = proc->unpack();
    if (rc == LIBRAW_SUCCESS) rc = proc->dcraw_process();
    if (rc != LIBRAW_SUCCESS) {
        if (error) *error = QString::fromUtf8(libraw_strerror(rc));
        return {};
    }

    int err = 0;
    libraw_processed_image_t* img = proc->dcraw_make_mem_image(&err);
    if (!img || img->type != LIBRAW_IMAGE_BITMAP || img->colors != 3 || img->bits != 16) {
        if (error) *error = QCoreApplication::translate("ImageLoader", "Unsupported RAW output layout");
        if (img) LibRaw::dcraw_clear_mem(img);
        return {};
    }

    ImageBuffer buf(img->width, img->height);
    const auto* data = reinterpret_cast<const quint16*>(img->data);
    const float gain = std::exp2(kRawBaselineEv) / 65535.0f;
    const std::size_t n = std::size_t(img->width) * img->height * 3;
    for (std::size_t i = 0; i < n; ++i)
        buf.pixels[i] = data[i] * gain;

    if (info) {
        info->isRaw = true;
        info->camera = QString::fromUtf8(proc->imgdata.idata.make).trimmed() + ' ' +
                       QString::fromUtf8(proc->imgdata.idata.model).trimmed();
        if (proc->imgdata.other.timestamp > 0)
            info->captureTime = QDateTime::fromSecsSinceEpoch(qint64(proc->imgdata.other.timestamp));
        const int scale = halfSize ? 2 : 1;
        info->size = QSize(img->width * scale, img->height * scale);
    }
    LibRaw::dcraw_clear_mem(img);
    return buf;
}

QImage loadRawThumbnail(const QString& path, ImageInfo* info) {
    auto proc = std::make_unique<LibRaw>();
    const QByteArray encoded = QFile::encodeName(path);
    if (proc->open_file(encoded.constData()) != LIBRAW_SUCCESS)
        return {};

    if (info) {
        info->isRaw = true;
        info->camera = QString::fromUtf8(proc->imgdata.idata.make).trimmed() + ' ' +
                       QString::fromUtf8(proc->imgdata.idata.model).trimmed();
        if (proc->imgdata.other.timestamp > 0)
            info->captureTime = QDateTime::fromSecsSinceEpoch(qint64(proc->imgdata.other.timestamp));
        const auto& s = proc->imgdata.sizes;
        const bool swap = s.flip == 5 || s.flip == 6;
        info->size = swap ? QSize(s.height, s.width) : QSize(s.width, s.height);
    }

    QImage result;
    if (proc->unpack_thumb() == LIBRAW_SUCCESS) {
        int err = 0;
        libraw_processed_image_t* t = proc->dcraw_make_mem_thumb(&err);
        if (t) {
            if (t->type == LIBRAW_IMAGE_JPEG) {
                result.loadFromData(t->data, int(t->data_size), "JPEG");
            } else if (t->type == LIBRAW_IMAGE_BITMAP && t->colors == 3 && t->bits == 8) {
                result = QImage(t->data, t->width, t->height, t->width * 3, QImage::Format_RGB888).copy();
            }
            LibRaw::dcraw_clear_mem(t);
        }
    }
    if (!result.isNull())
        result = applyLibRawFlip(result, proc->imgdata.sizes.flip);
    return result;
}

} // namespace
#endif

ImageBuffer ImageLoader::load(const QString& path, Quality quality, int maxLongEdge, QString* error,
                              ImageInfo* info) {
    if (isRawFile(path)) {
#ifdef SVITLO_HAVE_LIBRAW
        ImageBuffer buf = loadRaw(path, quality == Quality::Preview, error, info);
        if (quality == Quality::Preview)
            buf = downscaleToFit(buf, maxLongEdge);
        return buf;
#else
        if (error) *error = QCoreApplication::translate("ImageLoader", "This build has no RAW support (LibRaw missing)");
        return {};
#endif
    }

    QImageReader reader(path);
    reader.setAutoTransform(true); // honour EXIF orientation
    if (info) {
        info->isRaw = false;
        info->captureTime = QFileInfo(path).lastModified();
    }
    QSize full = reader.size();
    if (reader.transformation() & QImageIOHandler::TransformationRotate90)
        full.transpose();

    if (quality == Quality::Preview && full.isValid() && std::max(full.width(), full.height()) > maxLongEdge) {
        QSize target = full.scaled(maxLongEdge, maxLongEdge, Qt::KeepAspectRatio);
        // setScaledSize works in the file's (unrotated) pixel space.
        if (reader.transformation() & QImageIOHandler::TransformationRotate90)
            target.transpose();
        reader.setScaledSize(target);
    }

    const QImage img = reader.read();
    if (img.isNull()) {
        if (error) *error = reader.errorString();
        return {};
    }
    if (info)
        info->size = full.isValid() ? full : img.size();
    return fromQImage(img);
}

QImage ImageLoader::loadThumbnail(const QString& path, int maxLongEdge, ImageInfo* info) {
    QImage img;
    if (isRawFile(path)) {
#ifdef SVITLO_HAVE_LIBRAW
        img = loadRawThumbnail(path, info);
        if (img.isNull()) {
            // No usable embedded preview: fall back to a fast half-size decode.
            const ImageBuffer buf = loadRaw(path, true, nullptr, info);
            if (!buf.isNull()) {
                img = QImage(buf.width, buf.height, QImage::Format_RGB888);
                for (int y = 0; y < buf.height; ++y) {
                    uchar* out = img.scanLine(y);
                    const float* in = buf.row(y);
                    for (int i = 0; i < buf.width * 3; ++i)
                        out[i] = uchar(color::clamp01(color::linearToSrgb(in[i])) * 255.0f + 0.5f);
                }
            }
        }
#endif
    } else {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        QSize full = reader.size();
        if (info) {
            info->isRaw = false;
            info->captureTime = QFileInfo(path).lastModified();
            info->size = (reader.transformation() & QImageIOHandler::TransformationRotate90)
                             ? full.transposed()
                             : full;
        }
        if (full.isValid() && std::max(full.width(), full.height()) > maxLongEdge)
            reader.setScaledSize(full.scaled(maxLongEdge, maxLongEdge, Qt::KeepAspectRatio));
        img = reader.read();
    }
    if (!img.isNull() && std::max(img.width(), img.height()) > maxLongEdge)
        img = img.scaled(maxLongEdge, maxLongEdge, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return img;
}

} // namespace svitlo
