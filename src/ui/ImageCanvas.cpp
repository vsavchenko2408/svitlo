#include "ui/ImageCanvas.h"

#include "ui/Theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace svitlo {

namespace {
constexpr double kGrab = 10.0;      // handle grab distance in px
constexpr double kMinCrop = 0.02;   // minimum crop size (normalised)
constexpr double kMaxZoom = 8.0;
} // namespace

ImageCanvas::ImageCanvas(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(320, 240);
    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void ImageCanvas::setImage(const QImage& image) {
    m_image = image;
    m_message.clear();
    clampPan();
    update();
}

void ImageCanvas::setMessage(const QString& text) {
    m_message = text;
    update();
}

void ImageCanvas::setBeforeImage(const QImage& image) {
    m_before = image;
    if (m_showBefore)
        update();
}

void ImageCanvas::setShowBefore(bool on) {
    m_showBefore = on;
    update();
}

void ImageCanvas::setCropMode(bool on) {
    m_cropMode = on;
    if (on)
        setFitToWindow(true);
    update();
}

void ImageCanvas::setCropRect(const QRectF& crop) {
    m_crop = crop;
    update();
}

void ImageCanvas::setCropAspect(double aspect) { m_aspect = aspect; }

void ImageCanvas::setFitToWindow(bool fit) {
    m_fit = fit;
    if (fit)
        m_pan = {};
    update();
}

QRectF ImageCanvas::imageRect() const {
    const QImage& img = (m_showBefore && !m_before.isNull()) ? m_before : m_image;
    if (img.isNull())
        return {};
    QSizeF size = img.size();
    const double pad = m_cropMode ? 40.0 : 16.0;
    if (m_fit) {
        size.scale(QSizeF(width() - 2 * pad, height() - 2 * pad), Qt::KeepAspectRatio);
    } else {
        // devicePixelRatio: on Retina screens 1 image px = 1 physical px at 100%.
        size *= m_zoom / devicePixelRatioF();
    }
    QRectF r(QPointF(0, 0), size);
    r.moveCenter(QPointF(width() / 2.0, height() / 2.0) + m_pan);
    return r;
}

void ImageCanvas::clampPan() {
    if (m_fit || m_image.isNull()) {
        m_pan = {};
        return;
    }
    const QSizeF s = QSizeF(m_image.size()) * m_zoom / devicePixelRatioF();
    const double maxX = std::max(0.0, (s.width() - width()) / 2.0);
    const double maxY = std::max(0.0, (s.height() - height()) / 2.0);
    m_pan.setX(std::clamp(m_pan.x(), -maxX, maxX));
    m_pan.setY(std::clamp(m_pan.y(), -maxY, maxY));
}

QRectF ImageCanvas::cropWidgetRect() const {
    const QRectF ir = imageRect();
    return {ir.left() + m_crop.x() * ir.width(), ir.top() + m_crop.y() * ir.height(),
            m_crop.width() * ir.width(), m_crop.height() * ir.height()};
}

void ImageCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), theme::kCanvas);

    const QImage& img = (m_showBefore && !m_before.isNull()) ? m_before : m_image;
    if (img.isNull()) {
        p.setPen(theme::kTextDim);
        p.drawText(rect(), Qt::AlignCenter, m_message.isEmpty() ? tr("No photo selected") : m_message);
        return;
    }

    p.setRenderHint(QPainter::SmoothPixmapTransform, m_fit || m_zoom < 1.0);
    const QRectF ir = imageRect();
    p.drawImage(ir, img);

    if (m_showBefore) {
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF badge(12, 12, 74, 24);
        QPainterPath path;
        path.addRoundedRect(badge, 5, 5);
        p.fillPath(path, QColor(0, 0, 0, 150));
        p.setPen(theme::kText);
        p.drawText(badge, Qt::AlignCenter, tr("Before"));
    }

    if (!m_cropMode || m_showBefore)
        return;

    // Crop overlay: dim the outside, thirds grid, frame and handles.
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF cr = cropWidgetRect();
    QPainterPath outside;
    outside.addRect(ir);
    QPainterPath inside;
    inside.addRect(cr);
    p.fillPath(outside.subtracted(inside), QColor(0, 0, 0, 150));

    p.setPen(QPen(QColor(255, 255, 255, 90), 1));
    for (int i = 1; i < 3; ++i) {
        const double x = cr.left() + cr.width() * i / 3.0;
        const double y = cr.top() + cr.height() * i / 3.0;
        p.drawLine(QPointF(x, cr.top()), QPointF(x, cr.bottom()));
        p.drawLine(QPointF(cr.left(), y), QPointF(cr.right(), y));
    }
    p.setPen(QPen(theme::kAccent, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRect(cr);

    p.setPen(QPen(theme::kAccent, 3.5, Qt::SolidLine, Qt::SquareCap));
    const double L = 14;
    auto corner = [&](QPointF c, double dx, double dy) {
        p.drawLine(c, c + QPointF(dx * L, 0));
        p.drawLine(c, c + QPointF(0, dy * L));
    };
    corner(cr.topLeft(), 1, 1);
    corner(cr.topRight(), -1, 1);
    corner(cr.bottomLeft(), 1, -1);
    corner(cr.bottomRight(), -1, -1);
}

ImageCanvas::Handle ImageCanvas::handleAt(QPointF pos) const {
    const QRectF r = cropWidgetRect();
    const bool nearL = std::abs(pos.x() - r.left()) < kGrab;
    const bool nearR = std::abs(pos.x() - r.right()) < kGrab;
    const bool nearT = std::abs(pos.y() - r.top()) < kGrab;
    const bool nearB = std::abs(pos.y() - r.bottom()) < kGrab;
    const bool inX = pos.x() > r.left() - kGrab && pos.x() < r.right() + kGrab;
    const bool inY = pos.y() > r.top() - kGrab && pos.y() < r.bottom() + kGrab;

    if (nearL && nearT) return TopLeft;
    if (nearR && nearT) return TopRight;
    if (nearL && nearB) return BottomLeft;
    if (nearR && nearB) return BottomRight;
    if (nearL && inY) return Left;
    if (nearR && inY) return Right;
    if (nearT && inX) return Top;
    if (nearB && inX) return Bottom;
    if (r.contains(pos)) return Move;
    return None;
}

void ImageCanvas::updateCursor(QPointF pos) {
    if (!m_cropMode) {
        setCursor(m_fit ? Qt::ArrowCursor : Qt::OpenHandCursor);
        return;
    }
    switch (handleAt(pos)) {
    case TopLeft: case BottomRight: setCursor(Qt::SizeFDiagCursor); break;
    case TopRight: case BottomLeft: setCursor(Qt::SizeBDiagCursor); break;
    case Left: case Right: setCursor(Qt::SizeHorCursor); break;
    case Top: case Bottom: setCursor(Qt::SizeVerCursor); break;
    case Move: setCursor(Qt::SizeAllCursor); break;
    default: setCursor(Qt::ArrowCursor); break;
    }
}

QRectF ImageCanvas::constrainAspect(QRectF r, Handle h) const {
    if (m_aspect <= 0.0 || m_image.isNull())
        return r;
    // m_aspect is width/height in pixels; convert to normalised units.
    const double normAspect = m_aspect * m_image.height() / double(m_image.width());

    switch (h) {
    case Left: case Right: {
        const double newH = r.width() / normAspect;
        const double cy = r.center().y();
        r.setTop(cy - newH / 2);
        r.setHeight(newH);
        break;
    }
    case Top: case Bottom: {
        const double newW = r.height() * normAspect;
        const double cx = r.center().x();
        r.setLeft(cx - newW / 2);
        r.setWidth(newW);
        break;
    }
    case TopLeft: case TopRight: case BottomLeft: case BottomRight: {
        // Keep the opposite corner anchored; follow the dominant axis.
        double w = r.width(), hh = r.height();
        if (w / hh > normAspect) w = hh * normAspect;
        else hh = w / normAspect;
        const QPointF anchor = h == TopLeft    ? r.bottomRight()
                               : h == TopRight ? r.bottomLeft()
                               : h == BottomLeft ? r.topRight()
                                                 : r.topLeft();
        const double sx = (h == TopLeft || h == BottomLeft) ? -1 : 1;
        const double sy = (h == TopLeft || h == TopRight) ? -1 : 1;
        r = QRectF(anchor, anchor + QPointF(sx * w, sy * hh)).normalized();
        break;
    }
    default: break;
    }

    // If the constrained rect leaves the frame, scale it back in.
    if (r.width() > 1.0 || r.height() > 1.0) {
        const double s = std::min(1.0 / r.width(), 1.0 / r.height());
        const QPointF c = r.center();
        r.setSize(r.size() * s);
        r.moveCenter(c);
    }
    if (r.left() < 0) r.moveLeft(0);
    if (r.top() < 0) r.moveTop(0);
    if (r.right() > 1) r.moveRight(1);
    if (r.bottom() > 1) r.moveBottom(1);
    return r;
}

void ImageCanvas::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton)
        return;
    m_dragStart = e->position();
    if (m_cropMode && !m_showBefore) {
        m_handle = handleAt(e->position());
        m_cropAtPress = m_crop;
        return;
    }
    if (!m_fit) {
        m_panning = true;
        m_panStart = m_pan;
        setCursor(Qt::ClosedHandCursor);
    }
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* e) {
    const QPointF pos = e->position();
    if (m_panning) {
        m_pan = m_panStart + (pos - m_dragStart);
        clampPan();
        update();
        return;
    }
    if (!m_cropMode || m_handle == None || !(e->buttons() & Qt::LeftButton)) {
        updateCursor(pos);
        return;
    }

    const QRectF ir = imageRect();
    const QPointF d((pos.x() - m_dragStart.x()) / ir.width(), (pos.y() - m_dragStart.y()) / ir.height());
    QRectF r = m_cropAtPress;

    if (m_handle == Move) {
        r.translate(d);
        r.moveLeft(std::clamp(r.left(), 0.0, 1.0 - r.width()));
        r.moveTop(std::clamp(r.top(), 0.0, 1.0 - r.height()));
    } else {
        if (m_handle == Left || m_handle == TopLeft || m_handle == BottomLeft)
            r.setLeft(std::clamp(r.left() + d.x(), 0.0, r.right() - kMinCrop));
        if (m_handle == Right || m_handle == TopRight || m_handle == BottomRight)
            r.setRight(std::clamp(r.right() + d.x(), r.left() + kMinCrop, 1.0));
        if (m_handle == Top || m_handle == TopLeft || m_handle == TopRight)
            r.setTop(std::clamp(r.top() + d.y(), 0.0, r.bottom() - kMinCrop));
        if (m_handle == Bottom || m_handle == BottomLeft || m_handle == BottomRight)
            r.setBottom(std::clamp(r.bottom() + d.y(), r.top() + kMinCrop, 1.0));
        r = constrainAspect(r, m_handle);
    }
    m_crop = r;
    emit cropChanged(m_crop);
    update();
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* e) {
    if (m_panning) {
        m_panning = false;
        updateCursor(e->position());
    }
    if (m_cropMode && m_handle != None) {
        m_handle = None;
        emit cropEditingFinished();
    }
}

