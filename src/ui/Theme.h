#pragma once

#include <QColor>

class QApplication;

namespace svitlo::theme {

// Svitlo has a dark and a light look. Both use neutral greys so colours in
// photos are judged against a neutral surround, with a warm amber accent
// ("svitlo" = light). "System" follows the operating system and switches
// live when the system appearance changes.
enum class Mode { System, Dark, Light };

struct Colors {
    bool dark = true;
    QColor window;   // main background
    QColor panel;    // side panels, input fields
    QColor canvas;   // behind photos, histogram, curve
    QColor raised;   // buttons, hovered cells
    QColor border;
    QColor strong;   // slider grooves, scroll bar handles
    QColor hover;    // border of hovered buttons
    QColor text;
    QColor textDim;
    QColor accent;
    QColor onAccent; // text drawn on the accent colour
    QColor pick;
    QColor reject;
};

// Colours of the look currently in use. Custom-painted widgets read them in
// paintEvent(), so they follow a theme switch without restarting.
const Colors& colors();

Mode savedMode();
void saveMode(Mode mode);

// Applies the saved mode. Call once after QApplication is created; later
// calls (e.g. after saveMode) switch the look of all open windows.
void apply(QApplication& app);

} // namespace svitlo::theme
