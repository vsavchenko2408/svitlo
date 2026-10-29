#pragma once

#include "core/EditSettings.h"

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

    // Decodes the original at full resolution, applies the settings and
    // writes the result. Never overwrites: a numeric suffix is added if the
    // target exists. Safe to call from a worker thread.
    static bool exportPhoto(const QString& sourcePath, const EditSettings& settings,
                            const ExportOptions& options, QString* writtenPath, QString* error);
};

} // namespace svitlo
