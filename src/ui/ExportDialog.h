#pragma once

#include "core/Exporter.h"

#include <QDialog>

class QComboBox;
class QSpinBox;
class QLineEdit;
class QCheckBox;

namespace svitlo {

class ExportDialog : public QDialog {
    Q_OBJECT
public:
    ExportDialog(int photoCount, QWidget* parent = nullptr);

    ExportOptions options() const;

private:
    void load();
    void save() const;

    QComboBox* m_format;
    QSpinBox* m_quality;
    QCheckBox* m_resize;
    QSpinBox* m_longEdge;
    QLineEdit* m_folder;
    QLineEdit* m_suffix;
};

} // namespace svitlo
