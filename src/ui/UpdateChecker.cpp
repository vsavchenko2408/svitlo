#include "ui/UpdateChecker.h"

#include "ui/UpdateDialog.h"

#include <QApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QWidget>

namespace svitlo {

namespace {
const char* kKeyEnabled = "updates/checkAtStartup";
const char* kKeyLastCheck = "updates/lastCheck";
const char* kKeySkip = "updates/skipVersion";
constexpr qint64 kMinSecondsBetweenChecks = 24 * 60 * 60;
} // namespace

UpdateChecker::UpdateChecker(QWidget* window)
    : QObject(window), m_window(window), m_network(new QNetworkAccessManager(this)) {}

bool UpdateChecker::checkAtStartupEnabled() { return QSettings().value(kKeyEnabled, true).toBool(); }

void UpdateChecker::setCheckAtStartupEnabled(bool enabled) { QSettings().setValue(kKeyEnabled, enabled); }

void UpdateChecker::checkOnStartup() {
    if (!checkAtStartupEnabled())
        return;
    const qint64 last = QSettings().value(kKeyLastCheck, 0).toLongLong();
    if (QDateTime::currentSecsSinceEpoch() - last < kMinSecondsBetweenChecks)
        return;
    // A few seconds after start, so the window is up and the network idle.
    QTimer::singleShot(4000, this, [this] { check(false); });
}

void UpdateChecker::checkNow() { check(true); }

void UpdateChecker::check(bool manual) {
    if (m_busy)
        return;
    m_busy = true;

    // SVITLO_UPDATE_API lets developers test against a local server.
    const QString api = qEnvironmentVariableIsSet("SVITLO_UPDATE_API")
                            ? qEnvironmentVariable("SVITLO_UPDATE_API")
                            : QString::fromLatin1(update::kLatestReleaseApi);
    QNetworkRequest request{QUrl(api)};
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Svitlo/%1").arg(QApplication::applicationVersion()));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(15000);

    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manual] {
        reply->deleteLater();
        m_busy = false;
        if (reply->error() != QNetworkReply::NoError) {
            if (manual)
                QMessageBox::warning(m_window, tr("Check for Updates"),
                                     tr("Could not check for updates:\n%1").arg(reply->errorString()));
            return;
        }
        QSettings().setValue(kKeyLastCheck, QDateTime::currentSecsSinceEpoch());
        // SVITLO_UPDATE_ASSET_SUFFIX: developer override for testing downloads.
        const QString suffix = qEnvironmentVariableIsSet("SVITLO_UPDATE_ASSET_SUFFIX")
                                   ? qEnvironmentVariable("SVITLO_UPDATE_ASSET_SUFFIX")
                                   : update::platformAssetSuffix();
        showResult(update::parseLatestRelease(reply->readAll(), suffix), manual);
    });
}

void UpdateChecker::showResult(const update::ReleaseInfo& info, bool manual) {
    const QString current = QApplication::applicationVersion();
    if (!info.valid || !update::isNewerVersion(info.version, current)) {
        if (manual)
            QMessageBox::information(m_window, tr("Check for Updates"),
                                     tr("You have the latest version of Svitlo (%1).").arg(current));
        return;
    }
    // Automatic checks stay quiet about a version the user chose to skip.
    if (!manual && QSettings().value(kKeySkip).toString() == info.version)
        return;

    UpdateDialog dialog(info, m_window);
    if (dialog.exec() == UpdateDialog::SkipVersion)
        QSettings().setValue(kKeySkip, info.version);
}

} // namespace svitlo
