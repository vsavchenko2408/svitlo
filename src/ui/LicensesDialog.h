#pragma once

#include <QDialog>

namespace svitlo {

// Help → Licenses: Svitlo's own licence and notices for Qt and LibRaw.
class LicensesDialog : public QDialog {
    Q_OBJECT
public:
    explicit LicensesDialog(QWidget* parent = nullptr);
};

} // namespace svitlo
