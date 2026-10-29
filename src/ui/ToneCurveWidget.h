#pragma once

#include "core/Histogram.h"

#include <QPointF>
#include <QVector>
#include <QWidget>

namespace svitlo {

// Interactive tone curve editor.
//   click empty space  -> add a point
//   drag a point       -> move it (stays between its neighbours)
//   double-click point -> remove it (end points cannot be removed)
class ToneCurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit ToneCurveWidget(QWidget* parent = nullptr);

    void setPoints(const QVector<QPointF>& pts); // no signal
    QVector<QPointF> points() const { return m_points; }
    void setHistogram(const Histogram& h);

    QSize sizeHint() const override { return {240, 240}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override { return w; }

signals:
    void curveChanged(const QVector<QPointF>& pts); // live
    void editingFinished();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;

private:
    QRectF plotRect() const;
    QPointF toWidget(QPointF p) const;
    QPointF toCurve(QPointF w) const;
    int hitTest(QPointF w) const;

    QVector<QPointF> m_points{QPointF(0, 0), QPointF(1, 1)};
    Histogram m_hist;
    int m_drag = -1;
};

} // namespace svitlo
