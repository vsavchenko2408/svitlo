#pragma once

#include "core/EditSettings.h"

#include <QSet>
#include <QString>
#include <QStringList>

namespace svitlo {

struct ExportOptions {
    QString format = "jpg";   // jpg | png | tif (tif only if the Qt plugin exists)
    int quality = 92;         // JPEG quality 1..100
    int longEdge = 0;         // 0 = keep full resolution
    QString outputDir;
    QString fileSuffix;       // appended to the base name, e.g. "_edit"
};

class Exporter {
public:
    static QStringList availableFormats();

    // Chooses the output file for a photo: "<name><suffix>.<format>" in the
    // output folder, or "<name><suffix>_2.<format>" etc. if that file exists
    // or was already given to another photo of the same batch (`reserved`).
    // Used before exporting several photos in parallel, so two photos with
    // the same name never write to the same file.
    static QString reserveTargetPath(const QString& sourcePath, const ExportOptions& options,
                                     QSet<QString>* reserved);

    // Decodes the original at full resolution, applies the settings and
    // writes the result. Never overwrites: without `targetPath` a numeric
    // suffix is added if the file exists. Safe to call from a worker thread.
    static bool exportPhoto(const QString& sourcePath, const EditSettings& settings,
                            const ExportOptions& options, QString* writtenPath, QString* error,
                            const QString& targetPath = QString());
};

} // namespace svitlo
