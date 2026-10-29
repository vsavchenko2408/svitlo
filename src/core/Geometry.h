#pragma once

#include "core/ImageBuffer.h"

#include <QPointF>
#include <QRectF>
#include <QSize>

namespace svitlo {

struct EditSettings;

namespace geometry {

// Size after applying quarter-turn orientation.
QSize orientedSize(QSize size, int quarterTurns);

// Rotate a buffer by 90° steps clockwise (lossless).
ImageBuffer applyQuarterTurns(const ImageBuffer& src, int quarterTurns);

// Map a point in the rotated frame (pixels) back to the source frame.
QPointF rotatedToSource(QPointF p, double angleDeg, QSizeF frame);

// True if every corner of the (pixel-space) rect lies inside the image after
// inverse rotation, i.e. the crop shows no empty corners.
bool cropFitsImage(const QRectF& cropPx, double angleDeg, QSizeF frame);

// Shrink a normalised crop rect about its centre (keeping aspect ratio) until
// it contains no area outside the rotated image. Returns a normalised rect.
QRectF fitCropToImage(const QRectF& cropNorm, double angleDeg, QSizeF frame);

// Applies orientation, fine rotation and (optionally) the crop rectangle with
// bilinear resampling. With applyCrop == false the full rotated frame is
// returned — the crop tool shows this so the user can see what is cut away.
ImageBuffer apply(const ImageBuffer& src, const EditSettings& s, bool applyCrop);

} // namespace geometry
} // namespace svitlo
