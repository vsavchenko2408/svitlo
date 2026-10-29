#include "core/Histogram.h"

#include <algorithm>

namespace svitlo {

quint32 Histogram::peak() const {
    quint32 m = 1;
    for (int i = 1; i < 255; ++i)
        m = std::max({m, red[i], green[i], blue[i]});
    return m;
}

double Histogram::shadowClipFraction() const {
    if (!pixelCount)
        return 0.0;
    return double(std::max({red[0], green[0], blue[0]})) / double(pixelCount);
}

double Histogram::highlightClipFraction() const {
    if (!pixelCount)
        return 0.0;
    return double(std::max({red[255], green[255], blue[255]})) / double(pixelCount);
}

Histogram Histogram::compute(const QImage& input) {
    Histogram h;
    if (input.isNull())
        return h;
    const QImage img = input.format() == QImage::Format_RGB888 ? input
                                                               : input.convertToFormat(QImage::Format_RGB888);
    // Sample every pixel for small images, every other row/column for big ones.
    const int step = (qint64(img.width()) * img.height() > 4'000'000) ? 2 : 1;
    for (int y = 0; y < img.height(); y += step) {
        const uchar* p = img.constScanLine(y);
        for (int x = 0; x < img.width(); x += step) {
            const uchar r = p[x * 3], g = p[x * 3 + 1], b = p[x * 3 + 2];
            ++h.red[r];
            ++h.green[g];
            ++h.blue[b];
            ++h.luma[(r * 54 + g * 183 + b * 19) >> 8];
            ++h.pixelCount;
        }
    }
    return h;
}

} // namespace svitlo
