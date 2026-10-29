#include "ui/PhotoDelegate.h"

#include "ui/PhotoModel.h"
#include "ui/Theme.h"

#include <QPainter>
#include <QPainterPath>

namespace svitlo {

namespace {
constexpr int kFooter = 22;
constexpr int kMargin = 6;
} // namespace

PhotoDelegate::PhotoDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void PhotoDelegate::setCellSize(int px) { m_cell = std::clamp(px, 60, 600); }

QSize PhotoDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    return m_compact ? QSize(m_cell, m_cell) : QSize(m_cell, m_cell + kFooter);
}

void PhotoDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& index) const {
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::SmoothPixmapTransform);

    const bool selected = opt.state & QStyle::State_Selected;
    const bool hovered = opt.state & QStyle::State_MouseOver;
    const QRect cell = opt.rect.adjusted(2, 2, -2, -2);

    // Cell background.
    QColor bg = theme::kPanel;
    if (selected) bg = theme::kRaised.lighter(115);
    else if (hovered) bg = theme::kRaised;
    QPainterPath bgPath;
    bgPath.addRoundedRect(cell, 6, 6);
    p->fillPath(bgPath, bg);
    if (selected) {
        p->setPen(QPen(theme::kAccent, 1.5));
        p->drawPath(bgPath);
    }

    const int flag = index.data(PhotoModel::FlagRole).toInt();
    const QRect imageArea = m_compact ? cell.adjusted(kMargin, kMargin, -kMargin, -kMargin)
                                      : QRect(cell.left() + kMargin, cell.top() + kMargin,
                                              cell.width() - 2 * kMargin, cell.height() - 2 * kMargin - kFooter);

    // Thumbnail, fitted.
    const QImage thumb = index.data(PhotoModel::ThumbnailRole).value<QImage>();
    if (!thumb.isNull()) {
        const QSize s = thumb.size().scaled(imageArea.size(), Qt::KeepAspectRatio);
        const QRect target(imageArea.center().x() - s.width() / 2 + 1,
                           imageArea.center().y() - s.height() / 2 + 1, s.width(), s.height());
        if (flag < 0)
            p->setOpacity(0.35); // rejected photos are dimmed
        p->drawImage(target, thumb);
        p->setOpacity(1.0);
    } else {
        p->setPen(theme::kTextDim);
        p->drawText(imageArea, Qt::AlignCenter, QStringLiteral("…"));
    }

    // Flag badge, top-left.
    if (flag != 0) {
        const QRect badge(imageArea.left() + 4, imageArea.top() + 4, 16, 16);
        QPainterPath path;
        path.addRoundedRect(badge, 4, 4);
        p->fillPath(path, flag > 0 ? theme::kPick : theme::kReject);
        p->setPen(QPen(QColor(0x15, 0x16, 0x17), 2.0, Qt::SolidLine, Qt::RoundCap));
        if (flag > 0) {
            p->drawLine(badge.left() + 4, badge.center().y(), badge.center().x() - 1, badge.bottom() - 4);
            p->drawLine(badge.center().x() - 1, badge.bottom() - 4, badge.right() - 3, badge.top() + 4);
        } else {
            p->drawLine(badge.topLeft() + QPoint(4, 4), badge.bottomRight() - QPoint(4, 4));
            p->drawLine(badge.topRight() + QPoint(-4, 4), badge.bottomLeft() + QPoint(4, -4));
        }
    }

    // "Edited" mark, top-right: a small accent dot.
    if (index.data(PhotoModel::EditedRole).toBool()) {
        p->setPen(Qt::NoPen);
        p->setBrush(theme::kAccent);
        p->drawEllipse(QPointF(imageArea.right() - 8, imageArea.top() + 8), 4, 4);
    }

    // Footer: rating dots and the file name.
    if (!m_compact) {
        const QRect footer(cell.left() + kMargin, cell.bottom() - kFooter, cell.width() - 2 * kMargin, kFooter);
        const int rating = index.data(PhotoModel::RatingRole).toInt();
        for (int i = 0; i < 5; ++i) {
            const QPointF c(footer.left() + 5 + i * 11, footer.center().y());
            p->setPen(Qt::NoPen);
            p->setBrush(i < rating ? theme::kAccent : theme::kBorder);
            p->drawEllipse(c, i < rating ? 3.5 : 2.0, i < rating ? 3.5 : 2.0);
        }
        p->setPen(theme::kTextDim);
        QFont f = opt.font;
        f.setPointSizeF(f.pointSizeF() * 0.85);
        p->setFont(f);
        const QRect nameRect = footer.adjusted(62, 0, 0, 0);
        const QString name = p->fontMetrics().elidedText(index.data(Qt::DisplayRole).toString(),
                                                         Qt::ElideMiddle, nameRect.width());
        p->drawText(nameRect, Qt::AlignRight | Qt::AlignVCenter, name);
    }
    p->restore();
}

} // namespace svitlo
