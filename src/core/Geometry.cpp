#include "core/Geometry.h"

#include "core/EditSettings.h"

#include <QtConcurrent/QtConcurrentMap>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace svitlo::geometry {

namespace {

constexpr double kPi = 3.14159265358979323846;

// Bilinear sample with edge clamping.
inline void sampleBilinear(const ImageBuffer& src, double x, double y, float* out) {
    x = std::clamp(x - 0.5, 0.0, double(src.width - 1));
    y = std::clamp(y - 0.5, 0.0, double(src.height - 1));
    const int x0 = int(x), y0 = int(y);
    const int x1 = std::min(x0 + 1, src.width - 1);
    const int y1 = std::min(y0 + 1, src.height - 1);
    const float fx = float(x - x0), fy = float(y - y0);
    const float* p00 = src.at(x0, y0);
    const float* p10 = src.at(x1, y0);
    const float* p01 = src.at(x0, y1);
    const float* p11 = src.at(x1, y1);
    for (int c = 0; c < 3; ++c) {
        const float top = p00[c] + (p10[c] - p00[c]) * fx;
        const float bot = p01[c] + (p11[c] - p01[c]) * fx;
        out[c] = top + (bot - top) * fy;
    }
}

} // namespace

QSize orientedSize(QSize size, int quarterTurns) {
    return (quarterTurns & 1) ? size.transposed() : size;
}

ImageBuffer applyQuarterTurns(const ImageBuffer& src, int quarterTurns) {
    quarterTurns = ((quarterTurns % 4) + 4) % 4;
    if (quarterTurns == 0 || src.isNull())
        return src;

    const QSize outSize = orientedSize(QSize(src.width, src.height), quarterTurns);
    ImageBuffer dst(outSize.width(), outSize.height());
    for (int y = 0; y < src.height; ++y) {
        for (int x = 0; x < src.width; ++x) {
            int dx = x, dy = y;
            switch (quarterTurns) {
            case 1: dx = src.height - 1 - y; dy = x; break;                 // 90° CW
            case 2: dx = src.width - 1 - x;  dy = src.height - 1 - y; break; // 180°
            case 3: dx = y;                  dy = src.width - 1 - x; break;  // 90° CCW
            }
            std::copy_n(src.at(x, y), 3, dst.at(dx, dy));
        }
    }
    return dst;
}

QPointF rotatedToSource(QPointF p, double angleDeg, QSizeF frame) {
    const double a = -angleDeg * kPi / 180.0;
    const double cx = frame.width() * 0.5, cy = frame.height() * 0.5;
    const double dx = p.x() - cx, dy = p.y() - cy;
    return {cx + dx * std::cos(a) - dy * std::sin(a), cy + dx * std::sin(a) + dy * std::cos(a)};
}

bool cropFitsImage(const QRectF& r, double angleDeg, QSizeF frame) {
    constexpr double eps = 1e-6;
    const QPointF corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
    for (const QPointF& c : corners) {
        const QPointF s = rotatedToSource(c, angleDeg, frame);
        if (s.x() < -eps || s.y() < -eps || s.x() > frame.width() + eps || s.y() > frame.height() + eps)
            return false;
    }
    return true;
}

QRectF fitCropToImage(const QRectF& cropNorm, double angleDeg, QSizeF frame) {
    const QRectF px(cropNorm.x() * frame.width(), cropNorm.y() * frame.height(),
                    cropNorm.width() * frame.width(), cropNorm.height() * frame.height());
    if (cropFitsImage(px, angleDeg, frame))
        return cropNorm;

    // Binary search the largest scale (about the crop centre) that fits.
    // If even a tiny rect at that centre does not fit, pull the centre
    // towards the image centre as well.
    const QPointF frameCentre(frame.width() * 0.5, frame.height() * 0.5);
    QPointF centre = px.center();
    for (int attempt = 0; attempt < 12; ++attempt) {
        QRectF tiny(0, 0, px.width() * 0.01, px.height() * 0.01);
        tiny.moveCenter(centre);
        if (cropFitsImage(tiny, angleDeg, frame))
            break;
        centre = (centre + frameCentre) * 0.5;
    }

    double lo = 0.0, hi = 1.0;
    for (int i = 0; i < 40; ++i) {
        const double mid = (lo + hi) * 0.5;
        QRectF r(0, 0, px.width() * mid, px.height() * mid);
        r.moveCenter(centre);
        (cropFitsImage(r, angleDeg, frame) ? lo : hi) = mid;
    }
    QRectF r(0, 0, px.width() * lo, px.height() * lo);
    r.moveCenter(centre);
    return {r.x() / frame.width(), r.y() / frame.height(), r.width() / frame.width(),
            r.height() / frame.height()};
}

ImageBuffer apply(const ImageBuffer& srcIn, const EditSettings& s, bool applyCrop) {
    ImageBuffer oriented = applyQuarterTurns(srcIn, s.orientationQuarterTurns);
    const bool rotate = std::abs(s.rotation) > 1e-6;
    const QRectF crop = applyCrop ? s.crop : QRectF(0, 0, 1, 1);
    if (!rotate && crop == QRectF(0, 0, 1, 1))
        return oriented;

    const QSizeF frame(oriented.width, oriented.height);
    const QRectF cropPx(crop.x() * frame.width(), crop.y() * frame.height(),
                        crop.width() * frame.width(), crop.height() * frame.height());
    const int outW = std::max(1, int(std::lround(cropPx.width())));
    const int outH = std::max(1, int(std::lround(cropPx.height())));

    ImageBuffer dst(outW, outH);
    std::vector<int> rows(outH);
    std::iota(rows.begin(), rows.end(), 0);

    const double a = -s.rotation * kPi / 180.0;
    const double ca = std::cos(a), sa = std::sin(a);
    const double cx = frame.width() * 0.5, cy = frame.height() * 0.5;
    const double sx = cropPx.width() / outW, sy = cropPx.height() / outH;

    QtConcurrent::blockingMap(rows, [&](int y) {
        float* out = dst.row(y);
        const double fy = cropPx.y() + (y + 0.5) * sy - cy;
        for (int x = 0; x < outW; ++x) {
            const double fx = cropPx.x() + (x + 0.5) * sx - cx;
            const double srcX = cx + fx * ca - fy * sa;
            const double srcY = cy + fx * sa + fy * ca;
            sampleBilinear(oriented, srcX, srcY, out + x * 3);
        }
    });
    return dst;
}

} // namespace svitlo::geometry
