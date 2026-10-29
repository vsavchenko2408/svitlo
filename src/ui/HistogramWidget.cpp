#include "ui/HistogramWidget.h"

#include "ui/Theme.h"

#include <QPainter>
#include <QPainterPath>

#include <cmath>
#include <algorithm>

namespace svitlo {

HistogramWidget::HistogramWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(110);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void HistogramWidget::setHistogram(const Histogram& h) {
    m_hist = h;
    update();
}

void HistogramWidget::setCaption(const QString& text) {
    m_caption = text;
    update();
}

void HistogramWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = QRectF(rect()).adjusted(1, 1, -1, -18);
    p.fillRect(rect(), theme::kCanvas);

    if (!m_hist.isEmpty()) {
        // Square-root scaling shows detail in sparse tones without letting a
        // single dominant tone squash the rest.
        const double peak = std::sqrt(double(m_hist.peak()));
        auto channelPath = [&](const std::array<quint32, 256>& bins) {
            QPainterPath path;
            path.moveTo(r.bottomLeft());
            for (int i = 0; i < 256; ++i) {
                const double v = std::min(1.0, std::sqrt(double(bins[i])) / peak);
                path.lineTo(r.left() + i / 255.0 * r.width(), r.bottom() - v * r.height());
            }
            path.lineTo(r.bottomRight());
            path.closeSubpath();
            return path;
        };

        p.setCompositionMode(QPainter::CompositionMode_Plus);
        p.fillPath(channelPath(m_hist.red), QColor(200, 60, 60, 170));
        p.fillPath(channelPath(m_hist.green), QColor(60, 180, 70, 170));
        p.fillPath(channelPath(m_hist.blue), QColor(60, 90, 210, 170));
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        // Clipping indicators: lit when more than 0.1% of pixels clip.
        auto indicator = [&](bool left, double fraction) {
            const bool lit = fraction > 0.001;
            QPainterPath tri;
            const double x = left ? r.left() + 4 : r.right() - 4;
            const double dir = left ? 1 : -1;
            tri.moveTo(x, r.top() + 4);
            tri.lineTo(x + dir * 9, r.top() + 4);
            tri.lineTo(x, r.top() + 13);
            tri.closeSubpath();
            p.fillPath(tri, lit ? (left ? QColor(90, 140, 255) : QColor(255, 90, 80)) : theme::kBorder);
        };
        indicator(true, m_hist.shadowClipFraction());
        indicator(false, m_hist.highlightClipFraction());
    }

    p.setPen(theme::kTextDim);
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() * 0.85);
    p.setFont(f);
    p.drawText(QRectF(6, height() - 17, width() - 12, 16), Qt::AlignLeft | Qt::AlignVCenter,
               p.fontMetrics().elidedText(m_caption, Qt::ElideRight, width() - 12));
}

} // namespace svitlo
