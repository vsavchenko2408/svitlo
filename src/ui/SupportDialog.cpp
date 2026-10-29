#include "ui/SupportDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace svitlo {

namespace {

// Reminder schedule (in app launches).
constexpr int kFirstReminder = 5;    // never before the 5th launch
constexpr int kRemindLater = 15;     // "Remind me later"
constexpr int kAfterSupport = 60;    // after clicking Ko-fi / copying the address

const char* kKeyLaunches = "support/launchCount";
const char* kKeyNext = "support/nextReminder";
const char* kKeyDisabled = "support/disabled";

QLabel* qrLabel(const QString& resource) {
    auto* label = new QLabel;
    QPixmap pix(resource);
    label->setPixmap(pix.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    label->setFixedSize(158, 158);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("background: white; border-radius: 8px;");
    return label;
}

QFrame* card() {
    auto* f = new QFrame;
    f->setStyleSheet("QFrame#card { background: #1E1F21; border: 1px solid #3A3C40; border-radius: 10px; }");
    f->setObjectName("card");
    return f;
}

} // namespace

SupportDialog::SupportDialog(Mode mode, QWidget* parent) : QDialog(parent), m_mode(mode) {
    setWindowTitle(tr("Support Svitlo"));
    setModal(true);

    auto* title = new QLabel(tr("<span style='font-size:18px; font-weight:600'>Enjoying Svitlo?</span>"));
    auto* text = new QLabel(tr(
        "Svitlo is free and open source, made in my spare time.<br>"
        "If it is useful to you, a small donation helps me keep improving it. "
        "Thank you! ♥"));
    text->setWordWrap(true);
    text->setStyleSheet("color: #8D9095;");

    // --- Ko-fi ---------------------------------------------------------------
    auto* kofi = card();
    auto* kofiLayout = new QVBoxLayout(kofi);
    kofiLayout->setContentsMargins(14, 14, 14, 14);
    auto* kofiTitle = new QLabel(tr("<b>Card or PayPal</b>"));
    auto* kofiButton = new QPushButton(tr("Open Ko-fi"));
    kofiButton->setDefault(true);
    kofiLayout->addWidget(kofiTitle, 0, Qt::AlignHCenter);
    kofiLayout->addWidget(qrLabel(":/support/qr-kofi.png"), 0, Qt::AlignHCenter);
    kofiLayout->addWidget(new QLabel("ko-fi.com/dermeister"), 0, Qt::AlignHCenter);
    kofiLayout->addStretch();
    kofiLayout->addWidget(kofiButton);

    // --- Bitcoin -------------------------------------------------------------
    auto* btc = card();
    auto* btcLayout = new QVBoxLayout(btc);
    btcLayout->setContentsMargins(14, 14, 14, 14);
    auto* btcTitle = new QLabel(tr("<b>Bitcoin (BTC)</b>"));
    // The address has no spaces, so it is split into two lines for display.
    // It is deliberately not selectable: copying from a wrapped label could
    // pick up the line break — the "Copy address" button copies it exactly.
    const QString addr = QString::fromLatin1(kBitcoinAddress);
    const int half = (addr.size() + 1) / 2;
    auto* address = new QLabel(addr.left(half) + '\n' + addr.mid(half));
    address->setToolTip(tr("Use “Copy address” to copy it exactly"));
    address->setAlignment(Qt::AlignCenter);
    QFont mono("Menlo");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSizeF(address->font().pointSizeF() * 0.9);
    address->setFont(mono);
    address->setMaximumWidth(220);
    auto* warning = new QLabel(tr("Send only BTC on the Bitcoin network.\n"
                                  "Other coins sent here will be lost."));
    warning->setAlignment(Qt::AlignCenter);
    warning->setStyleSheet("color: #E8A33D; font-size: 11px;");
    auto* copyButton = new QPushButton(tr("Copy address"));
    btcLayout->addWidget(btcTitle, 0, Qt::AlignHCenter);
    btcLayout->addWidget(qrLabel(":/support/qr-bitcoin.png"), 0, Qt::AlignHCenter);
    btcLayout->addWidget(address, 0, Qt::AlignHCenter);
    btcLayout->addWidget(warning);
    btcLayout->addStretch();
    btcLayout->addWidget(copyButton);

    auto* cards = new QHBoxLayout;
    cards->setSpacing(12);
    cards->addWidget(kofi);
    cards->addWidget(btc);

    // --- Bottom buttons --------------------------------------------------------
    auto* buttons = new QHBoxLayout;
    if (m_mode == Mode::Reminder) {
        auto* never = new QPushButton(tr("Don't show again"));
        auto* later = new QPushButton(tr("Remind me later"));
        buttons->addWidget(never);
        buttons->addStretch();
        buttons->addWidget(later);
        connect(never, &QPushButton::clicked, this, [this] {
            QSettings().setValue(kKeyDisabled, true);
            reject();
        });
        connect(later, &QPushButton::clicked, this, [this] {
            QSettings s;
            s.setValue(kKeyNext, s.value(kKeyLaunches, 0).toInt() + kRemindLater);
            reject();
        });
    } else {
        auto* close = new QPushButton(tr("Close"));
        buttons->addStretch();
        buttons->addWidget(close);
        connect(close, &QPushButton::clicked, this, &QDialog::accept);
    }

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(text);
    layout->addLayout(cards);
    layout->addLayout(buttons);

    connect(kofiButton, &QPushButton::clicked, this, [this] {
        QDesktopServices::openUrl(QUrl(QString::fromLatin1(kKofiUrl)));
        rememberSupported();
    });
    connect(copyButton, &QPushButton::clicked, this, [this, copyButton] {
        QApplication::clipboard()->setText(QString::fromLatin1(kBitcoinAddress));
        copyButton->setText(tr("Copied ✓"));
        QTimer::singleShot(2000, copyButton, [copyButton] { copyButton->setText(tr("Copy address")); });
        rememberSupported();
    });
}

void SupportDialog::rememberSupported() {
    // We cannot know whether a donation was actually made, so just give the
    // person a long break from reminders.
    QSettings s;
    s.setValue(kKeyNext, s.value(kKeyLaunches, 0).toInt() + kAfterSupport);
}

void SupportDialog::maybeShowOnStartup(QWidget* parent) {
    QSettings s;
    const int launches = s.value(kKeyLaunches, 0).toInt() + 1;
    s.setValue(kKeyLaunches, launches);
    if (s.value(kKeyDisabled, false).toBool())
        return;
    if (launches < s.value(kKeyNext, kFirstReminder).toInt())
        return;

    // Default next reminder in case the window is just closed.
    s.setValue(kKeyNext, launches + kRemindLater);
    // Small delay so the main window is fully drawn first.
    QTimer::singleShot(1500, parent, [parent] {
        SupportDialog dialog(Mode::Reminder, parent);
        dialog.exec();
    });
}

} // namespace svitlo
