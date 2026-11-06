#pragma once

#include "core/EditSettings.h"
#include "core/Histogram.h"

#include <QWidget>

#include <array>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QVBoxLayout;

namespace svitlo {

class SliderRow;
class ToneCurveWidget;
class HistogramWidget;

// The right-hand develop panel: histogram plus all adjustment controls.
class AdjustPanel : public QWidget {
    Q_OBJECT
public:
    explicit AdjustPanel(QWidget* parent = nullptr);

    void setSettings(const EditSettings& s); // updates controls, no signals
    const EditSettings& settings() const { return m_settings; }

    void setHistogram(const Histogram& h);
    void setInfoCaption(const QString& text);
    // The RAW profile selector is shown only for RAW files.
    void setRawControlsVisible(bool visible);

    // Aspect choice: 0 = free, -1 = original, otherwise long/short ratio.
    double aspectChoice() const;

signals:
    void settingsChanged(const svitlo::EditSettings& s); // live
    void editingFinished();
    void aspectChoiceChanged(double choice);
    void rotateRequested(int quarterTurnsDelta);
    void resetCropRequested();
    void autoToneRequested();

private:
    SliderRow* addSlider(QVBoxLayout* layout, const QString& label, double min, double max,
                         double def, int decimals, double EditSettings::*field);
    // Colour mixer: the 8 colour rows show hue (0), saturation (1) or
    // luminance (2) sliders, switched with the buttons above them.
    void showHslMode(int mode);

    EditSettings m_settings;
    QVector<QPair<SliderRow*, double EditSettings::*>> m_sliders;
    ToneCurveWidget* m_curve = nullptr;
    HistogramWidget* m_histogram = nullptr;
    QCheckBox* m_mono = nullptr;
    QComboBox* m_aspect = nullptr;
    QComboBox* m_profile = nullptr;
    QWidget* m_profileRow = nullptr;
    std::array<SliderRow*, 8> m_hslRows{};
    QButtonGroup* m_hslModes = nullptr;
    int m_hslMode = 0;
};

} // namespace svitlo
