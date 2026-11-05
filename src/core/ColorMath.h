#pragma once

#include <algorithm>
#include <cmath>
#include <array>

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

// Hue of a colour in degrees, 0..360 (0 = red, 120 = green, 240 = blue).
// Grey has no hue: returns 0.
inline float hueOf(float r, float g, float b) {
    const float mx = std::max({r,g,b});
    const float mn = std::min({r,g,b});
    const float d = mx - mn;

    if(d <= 0.0f)
        return 0.0f;
    float h;
    if(mx ==r)
        h = 60.0f * (g - b)/d;
    else if(mx ==g)
        h = 60.0f * ((b - r)/ d + 2);
    else
        h = 60 * ((r - g) / d + 4);
    if(h < 0.0f) 
        h = h + 360.0f;
    return h;
}

// The HSL panel has 8 colour bands. A hue between two bands is shared by
// both: `first` and `second` are the band numbers (0..7), `t` (0..1) is how
// far the hue is from `first` towards `second`.
struct HueBands {
    int first;
    int second;
    float t;
};

inline HueBands hueBands(float hue) {
    // Angle of each band; the last 360 is red again, closing the circle.
    constexpr float kAngles[9] = {0, 30, 60, 120, 180, 240, 270, 300, 360};

    for (int i = 0; i < 8; ++i) {
        if (hue < kAngles[i + 1]){              
            HueBands result;
            result.first = i;
            result.second =  (i + 1) % 8;         
            result.t = (hue - kAngles[i]) / (kAngles[i + 1] - kAngles[i]);              
            return result;
        }
    }
    return {0, 1, 0.0f};       
}

// Value of one kind of HSL slider (e.g. saturation) for a hue that lies
// between two bands: a blend of the two neighbouring sliders.
inline float mixBands(const std::array<float, 8>& values, const HueBands& bands) {
    return lerp(values[bands.first],values[bands.second], bands.t);
}

// Applies the colour mixer to one pixel (linear RGB). `sat` and `lum` are
// the 8 saturation and 8 luminance sliders (-100..100). Hue comes later.
inline void applyHsl(float& r, float& g, float& b,
    const std::array<float, 8>& sat,
    const std::array<float, 8>& lum) {
const float mx = std::max({r, g, b});
const float mn = std::min({r, g, b});
if (mx <= 0.0f)
return;                                    // black: nothing to change

const float chroma = (mx - mn) / mx;           // 0 = grey, 1 = pure colour
const float strength = smoothstep(0.0f, 0.25f, chroma); // greys stay as they are

const HueBands bands = hueBands(hueOf(r, g, b));
const float ds = mixBands(sat, bands) / 100.0f * strength;   // -1..1
const float dl = mixBands(lum,bands) / 100.f * strength;                          

// Saturation: move each channel away from grey (or towards it).
const float y = luminance(r, g, b);
const float f = 1.0f + ds;
r = y + (r - y) * f;
g = y + (g - y) * f;
b = y + (b - y) * f;

// Luminance: up to twice as bright or half as bright.
const float m = std::exp2(dl);
r *= m;
g *= m;
b *= m;
}
} // namespace svitlo::color
