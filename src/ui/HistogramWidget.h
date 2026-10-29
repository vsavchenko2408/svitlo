#pragma once

#include "core/Histogram.h"

#include <QWidget>

namespace svitlo {

// RGB histogram with additive channel overlap and clipping indicators.
class HistogramWidget : public QWidget {
    Q_OBJECT
public:
    explicit HistogramWidget(QWidget* parent = nullptr);

    void setHistogram(const Histogram& h);
    void setCaption(const QString& text);

    QSize sizeHint() const override { return {260, 120}; }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    Histogram m_hist;
    QString m_caption;
};

} // namespace svitlo
