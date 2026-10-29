#include "core/ToneCurve.h"

#include <algorithm>
#include <cmath>

namespace svitlo {

ToneCurve::ToneCurve(const QVector<QPointF>& points) {
    QVector<QPointF> pts = points;
    std::sort(pts.begin(), pts.end(), [](const QPointF& a, const QPointF& b) { return a.x() < b.x(); });

    // Drop points that share an x with their predecessor.
    for (const QPointF& p : pts) {
        if (!m_x.isEmpty() && p.x() - m_x.last() < 1e-6)
            continue;
        m_x.append(std::clamp(p.x(), 0.0, 1.0));
        m_y.append(std::clamp(p.y(), 0.0, 1.0));
    }
    if (m_x.size() < 2) {
        m_x = {0.0, 1.0};
        m_y = {0.0, 1.0};
    }

    const int n = m_x.size();
    QVector<double> slope(n - 1);
    for (int i = 0; i < n - 1; ++i)
        slope[i] = (m_y[i + 1] - m_y[i]) / (m_x[i + 1] - m_x[i]);

    m_tangent.resize(n);
    m_tangent[0] = slope[0];
    m_tangent[n - 1] = slope[n - 2];
    for (int i = 1; i < n - 1; ++i)
        m_tangent[i] = (slope[i - 1] * slope[i] <= 0.0) ? 0.0 : (slope[i - 1] + slope[i]) * 0.5;

    // Fritsch–Carlson: limit tangents so each segment stays monotone.
    for (int i = 0; i < n - 1; ++i) {
        if (std::abs(slope[i]) < 1e-12) {
            m_tangent[i] = m_tangent[i + 1] = 0.0;
            continue;
        }
        const double a = m_tangent[i] / slope[i];
        const double b = m_tangent[i + 1] / slope[i];
        const double h = a * a + b * b;
        if (h > 9.0) {
            const double t = 3.0 / std::sqrt(h);
            m_tangent[i] = t * a * slope[i];
            m_tangent[i + 1] = t * b * slope[i];
        }
    }
}

double ToneCurve::evaluate(double x) const {
    if (x <= m_x.first())
        return m_y.first();
    if (x >= m_x.last())
        return m_y.last();

    const auto it = std::upper_bound(m_x.begin(), m_x.end(), x);
    const int i = int(it - m_x.begin()) - 1;
    const double h = m_x[i + 1] - m_x[i];
    const double t = (x - m_x[i]) / h;
    const double t2 = t * t, t3 = t2 * t;

    // Cubic Hermite basis.
    const double h00 = 2 * t3 - 3 * t2 + 1;
    const double h10 = t3 - 2 * t2 + t;
    const double h01 = -2 * t3 + 3 * t2;
    const double h11 = t3 - t2;
    const double y = h00 * m_y[i] + h10 * h * m_tangent[i] + h01 * m_y[i + 1] + h11 * h * m_tangent[i + 1];
    return std::clamp(y, 0.0, 1.0);
}

} // namespace svitlo
