#include "ui/AdjustPanel.h"

#include "ui/CollapsibleSection.h"
#include "ui/HistogramWidget.h"
#include "ui/SliderRow.h"
#include "ui/ToneCurveWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace svitlo {

AdjustPanel::AdjustPanel(QWidget* parent) : QWidget(parent) {
    setMinimumWidth(290);
    setMaximumWidth(380);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(10, 10, 4, 0);
    outer->setSpacing(8);

    m_histogram = new HistogramWidget;
    outer->addWidget(m_histogram);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* content = new QWidget;
    auto* col = new QVBoxLayout(content);
    col->setContentsMargins(0, 0, 8, 12);
    col->setSpacing(0);
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    // --- Light & colour ------------------------------------------------------
    auto* basic = new CollapsibleSection(tr("Light & Colour"));
    auto* b = basic->contentLayout();

    auto* autoButton = new QPushButton(tr("Auto tone"));
    autoButton->setToolTip(tr("Set exposure, contrast, whites and blacks from the histogram"));
    connect(autoButton, &QPushButton::clicked, this, &AdjustPanel::autoToneRequested);
    b->addWidget(autoButton);

    auto* temp = addSlider(b, tr("Temperature"), -100, 100, 0, 0, &EditSettings::temperature);
    temp->setTrackGradient(QColor(0x4A, 0x7B, 0xD0), QColor(0xE0, 0xC0, 0x4A));
    auto* tint = addSlider(b, tr("Tint"), -100, 100, 0, 0, &EditSettings::tint);
    tint->setTrackGradient(QColor(0x5A, 0xB0, 0x5A), QColor(0xC0, 0x5A, 0xC0));
    addSlider(b, tr("Exposure"), -5, 5, 0, 2, &EditSettings::exposure);
    addSlider(b, tr("Contrast"), -100, 100, 0, 0, &EditSettings::contrast);
    addSlider(b, tr("Highlights"), -100, 100, 0, 0, &EditSettings::highlights);
    addSlider(b, tr("Shadows"), -100, 100, 0, 0, &EditSettings::shadows);
    addSlider(b, tr("Whites"), -100, 100, 0, 0, &EditSettings::whites);
    addSlider(b, tr("Blacks"), -100, 100, 0, 0, &EditSettings::blacks);
    addSlider(b, tr("Clarity"), -100, 100, 0, 0, &EditSettings::clarity);
    addSlider(b, tr("Vibrance"), -100, 100, 0, 0, &EditSettings::vibrance);
    auto* sat = addSlider(b, tr("Saturation"), -100, 100, 0, 0, &EditSettings::saturation);
    sat->setTrackGradient(QColor(0x80, 0x80, 0x80), QColor(0xE0, 0x50, 0x80));

    m_mono = new QCheckBox(tr("Black && white"));
    connect(m_mono, &QCheckBox::toggled, this, [this](bool on) {
        m_settings.monochrome = on;
        emit settingsChanged(m_settings);
        emit editingFinished();
    });
    b->addWidget(m_mono);
    col->addWidget(basic);

    // --- Tone curve ------------------------------------------------------------
    auto* curveSection = new CollapsibleSection(tr("Tone Curve"));
    m_curve = new ToneCurveWidget;
    curveSection->contentLayout()->addWidget(m_curve);
    auto* curveReset = new QPushButton(tr("Reset curve"));
    curveSection->contentLayout()->addWidget(curveReset, 0, Qt::AlignRight);
    connect(m_curve, &ToneCurveWidget::curveChanged, this, [this](const QVector<QPointF>& pts) {
        m_settings.curve = pts;
        emit settingsChanged(m_settings);
    });
    connect(m_curve, &ToneCurveWidget::editingFinished, this, &AdjustPanel::editingFinished);
    connect(curveReset, &QPushButton::clicked, this, [this] {
        m_settings.curve = EditSettings{}.curve;
        m_curve->setPoints(m_settings.curve);
        emit settingsChanged(m_settings);
        emit editingFinished();
    });
    col->addWidget(curveSection);

    // --- Crop & rotate -----------------------------------------------------------
    auto* cropSection = new CollapsibleSection(tr("Crop & Straighten"));
    auto* c = cropSection->contentLayout();
    auto* hint = new QLabel(tr("Press <b>C</b> or the Crop button to drag the frame."));
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #8D9095;");
    c->addWidget(hint);

    auto* aspectRow = new QHBoxLayout;
    aspectRow->addWidget(new QLabel(tr("Aspect")));
    m_aspect = new QComboBox;
    m_aspect->addItem(tr("Free"), 0.0);
    m_aspect->addItem(tr("Original"), -1.0);
    m_aspect->addItem("1 : 1", 1.0);
    m_aspect->addItem("5 : 4", 5.0 / 4.0);
    m_aspect->addItem("4 : 3", 4.0 / 3.0);
    m_aspect->addItem("3 : 2", 3.0 / 2.0);
    m_aspect->addItem("16 : 9", 16.0 / 9.0);
    aspectRow->addWidget(m_aspect, 1);
    c->addLayout(aspectRow);
    connect(m_aspect, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this] { emit aspectChoiceChanged(aspectChoice()); });

    addSlider(c, tr("Straighten"), -45, 45, 0, 1, &EditSettings::rotation);

    auto* rotRow = new QHBoxLayout;
    auto* rotL = new QPushButton(tr("⟲ 90°"));
    auto* rotR = new QPushButton(tr("⟳ 90°"));
    auto* resetCrop = new QPushButton(tr("Reset"));
    rotRow->addWidget(rotL);
    rotRow->addWidget(rotR);
    rotRow->addWidget(resetCrop);
    c->addLayout(rotRow);
    connect(rotL, &QPushButton::clicked, this, [this] { emit rotateRequested(-1); });
    connect(rotR, &QPushButton::clicked, this, [this] { emit rotateRequested(1); });
    connect(resetCrop, &QPushButton::clicked, this, &AdjustPanel::resetCropRequested);
    col->addWidget(cropSection);

    // --- Detail ----------------------------------------------------------------------
    auto* detail = new CollapsibleSection(tr("Detail"), false);
    addSlider(detail->contentLayout(), tr("Sharpening"), 0, 150, 0, 0, &EditSettings::sharpenAmount);
    addSlider(detail->contentLayout(), tr("Radius"), 0.5, 3.0, 1.0, 1, &EditSettings::sharpenRadius);
    col->addWidget(detail);

    // --- Effects -------------------------------------------------------------------
    auto* effects = new CollapsibleSection(tr("Effects"), false);
    addSlider(effects->contentLayout(), tr("Vignette"), -100, 100, 0, 0, &EditSettings::vignette);
    addSlider(effects->contentLayout(), tr("Midpoint"), 0, 100, 50, 0, &EditSettings::vignetteMidpoint);
    col->addWidget(effects);

    col->addStretch(1);
}

SliderRow* AdjustPanel::addSlider(QVBoxLayout* layout, const QString& label, double min, double max,
                                  double def, int decimals, double EditSettings::*field) {
    auto* row = new SliderRow(label, min, max, def, decimals);
    layout->addWidget(row);
    m_sliders.append({row, field});
    connect(row, &SliderRow::valueChanged, this, [this, field](double v) {
        m_settings.*field = v;
        emit settingsChanged(m_settings);
    });
    connect(row, &SliderRow::editingFinished, this, &AdjustPanel::editingFinished);
    return row;
}

void AdjustPanel::setSettings(const EditSettings& s) {
    m_settings = s;
    for (const auto& [row, field] : m_sliders)
        row->setValue(s.*field);
    m_curve->setPoints(s.curve);
    const QSignalBlocker block(m_mono);
    m_mono->setChecked(s.monochrome);
}

void AdjustPanel::setHistogram(const Histogram& h) {
    m_histogram->setHistogram(h);
    m_curve->setHistogram(h);
}

void AdjustPanel::setInfoCaption(const QString& text) { m_histogram->setCaption(text); }

double AdjustPanel::aspectChoice() const { return m_aspect->currentData().toDouble(); }

} // namespace svitlo
