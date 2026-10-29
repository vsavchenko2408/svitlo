#pragma once

#include "core/UpdateInfo.h"

#include <QObject>

class QNetworkAccessManager;
class QWidget;

namespace svitlo {

// Checks GitHub for a newer Svitlo release.
//
// Only one anonymous request is made to api.github.com; nothing about the
// user or their photos is sent. Automatic checks run at most once a day and
// can be switched off in Help → "Check for Updates at Startup".
class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(QWidget* window);

    static bool checkAtStartupEnabled();
    static void setCheckAtStartupEnabled(bool enabled);

    // Called once the main window is shown: checks if enabled and due.
    void checkOnStartup();
    // Help → Check for Updates…: always reports the result.
    void checkNow();

private:
    void check(bool manual);
    void showResult(const update::ReleaseInfo& info, bool manual);

    QWidget* m_window;
    QNetworkAccessManager* m_network;
    bool m_busy = false;
};

} // namespace svitlo
