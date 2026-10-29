#include "core/EditSettings.h"

#include <QJsonArray>
#include <QJsonDocument>

#include <algorithm>
#include <cmath>

namespace svitlo {

namespace {

bool nearlyEqual(double a, double b) { return std::abs(a - b) < 1e-9; }

double clampField(const QJsonObject& o, const char* key, double def, double lo, double hi) {
    const QJsonValue v = o.value(QLatin1String(key));
    if (!v.isDouble())
        return def;
    return std::clamp(v.toDouble(), lo, hi);
}

} // namespace

bool EditSettings::curveIsIdentity() const {
    if (curve.size() != 2)
        return false;
    return curve[0] == QPointF(0, 0) && curve[1] == QPointF(1, 1);
}

bool EditSettings::hasGeometry() const {
    return !nearlyEqual(rotation, 0.0) || crop != QRectF(0, 0, 1, 1) ||
           orientationQuarterTurns != 0;
}

bool EditSettings::isIdentity() const { return *this == EditSettings{}; }

bool EditSettings::operator==(const EditSettings& o) const {
    return nearlyEqual(temperature, o.temperature) && nearlyEqual(tint, o.tint) &&
           nearlyEqual(exposure, o.exposure) && nearlyEqual(contrast, o.contrast) &&
           nearlyEqual(highlights, o.highlights) && nearlyEqual(shadows, o.shadows) &&
           nearlyEqual(whites, o.whites) && nearlyEqual(blacks, o.blacks) &&
           nearlyEqual(clarity, o.clarity) && nearlyEqual(vibrance, o.vibrance) &&
           nearlyEqual(saturation, o.saturation) && curve == o.curve &&
           nearlyEqual(sharpenAmount, o.sharpenAmount) &&
           nearlyEqual(sharpenRadius, o.sharpenRadius) && nearlyEqual(vignette, o.vignette) &&
           nearlyEqual(vignetteMidpoint, o.vignetteMidpoint) && monochrome == o.monochrome &&
           nearlyEqual(rotation, o.rotation) && crop == o.crop &&
           orientationQuarterTurns == o.orientationQuarterTurns;
}

QJsonObject EditSettings::toJson() const {
    QJsonObject o;
    o["version"] = kVersion;
    o["temperature"] = temperature;
    o["tint"] = tint;
    o["exposure"] = exposure;
    o["contrast"] = contrast;
    o["highlights"] = highlights;
    o["shadows"] = shadows;
    o["whites"] = whites;
    o["blacks"] = blacks;
    o["clarity"] = clarity;
    o["vibrance"] = vibrance;
    o["saturation"] = saturation;

    QJsonArray pts;
    for (const QPointF& p : curve)
        pts.append(QJsonArray{p.x(), p.y()});
    o["curve"] = pts;

    o["sharpenAmount"] = sharpenAmount;
    o["sharpenRadius"] = sharpenRadius;
    o["vignette"] = vignette;
    o["vignetteMidpoint"] = vignetteMidpoint;
    o["monochrome"] = monochrome;
    o["rotation"] = rotation;
    o["crop"] = QJsonArray{crop.x(), crop.y(), crop.width(), crop.height()};
    o["orientation"] = orientationQuarterTurns;
    return o;
}

EditSettings EditSettings::fromJson(const QJsonObject& o) {
    EditSettings s;
    s.temperature = clampField(o, "temperature", 0, -100, 100);
    s.tint = clampField(o, "tint", 0, -100, 100);
    s.exposure = clampField(o, "exposure", 0, -5, 5);
    s.contrast = clampField(o, "contrast", 0, -100, 100);
    s.highlights = clampField(o, "highlights", 0, -100, 100);
    s.shadows = clampField(o, "shadows", 0, -100, 100);
    s.whites = clampField(o, "whites", 0, -100, 100);
    s.blacks = clampField(o, "blacks", 0, -100, 100);
    s.clarity = clampField(o, "clarity", 0, -100, 100);
    s.vibrance = clampField(o, "vibrance", 0, -100, 100);
    s.saturation = clampField(o, "saturation", 0, -100, 100);

    const QJsonArray pts = o.value("curve").toArray();
    if (pts.size() >= 2) {
        QVector<QPointF> curve;
        for (const QJsonValue& v : pts) {
            const QJsonArray p = v.toArray();
            if (p.size() == 2)
                curve.append(QPointF(std::clamp(p[0].toDouble(), 0.0, 1.0),
                                     std::clamp(p[1].toDouble(), 0.0, 1.0)));
        }
        std::sort(curve.begin(), curve.end(),
                  [](const QPointF& a, const QPointF& b) { return a.x() < b.x(); });
        if (curve.size() >= 2)
            s.curve = curve;
    }

    s.sharpenAmount = clampField(o, "sharpenAmount", 0, 0, 150);
    s.sharpenRadius = clampField(o, "sharpenRadius", 1.0, 0.5, 3.0);
    s.vignette = clampField(o, "vignette", 0, -100, 100);
    s.vignetteMidpoint = clampField(o, "vignetteMidpoint", 50, 0, 100);
    s.monochrome = o.value("monochrome").toBool(false);
    s.rotation = clampField(o, "rotation", 0, -45, 45);

    const QJsonArray c = o.value("crop").toArray();
    if (c.size() == 4) {
        QRectF r(c[0].toDouble(), c[1].toDouble(), c[2].toDouble(), c[3].toDouble());
        r = r.intersected(QRectF(0, 0, 1, 1));
        if (r.width() > 0.001 && r.height() > 0.001)
            s.crop = r;
    }
    s.orientationQuarterTurns = std::clamp(o.value("orientation").toInt(0), 0, 3);
    return s;
}

QString EditSettings::toJsonString() const {
    return QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact));
}

EditSettings EditSettings::fromJsonString(const QString& json) {
    if (json.isEmpty())
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    return doc.isObject() ? fromJson(doc.object()) : EditSettings{};
}

} // namespace svitlo
