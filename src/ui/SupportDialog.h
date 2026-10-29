#pragma once

#include <QDialog>

namespace svitlo {

// "Support Svitlo" dialog: Ko-fi link and Bitcoin address with QR codes.
//
// Shown from Help → Support Svitlo, and occasionally on start-up (see
// maybeShowOnStartup) — never on the first launches and never again once the
// user chooses "Don't show again".
class SupportDialog : public QDialog {
    Q_OBJECT
public:
    // Donation details, kept in one place.
    static constexpr const char* kKofiUrl = "https://ko-fi.com/dermeister";
    static constexpr const char* kBitcoinAddress = "bc1qcqw855rmkvqn0zryfwecfkukwhspdy6rqlnjux";

    enum class Mode { Manual, Reminder };

    explicit SupportDialog(Mode mode, QWidget* parent = nullptr);

    // Counts launches and shows the reminder when it is due. Call once the
    // main window is visible.
    static void maybeShowOnStartup(QWidget* parent);

private:
    void rememberSupported();

    Mode m_mode;
};

} // namespace svitlo
