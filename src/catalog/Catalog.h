#pragma once

#include "catalog/PhotoRecord.h"
#include "core/EditSettings.h"

#include <QObject>
#include <QSqlDatabase>
#include <QVector>

namespace svitlo {

struct ImageInfo;

// SQLite-backed photo catalog. Lives on the GUI thread.
//
// Edits are stored in the database and, for portability, also as a small
// JSON sidecar next to the original ("photo.nef.svitlo.json"). On import an
// existing sidecar is picked up, so edits survive moving to a new catalog.
class Catalog : public QObject {
    Q_OBJECT
public:
    explicit Catalog(QObject* parent = nullptr);
    ~Catalog() override;

    bool open(const QString& databasePath, QString* error = nullptr);
    QString databasePath() const { return m_path; }

    // Adds every supported file in the folder. Returns the number of new photos.
    int importFolder(const QString& folder, bool recursive);

    QVector<PhotoRecord> allPhotos() const;
    PhotoRecord photo(qint64 id) const;

    void setRating(const QVector<qint64>& ids, int rating);
    void setFlag(const QVector<qint64>& ids, Flag flag);
    void setSettings(qint64 id, const EditSettings& settings);
    void updateInfo(qint64 id, const ImageInfo& info);
    void remove(const QVector<qint64>& ids);

    static QString sidecarPath(const QString& photoPath);

signals:
    void photosAdded();
    void photosRemoved();
    void photoChanged(qint64 id);

private:
    PhotoRecord readRow(const class QSqlQuery& q) const;

    QSqlDatabase m_db;
    QString m_path;
    QString m_connectionName;
};

} // namespace svitlo
