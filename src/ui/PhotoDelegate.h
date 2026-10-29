#pragma once

#include <QStyledItemDelegate>

namespace svitlo {

// Paints a photo cell: thumbnail, rating dots, flag and an "edited" mark.
class PhotoDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit PhotoDelegate(QObject* parent = nullptr);

    void setCellSize(int px);
    int cellSize() const { return m_cell; }
    void setCompact(bool compact) { m_compact = compact; } // filmstrip: no footer

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int m_cell = 200;
    bool m_compact = false;
};

} // namespace svitlo
