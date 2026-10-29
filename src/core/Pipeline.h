#pragma once

#include "core/EditSettings.h"
#include "core/ImageBuffer.h"

#include <QImage>

namespace svitlo {

// The develop pipeline. Order of operations:
//   1. geometry  — orientation, straighten, crop
//   2. white balance and exposure (linear light)
//      (RAW files: base look curve + colour, see EditSettings::rawProfile)
//   3. clarity   — local contrast on perceptual lightness
//   4. tone      — blacks/whites, shadows/highlights, contrast, tone curve
//   5. colour    — vibrance, saturation, monochrome
//   6. vignette  — applied to the cropped frame
//   7. gamut     — soft clip into [0,1] preserving luminance
//   8. sharpen   — unsharp mask on the encoded image
//   9. output    — 8-bit sRGB
class Pipeline {
public:
    struct Options {
        bool applyCrop = true;
        // Pixels in the source buffer per pixel of the full-resolution image.
        // Used to keep radius-based effects (sharpening) consistent between
        // the downscaled preview and the full-size export.
        double resolutionScale = 1.0;
        // The source is a RAW file: apply the RAW base look selected by
        // EditSettings::rawProfile before the user's adjustments.
        bool isRaw = false;
    };

    static QImage render(const ImageBuffer& source, const EditSettings& settings,
                         const Options& options);
    static QImage render(const ImageBuffer& source, const EditSettings& settings) {
        return render(source, settings, Options{});
    }

    // Exposed for tests: the tone function on perceptual lightness.
    static float toneValue(const EditSettings& settings, float lightness, bool rawBaseLook = false);
};

} // namespace svitlo