void ImageCanvas::mouseDoubleClickEvent(QMouseEvent* e) {
    if (m_cropMode || m_image.isNull())
        return;
    // Toggle between fit and 100% centred on the clicked point.
    if (m_fit) {
        const QRectF before = imageRect();
        const QPointF rel((e->position().x() - before.left()) / before.width(),
                          (e->position().y() - before.top()) / before.height());
        m_fit = false;
        m_zoom = 1.0;
        const QSizeF s = QSizeF(m_image.size()) / devicePixelRatioF();
        m_pan = QPointF((0.5 - rel.x()) * s.width(), (0.5 - rel.y()) * s.height());
        clampPan();
    } else {
        m_fit = true;
        m_pan = {};
    }
    emit zoomToggled(m_fit);
    updateCursor(e->position());
    update();
}

void ImageCanvas::wheelEvent(QWheelEvent* e) {
    if (m_cropMode || m_image.isNull())
        return;
    if (!(e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) && !m_fit) {
        // Plain scroll pans when zoomed in (trackpad friendly).
        m_pan += e->pixelDelta().isNull() ? QPointF(e->angleDelta()) / 4.0 : QPointF(e->pixelDelta());
        clampPan();
        update();
        return;
    }
    if (!(e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)))
        return;

    const QRectF before = imageRect();
    double zoom = m_fit ? before.width() * devicePixelRatioF() / m_image.width() : m_zoom;
    zoom *= std::pow(1.0015, e->angleDelta().y());
    const double fitZoom = QSizeF(m_image.size()).scaled(size() - QSize(32, 32), Qt::KeepAspectRatio).width() *
                           devicePixelRatioF() / m_image.width();
    if (zoom <= fitZoom) {
        setFitToWindow(true);
        emit zoomToggled(true);
        return;
    }
    zoom = std::min(zoom, kMaxZoom);

    // Keep the point under the cursor fixed.
    const QPointF cursor = e->position();
    const QPointF rel((cursor.x() - before.left()) / before.width(), (cursor.y() - before.top()) / before.height());
    m_fit = false;
    m_zoom = zoom;
    const QSizeF s = QSizeF(m_image.size()) * m_zoom / devicePixelRatioF();
    const QPointF newTopLeft = cursor - QPointF(rel.x() * s.width(), rel.y() * s.height());
    m_pan = newTopLeft + QPointF(s.width() / 2, s.height() / 2) - QPointF(width() / 2.0, height() / 2.0);
    clampPan();
    emit zoomToggled(false);
    update();
}

void ImageCanvas::resizeEvent(QResizeEvent*) { clampPan(); }

} // namespace svitlo
