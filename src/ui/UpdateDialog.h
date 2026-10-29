#pragma once

#include "core/UpdateInfo.h"

#include <QDialog>

class QNetworkAccessManager;
class QProgressBar;
class QPushButton;
class QLabel;

namespace svitlo {

// Shows a new release and downloads the installer for this platform into the
// Downloads folder, then opens it (macOS mounts the .dmg; on Windows the
// folder with the .zip is shown). Installing is left to the user: the app is
// not code-signed yet, so it does not replace itself silently.
class UpdateDialog : public QDialog {
    Q_OBJECT
public:
    enum Result { Later = QDialog::Rejected, Done = QDialog::Accepted, SkipVersion = 2 };

    UpdateDialog(const update::ReleaseInfo& info, QWidget* parent = nullptr);

private:
    void startDownload();
    void downloadFinished(const QString& path);

    update::ReleaseInfo m_info;
    QNetworkAccessManager* m_network;
    QProgressBar* m_progress;
    QLabel* m_status;
    QPushButton* m_downloadButton;
};

} // namespace svitlo
