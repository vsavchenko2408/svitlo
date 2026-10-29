#pragma once

#include <algorithm>
#include <cmath>

// Small, header-only colour helpers shared by the pipeline and the loader.
namespace svitlo::color {

// Rec.709 / sRGB luminance weights (linear light).
constexpr float kLumR = 0.2126f;
constexpr float kLumG = 0.7152f;
constexpr float kLumB = 0.0722f;

inline float luminance(float r, float g, float b) {
    return kLumR * r + kLumG * g + kLumB * b;
}

inline float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

inline float smoothstep(float edge0, float edge1, float x) {
    const float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

// IEC 61966-2-1 sRGB transfer functions.
inline float srgbToLinear(float v) {
    return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
}

inline float linearToSrgb(float v) {
    return v <= 0.0031308f ? v * 12.92f : 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
}

} // namespace svitlo::color
