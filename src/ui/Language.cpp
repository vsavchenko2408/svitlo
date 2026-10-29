#include "ui/Language.h"

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

namespace svitlo::language {

namespace {
const char* kKey = "ui/language";
}

const QVector<Info>& supported() {
    static const QVector<Info> list = {
        {"en", QStringLiteral("English")},
        {"uk", QStringLiteral("Українська")},
        {"da", QStringLiteral("Dansk")},
        {"de", QStringLiteral("Deutsch")},
        {"pl", QStringLiteral("Polski")},
    };
    return list;
}

static bool isSupported(const QString& code) {
    for (const Info& i : supported())
        if (i.code == code)
            return true;
    return false;
}

QString savedChoice() { return QSettings().value(kKey).toString(); }

void saveChoice(const QString& code) { QSettings().setValue(kKey, code); }

QString resolve() {
    const QString saved = savedChoice();
    if (isSupported(saved))
        return saved;

    // e.g. {"da-DK", "uk-UA", "en-US"} — take the first one we support.
    // Languages we do not translate into fall through to English.
    for (const QString& tag : QLocale::system().uiLanguages()) {
        const QString code = tag.section('-', 0, 0).section('_', 0, 0).toLower();
        if (isSupported(code))
            return code;
    }
    return QStringLiteral("en");
}

void install(QApplication& app, const QString& code) {
    if (code == QLatin1String("en"))
        return;

    // Svitlo's own strings, compiled into the app under :/i18n.
    auto* appTr = new QTranslator(&app);
    if (appTr->load(QStringLiteral(":/i18n/svitlo_%1.qm").arg(code)))
        QApplication::installTranslator(appTr);

    // Qt's standard strings (OK/Cancel buttons, file dialogs, …). Bundled
    // copies under :/qt_i18n first, then the Qt installation as a fallback.
    auto* qtTr = new QTranslator(&app);
    const QString name = QStringLiteral("qtbase_%1").arg(code);
    if (qtTr->load(name, QStringLiteral(":/qt_i18n")) ||
        qtTr->load(name, QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QApplication::installTranslator(qtTr);
}

} // namespace svitlo::language
