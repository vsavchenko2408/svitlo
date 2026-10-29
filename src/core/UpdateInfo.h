#pragma once

#include <QByteArray>
#include <QString>

namespace svitlo::update {

// The latest release as published on GitHub.
struct ReleaseInfo {
    bool valid = false;
    QString tag;        // e.g. "v0.3.0"
    QString version;    // e.g. "0.3.0"
    QString notes;      // release description (Markdown)
    QString pageUrl;    // release page on GitHub
    QString assetName;  // download for this platform, empty if none
    QString assetUrl;
    qint64 assetSize = 0;
};

// Where Svitlo asks for the latest release (public GitHub API, no account needed).
constexpr const char* kLatestReleaseApi =
    "https://api.github.com/repos/vsavchenko2408/svitlo/releases/latest";

// "v0.3.0", "V.0.3.0", "0.3.0" -> "0.3.0"
QString normalizeVersion(const QString& tag);

// True if `remote` is a newer version than `current` (both may carry a "v").
bool isNewerVersion(const QString& remote, const QString& current);

// File-name ending of the download for the running platform:
// ".dmg" on macOS, "-windows-x64.zip" on Windows, empty elsewhere.
QString platformAssetSuffix();

// Parses the JSON from kLatestReleaseApi and picks the asset whose name ends
// with `assetSuffix` (no asset is picked when the suffix is empty).
ReleaseInfo parseLatestRelease(const QByteArray& json, const QString& assetSuffix);

} // namespace svitlo::update
