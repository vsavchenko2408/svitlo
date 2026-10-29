#pragma once

#include "core/ImageBuffer.h"

#include <QDateTime>
#include <QImage>
#include <QSize>
#include <QString>
#include <QStringList>

namespace svitlo {

struct ImageInfo {
    QSize size;              // oriented pixel size of the full image
    QDateTime captureTime;   // from RAW metadata when available, else file time
    QString camera;          // "Make Model" for RAW files
    bool isRaw = false;
};

class ImageLoader {
public:
    enum class Quality {
        Preview, // fast: half-size RAW demosaic, downscaled to maxLongEdge
        Full     // full-resolution decode, used for export
    };

    // File extensions (lower case, no dot) that can be imported.
    static QStringList supportedExtensions();
    static bool isSupported(const QString& path);
    static bool isRawFile(const QString& path);
    static bool rawSupportCompiled();

    // Decodes to linear float RGB. Returns a null buffer on failure and sets
    // *error. For Preview the longer side is limited to maxLongEdge.
    static ImageBuffer load(const QString& path, Quality quality, int maxLongEdge = 2560,
                            QString* error = nullptr, ImageInfo* info = nullptr);

    // Fast thumbnail: embedded JPEG for RAW files, scaled decode otherwise.
    // Orientation is applied.
    static QImage loadThumbnail(const QString& path, int maxLongEdge, ImageInfo* info = nullptr);

    // Converts a display-referred QImage (assumed sRGB) to a linear buffer.
    static ImageBuffer fromQImage(const QImage& image);
};

} // namespace svitlo
