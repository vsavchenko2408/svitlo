#include "ui/Theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace svitlo::theme {

void apply(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window, kWindow);
    p.setColor(QPalette::WindowText, kText);
    p.setColor(QPalette::Base, kPanel);
    p.setColor(QPalette::AlternateBase, kRaised);
    p.setColor(QPalette::ToolTipBase, kRaised);
    p.setColor(QPalette::ToolTipText, kText);
    p.setColor(QPalette::Text, kText);
    p.setColor(QPalette::Button, kRaised);
    p.setColor(QPalette::ButtonText, kText);
    p.setColor(QPalette::BrightText, Qt::white);
    p.setColor(QPalette::Highlight, kAccent);
    p.setColor(QPalette::HighlightedText, QColor(0x1A, 0x1A, 0x1A));
    p.setColor(QPalette::Link, kAccent);
    p.setColor(QPalette::PlaceholderText, kTextDim);
    p.setColor(QPalette::Disabled, QPalette::Text, kTextDim);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, kTextDim);
    p.setColor(QPalette::Disabled, QPalette::WindowText, kTextDim);
    app.setPalette(p);

    app.setStyleSheet(QStringLiteral(R"(
        QToolTip { color: #DCDDDF; background: #313336; border: 1px solid #3A3C40; padding: 4px; }

        QToolBar { background: #1E1F21; border: none; spacing: 4px; padding: 4px; }
        QToolButton { color: #DCDDDF; background: transparent; border: 1px solid transparent;
                      border-radius: 5px; padding: 4px 9px; }
        QToolButton:hover { background: #313336; }
        QToolButton:checked { background: #3A3C40; border-color: #E8A33D; }

        QToolButton#moduleTab { font-size: 14px; padding: 6px 16px; color: #8D9095; }
        QToolButton#moduleTab:checked { color: #E8A33D; background: transparent;
                                        border: none; border-bottom: 2px solid #E8A33D;
                                        border-radius: 0; }

        QPushButton { background: #313336; border: 1px solid #3A3C40; border-radius: 5px;
                      padding: 5px 12px; }
        QPushButton:hover { border-color: #5A5D62; }
        QPushButton:default { border-color: #E8A33D; }

        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            background: #1E1F21; border: 1px solid #3A3C40; border-radius: 4px; padding: 3px 6px; }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: #E8A33D; }

        QSlider::groove:horizontal { height: 3px; background: #44474C; border-radius: 1px; }
        QSlider::handle:horizontal { background: #DCDDDF; width: 11px; height: 11px;
                                     margin: -5px 0; border-radius: 6px; }
        QSlider::handle:horizontal:hover { background: #E8A33D; }

        QScrollArea { border: none; background: transparent; }
        QScrollBar:vertical { background: transparent; width: 9px; margin: 0; }
        QScrollBar::handle:vertical { background: #44474C; border-radius: 4px; min-height: 30px; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
        QScrollBar:horizontal { background: transparent; height: 9px; margin: 0; }
        QScrollBar::handle:horizontal { background: #44474C; border-radius: 4px; min-width: 30px; }

        QListView { background: #151617; border: none; outline: none; }
        QListWidget#folderList { background: #1E1F21; border: none; }
        QListWidget#folderList::item { padding: 5px 8px; border-radius: 4px; }
        QListWidget#folderList::item:selected { background: #3A3C40; color: #E8A33D; }

        QLabel#sectionTitle { color: #8D9095; font-weight: 600; letter-spacing: 1px; }
        QStatusBar { background: #1E1F21; color: #8D9095; }
        QMenu { background: #26272A; border: 1px solid #3A3C40; }
        QMenu::item:selected { background: #3A3C40; }
        QProgressBar { border: 1px solid #3A3C40; border-radius: 4px; text-align: center; }
        QProgressBar::chunk { background: #E8A33D; }
    )"));
}

} // namespace svitlo::theme
