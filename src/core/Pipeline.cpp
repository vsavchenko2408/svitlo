#include "core/Pipeline.h"

#include "core/ColorMath.h"
#include "core/Geometry.h"
#include "core/ToneCurve.h"

#include <QtConcurrent/QtConcurrentMap>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>
#include <memory>

namespace svitlo {

namespace {

using namespace color;

// Lightness used by the tone stage: a simple perceptual encoding of linear
// luminance. The tone LUT covers L in [0, kMaxL], i.e. about +2.2 EV of
// headroom above diffuse white before values are clipped.
constexpr float kToneGamma = 2.2f;
constexpr float kMaxL = 2.0f;
constexpr int kToneLutSize = 4096;
constexpr int kSrgbLutSize = 16384;

inline float toPerceptual(float y) { return y > 0.0f ? std::pow(y, 1.0f / kToneGamma) : 0.0f; }
inline float fromPerceptual(float l) { return l > 0.0f ? std::pow(l, kToneGamma) : 0.0f; }

std::vector<int> rowIndices(int h) {
    std::vector<int> rows(h);
    std::iota(rows.begin(), rows.end(), 0);
    return rows;
}

// --- Tone function -----------------------------------------------------------

float toneFunction(const EditSettings& s, const ToneCurve* curve, float x) {
    // Blacks / whites: move the end points of the range.
    const float black = float(-s.blacks / 100.0 * 0.12);
    const float white = float(1.0 - s.whites / 100.0 * 0.25);
    x = std::max(0.0f, (x - black) / (white - black));

    // Shadows: gamma lift/crush, faded out towards the midtones.
    if (s.shadows != 0.0 && x < 1.0f) {
        const float amt = float(s.shadows / 100.0);
        const float g = amt > 0 ? 1.0f / (1.0f + 0.8f * amt) : 1.0f + 0.8f * -amt;
        const float w = 1.0f - smoothstep(0.1f, 0.7f, x);
        x = lerp(x, std::pow(x, g), w);
    }

    // Highlights: proportional gain in the upper range. With negative values
    // this also pulls the +EV headroom back below white ("recovery").
    if (s.highlights != 0.0) {
        const float amt = float(s.highlights / 100.0);
        x *= 1.0f + 0.33f * amt * smoothstep(0.4f, 1.0f, x);
    }

    // Contrast: blend towards (or away from) a smoothstep S-curve.
    if (s.contrast != 0.0 && x < 1.0f) {
        const float c = float(s.contrast / 100.0);
        const float sc = x * x * (3.0f - 2.0f * x);
        x += c * (sc - x);
    }

    // User tone curve on [0,1]; linear continuation above 1.
    if (curve) {
        if (x <= 1.0f)
            x = float(curve->evaluate(x));
        else
            x = float(curve->evaluate(1.0)) + (x - 1.0f);
    }
    return std::max(0.0f, x);
}

struct ToneLut {
    std::vector<float> table;

    ToneLut(const EditSettings& s) : table(kToneLutSize) {
        std::unique_ptr<ToneCurve> curve;
        if (!s.curveIsIdentity())
            curve = std::make_unique<ToneCurve>(s.curve);
        for (int i = 0; i < kToneLutSize; ++i) {
            const float x = kMaxL * float(i) / float(kToneLutSize - 1);
            table[i] = toneFunction(s, curve.get(), x);
        }
    }

