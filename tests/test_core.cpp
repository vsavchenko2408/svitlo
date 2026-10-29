// Core tests: run without a display (QT_QPA_PLATFORM=offscreen).
// Plain asserts keep the project free of a test-framework dependency.

#include "core/EditSettings.h"
#include "core/Exporter.h"
#include "core/Geometry.h"
#include "core/Histogram.h"
#include "core/ImageLoader.h"
#include "core/Pipeline.h"
#include "core/ToneCurve.h"

#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QTemporaryDir>

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace svitlo;

static int g_failures = 0;

#define CHECK(cond)                                                               \
    do {                                                                          \
        if (!(cond)) {                                                            \
            std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);  \
            ++g_failures;                                                         \
        }                                                                         \
    } while (0)

static QImage gradientImage(int w, int h) {
    QImage img(w, h, QImage::Format_RGB888);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            img.setPixel(x, y, qRgb(x * 255 / (w - 1), y * 255 / (h - 1), 128));
    return img;
}

static int maxAbsDiff(const QImage& a, const QImage& b) {
    int m = 0;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x) {
            const QRgb p = a.pixel(x, y), q = b.pixel(x, y);
            m = std::max({m, std::abs(qRed(p) - qRed(q)), std::abs(qGreen(p) - qGreen(q)),
                          std::abs(qBlue(p) - qBlue(q))});
        }
    return m;
}

static void testIdentityRoundTrip() {
    const QImage src = gradientImage(64, 48);
    const QImage out = Pipeline::render(ImageLoader::fromQImage(src), EditSettings{});
    CHECK(out.size() == src.size());
    CHECK(maxAbsDiff(src, out) <= 1); // default settings must not change the photo
}

static void testExposureBrightens() {
    const ImageBuffer buf = ImageLoader::fromQImage(gradientImage(32, 32));
    EditSettings s;
    s.exposure = 1.0;
    const QImage brighter = Pipeline::render(buf, s);
    s.exposure = -1.0;
    const QImage darker = Pipeline::render(buf, s);
    const QRgb mid = Pipeline::render(buf, EditSettings{}).pixel(16, 16);
    CHECK(qGray(brighter.pixel(16, 16)) > qGray(mid));
    CHECK(qGray(darker.pixel(16, 16)) < qGray(mid));
}

static void testToneMonotonic() {
    EditSettings s;
    s.contrast = 100;
    s.shadows = 100;
    s.highlights = -100;
    s.whites = 50;
    s.blacks = -50;
    float prev = -1.0f;
    for (int i = 0; i <= 400; ++i) {
        const float v = Pipeline::toneValue(s, 2.0f * i / 400.0f);
        CHECK(v >= prev - 1e-4f);
        prev = v;
    }
}

static void testToneCurve() {
    ToneCurve identity({QPointF(0, 0), QPointF(1, 1)});
    CHECK(std::abs(identity.evaluate(0.3) - 0.3) < 1e-9);

    ToneCurve s({QPointF(0, 0), QPointF(0.25, 0.15), QPointF(0.75, 0.85), QPointF(1, 1)});
    double prev = -1;
    for (int i = 0; i <= 100; ++i) {
        const double v = s.evaluate(i / 100.0);
        CHECK(v >= prev - 1e-12);
        prev = v;
    }
    CHECK(s.evaluate(0.25) == 0.15);
}

static void testMonochrome() {
    EditSettings s;
    s.monochrome = true;
    const QImage out = Pipeline::render(ImageLoader::fromQImage(gradientImage(16, 16)), s);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            const QRgb p = out.pixel(x, y);
            CHECK(qRed(p) == qGreen(p) && qGreen(p) == qBlue(p));
        }
}

static void testGeometry() {
    const ImageBuffer buf = ImageLoader::fromQImage(gradientImage(40, 20));
    EditSettings s;
    s.orientationQuarterTurns = 1;
    QImage out = Pipeline::render(buf, s);
    CHECK(out.width() == 20 && out.height() == 40);

    s = {};
    s.crop = QRectF(0.25, 0.25, 0.5, 0.5);
    out = Pipeline::render(buf, s);
    CHECK(out.width() == 20 && out.height() == 10);

    // A full-frame crop never fits a rotated image; fitting must shrink it.
    const QRectF fitted = geometry::fitCropToImage(QRectF(0, 0, 1, 1), 10.0, QSizeF(400, 300));
    CHECK(fitted.width() < 1.0 && fitted.height() < 1.0);
    const QRectF px(fitted.x() * 400, fitted.y() * 300, fitted.width() * 400, fitted.height() * 300);
    CHECK(geometry::cropFitsImage(px, 10.0, QSizeF(400, 300)));
    CHECK(std::abs(fitted.width() / fitted.height() - 1.0) < 1e-3); // aspect kept
}

static void testSettingsJson() {
    EditSettings s;
    s.exposure = 1.25;
    s.contrast = -30;
    s.curve = {QPointF(0, 0.05), QPointF(0.5, 0.6), QPointF(1, 1)};
    s.crop = QRectF(0.1, 0.1, 0.7, 0.6);
    s.rotation = 3.5;
    s.monochrome = true;
    const EditSettings back = EditSettings::fromJsonString(s.toJsonString());
    CHECK(back == s);
    CHECK(EditSettings::fromJsonString("garbage") == EditSettings{});
    CHECK(EditSettings{}.isIdentity());
}

static void testHistogram() {
    QImage img(10, 10, QImage::Format_RGB888);
    img.fill(Qt::white);
    const Histogram h = Histogram::compute(img);
    CHECK(h.pixelCount == 100);
    CHECK(h.red[255] == 100);
    CHECK(h.highlightClipFraction() == 1.0);
}

static void testExport() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString src = dir.filePath("source.png");
    CHECK(gradientImage(300, 200).save(src));

    ExportOptions opt;
    opt.outputDir = dir.filePath("out");
    opt.longEdge = 150;
    EditSettings s;
    s.sharpenAmount = 50;
    s.clarity = 30;
    QString written, error;
    CHECK(Exporter::exportPhoto(src, s, opt, &written, &error));
    const QImage result(written);
    CHECK(result.width() == 150 && result.height() == 100);

    // Second export must not overwrite the first.
    QString second;
    CHECK(Exporter::exportPhoto(src, s, opt, &second, &error));
    CHECK(second != written);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testIdentityRoundTrip();
    testExposureBrightens();
    testToneMonotonic();
    testToneCurve();
    testMonochrome();
    testGeometry();
    testSettingsJson();
    testHistogram();
    testExport();
    if (g_failures) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("All core tests passed\n");
    return EXIT_SUCCESS;
}
