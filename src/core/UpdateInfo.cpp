#include "core/UpdateInfo.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>

namespace svitlo::update {

QString normalizeVersion(const QString& tag) {
    QString v = tag.trimmed();
    while (!v.isEmpty() && (v.front() == 'v' || v.front() == 'V' || v.front() == '.'))
        v.remove(0, 1);
    return v;
}

bool isNewerVersion(const QString& remote, const QString& current) {
    const QVersionNumber r = QVersionNumber::fromString(normalizeVersion(remote));
    const QVersionNumber c = QVersionNumber::fromString(normalizeVersion(current));
    if (r.isNull())
        return false;
    return QVersionNumber::compare(r, c) > 0;
}

QString platformAssetSuffix() {
#if defined(Q_OS_MACOS)
    return QStringLiteral(".dmg");
#elif defined(Q_OS_WIN)
    return QStringLiteral("-windows-x64.zip");
#else
    return {};
#endif
}

ReleaseInfo parseLatestRelease(const QByteArray& json, const QString& assetSuffix) {
    ReleaseInfo info;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject())
        return info;
    const QJsonObject o = doc.object();
    if (o.value("draft").toBool() || o.value("prerelease").toBool())
        return info;

    info.tag = o.value("tag_name").toString();
    info.version = normalizeVersion(info.tag);
    info.notes = o.value("body").toString();
    info.pageUrl = o.value("html_url").toString();
    info.valid = !info.version.isEmpty() && !QVersionNumber::fromString(info.version).isNull();

    if (!assetSuffix.isEmpty()) {
        for (const QJsonValue& a : o.value("assets").toArray()) {
            const QJsonObject asset = a.toObject();
            const QString name = asset.value("name").toString();
            if (name.endsWith(assetSuffix, Qt::CaseInsensitive)) {
                info.assetName = name;
                info.assetUrl = asset.value("browser_download_url").toString();
                info.assetSize = qint64(asset.value("size").toDouble());
                break;
            }
        }
    }
    return info;
}

} // namespace svitlo::update
