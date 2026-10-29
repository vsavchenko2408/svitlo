#include "ui/DevelopView.h"

#include "catalog/Catalog.h"
#include "catalog/ThumbnailCache.h"
#include "core/ColorMath.h"
#include "core/Geometry.h"
#include "core/Pipeline.h"
#include "ui/AdjustPanel.h"
#include "ui/ImageCanvas.h"
#include "ui/PhotoDelegate.h"
#include "ui/PhotoModel.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QListView>
#include <QSplitter>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <cmath>

namespace svitlo {

namespace {
constexpr int kPreviewLongEdge = 2560;
constexpr int kMaxHistory = 200;
} // namespace

DevelopView::DevelopView(Catalog* catalog, ThumbnailCache* thumbs, PhotoModel* model, QWidget* parent)
    : QWidget(parent), m_catalog(catalog), m_thumbs(thumbs), m_model(model) {
    // --- Toolbar ---------------------------------------------------------------
    auto* bar = new QToolBar;
    auto addButton = [bar](const QString& text, const QString& tip, bool checkable = false) {
        auto* b = new QToolButton;
        b->setText(text);
        b->setToolTip(tip);
        b->setCheckable(checkable);
        bar->addWidget(b);
        return b;
    };
    m_undoButton = addButton(tr("Undo"), tr("Undo (⌘Z)"));
    m_redoButton = addButton(tr("Redo"), tr("Redo (⇧⌘Z)"));
    bar->addSeparator();
    m_beforeButton = addButton(tr("Before / After"), tr("Show the original (\\)"), true);
    m_cropButton = addButton(tr("Crop"), tr("Crop & straighten (C)"), true);
    auto* zoom = addButton(tr("Fit / 100%"), tr("Toggle zoom (Z) — or double-click the photo"));
    bar->addSeparator();
    auto* copy = addButton(tr("Copy settings"), tr("Copy develop settings (⇧⌘C)"));
    auto* paste = addButton(tr("Paste settings"), tr("Paste develop settings (⇧⌘V)"));
    auto* reset = addButton(tr("Reset"), tr("Reset all adjustments"));
    auto* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bar->addWidget(spacer);
    auto* exportButton = addButton(tr("Export…"), tr("Export (⇧⌘E)"));

    // --- Canvas, panel, filmstrip ------------------------------------------------
    m_canvas = new ImageCanvas;
    m_panel = new AdjustPanel;

    m_filmstrip = new QListView;
    auto* stripDelegate = new PhotoDelegate(m_filmstrip);
    stripDelegate->setCompact(true);
    stripDelegate->setCellSize(92);
    m_filmstrip->setModel(m_model);
    m_filmstrip->setItemDelegate(stripDelegate);
    m_filmstrip->setFlow(QListView::LeftToRight);
    m_filmstrip->setWrapping(false);
    m_filmstrip->setUniformItemSizes(true);
    m_filmstrip->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_filmstrip->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_filmstrip->setFixedHeight(110);
    m_filmstrip->setMouseTracking(true);

    auto* centre = new QWidget;
    auto* centreLayout = new QVBoxLayout(centre);
    centreLayout->setContentsMargins(0, 0, 0, 0);
    centreLayout->setSpacing(0);
    centreLayout->addWidget(bar);
    centreLayout->addWidget(m_canvas, 1);
    centreLayout->addWidget(m_filmstrip);

    auto* splitter = new QSplitter;
    splitter->addWidget(centre);
    splitter->addWidget(m_panel);
    splitter->setStretchFactor(0, 1);
    splitter->setCollapsible(0, false);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    // --- Wiring -----------------------------------------------------------------------
    m_commitTimer.setSingleShot(true);
    m_commitTimer.setInterval(600);
    connect(&m_commitTimer, &QTimer::timeout, this, &DevelopView::commit);

    connect(m_panel, &AdjustPanel::settingsChanged, this, [this](const EditSettings& s) {
        const bool rotationChanged = s.rotation != m_settings.rotation;
        const bool profileChanged = s.rawProfile != m_settings.rawProfile;
        EditSettings next = s;
        // The panel does not own the crop rectangle; keep ours.
        next.crop = m_settings.crop;
        next.orientationQuarterTurns = m_settings.orientationQuarterTurns;
        m_settings = next;
        if (rotationChanged)
            refitCrop();
        if (profileChanged)
            m_canvas->setBeforeImage(renderBefore());
        requestRender();
        m_commitTimer.start();
    });
    connect(m_panel, &AdjustPanel::editingFinished, this, &DevelopView::commit);
    connect(m_panel, &AdjustPanel::rotateRequested, this, &DevelopView::rotate);
    connect(m_panel, &AdjustPanel::resetCropRequested, this, [this] {
        m_settings.crop = QRectF(0, 0, 1, 1);
        m_settings.rotation = 0;
        m_panel->setSettings(m_settings);
        applyAspectChoice(m_aspectChoice);
        requestRender();
        commit();
    });
    connect(m_panel, &AdjustPanel::aspectChoiceChanged, this, [this](double c) {
        applyAspectChoice(c);
        requestRender();
        commit();
    });
    connect(m_panel, &AdjustPanel::autoToneRequested, this, &DevelopView::autoTone);

    connect(m_canvas, &ImageCanvas::cropChanged, this, [this](const QRectF& r) {
        const QSize f = frameSize();
        const QRectF fitted = geometry::fitCropToImage(r, m_settings.rotation, QSizeF(f));
        m_settings.crop = fitted;
        if (fitted != r)
            m_canvas->setCropRect(fitted);
    });
    connect(m_canvas, &ImageCanvas::cropEditingFinished, this, &DevelopView::commit);

    connect(m_undoButton, &QToolButton::clicked, this, &DevelopView::undo);
    connect(m_redoButton, &QToolButton::clicked, this, &DevelopView::redo);
    connect(m_beforeButton, &QToolButton::clicked, this, &DevelopView::toggleBefore);
    connect(m_cropButton, &QToolButton::clicked, this, &DevelopView::toggleCropMode);
    connect(zoom, &QToolButton::clicked, this, &DevelopView::toggleZoom);
    connect(copy, &QToolButton::clicked, this, &DevelopView::copySettingsRequested);
    connect(paste, &QToolButton::clicked, this, &DevelopView::pasteSettingsRequested);
    connect(reset, &QToolButton::clicked, this, &DevelopView::resetAll);
    connect(exportButton, &QToolButton::clicked, this, &DevelopView::exportRequested);

    connect(m_filmstrip, &QListView::clicked, this, [this](const QModelIndex& idx) {
        openPhoto(idx.data(PhotoModel::IdRole).toLongLong());
    });

    connect(&m_loadWatcher, &QFutureWatcherBase::finished, this, &DevelopView::onPreviewLoaded);
    connect(&m_renderWatcher, &QFutureWatcherBase::finished, this, &DevelopView::onRenderFinished);

    updateUndoButtons();
}

DevelopView::~DevelopView() {
    m_loadWatcher.waitForFinished();
    m_renderWatcher.waitForFinished();
}

// --- Loading -------------------------------------------------------------------------

void DevelopView::openPhoto(qint64 id) {
    if (id == m_record.id && m_preview)
        return;
    flush();

    const PhotoRecord rec = m_catalog->photo(id);
    if (!rec.id)
        return;
    m_record = rec;
    m_settings = EditSettings::fromJsonString(rec.settingsJson);
    m_panel->setSettings(m_settings);
    m_history = {m_settings};
    m_historyIndex = 0;
    updateUndoButtons();

    if (m_cropMode)
        toggleCropMode();
    m_preview.reset();
    m_lastRender = QImage();

    // Show the thumbnail immediately while the real preview decodes.
    const QImage thumb = m_thumbs->thumbnail(rec);
    if (!thumb.isNull())
        m_canvas->setImage(thumb);
    else
        m_canvas->setImage(QImage());
    m_canvas->setMessage(tr("Loading %1…").arg(rec.fileName));
    m_canvas->setBeforeImage(QImage());
    m_panel->setInfoCaption(rec.fileName);

    const int row = m_model->rowOf(id);
    if (row >= 0) {
        m_filmstrip->setCurrentIndex(m_model->index(row));
        m_filmstrip->scrollTo(m_model->index(row), QAbstractItemView::PositionAtCenter);
    }

    const int generation = ++m_loadGeneration;
    const QString path = rec.path;
    m_loadWatcher.setFuture(QtConcurrent::run([path, generation] {
        LoadOutput out;
        out.generation = generation;
        ImageBuffer buf = ImageLoader::load(path, ImageLoader::Quality::Preview, kPreviewLongEdge,
                                            &out.error, &out.info);
        if (!buf.isNull())
            out.buffer = std::make_shared<const ImageBuffer>(std::move(buf));
        return out;
    }));
    emit photoOpened(id);
}

void DevelopView::onPreviewLoaded() {
    const LoadOutput out = m_loadWatcher.result();
    if (out.generation != m_loadGeneration) {
        // A newer photo was requested meanwhile; start its load if pending.
        return;
    }
    if (!out.buffer) {
        m_canvas->setImage(QImage());
        m_canvas->setMessage(tr("Cannot open %1\n%2").arg(m_record.fileName, out.error));
        return;
    }
    m_preview = out.buffer;
    m_info = out.info;
    if (m_info.size.isValid())
        m_catalog->updateInfo(m_record.id, m_info);

    const bool isRaw = ImageLoader::isRawFile(m_record.path);
    m_panel->setRawControlsVisible(isRaw);
    QString caption = isRaw ? QStringLiteral("RAW  ·  ") + m_record.fileName : m_record.fileName;
    if (m_info.size.isValid())
        caption += QStringLiteral("  ·  %1 × %2").arg(m_info.size.width()).arg(m_info.size.height());
    if (!m_info.camera.trimmed().isEmpty())
        caption += QStringLiteral("  ·  ") + m_info.camera.trimmed();
    m_panel->setInfoCaption(caption);

    // "Before" = the photo as decoded, without any adjustments or crop.
    m_canvas->setBeforeImage(renderBefore());
    requestRender();
}

// "Before" = the photo as decoded, without the user's adjustments or crop,
// but with its orientation and (for RAW) the chosen base look.
QImage DevelopView::renderBefore() const {
    if (!m_preview)
        return {};
    EditSettings original;
    original.orientationQuarterTurns = m_settings.orientationQuarterTurns;
    original.rawProfile = m_settings.rawProfile;
    Pipeline::Options opt;
    opt.isRaw = ImageLoader::isRawFile(m_record.path);
    return Pipeline::render(*m_preview, original, opt);
}

// --- Rendering ------------------------------------------------------------------------

double DevelopView::resolutionScale() const {
    if (!m_preview || !m_info.size.isValid() || m_info.size.isEmpty())
        return 1.0;
    return double(std::max(m_preview->width, m_preview->height)) /
           std::max(m_info.size.width(), m_info.size.height());
}

QSize DevelopView::frameSize() const {
    if (!m_preview)
        return {1, 1};
    return geometry::orientedSize(QSize(m_preview->width, m_preview->height), m_settings.orientationQuarterTurns);
}

void DevelopView::requestRender() {
    if (!m_preview)
        return;
    if (m_renderWatcher.isRunning()) {
        m_renderDirty = true; // coalesce: render again with the latest settings
        return;
    }
    m_renderDirty = false;
    const auto preview = m_preview;
    const EditSettings settings = m_settings;
    const bool cropMode = m_cropMode;
    Pipeline::Options opt;
    opt.applyCrop = !cropMode;
    opt.isRaw = ImageLoader::isRawFile(m_record.path);
    opt.resolutionScale = resolutionScale();

    m_renderWatcher.setFuture(QtConcurrent::run([preview, settings, opt, cropMode] {
        RenderOutput out;
        out.image = Pipeline::render(*preview, settings, opt);
        out.histogram = Histogram::compute(out.image);
        out.cropMode = cropMode;
        return out;
    }));
}

void DevelopView::onRenderFinished() {
    const RenderOutput out = m_renderWatcher.result();
    if (out.cropMode == m_cropMode) {
        m_canvas->setImage(out.image);
        m_panel->setHistogram(out.histogram);
        if (!out.cropMode) {
            m_lastRender = out.image;
            m_lastHistogram = out.histogram;
        }
    }
    if (m_renderDirty)
        requestRender();
}

// --- Editing & history -------------------------------------------------------------------

void DevelopView::setSettingsLive(const EditSettings& s) {
    m_settings = s;
    m_panel->setSettings(s);
    requestRender();
}

void DevelopView::commit() {
    m_commitTimer.stop();
    if (!m_record.id)
        return;
    if (m_historyIndex >= 0 && m_history[m_historyIndex] == m_settings)
        return;

    // Drop the redo branch and append.
    m_history.resize(m_historyIndex + 1);
    m_history.append(m_settings);
    if (m_history.size() > kMaxHistory)
        m_history.removeFirst();
    m_historyIndex = m_history.size() - 1;
    updateUndoButtons();

    m_catalog->setSettings(m_record.id, m_settings);
    m_record = m_catalog->photo(m_record.id);
    // Use the preview we already rendered as the new thumbnail, if it is
    // current (not mid-crop and not about to be re-rendered).
    if (!m_cropMode && !m_renderWatcher.isRunning() && !m_lastRender.isNull())
        m_thumbs->store(m_record, m_lastRender);
}

void DevelopView::flush() {
    if (m_commitTimer.isActive() || (m_historyIndex >= 0 && m_history[m_historyIndex] != m_settings))
        commit();
}

void DevelopView::applyHistoryEntry() {
    setSettingsLive(m_history[m_historyIndex]);
    if (m_cropMode)
        m_canvas->setCropRect(m_settings.crop);
    m_catalog->setSettings(m_record.id, m_settings);
    m_record = m_catalog->photo(m_record.id);
    updateUndoButtons();
}

void DevelopView::undo() {
    flush();
    if (m_historyIndex <= 0)
        return;
    --m_historyIndex;
    applyHistoryEntry();
}

void DevelopView::redo() {
    if (m_historyIndex + 1 >= m_history.size())
        return;
    ++m_historyIndex;
    applyHistoryEntry();
}

void DevelopView::updateUndoButtons() {
    m_undoButton->setEnabled(m_historyIndex > 0);
    m_redoButton->setEnabled(m_historyIndex + 1 < m_history.size());
}

void DevelopView::resetAll() {
    if (!m_record.id)
        return;
    setSettingsLive(EditSettings{});
    if (m_cropMode)
        m_canvas->setCropRect(m_settings.crop);
    commit();
}

void DevelopView::pasteSettings(const EditSettings& source) {
    if (!m_record.id)
        return;
    EditSettings s = source;
    s.crop = m_settings.crop;
    s.rotation = m_settings.rotation;
    s.orientationQuarterTurns = m_settings.orientationQuarterTurns;
    setSettingsLive(s);
    commit();
}

// --- Tools ---------------------------------------------------------------------------------

void DevelopView::toggleBefore() {
    const bool on = !m_canvas->showBefore();
    m_canvas->setShowBefore(on);
    m_beforeButton->setChecked(on);
}

void DevelopView::toggleCropMode() {
    if (!m_record.id) {
        m_cropButton->setChecked(false);
        return;
    }
    m_cropMode = !m_cropMode;
    m_cropButton->setChecked(m_cropMode);
    m_canvas->setCropMode(m_cropMode);
    if (m_cropMode) {
        applyAspectChoice(m_aspectChoice);
        m_canvas->setCropRect(m_settings.crop);
    } else {
        commit();
    }
    requestRender();
}

void DevelopView::rotate(int delta) {
    if (!m_record.id)
        return;
    m_settings.orientationQuarterTurns = ((m_settings.orientationQuarterTurns + delta) % 4 + 4) % 4;
    // Crop coordinates are relative to the old frame; start over.
    m_settings.crop = QRectF(0, 0, 1, 1);
    if (m_cropMode)
        m_canvas->setCropRect(m_settings.crop);
    if (m_preview) {
        m_canvas->setBeforeImage(renderBefore());
    }
    requestRender();
    commit();
}

void DevelopView::refitCrop() {
    if (!m_preview)
        return;
    m_settings.crop = geometry::fitCropToImage(m_settings.crop, m_settings.rotation, QSizeF(frameSize()));
    if (m_cropMode)
        m_canvas->setCropRect(m_settings.crop);
}

void DevelopView::applyAspectChoice(double choice) {
    m_aspectChoice = choice;
    if (!m_preview)
        return;
    const QSize f = frameSize();
    if (choice == 0.0) {
        m_canvas->setCropAspect(0.0);
        return;
    }

    // Pixel aspect (w/h) of the requested shape, matching the current
    // crop's orientation for long:short ratios.
    const QRectF c = m_settings.crop;
    const double cw = c.width() * f.width(), ch = c.height() * f.height();
    double aspect;
    if (choice < 0) {
        aspect = double(f.width()) / f.height();
    } else {
        aspect = cw >= ch ? choice : 1.0 / choice;
    }
    m_canvas->setCropAspect(aspect);

    // Largest rect of that aspect inside the current crop, same centre.
    double w = cw, h = ch;
    if (w / h > aspect) w = h * aspect;
    else h = w / aspect;
    QRectF r(0, 0, w / f.width(), h / f.height());
    r.moveCenter(c.center());
    m_settings.crop = geometry::fitCropToImage(r, m_settings.rotation, QSizeF(f));
    if (m_cropMode)
        m_canvas->setCropRect(m_settings.crop);
}

void DevelopView::step(int delta) {
    const int row = m_model->rowOf(m_record.id);
    const int next = row + delta;
    if (row < 0 || next < 0 || next >= m_model->rowCount())
        return;
    openPhoto(m_model->recordAt(next)->id);
}

void DevelopView::toggleZoom() {
    if (m_cropMode)
        return;
    m_canvas->setFitToWindow(!m_canvas->fitToWindow());
}

void DevelopView::autoTone() {
    if (!m_preview)
        return;
    // Statistics of the white-balanced linear image, ignoring the user's
    // tone settings, sampled on a grid.
    const ImageBuffer& img = *m_preview;
    std::vector<float> lum;
    const int stepPx = std::max(1, int(std::sqrt(double(img.width) * img.height / 40000.0)));
    for (int y = 0; y < img.height; y += stepPx)
        for (int x = 0; x < img.width; x += stepPx) {
            const float* p = img.at(x, y);
            lum.push_back(color::luminance(p[0], p[1], p[2]));
        }
    if (lum.empty())
        return;
    auto percentile = [&](double q) {
        const std::size_t k = std::min(lum.size() - 1, std::size_t(q * (lum.size() - 1)));
        std::nth_element(lum.begin(), lum.begin() + k, lum.end());
        return lum[k];
    };
    const float median = std::max(1e-4f, percentile(0.5));
    const float low = percentile(0.005);
    const float high = percentile(0.995);

    EditSettings s = m_settings;
    // Bring the median towards middle grey (18%), but only part of the way.
    s.exposure = std::clamp(0.7 * std::log2(0.18 / median), -3.0, 3.0);
    const double gain = std::exp2(s.exposure);
    const double lowL = std::pow(low * gain, 1.0 / 2.2);
    const double highL = std::pow(high * gain, 1.0 / 2.2);
    s.whites = std::clamp((1.0 - highL) / 0.25 * 100.0, -60.0, 60.0);
    s.blacks = std::clamp(-lowL / 0.12 * 100.0, -60.0, 60.0);
    s.contrast = 10;
    s.highlights = highL > 1.0 ? -40 : 0;
    s.shadows = lowL < 0.08 ? 20 : 0;
    setSettingsLive(s);
    commit();
}

} // namespace svitlo
