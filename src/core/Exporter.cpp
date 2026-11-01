#include "core/Exporter.h"

#include "core/ImageLoader.h"
#include "core/Pipeline.h"

#include <QDir>
#include <QFileInfo>
#include <QImageWriter>

#include <algorithm>

namespace svitlo {

QStringList Exporter::availableFormats() {
    QStringList formats = {"jpg", "png"};
    if (QImageWriter::supportedImageFormats().contains("tiff"))
        formats << "tif";
    return formats;
}

QString Exporter::reserveTargetPath(const QString& sourcePath, const ExportOptions& options,
                                   QSet<QString>* reserved) {
    const QDir dir(options.outputDir);
    const QString base = QFileInfo(sourcePath).completeBaseName() + options.fileSuffix;
    // macOS and Windows file names are case-insensitive: "IMG.jpg" and
    // "img.jpg" are the same file there, so compare in lower case.
    auto taken = [reserved](const QString& path) {
        return QFileInfo::exists(path) || (reserved && reserved->contains(path.toLower()));
    };
    QString target = dir.filePath(base + '.' + options.format);
    for (int n = 2; taken(target); ++n)
        target = dir.filePath(QStringLiteral("%1_%2.%3").arg(base).arg(n).arg(options.format));
    if (reserved)
        reserved->insert(target.toLower());
    return target;
}

bool Exporter::exportPhoto(const QString& sourcePath, const EditSettings& settings,
                           const ExportOptions& options, QString* writtenPath, QString* error,
                           const QString& targetPath) {
    QString loadError;
    const ImageBuffer full = ImageLoader::load(sourcePath, ImageLoader::Quality::Full, 0, &loadError);
    if (full.isNull()) {
        if (error) *error = QObject::tr("Cannot read %1: %2").arg(sourcePath, loadError);
        return false;
    }

    // Downscale before processing when a smaller size was requested — it is
    // faster and gives the same look, since radius-based effects are scaled.
    ImageBuffer working = full;
    double scale = 1.0;
    if (options.longEdge > 0) {
        // The crop changes the final aspect, so size the long edge of the
        // cropped result, not of the whole frame.
        const QSize oriented = (settings.orientationQuarterTurns & 1) ? QSize(full.height, full.width)
                                                                      : QSize(full.width, full.height);
        const double croppedLong = std::max(oriented.width() * settings.crop.width(),
                                            oriented.height() * settings.crop.height());
        if (croppedLong > options.longEdge) {
            scale = options.longEdge / croppedLong;
            const int target = int(std::max(full.width, full.height) * scale + 0.5);
            working = downscaleToFit(full, target);
            scale = double(std::max(working.width, working.height)) / std::max(full.width, full.height);
        }
    }

    Pipeline::Options popt;
    popt.resolutionScale = scale;
    popt.isRaw = ImageLoader::isRawFile(sourcePath);
    const QImage out = Pipeline::render(working, settings, popt);
    if (out.isNull()) {
        if (error) *error = QObject::tr("Processing failed");
        return false;
    }

    QDir dir(options.outputDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        if (error) *error = QObject::tr("Cannot create folder %1").arg(options.outputDir);
        return false;
    }

    const QString target = targetPath.isEmpty() ? reserveTargetPath(sourcePath, options, nullptr) : targetPath;

    QImageWriter writer(target, options.format == "tif" ? QByteArray("tiff") : options.format.toLatin1());
    if (options.format == "jpg")
        writer.setQuality(std::clamp(options.quality, 1, 100));
    if (!writer.write(out)) {
        if (error) *error = writer.errorString();
        return false;
    }
    if (writtenPath)
        *writtenPath = target;
    return true;
}

} // namespace svitlo
