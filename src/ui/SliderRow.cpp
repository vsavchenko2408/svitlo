#include "ui/SliderRow.h"

#include <QDoubleValidator>
#include <QEvent>
#include <QFontMetrics>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QSlider>

#include <algorithm>
#include <cmath>

namespace svitlo {

SliderRow::SliderRow(const QString& label, double min, double max, double defaultValue, int decimals,
                     QWidget* parent)
    : QWidget(parent), m_min(min), m_max(max), m_default(defaultValue), m_value(defaultValue),
      m_scale(std::pow(10.0, decimals)), m_decimals(decimals) {
    m_label = new QLabel(label);
    m_label->setToolTip(tr("Double-click to reset"));
    m_label->installEventFilter(this);

    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(int(std::lround(min * m_scale)), int(std::lround(max * m_scale)));
    m_slider->setValue(int(std::lround(defaultValue * m_scale)));
    m_slider->setSingleStep(1);
    m_slider->setPageStep(int(m_scale * (max - min) / 20.0));
    m_slider->installEventFilter(this);

    // A plain QLineEdit rather than QDoubleSpinBox: the spin box contains an
    // internal line edit that picked up the application-wide QLineEdit
    // padding/border, leaving no room for the text on macOS. Here the widget
    // has its own stylesheet (which wins over the application one) and a size
    // computed from the actual font.
    m_field = new QLineEdit;
    m_field->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_field->setStyleSheet(QStringLiteral(
        "QLineEdit { background: #1E1F21; color: #DCDDDF; border: 1px solid #3A3C40;"
        " border-radius: 4px; padding: 0px 4px; margin: 0px; }"
        "QLineEdit:focus { border-color: #E8A33D; }"));
    auto* validator = new QDoubleValidator(min, max, decimals, m_field);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::c()); // always "." as decimal separator
    m_field->setValidator(validator);
    {
        const QFontMetrics fm(m_field->font());
        const QString widest = QString::number(std::min(min, -max), 'f', decimals);
        m_field->setFixedSize(fm.horizontalAdvance(widest) + fm.horizontalAdvance('0') + 16,
                              fm.height() + 8);
    }
    m_field->installEventFilter(this);
    updateField();

    auto* grid = new QGridLayout(this);
    grid->setContentsMargins(0, 2, 0, 2);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(0);
    grid->addWidget(m_label, 0, 0);
    grid->addWidget(m_field, 0, 1, Qt::AlignRight);
    grid->addWidget(m_slider, 1, 0, 1, 2);

    connect(m_slider, &QSlider::valueChanged, this, &SliderRow::applyFromSlider);
    connect(m_slider, &QSlider::sliderReleased, this, &SliderRow::editingFinished);
    connect(m_field, &QLineEdit::editingFinished, this, &SliderRow::commitField);
}

QString SliderRow::format(double v) const {
    // Avoid "-0" for values that round to zero.
    if (std::abs(v) < 0.5 / m_scale)
        v = 0.0;
    return QString::number(v, 'f', m_decimals);
}

void SliderRow::updateField() {
    const QSignalBlocker b(m_field);
    m_field->setText(format(m_value));
    m_field->setCursorPosition(0);
}

void SliderRow::commitField() {
    bool ok = false;
    double v = QLocale::c().toDouble(m_field->text().trimmed().replace(',', '.'), &ok);
    if (!ok) {
        updateField(); // restore the current value
        return;
    }
    setFromUser(v);
}

void SliderRow::setFromUser(double v) {
    v = std::clamp(std::round(v * m_scale) / m_scale, m_min, m_max);
    const bool changed = std::abs(v - m_value) >= 1e-9;
    m_value = v;
    {
        const QSignalBlocker b(m_slider);
        m_slider->setValue(int(std::lround(v * m_scale)));
    }
    updateField();
    if (changed) {
        emit valueChanged(m_value);
        emit editingFinished();
    }
}

void SliderRow::applyFromSlider(int pos) {
    const double v = pos / m_scale;
    if (std::abs(v - m_value) < 1e-9)
        return;
    m_value = v;
    updateField();
    emit valueChanged(m_value);
    if (!m_slider->isSliderDown())
        emit editingFinished(); // keyboard / click-on-track changes
}

void SliderRow::setValue(double v) {
    m_value = std::clamp(v, m_min, m_max);
    const QSignalBlocker b(m_slider);
    m_slider->setValue(int(std::lround(m_value * m_scale)));
    updateField();
}

void SliderRow::setTrackGradient(const QColor& from, const QColor& to) {
    m_slider->setStyleSheet(QStringLiteral(
        "QSlider::groove:horizontal { height: 4px; border-radius: 2px;"
        " background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %1, stop:1 %2); }")
                                .arg(from.name(), to.name()));
}

bool SliderRow::eventFilter(QObject* obj, QEvent* ev) {
    if (ev->type() == QEvent::MouseButtonDblClick && (obj == m_label || obj == m_slider)) {
        setFromUser(m_default);
        return true;
    }
    // Up/Down arrows in the field nudge the value (Shift = 10x).
    if (obj == m_field && ev->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(ev);
        if (key->key() == Qt::Key_Up || key->key() == Qt::Key_Down) {
            const double step = (key->modifiers() & Qt::ShiftModifier ? 10.0 : 1.0) / m_scale;
            setFromUser(m_value + (key->key() == Qt::Key_Up ? step : -step));
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}

} // namespace svitlo
