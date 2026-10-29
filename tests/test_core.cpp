// Core tests: run without a display (QT_QPA_PLATFORM=offscreen).
// Plain asserts keep the project free of a test-framework dependency.

#include "core/EditSettings.h"
#include "core/Exporter.h"
#include "core/Geometry.h"
#include "core/Histogram.h"
#include "core/ImageLoader.h"
#include "core/Pipeline.h"
#include "core/ToneCurve.h"
#include "core/UpdateInfo.h"

#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QTemporaryDir>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

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


static void testRawBaseLook() {
    // Left half dark grey, right half light grey.
    QImage greys(64, 8, QImage::Format_RGB888);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 64; ++x)
            greys.setPixel(x, y, x < 32 ? qRgb(40, 40, 40) : qRgb(180, 180, 180));
    const ImageBuffer buf = ImageLoader::fromQImage(greys);
    Pipeline::Options raw;
    raw.isRaw = true;
    EditSettings standard;               // rawProfile = 0
    EditSettings neutral;
    neutral.rawProfile = 1;

    const QImage plain = Pipeline::render(buf, EditSettings{});
    const QImage std_ = Pipeline::render(buf, standard, raw);
    const QImage neu = Pipeline::render(buf, neutral, raw);

    // Neutral RAW == no base look at all.
    CHECK(maxAbsDiff(plain, neu) <= 1);
    // Standard adds contrast: shadows darker, upper mids brighter.
    CHECK(qGray(std_.pixel(10, 4)) < qGray(plain.pixel(10, 4)));
    CHECK(qGray(std_.pixel(50, 4)) > qGray(plain.pixel(50, 4)));
    // The base look must keep tones in order.
    float prev = -1.0f;
    for (int i = 0; i <= 200; ++i) {
        const float v = Pipeline::toneValue(EditSettings{}, 2.0f * i / 200.0f, true);
        CHECK(v >= prev - 1e-4f);
        prev = v;
    }
    // And it is saved with the edit.
    CHECK(EditSettings::fromJsonString(neutral.toJsonString()).rawProfile == 1);
}


static void testUpdateInfo() {
    using namespace svitlo::update;
    CHECK(normalizeVersion("v0.3.0") == "0.3.0");
    CHECK(normalizeVersion("v.0.1.0") == "0.1.0");      // the very first tag was written like this
    CHECK(isNewerVersion("v0.3.0", "0.2.0"));
    CHECK(isNewerVersion("v0.10.0", "0.9.1"));          // numeric, not alphabetical
    CHECK(!isNewerVersion("v0.2.0", "0.2.0"));
    CHECK(!isNewerVersion("v0.1.0", "0.2.0"));
    CHECK(!isNewerVersion("nonsense", "0.2.0"));

    const QByteArray json = R"({
        "tag_name": "v0.3.0", "html_url": "https://example.org/r", "body": "Notes",
        "draft": false, "prerelease": false,
        "assets": [
            {"name": "Svitlo-0.3.0-x86_64.dmg", "browser_download_url": "https://example.org/a.dmg", "size": 100},
            {"name": "Svitlo-0.3.0-windows-x64.zip", "browser_download_url": "https://example.org/a.zip", "size": 200}
        ]})";
    const ReleaseInfo mac = parseLatestRelease(json, ".dmg");
    CHECK(mac.valid && mac.version == "0.3.0" && mac.assetUrl == "https://example.org/a.dmg");
    const ReleaseInfo win = parseLatestRelease(json, "-windows-x64.zip");
    CHECK(win.assetName == "Svitlo-0.3.0-windows-x64.zip" && win.assetSize == 200);
    const ReleaseInfo linux_ = parseLatestRelease(json, QString());
    CHECK(linux_.valid && linux_.assetUrl.isEmpty());
    CHECK(!parseLatestRelease("not json", ".dmg").valid);
    CHECK(!parseLatestRelease(R"({"tag_name":"v9.0.0","prerelease":true})", ".dmg").valid);
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
    testRawBaseLook();
    testUpdateInfo();
    if (g_failures) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("All core tests passed\n");
    return EXIT_SUCCESS;
}
