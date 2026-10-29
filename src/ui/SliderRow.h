#pragma once

#include <QWidget>

class QLabel;
class QSlider;
class QLineEdit;

namespace svitlo {

// Label + slider + numeric field for one adjustment.
// Double-click the label to reset to the default value.
class SliderRow : public QWidget {
    Q_OBJECT
public:
    SliderRow(const QString& label, double min, double max, double defaultValue, int decimals = 0,
              QWidget* parent = nullptr);

    double value() const { return m_value; }
    void setValue(double v); // no signal

    // Optional coloured track, e.g. blue→yellow for temperature.
    void setTrackGradient(const QColor& from, const QColor& to);

signals:
    void valueChanged(double value);  // live while dragging
    void editingFinished();           // slider released / field committed

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    void applyFromSlider(int pos);
    void commitField();
    void setFromUser(double v);
    void updateField();
    QString format(double v) const;

    QLabel* m_label;
    QSlider* m_slider;
    QLineEdit* m_field;
    double m_min, m_max, m_default, m_value;
    double m_scale;
    int m_decimals;
};

} // namespace svitlo
