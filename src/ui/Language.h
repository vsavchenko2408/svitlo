#pragma once

#include <QString>
#include <QVector>

class QApplication;

namespace svitlo::language {

struct Info {
    QString code;        // ISO 639-1, e.g. "uk"
    QString nativeName;  // shown in the menu in its own language
};

// Languages the interface is translated into. English is the source language.
const QVector<Info>& supported();

// The user's choice from settings: "" means "follow the system".
QString savedChoice();
void saveChoice(const QString& code);

// Picks the language to use: the saved choice if valid, otherwise the first
// supported language in the system's preferred list, otherwise English.
QString resolve();

// Loads Svitlo's and Qt's own translations for `code` into the application.
// Must be called before any window is created.
void install(QApplication& app, const QString& code);

} // namespace svitlo::language
