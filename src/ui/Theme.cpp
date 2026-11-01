#include "ui/Theme.h"

#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTimer>
#include <QWidget>

namespace svitlo::theme {

#ifdef Q_OS_MACOS
namespace detail {
void setMacAppearance(int mode); // MacAppearance.mm
}
#endif

namespace {

const char* kKeyMode = "ui/theme";

Colors darkColors() {
    Colors c;
    c.dark = true;
    c.window = QColor(0x26, 0x27, 0x29);
    c.panel = QColor(0x1E, 0x1F, 0x21);
    c.canvas = QColor(0x15, 0x16, 0x17);
    c.raised = QColor(0x31, 0x33, 0x36);
    c.border = QColor(0x3A, 0x3C, 0x40);
    c.strong = QColor(0x44, 0x47, 0x4C);
    c.hover = QColor(0x5A, 0x5D, 0x62);
    c.text = QColor(0xDC, 0xDD, 0xDF);
    c.textDim = QColor(0x8D, 0x90, 0x95);
    c.accent = QColor(0xE8, 0xA3, 0x3D);
    c.onAccent = QColor(0x1A, 0x1A, 0x1A);
    c.pick = QColor(0x6F, 0xC2, 0x76);
    c.reject = QColor(0xD9, 0x5C, 0x52);
    return c;
}

Colors lightColors() {
    Colors c;
    c.dark = false;
    c.window = QColor(0xE8, 0xE8, 0xEA);
    c.panel = QColor(0xF6, 0xF6, 0xF7);
    c.canvas = QColor(0xD3, 0xD4, 0xD6); // mid grey: a white surround distorts how photos look
    c.raised = QColor(0xFF, 0xFF, 0xFF);
    c.border = QColor(0xC3, 0xC5, 0xC9);
    c.strong = QColor(0xA9, 0xAC, 0xB1);
    c.hover = QColor(0x8E, 0x91, 0x96);
    c.text = QColor(0x1E, 0x1F, 0x21);
    c.textDim = QColor(0x64, 0x67, 0x6C);
    c.accent = QColor(0xC2, 0x7A, 0x12); // darker amber, readable on light grey
    c.onAccent = QColor(0xFF, 0xFF, 0xFF);
    c.pick = QColor(0x3A, 0x95, 0x44);
    c.reject = QColor(0xC2, 0x45, 0x3B);
    return c;
}

Colors g_colors = darkColors();
bool g_applying = false;
bool g_initialised = false;
QColor g_initialSystemWindow; // fallback for Qt < 6.5, which cannot report the scheme

bool systemIsDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme != Qt::ColorScheme::Unknown)
        return scheme == Qt::ColorScheme::Dark;
#endif
    return g_initialSystemWindow.isValid() ? g_initialSystemWindow.lightness() < 128 : true;
}

// Window frames, title bars and native dialogs follow the chosen look too.
void setNativeScheme(Mode mode) {
#ifdef Q_OS_MACOS
    detail::setMacAppearance(mode == Mode::Dark ? 1 : mode == Mode::Light ? 2 : 0);
#elif QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    if (mode == Mode::System)
        QGuiApplication::styleHints()->unsetColorScheme();
    else
        QGuiApplication::styleHints()->setColorScheme(mode == Mode::Dark ? Qt::ColorScheme::Dark
                                                                         : Qt::ColorScheme::Light);
#else
    Q_UNUSED(mode);
#endif
}

QPalette makePalette(const Colors& c) {
    QPalette p;
    p.setColor(QPalette::Window, c.window);
    p.setColor(QPalette::WindowText, c.text);
    p.setColor(QPalette::Base, c.panel);
    p.setColor(QPalette::AlternateBase, c.raised);
    p.setColor(QPalette::ToolTipBase, c.raised);
    p.setColor(QPalette::ToolTipText, c.text);
    p.setColor(QPalette::Text, c.text);
    p.setColor(QPalette::Button, c.raised);
    p.setColor(QPalette::ButtonText, c.text);
    p.setColor(QPalette::BrightText, c.dark ? Qt::white : Qt::black);
    p.setColor(QPalette::Light, c.raised.lighter(120));
    p.setColor(QPalette::Midlight, c.raised);
    p.setColor(QPalette::Mid, c.border);
    p.setColor(QPalette::Dark, c.strong);
    p.setColor(QPalette::Shadow, c.dark ? Qt::black : c.strong);
    p.setColor(QPalette::Highlight, c.accent);
    p.setColor(QPalette::HighlightedText, c.onAccent);
    p.setColor(QPalette::Link, c.accent);
    p.setColor(QPalette::PlaceholderText, c.textDim);
    p.setColor(QPalette::Disabled, QPalette::Text, c.textDim);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, c.textDim);
    p.setColor(QPalette::Disabled, QPalette::WindowText, c.textDim);
    return p;
}

