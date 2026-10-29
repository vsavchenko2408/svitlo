#include "ui/LicensesDialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFile>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace svitlo {

namespace {

QTextBrowser* page(const QString& html) {
    auto* b = new QTextBrowser;
    b->setOpenExternalLinks(true);
    b->setHtml(html);
    return b;
}

QString readResource(const QString& path) {
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

} // namespace

LicensesDialog::LicensesDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Licenses"));
    resize(640, 520);

    auto* tabs = new QTabWidget;

    const QString svitlo = readResource(":/LICENSE").toHtmlEscaped();
    tabs->addTab(page(tr("<h3>Svitlo %1</h3><p>Released under the MIT License.</p><pre style='white-space: pre-wrap'>%2</pre>")
                          .arg(QApplication::applicationVersion(), svitlo)),
                 "Svitlo");

    tabs->addTab(page(tr(
        "<h3>Qt %1</h3>"
        "<p>This application uses the Qt framework, © The Qt Company Ltd. and other contributors, "
        "under the <b>GNU Lesser General Public License version 3</b> (LGPL v3).</p>"
        "<p>Qt is linked dynamically: the Qt libraries are separate files inside the application "
        "bundle (<code>Svitlo.app/Contents/Frameworks</code>) and can be replaced with a "
        "compatible, modified version.</p>"
        "<p>Licence text: <a href='https://www.gnu.org/licenses/lgpl-3.0.html'>"
        "gnu.org/licenses/lgpl-3.0.html</a><br>"
        "Qt source code: <a href='https://download.qt.io/archive/qt/'>download.qt.io/archive/qt</a></p>")
                          .arg(QT_VERSION_STR)),
                 "Qt");

    tabs->addTab(page(tr(
        "<h3>LibRaw</h3>"
        "<p>RAW decoding uses LibRaw, © LibRaw LLC. LibRaw is available under a choice of the "
        "<b>GNU LGPL 2.1</b> or the <b>CDDL 1.0</b>; this application uses it under the "
        "<b>CDDL 1.0</b>. The unmodified LibRaw source code is available at "
        "<a href='https://github.com/LibRaw/LibRaw'>github.com/LibRaw/LibRaw</a>.</p>"
        "<p>LibRaw contains code from dcraw by Dave Coffin and DCB/FBDD demosaic code by Jacek "
        "Gozdz (BSD-style licence).</p>"
        "<p>The full licence texts are included in the application bundle in "
        "<code>Svitlo.app/Contents/Resources/licenses</code>.</p>")),
                 "LibRaw");

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);
}

} // namespace svitlo
