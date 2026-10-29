#pragma once

#include <QPointF>
#include <QVector>

namespace svitlo {

// Monotone cubic (Fritsch–Carlson) interpolation through user control points.
// Monotone interpolation never overshoots, so the curve cannot invert tones
// between two points the way a plain cubic spline can.
class ToneCurve {
public:
    explicit ToneCurve(const QVector<QPointF>& points);

    // x in [0,1] -> y in [0,1]
    double evaluate(double x) const;

private:
    QVector<double> m_x, m_y, m_tangent;
};

} // namespace svitlo
