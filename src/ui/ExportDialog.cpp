#include "ui/ExportDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <algorithm>

namespace svitlo {

ExportDialog::ExportDialog(int photoCount, QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Export"));
    setMinimumWidth(460);

    m_format = new QComboBox;
    for (const QString& f : Exporter::availableFormats()) {
        const QString label = f == "jpg" ? tr("JPEG") : f == "png" ? tr("PNG") : tr("TIFF");
        m_format->addItem(label, f);
    }

    m_quality = new QSpinBox;
    m_quality->setRange(1, 100);
    m_quality->setSuffix(" %");

    m_resize = new QCheckBox(tr("Resize to long edge"));
    m_longEdge = new QSpinBox;
    m_longEdge->setRange(256, 20000);
    m_longEdge->setSuffix(" px");
    auto* resizeRow = new QHBoxLayout;
    resizeRow->addWidget(m_resize);
    resizeRow->addWidget(m_longEdge);

    m_folder = new QLineEdit;
    auto* browse = new QPushButton(tr("Choose…"));
    auto* folderRow = new QHBoxLayout;
    folderRow->addWidget(m_folder, 1);
    folderRow->addWidget(browse);

    m_suffix = new QLineEdit;
    m_suffix->setPlaceholderText(tr("e.g. _edit"));

    auto* form = new QFormLayout;
    form->addRow(tr("Format"), m_format);
    form->addRow(tr("JPEG quality"), m_quality);
    form->addRow(tr("Size"), resizeRow);
    form->addRow(tr("Folder"), folderRow);
    form->addRow(tr("File name suffix"), m_suffix);

    auto* info = new QLabel(tr("Exporting %n photo(s). Originals are not modified.", "", photoCount));
    info->setStyleSheet("color: #8D9095;");

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Export"));

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(info);
    layout->addWidget(buttons);

    connect(browse, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Export to"), m_folder->text());
        if (!dir.isEmpty())
            m_folder->setText(dir);
    });
    connect(m_resize, &QCheckBox::toggled, m_longEdge, &QWidget::setEnabled);
    connect(m_format, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this] { m_quality->setEnabled(m_format->currentData().toString() == "jpg"); });
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        save();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    load();
}

void ExportDialog::load() {
    QSettings s;
    const int idx = m_format->findData(s.value("export/format", "jpg").toString());
    m_format->setCurrentIndex(std::max(0, idx));
    m_quality->setValue(s.value("export/quality", 92).toInt());
    m_quality->setEnabled(m_format->currentData().toString() == "jpg");
    const int edge = s.value("export/longEdge", 0).toInt();
    m_resize->setChecked(edge > 0);
    m_longEdge->setValue(edge > 0 ? edge : 2048);
    m_longEdge->setEnabled(edge > 0);
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    m_folder->setText(s.value("export/folder", pictures + "/Svitlo Export").toString());
    m_suffix->setText(s.value("export/suffix", "").toString());
}

void ExportDialog::save() const {
    QSettings s;
    s.setValue("export/format", m_format->currentData().toString());
    s.setValue("export/quality", m_quality->value());
    s.setValue("export/longEdge", m_resize->isChecked() ? m_longEdge->value() : 0);
    s.setValue("export/folder", m_folder->text());
    s.setValue("export/suffix", m_suffix->text());
}

ExportOptions ExportDialog::options() const {
    ExportOptions o;
    o.format = m_format->currentData().toString();
    o.quality = m_quality->value();
    o.longEdge = m_resize->isChecked() ? m_longEdge->value() : 0;
    o.outputDir = m_folder->text();
    o.fileSuffix = m_suffix->text();
    return o;
}

} // namespace svitlo
