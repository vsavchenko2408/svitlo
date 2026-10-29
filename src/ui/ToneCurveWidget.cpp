#include "ui/ToneCurveWidget.h"

#include "core/ToneCurve.h"
#include "ui/Theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace svitlo {

namespace {
constexpr double kHitRadius = 8.0;
constexpr double kMinGap = 0.02; // minimum x distance between points
} // namespace

ToneCurveWidget::ToneCurveWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(180, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMouseTracking(true);
    setToolTip(tr("Click to add a point, drag to move, double-click a point to remove it"));
}

void ToneCurveWidget::setPoints(const QVector<QPointF>& pts) {
    m_points = pts.size() >= 2 ? pts : QVector<QPointF>{QPointF(0, 0), QPointF(1, 1)};
    update();
}

void ToneCurveWidget::setHistogram(const Histogram& h) {
    m_hist = h;
    update();
}

QRectF ToneCurveWidget::plotRect() const {
    const double side = std::min(width(), height()) - 12.0;
    return {(width() - side) / 2.0, 6.0, side, side};
}

QPointF ToneCurveWidget::toWidget(QPointF p) const {
    const QRectF r = plotRect();
    return {r.left() + p.x() * r.width(), r.bottom() - p.y() * r.height()};
}

QPointF ToneCurveWidget::toCurve(QPointF w) const {
    const QRectF r = plotRect();
    return {std::clamp((w.x() - r.left()) / r.width(), 0.0, 1.0),
            std::clamp((r.bottom() - w.y()) / r.height(), 0.0, 1.0)};
}

int ToneCurveWidget::hitTest(QPointF w) const {
    for (int i = 0; i < m_points.size(); ++i)
        if (QLineF(toWidget(m_points[i]), w).length() <= kHitRadius)
            return i;
    return -1;
}

void ToneCurveWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = plotRect();

    p.fillRect(r, theme::kCanvas);

    // Luma histogram as a faint backdrop.
    if (!m_hist.isEmpty()) {
        const double peak = m_hist.peak();
        QPainterPath hist;
        hist.moveTo(r.bottomLeft());
        for (int i = 0; i < 256; ++i) {
            const double v = std::min(1.0, m_hist.luma[i] / peak);
            hist.lineTo(r.left() + i / 255.0 * r.width(), r.bottom() - v * r.height() * 0.9);
        }
        hist.lineTo(r.bottomRight());
        hist.closeSubpath();
        p.fillPath(hist, QColor(255, 255, 255, 22));
    }

    // Quarter grid and the identity diagonal.
    p.setPen(QPen(theme::kBorder, 1));
    for (int i = 1; i < 4; ++i) {
        const double x = r.left() + r.width() * i / 4.0;
        const double y = r.top() + r.height() * i / 4.0;
        p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
    }
    p.setPen(QPen(theme::kBorder, 1, Qt::DashLine));
    p.drawLine(r.bottomLeft(), r.topRight());
    p.setPen(QPen(theme::kBorder, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    // The curve itself, drawn through the same interpolator the pipeline uses.
    const ToneCurve curve(m_points);
    QPainterPath path;
    const int steps = int(r.width());
    for (int i = 0; i <= steps; ++i) {
        const double x = double(i) / steps;
        const QPointF w = toWidget(QPointF(x, curve.evaluate(x)));
        i == 0 ? path.moveTo(w) : path.lineTo(w);
    }
    p.setPen(QPen(theme::kText, 1.6));
    p.drawPath(path);

    for (int i = 0; i < m_points.size(); ++i) {
        const QPointF w = toWidget(m_points[i]);
        p.setPen(QPen(theme::kText, 1.2));
        p.setBrush(i == m_drag ? theme::kAccent : theme::kCanvas);
        p.drawEllipse(w, 4.5, 4.5);
    }
}

void ToneCurveWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton)
        return;
    const QPointF pos = e->position();
    m_drag = hitTest(pos);
    if (m_drag < 0 && plotRect().adjusted(-4, -4, 4, 4).contains(pos)) {
        const QPointF c = toCurve(pos);
        // Insert in x order, unless it would crowd an existing point.
        int insertAt = 0;
        while (insertAt < m_points.size() && m_points[insertAt].x() < c.x())
            ++insertAt;
        const bool crowded = (insertAt > 0 && c.x() - m_points[insertAt - 1].x() < kMinGap) ||
                             (insertAt < m_points.size() && m_points[insertAt].x() - c.x() < kMinGap);
        if (!crowded && insertAt > 0 && insertAt < m_points.size()) {
            m_points.insert(insertAt, c);
            m_drag = insertAt;
            emit curveChanged(m_points);
        }
    }
    update();
}

void ToneCurveWidget::mouseMoveEvent(QMouseEvent* e) {
    if (m_drag < 0) {
        setCursor(hitTest(e->position()) >= 0 ? Qt::PointingHandCursor : Qt::CrossCursor);
        return;
    }
    QPointF c = toCurve(e->position());
    const int last = m_points.size() - 1;
    if (m_drag == 0) {
        c.setX(std::min(c.x(), m_points[1].x() - kMinGap));
    } else if (m_drag == last) {
        c.setX(std::max(c.x(), m_points[last - 1].x() + kMinGap));
    } else {
        c.setX(std::clamp(c.x(), m_points[m_drag - 1].x() + kMinGap, m_points[m_drag + 1].x() - kMinGap));
    }
    m_points[m_drag] = c;
    emit curveChanged(m_points);
    update();
}

void ToneCurveWidget::mouseReleaseEvent(QMouseEvent*) {
    if (m_drag >= 0) {
        m_drag = -1;
        emit editingFinished();
        update();
    }
}

void ToneCurveWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    const int i = hitTest(e->position());
    if (i > 0 && i < m_points.size() - 1) {
        m_points.removeAt(i);
        m_drag = -1;
        emit curveChanged(m_points);
        emit editingFinished();
        update();
    }
}

} // namespace svitlo
