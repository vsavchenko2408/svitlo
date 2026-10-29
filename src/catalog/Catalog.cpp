#include "catalog/Catalog.h"

#include "core/ImageLoader.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace svitlo {

namespace {

constexpr int kSchemaVersion = 1;

// A null QString binds as SQL NULL; the settings column is NOT NULL, and with
// INSERT OR IGNORE a NULL would make SQLite silently skip the whole row.
QString nonNull(const QString& s) { return s.isNull() ? QStringLiteral("") : s; }

QString idList(const QVector<qint64>& ids) {
    QStringList parts;
    parts.reserve(ids.size());
    for (qint64 id : ids)
        parts << QString::number(id);
    return parts.join(',');
}

} // namespace

Catalog::Catalog(QObject* parent)
    : QObject(parent), m_connectionName("svitlo-" + QUuid::createUuid().toString(QUuid::WithoutBraces)) {}

Catalog::~Catalog() {
    if (m_db.isOpen())
        m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool Catalog::open(const QString& databasePath, QString* error) {
    QDir().mkpath(QFileInfo(databasePath).absolutePath());
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(databasePath);
    if (!m_db.open()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }
    m_path = databasePath;

    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA foreign_keys=ON");
    const bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS photos (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            path          TEXT NOT NULL UNIQUE,
            folder        TEXT NOT NULL,
            file_name     TEXT NOT NULL,
            capture_time  INTEGER,
            imported_at   INTEGER NOT NULL,
            width         INTEGER DEFAULT 0,
            height        INTEGER DEFAULT 0,
            camera        TEXT DEFAULT '',
            rating        INTEGER NOT NULL DEFAULT 0,
            flag          INTEGER NOT NULL DEFAULT 0,
            settings      TEXT NOT NULL DEFAULT '',
            edit_stamp    INTEGER NOT NULL DEFAULT 0
        ))");
    if (!ok) {
        if (error) *error = q.lastError().text();
        return false;
    }
    q.exec("CREATE INDEX IF NOT EXISTS idx_photos_folder ON photos(folder)");
    q.exec(QString("PRAGMA user_version=%1").arg(kSchemaVersion));
    return true;
}

int Catalog::importFolder(const QString& folder, bool recursive) {
    QDirIterator it(folder, QDir::Files | QDir::Readable,
                    recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
    QStringList paths;
    while (it.hasNext()) {
        const QString path = it.next();
        if (ImageLoader::isSupported(path))
            paths << QFileInfo(path).absoluteFilePath();
    }
    paths.sort();

    int added = 0;
    m_db.transaction();
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT OR IGNORE INTO photos
                 (path, folder, file_name, capture_time, imported_at, settings, edit_stamp)
                 VALUES (?, ?, ?, ?, ?, ?, ?))");
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const QString& path : paths) {
        const QFileInfo fi(path);
        QString settings;
        QFile sidecar(sidecarPath(path));
        if (sidecar.open(QIODevice::ReadOnly)) {
            const EditSettings s = EditSettings::fromJsonString(QString::fromUtf8(sidecar.readAll()));
            if (!s.isIdentity())
                settings = s.toJsonString();
        }
        q.addBindValue(path);
        q.addBindValue(fi.absolutePath());
        q.addBindValue(fi.fileName());
        q.addBindValue(fi.lastModified().toSecsSinceEpoch());
        q.addBindValue(now);
        q.addBindValue(nonNull(settings));
        q.addBindValue(settings.isEmpty() ? 0 : 1);
        if (!q.exec())
            qWarning("Svitlo: import failed for %s: %s", qPrintable(path), qPrintable(q.lastError().text()));
        else if (q.numRowsAffected() > 0)
            ++added;
    }
    m_db.commit();
    if (added > 0)
        emit photosAdded();
    return added;
}

