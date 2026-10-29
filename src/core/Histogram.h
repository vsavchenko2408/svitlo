#pragma once

#include <QImage>

#include <array>

namespace svitlo {

// 256-bin histogram of an 8-bit sRGB image, per channel plus luma.
struct Histogram {
    std::array<quint32, 256> red{}, green{}, blue{}, luma{};
    quint64 pixelCount = 0;

    bool isEmpty() const { return pixelCount == 0; }

    // Largest bin, ignoring the two end bins so a few clipped pixels don't
    // flatten the rest of the graph.
    quint32 peak() const;

    // Fraction of pixels at 0 / 255 in any channel.
    double shadowClipFraction() const;
    double highlightClipFraction() const;

    static Histogram compute(const QImage& image);
};

} // namespace svitlo
