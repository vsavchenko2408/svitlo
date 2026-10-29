#pragma once

#include <QWidget>

class QToolButton;
class QVBoxLayout;

namespace svitlo {

// A titled panel section that folds open/closed when its header is clicked.
class CollapsibleSection : public QWidget {
    Q_OBJECT
public:
    explicit CollapsibleSection(const QString& title, bool expanded = true, QWidget* parent = nullptr);

    QVBoxLayout* contentLayout() const { return m_contentLayout; }
    void setExpanded(bool expanded);

private:
    QToolButton* m_header;
    QWidget* m_content;
    QVBoxLayout* m_contentLayout;
};

} // namespace svitlo