    float operator()(float x) const {
        const float f = std::clamp(x, 0.0f, kMaxL) / kMaxL * float(kToneLutSize - 1);
        const int i = std::min(int(f), kToneLutSize - 2);
        const float t = f - float(i);
        return table[i] + (table[i + 1] - table[i]) * t;
    }
};

// --- Output encoding -----------------------------------------------------------

const std::vector<float>& srgbEncodeLut() {
    static const std::vector<float> lut = [] {
        std::vector<float> t(kSrgbLutSize + 1);
        for (int i = 0; i <= kSrgbLutSize; ++i)
            t[i] = linearToSrgb(float(i) / kSrgbLutSize);
        return t;
    }();
    return lut;
}

inline float encodeSrgb(float v) {
    const auto& lut = srgbEncodeLut();
    const float f = clamp01(v) * kSrgbLutSize;
    const int i = std::min(int(f), kSrgbLutSize - 1);
    return lut[i] + (lut[i + 1] - lut[i]) * (f - float(i));
}

// --- Blur ----------------------------------------------------------------------

// One box-blur pass along rows of a single-channel plane, using a running sum
// so cost is independent of radius.
void boxBlurRows(const std::vector<float>& src, std::vector<float>& dst, int w, int h, int r) {
    auto rows = rowIndices(h);
    QtConcurrent::blockingMap(rows, [&](int y) {
        const float* in = src.data() + std::size_t(y) * w;
        float* out = dst.data() + std::size_t(y) * w;
        const float norm = 1.0f / float(2 * r + 1);
        double acc = 0.0;
        for (int i = -r; i <= r; ++i)
            acc += in[std::clamp(i, 0, w - 1)];
        for (int x = 0; x < w; ++x) {
            out[x] = float(acc) * norm;
            acc += in[std::min(x + r + 1, w - 1)] - in[std::max(x - r, 0)];
        }
    });
}

void transpose(const std::vector<float>& src, std::vector<float>& dst, int w, int h) {
    auto rows = rowIndices(h);
    QtConcurrent::blockingMap(rows, [&](int y) {
        for (int x = 0; x < w; ++x)
            dst[std::size_t(x) * h + y] = src[std::size_t(y) * w + x];
    });
}

// Approximate Gaussian blur: three box passes in each direction.
std::vector<float> gaussianish(const std::vector<float>& plane, int w, int h, float sigma) {
    // Box radius whose three-pass variance matches sigma^2.
    const int r = std::max(1, int(std::lround(std::sqrt(sigma * sigma + 1.0f) - 0.5f)));
    std::vector<float> a = plane, b(plane.size());
    for (int i = 0; i < 3; ++i) {
        boxBlurRows(a, b, w, h, r);
        std::swap(a, b);
    }
    transpose(a, b, w, h);
    std::swap(a, b);
    for (int i = 0; i < 3; ++i) {
        boxBlurRows(a, b, h, w, r);
        std::swap(a, b);
    }
    transpose(a, b, h, w);
    return b;
}

} // namespace

float Pipeline::toneValue(const EditSettings& s, float lightness) {
    std::unique_ptr<ToneCurve> curve;
    if (!s.curveIsIdentity())
        curve = std::make_unique<ToneCurve>(s.curve);
    return toneFunction(s, curve.get(), lightness);
}

QImage Pipeline::render(const ImageBuffer& source, const EditSettings& s, const Options& opt) {
    if (source.isNull())
        return {};

    // 1. Geometry
    ImageBuffer img = geometry::apply(source, s, opt.applyCrop);
    const int w = img.width, h = img.height;
    const std::size_t npix = std::size_t(w) * h;

    // 2. White balance + exposure gains, normalised to keep neutral grey.
    float gain[3] = {1.0f, 1.0f, 1.0f};
    {
        const float t = float(s.temperature / 100.0);
        const float tn = float(s.tint / 100.0);
        gain[0] = std::exp2(0.5f * t);
        gain[2] = std::exp2(-0.5f * t);
        gain[1] = std::exp2(-0.35f * tn);
        const float norm = luminance(gain[0], gain[1], gain[2]);
        const float ev = std::exp2(float(s.exposure));
        for (float& g : gain)
            g = g / norm * ev;
    }

    // Perceptual lightness plane (also needed for clarity).
    std::vector<float> lightness(npix);
    auto rows = rowIndices(h);
    QtConcurrent::blockingMap(rows, [&](int y) {
        float* p = img.row(y);
        float* L = lightness.data() + std::size_t(y) * w;
        for (int x = 0; x < w; ++x, p += 3) {
            p[0] = std::max(0.0f, p[0] * gain[0]);
            p[1] = std::max(0.0f, p[1] * gain[1]);
            p[2] = std::max(0.0f, p[2] * gain[2]);
            L[x] = toPerceptual(luminance(p[0], p[1], p[2]));
        }
    });

    // 3. Clarity — radius is relative to the image so preview == export.
    std::vector<float> blurredL;
    const float clarity = float(s.clarity / 100.0);
    if (clarity != 0.0f) {
        const float sigma = std::max(2.0f, 0.012f * float(std::max(w, h)));
        blurredL = gaussianish(lightness, w, h, sigma);
    }

    // 4–7. Tone, colour, vignette, soft clip; writes sRGB-encoded floats.
    const ToneLut tone(s);
    const float vib = float(s.vibrance / 100.0);
    const float sat = 1.0f + float(s.saturation / 100.0);
    const float vig = float(s.vignette / 100.0);
    const float vigStart = 0.15f + 0.9f * float(s.vignetteMidpoint / 100.0);
    const bool mono = s.monochrome;

    std::vector<float> encoded(npix * 3);
    QtConcurrent::blockingMap(rows, [&](int y) {
        const float* p = img.row(y);
        const float* L = lightness.data() + std::size_t(y) * w;
        const float* LB = blurredL.empty() ? nullptr : blurredL.data() + std::size_t(y) * w;
        float* out = encoded.data() + std::size_t(y) * w * 3;
        const float ny = h > 1 ? (2.0f * y / float(h - 1) - 1.0f) : 0.0f;

        for (int x = 0; x < w; ++x, p += 3, out += 3) {
            float r = p[0], g = p[1], b = p[2];
            const float y0 = luminance(r, g, b);
            float l = L[x];

            if (LB) {
                // Local contrast, strongest in the midtones.
                const float detail = l - LB[x];
                const float mid = 1.0f - std::abs(clamp01(l) * 2.0f - 1.0f);
                l += clarity * 1.2f * detail * (0.35f + 0.65f * mid);
                l = std::max(0.0f, l);
            }

            const float y1 = fromPerceptual(tone(l));
            if (y0 > 1e-6f) {
                const float k = y1 / y0;
                r *= k; g *= k; b *= k;
            } else {
                r = g = b = y1;
            }

            // Vibrance boosts muted colours more than saturated ones.
            float yy = luminance(r, g, b);
            if (mono) {
                r = g = b = yy;
            } else if (vib != 0.0f || sat != 1.0f) {
                const float mx = std::max({r, g, b});
                const float mn = std::min({r, g, b});
                const float chroma = mx > 1e-6f ? (mx - mn) / mx : 0.0f;
                const float f = sat * (1.0f + vib * (1.0f - chroma));
                r = yy + (r - yy) * f;
                g = yy + (g - yy) * f;
                b = yy + (b - yy) * f;
            }

            if (vig != 0.0f) {
                const float nx = w > 1 ? (2.0f * x / float(w - 1) - 1.0f) : 0.0f;
                const float d = std::sqrt(nx * nx + ny * ny); // 0 centre, ~1.41 corners
                const float wgt = smoothstep(vigStart, 1.45f, d);
                const float m = std::exp2(1.5f * vig * wgt);
                r *= m; g *= m; b *= m;
            }

            // Soft clip: keep hue/luminance, pull channels into [0,1].
            yy = luminance(r, g, b);
            if (yy >= 1.0f) {
                r = g = b = 1.0f;
            } else {
                const float mx = std::max({r, g, b});
                const float mn = std::min({r, g, b});
                float t = 1.0f;
                if (mx > 1.0f) t = std::min(t, (1.0f - yy) / (mx - yy));
                if (mn < 0.0f) t = std::min(t, yy / (yy - mn));
                if (t < 1.0f) {
                    r = yy + (r - yy) * t;
                    g = yy + (g - yy) * t;
                    b = yy + (b - yy) * t;
                }
            }
            out[0] = encodeSrgb(r);
            out[1] = encodeSrgb(g);
            out[2] = encodeSrgb(b);
        }
    });

    // 8. Sharpening — unsharp mask on encoded luma, radius scaled to buffer.
    const float sharpAmount = float(s.sharpenAmount / 100.0);
    const float sharpSigma = float(s.sharpenRadius * opt.resolutionScale);
    std::vector<float> lumaDelta;
    if (sharpAmount > 0.0f && sharpSigma >= 0.25f) {
        std::vector<float> luma(npix);
        QtConcurrent::blockingMap(rows, [&](int y) {
            const float* e = encoded.data() + std::size_t(y) * w * 3;
            float* o = luma.data() + std::size_t(y) * w;
            for (int x = 0; x < w; ++x, e += 3)
                o[x] = luminance(e[0], e[1], e[2]);
        });
        const std::vector<float> blurred = gaussianish(luma, w, h, sharpSigma);
        lumaDelta.resize(npix);
        for (std::size_t i = 0; i < npix; ++i) {
            const float d = luma[i] - blurred[i];
            // Small threshold keeps flat areas (and noise) from being boosted.
            lumaDelta[i] = std::abs(d) < 0.004f ? 0.0f : sharpAmount * d;
        }
    }

    // 9. Quantise.
    QImage result(w, h, QImage::Format_RGB888);
    QtConcurrent::blockingMap(rows, [&](int y) {
        const float* e = encoded.data() + std::size_t(y) * w * 3;
        const float* d = lumaDelta.empty() ? nullptr : lumaDelta.data() + std::size_t(y) * w;
        uchar* out = result.scanLine(y);
        for (int x = 0; x < w; ++x) {
            const float add = d ? d[x] : 0.0f;
            for (int c = 0; c < 3; ++c)
                out[x * 3 + c] = uchar(clamp01(e[x * 3 + c] + add) * 255.0f + 0.5f);
        }
    });
    return result;
}

} // namespace svitlo