PhotoRecord Catalog::readRow(const QSqlQuery& q) const {
    PhotoRecord r;
    r.id = q.value(0).toLongLong();
    r.path = q.value(1).toString();
    r.folder = q.value(2).toString();
    r.fileName = q.value(3).toString();
    r.captureTime = QDateTime::fromSecsSinceEpoch(q.value(4).toLongLong());
    r.importedAt = QDateTime::fromSecsSinceEpoch(q.value(5).toLongLong());
    r.size = QSize(q.value(6).toInt(), q.value(7).toInt());
    r.camera = q.value(8).toString();
    r.rating = q.value(9).toInt();
    r.flag = Flag(std::clamp(q.value(10).toInt(), -1, 1));
    r.settingsJson = q.value(11).toString();
    r.editStamp = q.value(12).toLongLong();
    return r;
}

static const char* kSelect =
    "SELECT id, path, folder, file_name, capture_time, imported_at, width, height, camera, "
    "rating, flag, settings, edit_stamp FROM photos";

QVector<PhotoRecord> Catalog::allPhotos() const {
    QVector<PhotoRecord> result;
    QSqlQuery q(m_db);
    q.exec(QString(kSelect) + " ORDER BY capture_time, file_name");
    while (q.next())
        result.append(readRow(q));
    return result;
}

PhotoRecord Catalog::photo(qint64 id) const {
    QSqlQuery q(m_db);
    q.prepare(QString(kSelect) + " WHERE id = ?");
    q.addBindValue(id);
    if (q.exec() && q.next())
        return readRow(q);
    return {};
}

void Catalog::setRating(const QVector<qint64>& ids, int rating) {
    if (ids.isEmpty())
        return;
    QSqlQuery q(m_db);
    q.exec(QString("UPDATE photos SET rating = %1 WHERE id IN (%2)")
               .arg(std::clamp(rating, 0, 5))
               .arg(idList(ids)));
    for (qint64 id : ids)
        emit photoChanged(id);
}

void Catalog::setFlag(const QVector<qint64>& ids, Flag flag) {
    if (ids.isEmpty())
        return;
    QSqlQuery q(m_db);
    q.exec(QString("UPDATE photos SET flag = %1 WHERE id IN (%2)").arg(int(flag)).arg(idList(ids)));
    for (qint64 id : ids)
        emit photoChanged(id);
}

void Catalog::setSettings(qint64 id, const EditSettings& settings) {
    const PhotoRecord rec = photo(id);
    if (!rec.id)
        return;
    const QString json = settings.isIdentity() ? QString() : settings.toJsonString();
    if (json == rec.settingsJson)
        return;

    QSqlQuery q(m_db);
    q.prepare("UPDATE photos SET settings = ?, edit_stamp = edit_stamp + 1 WHERE id = ?");
    q.addBindValue(nonNull(json));
    q.addBindValue(id);
    q.exec();

    // Sidecar: written atomically; removed again when the edit is reset.
    const QString side = sidecarPath(rec.path);
    if (json.isEmpty()) {
        QFile::remove(side);
    } else {
        QSaveFile f(side);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(json.toUtf8());
            f.commit(); // may fail on read-only media; the database still has it
        }
    }
    emit photoChanged(id);
}

void Catalog::updateInfo(qint64 id, const ImageInfo& info) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE photos SET width = ?, height = ?, camera = ?, "
              "capture_time = COALESCE(?, capture_time) WHERE id = ?");
    q.addBindValue(info.size.width());
    q.addBindValue(info.size.height());
    q.addBindValue(nonNull(info.camera));
    q.addBindValue(info.captureTime.isValid() ? QVariant(info.captureTime.toSecsSinceEpoch()) : QVariant());
    q.addBindValue(id);
    q.exec();
}

void Catalog::remove(const QVector<qint64>& ids) {
    if (ids.isEmpty())
        return;
    QSqlQuery q(m_db);
    q.exec(QString("DELETE FROM photos WHERE id IN (%1)").arg(idList(ids)));
    emit photosRemoved();
}

QString Catalog::sidecarPath(const QString& photoPath) { return photoPath + ".svitlo.json"; }

} // namespace svitlo
