// macOS only: forces the window frames, title bars and native dialogs to the
// dark or light appearance chosen in Svitlo (or lets them follow the system).
// Qt 6.5 has no API for this; Qt 6.8+ has QStyleHints::setColorScheme.

#import <AppKit/AppKit.h>

namespace svitlo { namespace theme { namespace detail {

// 0 = follow the system, 1 = dark, 2 = light.
void setMacAppearance(int mode) {
    if (NSApp == nil)
        return;
    NSAppearance* appearance = nil; // nil = inherit from the system
    if (mode == 1)
        appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    else if (mode == 2)
        appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    [NSApp setAppearance:appearance];
}

}}} // namespace svitlo::theme::detail
