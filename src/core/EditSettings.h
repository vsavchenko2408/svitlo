#pragma once

#include <QJsonObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

namespace svitlo {

// Every edit is a parameter set; the original file is never modified.
// Slider-style values use the ranges noted next to each field and 0 means
// "no change", so a default-constructed EditSettings renders the photo as-is.
struct EditSettings {
    static constexpr int kVersion = 1;

    // White balance — relative to the camera / as-shot balance.
    double temperature = 0.0; // -100..100 (negative = cooler, positive = warmer)
    double tint = 0.0;        // -100..100 (negative = green, positive = magenta)

    // Tone
    double exposure = 0.0;    // EV, -5..5
    double contrast = 0.0;    // -100..100
    double highlights = 0.0;  // -100..100
    double shadows = 0.0;     // -100..100
    double whites = 0.0;      // -100..100
    double blacks = 0.0;      // -100..100

    // Presence
    double clarity = 0.0;     // -100..100 (local contrast)
    double vibrance = 0.0;    // -100..100
    double saturation = 0.0;  // -100..100

    // Tone curve: control points in [0,1]x[0,1], sorted by x, endpoints
    // included. The identity curve is {(0,0),(1,1)}.
    QVector<QPointF> curve{QPointF(0, 0), QPointF(1, 1)};

    // Detail
    double sharpenAmount = 0.0; // 0..150
    double sharpenRadius = 1.0; // 0.5..3.0 px at full resolution

    // Effects
    double vignette = 0.0;        // -100..100 (negative darkens corners)
    double vignetteMidpoint = 50; // 0..100

    // Treatment
    bool monochrome = false;

    // Base look for RAW files (ignored for JPEG/PNG/TIFF):
    // 0 = Standard — gentle contrast and colour, close to a camera JPEG;
    // 1 = Neutral  — flat, linear rendering for maximum latitude.
    int rawProfile = 0;

    // Geometry. Rotation (degrees) is applied about the image centre first,
    // then the crop rectangle is taken in the rotated frame, normalised to
    // [0,1] in both axes.
    double rotation = 0.0;               // -45..45
    QRectF crop{0.0, 0.0, 1.0, 1.0};
    int orientationQuarterTurns = 0;     // 0..3, clockwise 90° steps

    bool isIdentity() const;
    bool hasGeometry() const;
    bool curveIsIdentity() const;

    QJsonObject toJson() const;
    static EditSettings fromJson(const QJsonObject& obj);

    QString toJsonString() const;
    static EditSettings fromJsonString(const QString& json);

    bool operator==(const EditSettings& other) const;
    bool operator!=(const EditSettings& other) const { return !(*this == other); }
};

} // namespace svitlo
