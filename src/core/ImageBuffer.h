#pragma once

#include <cstddef>
#include <vector>

namespace svitlo {

// A scene-referred image: interleaved RGB, 32-bit float, linear light with
// sRGB/Rec.709 primaries. 1.0 is "diffuse white"; values above 1.0 are
// allowed (headroom from exposure) and are handled by the tone stage.
struct ImageBuffer {
    int width = 0;
    int height = 0;
    std::vector<float> pixels; // width * height * 3

    ImageBuffer() = default;
    ImageBuffer(int w, int h) : width(w), height(h), pixels(std::size_t(w) * h * 3, 0.0f) {}

    bool isNull() const { return width <= 0 || height <= 0 || pixels.empty(); }

    float* row(int y) { return pixels.data() + std::size_t(y) * width * 3; }
    const float* row(int y) const { return pixels.data() + std::size_t(y) * width * 3; }

    float* at(int x, int y) { return row(y) + std::size_t(x) * 3; }
    const float* at(int x, int y) const { return row(y) + std::size_t(x) * 3; }
};

// Box-filter downscale so that the longer side is at most maxLongEdge.
// Returns a copy when no scaling is needed.
ImageBuffer downscaleToFit(const ImageBuffer& src, int maxLongEdge);

} // namespace svitlo
