#pragma once

#include <QImage>
#include <QRectF>
#include <QWidget>

namespace svitlo {

// Displays the develop preview. Supports fit/zoom, panning and the
// interactive crop overlay.
class ImageCanvas : public QWidget {
    Q_OBJECT
public:
    explicit ImageCanvas(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void setMessage(const QString& text); // shown when there is no image
    void setBeforeImage(const QImage& image);
    void setShowBefore(bool on);
    bool showBefore() const { return m_showBefore; }

    // Crop mode: the image shown is the full rotated frame and `crop` is a
    // normalised rect inside it. aspect <= 0 means free.
    void setCropMode(bool on);
    bool cropMode() const { return m_cropMode; }
    void setCropRect(const QRectF& crop);
    void setCropAspect(double aspect);

    void setFitToWindow(bool fit);
    bool fitToWindow() const { return m_fit; }

signals:
    void cropChanged(const QRectF& crop);   // live
    void cropEditingFinished();
    void zoomToggled(bool fit);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void resizeEvent(QResizeEvent*) override;

private:
    enum Handle { None, Move, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };

    QRectF imageRect() const;            // where the image is drawn, widget coords
    QRectF cropWidgetRect() const;
    Handle handleAt(QPointF pos) const;
    void updateCursor(QPointF pos);
    void clampPan();
    QRectF constrainAspect(QRectF r, Handle h) const;

    QImage m_image;
    QImage m_before;
    QString m_message;
    bool m_showBefore = false;

    bool m_fit = true;
    double m_zoom = 1.0;         // used when !m_fit; 1.0 = one preview pixel per screen pixel
    QPointF m_pan;               // offset of image centre from widget centre
    QPointF m_dragStart;
    QPointF m_panStart;
    bool m_panning = false;

    bool m_cropMode = false;
    QRectF m_crop{0, 0, 1, 1};
    QRectF m_cropAtPress;
    double m_aspect = 0.0;
    Handle m_handle = None;
};

} // namespace svitlo
