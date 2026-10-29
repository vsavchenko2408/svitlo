#pragma once

#include "catalog/PhotoRecord.h"
#include "core/EditSettings.h"
#include "core/Histogram.h"
#include "core/ImageBuffer.h"
#include "core/ImageLoader.h"

#include <QFutureWatcher>
#include <QImage>
#include <QTimer>
#include <QWidget>

#include <memory>

class QListView;
class QToolButton;

namespace svitlo {

class Catalog;
class ThumbnailCache;
class PhotoModel;
class ImageCanvas;
class AdjustPanel;

// Develop module: preview canvas, adjustment panel and filmstrip.
class DevelopView : public QWidget {
    Q_OBJECT
public:
    DevelopView(Catalog* catalog, ThumbnailCache* thumbs, PhotoModel* model, QWidget* parent = nullptr);
    ~DevelopView() override;

    void openPhoto(qint64 id);
    qint64 currentPhotoId() const { return m_record.id; }
    const EditSettings& settings() const { return m_settings; }

    void undo();
    void redo();
    void resetAll();
    void pasteSettings(const EditSettings& source); // geometry is not pasted
    void toggleBefore();
    void toggleCropMode();
    void rotate(int quarterTurnsDelta);
    void step(int delta); // next / previous photo in the filmstrip
    void toggleZoom();

    // Commit any in-progress edit (called before switching away).
    void flush();

signals:
    void photoOpened(qint64 id);
    void exportRequested();
    void copySettingsRequested();
    void pasteSettingsRequested();

private:
    struct RenderOutput {
        QImage image;
        Histogram histogram;
        bool cropMode = false;
    };
    struct LoadOutput {
        std::shared_ptr<const ImageBuffer> buffer;
        ImageInfo info;
        QString error;
        int generation = 0;
    };

    void onPreviewLoaded();
    void setSettingsLive(const EditSettings& s);
    void requestRender();
    void onRenderFinished();
    void commit();
    void applyHistoryEntry();
    void applyAspectChoice(double choice);
    void refitCrop();
    void autoTone();
    void updateUndoButtons();
    QSize frameSize() const; // oriented preview size before crop
    double resolutionScale() const;

    Catalog* m_catalog;
    ThumbnailCache* m_thumbs;
    PhotoModel* m_model;

    ImageCanvas* m_canvas;
    AdjustPanel* m_panel;
    QListView* m_filmstrip;
    QToolButton* m_cropButton;
    QToolButton* m_beforeButton;
    QToolButton* m_undoButton;
    QToolButton* m_redoButton;

    PhotoRecord m_record;
    ImageInfo m_info;
    std::shared_ptr<const ImageBuffer> m_preview;
    EditSettings m_settings;
    QVector<EditSettings> m_history;
    int m_historyIndex = -1;
    bool m_cropMode = false;
    double m_aspectChoice = 0.0;

    int m_loadGeneration = 0;
    QFutureWatcher<LoadOutput> m_loadWatcher;
    QFutureWatcher<RenderOutput> m_renderWatcher;
    bool m_renderDirty = false;
    QImage m_lastRender;
    Histogram m_lastHistogram;
    QTimer m_commitTimer;
};

} // namespace svitlo
