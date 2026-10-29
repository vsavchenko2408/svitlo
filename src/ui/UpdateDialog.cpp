#include "ui/UpdateDialog.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

namespace svitlo {

UpdateDialog::UpdateDialog(const update::ReleaseInfo& info, QWidget* parent)
    : QDialog(parent), m_info(info), m_network(new QNetworkAccessManager(this)) {
    setWindowTitle(tr("Update Available"));
    resize(560, 460);

    auto* title = new QLabel(tr("<span style='font-size:17px; font-weight:600'>Svitlo %1 is available</span>")
                                 .arg(info.version));
    auto* current = new QLabel(tr("You have version %1.").arg(QApplication::applicationVersion()));
    current->setStyleSheet("color: #8D9095;");

    auto* notes = new QTextBrowser;
    notes->setOpenExternalLinks(true);
    notes->setMarkdown(info.notes.isEmpty() ? tr("No release notes.") : info.notes);

    m_progress = new QProgressBar;
    m_progress->setVisible(false);
    m_status = new QLabel;
    m_status->setWordWrap(true);
    m_status->setStyleSheet("color: #8D9095;");

    auto* skip = new QPushButton(tr("Skip This Version"));
    auto* later = new QPushButton(tr("Remind Me Later"));
    auto* page = new QPushButton(tr("Open Release Page"));
    m_downloadButton = new QPushButton(tr("Download"));
    // No download for this platform (e.g. Linux): the release page is the
    // main action instead, and the hidden button must not be the default.
    const bool canDownload = !info.assetUrl.isEmpty();
    m_downloadButton->setVisible(canDownload);
    (canDownload ? m_downloadButton : page)->setDefault(true);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(skip);
    buttons->addStretch();
    buttons->addWidget(later);
    buttons->addWidget(page);
    buttons->addWidget(m_downloadButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(current);
    layout->addWidget(notes, 1);
    layout->addWidget(m_progress);
    layout->addWidget(m_status);
    layout->addLayout(buttons);

    connect(skip, &QPushButton::clicked, this, [this] { done(SkipVersion); });
    connect(later, &QPushButton::clicked, this, &QDialog::reject);
    connect(page, &QPushButton::clicked, this, [this] {
        QDesktopServices::openUrl(QUrl(m_info.pageUrl));
    });
    connect(m_downloadButton, &QPushButton::clicked, this, &UpdateDialog::startDownload);
}

void UpdateDialog::startDownload() {
    if (m_info.assetUrl.isEmpty())
        return;
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir().mkpath(dir);
    QString target = QDir(dir).filePath(m_info.assetName);
    const QFileInfo fi(target);
    for (int n = 2; QFileInfo::exists(target); ++n)
        target = QDir(dir).filePath(QStringLiteral("%1 (%2).%3").arg(fi.completeBaseName()).arg(n).arg(fi.suffix()));

    QNetworkRequest request{QUrl(m_info.assetUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Svitlo/%1").arg(QApplication::applicationVersion()));
    // GitHub redirects asset downloads to its file servers (HTTPS → HTTPS).
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    auto* file = new QSaveFile(target, this);
    if (!file->open(QIODevice::WriteOnly)) {
        m_status->setText(tr("Cannot write to %1").arg(QDir::toNativeSeparators(target)));
        file->deleteLater();
        return;
    }

    m_downloadButton->setEnabled(false);
    m_progress->setVisible(true);
    m_progress->setRange(0, 0);
    m_status->setText(tr("Downloading %1…").arg(m_info.assetName));

    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::readyRead, this, [reply, file] { file->write(reply->readAll()); });
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_progress->setRange(0, 1000);
            m_progress->setValue(int(received * 1000 / total));
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, target] {
        reply->deleteLater();
        file->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            file->cancelWriting();
            file->commit();
            m_status->setText(tr("Download failed: %1").arg(reply->errorString()));
            m_downloadButton->setEnabled(true);
            m_progress->setVisible(false);
            return;
        }
        file->write(reply->readAll());
        if (!file->commit()) {
            m_status->setText(tr("Cannot save %1").arg(QDir::toNativeSeparators(target)));
            m_downloadButton->setEnabled(true);
            return;
        }
        downloadFinished(target);
    });
}

void UpdateDialog::downloadFinished(const QString& path) {
    m_progress->setRange(0, 1);
    m_progress->setValue(1);
#if defined(Q_OS_MACOS)
    m_status->setText(tr("Downloaded. Quit Svitlo, then drag the new Svitlo from the opened disk image "
                         "into Applications and replace the old one."));
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#else
    m_status->setText(tr("Downloaded to %1. Quit Svitlo, unzip the file and replace your old Svitlo "
                         "folder with the new one.")
                          .arg(QDir::toNativeSeparators(path)));
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
#endif
    m_downloadButton->setText(tr("Quit Svitlo"));
    m_downloadButton->setEnabled(true);
    disconnect(m_downloadButton, nullptr, this, nullptr);
    connect(m_downloadButton, &QPushButton::clicked, this, [this] {
        accept();
        QApplication::closeAllWindows();
    });
}

} // namespace svitlo
