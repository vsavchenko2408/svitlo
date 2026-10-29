#include "ui/CollapsibleSection.h"

#include <QToolButton>
#include <QVBoxLayout>

namespace svitlo {

CollapsibleSection::CollapsibleSection(const QString& title, bool expanded, QWidget* parent)
    : QWidget(parent) {
    m_header = new QToolButton;
    // "&" would otherwise be eaten as a keyboard-mnemonic marker.
    m_header->setText(title.toUpper().replace('&', QStringLiteral("&&")));
    m_header->setCheckable(true);
    m_header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_header->setStyleSheet(
        "QToolButton { color: #8D9095; font-weight: 600; letter-spacing: 1px; text-align: left;"
        " border: none; border-top: 1px solid #3A3C40; border-radius: 0; padding: 8px 2px; }"
        "QToolButton:checked { background: transparent; color: #DCDDDF; }"
        "QToolButton:hover { color: #E8A33D; }");

    m_content = new QWidget;
    m_contentLayout = new QVBoxLayout(m_content);
    m_contentLayout->setContentsMargins(2, 0, 2, 10);
    m_contentLayout->setSpacing(4);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_header);
    layout->addWidget(m_content);

    connect(m_header, &QToolButton::toggled, this, &CollapsibleSection::setExpanded);
    setExpanded(expanded);
}

void CollapsibleSection::setExpanded(bool expanded) {
    const QSignalBlocker b(m_header);
    m_header->setChecked(expanded);
    m_header->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    m_content->setVisible(expanded);
}

} // namespace svitlo