// Every colour comes from the palette above, so both looks share one sheet.
// Explicit backgrounds for windows and labels keep the look intact even if
// the platform pushes its own palette (macOS does on appearance changes).
QString makeStyleSheet(const Colors& c) {
    QString css = QStringLiteral(R"(
        QMainWindow, QDialog { background: @window@; }
        QLabel { color: @text@; }
        QLabel[dim="true"] { color: @textDim@; }
        QLabel[accent="true"] { color: @accent@; }
        QLabel#brand { color: @accent@; font-size: 16px; font-weight: 600; }
        QLabel#emptyState { color: @textDim@; background: @canvas@; }

        QToolTip { color: @text@; background: @raised@; border: 1px solid @border@; padding: 4px; }

        QToolBar { background: @panel@; border: none; spacing: 4px; padding: 4px; }
        QToolButton { color: @text@; background: transparent; border: 1px solid transparent;
                      border-radius: 5px; padding: 4px 9px; }
        QToolButton:hover { background: @raised@; }
        QToolButton:checked { background: @border@; border-color: @accent@; }

        QToolButton#moduleTab { font-size: 14px; padding: 6px 16px; color: @textDim@; }
        QToolButton#moduleTab:checked { color: @accent@; background: transparent;
                                        border: none; border-bottom: 2px solid @accent@;
                                        border-radius: 0; }

        QToolButton#sectionHeader { color: @textDim@; font-weight: 600; letter-spacing: 1px;
                                    text-align: left; border: none; border-top: 1px solid @border@;
                                    border-radius: 0; padding: 8px 2px; }
        QToolButton#sectionHeader:checked { background: transparent; color: @text@; }
        QToolButton#sectionHeader:hover { color: @accent@; }

        QPushButton { color: @text@; background: @raised@; border: 1px solid @border@;
                      border-radius: 5px; padding: 5px 12px; }
        QPushButton:hover { border-color: @hover@; }
        QPushButton:default { border-color: @accent@; }
        QPushButton:disabled { color: @textDim@; }

        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            color: @text@; background: @panel@; border: 1px solid @border@;
            border-radius: 4px; padding: 3px 6px; }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: @accent@; }
        QComboBox QAbstractItemView { color: @text@; background: @panel@;
                                      selection-background-color: @border@;
                                      selection-color: @text@; }

        QCheckBox { color: @text@; }

        QSlider::groove:horizontal { height: 3px; background: @strong@; border-radius: 1px; }
        QSlider::handle:horizontal { background: @text@; width: 11px; height: 11px;
                                     margin: -5px 0; border-radius: 6px; }
        QSlider::handle:horizontal:hover { background: @accent@; }

        QScrollArea { border: none; background: transparent; }
        QScrollBar:vertical { background: transparent; width: 9px; margin: 0; }
        QScrollBar::handle:vertical { background: @strong@; border-radius: 4px; min-height: 30px; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
        QScrollBar:horizontal { background: transparent; height: 9px; margin: 0; }
        QScrollBar::handle:horizontal { background: @strong@; border-radius: 4px; min-width: 30px; }

        QListView { background: @canvas@; border: none; outline: none; }
        QListWidget#folderList { color: @text@; background: @panel@; border: none; }
        QListWidget#folderList::item { padding: 5px 8px; border-radius: 4px; }
        QListWidget#folderList::item:selected { background: @border@; color: @accent@; }

        QFrame#card { background: @panel@; border: 1px solid @border@; border-radius: 10px; }

        QLabel#sectionTitle { color: @textDim@; font-weight: 600; letter-spacing: 1px; }
        QStatusBar { background: @panel@; color: @textDim@; }
        QMenu { color: @text@; background: @panel@; border: 1px solid @border@; }
        QMenu::item:selected { background: @border@; }
        QMenu::item:disabled { color: @textDim@; }
        QProgressBar { color: @text@; border: 1px solid @border@; border-radius: 4px; text-align: center; }
        QProgressBar::chunk { background: @accent@; }
    )");
    const std::pair<const char*, QColor> tokens[] = {
        {"@window@", c.window}, {"@panel@", c.panel},     {"@canvas@", c.canvas},
        {"@raised@", c.raised}, {"@border@", c.border},   {"@strong@", c.strong},
        {"@hover@", c.hover},   {"@textDim@", c.textDim}, {"@text@", c.text},
        {"@accent@", c.accent},
    };
    for (const auto& [token, color] : tokens)
        css.replace(QLatin1String(token), color.name());
    return css;
}

void reapplyLater() {
    QTimer::singleShot(0, qApp, [] {
        if (auto* app = qobject_cast<QApplication*>(QCoreApplication::instance()))
            apply(*app);
    });
}

// Re-applies Svitlo's palette if the platform replaces it (macOS does so
// when the system appearance changes, which left parts of the window light).
class PaletteGuard : public QObject {
public:
    using QObject::QObject;
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (!g_applying && obj == qApp && ev->type() == QEvent::ApplicationPaletteChange &&
            QGuiApplication::palette().color(QPalette::Window) != g_colors.window)
            reapplyLater();
        return QObject::eventFilter(obj, ev);
    }
};

} // namespace

const Colors& colors() { return g_colors; }

Mode savedMode() {
    const QString v = QSettings().value(kKeyMode, "system").toString();
    return v == "dark" ? Mode::Dark : v == "light" ? Mode::Light : Mode::System;
}

void saveMode(Mode mode) {
    QSettings().setValue(kKeyMode, mode == Mode::Dark ? "dark" : mode == Mode::Light ? "light" : "system");
}

void apply(QApplication& app) {
    if (!g_initialised) {
        g_initialised = true;
        g_initialSystemWindow = QGuiApplication::palette().color(QPalette::Window);
        app.setStyle(QStyleFactory::create("Fusion"));
        app.installEventFilter(new PaletteGuard(&app));
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, &app, [] {
            if (!g_applying && savedMode() == Mode::System)
                reapplyLater();
        });
#endif
    }

    g_applying = true;
    const Mode mode = savedMode();
    setNativeScheme(mode);
    const bool dark = mode == Mode::Dark || (mode == Mode::System && systemIsDark());
    g_colors = dark ? darkColors() : lightColors();
    app.setPalette(makePalette(g_colors));
    app.setStyleSheet(makeStyleSheet(g_colors));
    for (QWidget* w : QApplication::allWidgets())
        w->update();
    g_applying = false;
}

} // namespace svitlo::theme
