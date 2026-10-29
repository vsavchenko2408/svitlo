#pragma once

#include <QColor>

class QApplication;

namespace svitlo::theme {

// Svitlo's own dark theme: neutral greys so colours in photos are judged
// against a neutral surround, with a warm amber accent ("svitlo" = light).
inline const QColor kWindow{0x26, 0x27, 0x29};
inline const QColor kPanel{0x1E, 0x1F, 0x21};
inline const QColor kCanvas{0x15, 0x16, 0x17};
inline const QColor kRaised{0x31, 0x33, 0x36};
inline const QColor kBorder{0x3A, 0x3C, 0x40};
inline const QColor kText{0xDC, 0xDD, 0xDF};
inline const QColor kTextDim{0x8D, 0x90, 0x95};
inline const QColor kAccent{0xE8, 0xA3, 0x3D};
inline const QColor kPick{0x6F, 0xC2, 0x76};
inline const QColor kReject{0xD9, 0x5C, 0x52};

void apply(QApplication& app);

} // namespace svitlo::theme
