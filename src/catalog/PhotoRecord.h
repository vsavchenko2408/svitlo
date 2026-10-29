#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QSize>
#include <QString>

namespace svitlo {

enum class Flag : int { Rejected = -1, None = 0, Picked = 1 };

// One row of the catalog.
struct PhotoRecord {
    qint64 id = 0;
    QString path;          // absolute path of the original file
    QString folder;        // absolute path of the containing folder
    QString fileName;
    QDateTime captureTime;
    QDateTime importedAt;
    QSize size;            // oriented pixel size, may be unknown (invalid)
    QString camera;
    int rating = 0;        // 0..5
    Flag flag = Flag::None;
    QString settingsJson;  // serialised EditSettings, empty = unedited
    qint64 editStamp = 0;  // bumped on every edit; keys the thumbnail cache

    bool isEdited() const { return !settingsJson.isEmpty(); }
};

} // namespace svitlo

Q_DECLARE_METATYPE(svitlo::PhotoRecord)
