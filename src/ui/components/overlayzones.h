#ifndef OVERLAYZONES_H
#define OVERLAYZONES_H

#include <QColor>
#include <QString>

/// Pure helpers for WorkoutOverlay. Kept free of Account/Workout so they can be
/// unit-tested (tests/overlay).
namespace OverlayZones {

/// Coggan power zone for a target expressed as a fraction of FTP (0.75 = 75 %).
/// Returns 0 when there is no target.
inline int zoneForFtpFraction(double fraction)
{
    // Small epsilon: workout files store e.g. 0.55 as 0.5500000001.
    const double f = fraction - 1e-6;
    if (fraction <= 0) return 0;
    if (f <= 0.55) return 1;
    if (f <= 0.75) return 2;
    if (f <= 0.90) return 3;
    if (f <= 1.05) return 4;
    if (f <= 1.20) return 5;
    return 6;
}

inline QColor zoneColor(int zone)
{
    switch (zone) {
    case 1:  return QColor(128, 128, 128);   // recovery
    case 2:  return QColor(51, 140, 255);    // endurance
    case 3:  return QColor(89, 191, 89);     // tempo
    case 4:  return QColor(255, 204, 63);    // threshold
    case 5:  return QColor(255, 102, 57);    // VO2max
    case 6:  return QColor(230, 40, 40);     // anaerobic
    default: return QColor(70, 70, 70);      // no target
    }
}

inline QColor zoneColorForFtpFraction(double fraction)
{
    return zoneColor(zoneForFtpFraction(fraction));
}

enum class TargetState { None, TooLow, OnTarget, TooHigh };

/// Same rule as the workout window's power box: on target within ± range.
inline TargetState targetState(int power, int target, int range)
{
    if (power < 0 || target <= 0) return TargetState::None;
    if (power < target - range) return TargetState::TooLow;
    if (power > target + range) return TargetState::TooHigh;
    return TargetState::OnTarget;
}

/// Bright variants readable as text on the dark overlay (Util::getColor's
/// TOO_LOW / TOO_HIGH are frame backgrounds and too dark for that).
inline QColor targetColor(TargetState state)
{
    switch (state) {
    case TargetState::TooLow:   return QColor(90, 170, 255);
    case TargetState::OnTarget: return QColor(76, 217, 100);
    case TargetState::TooHigh:  return QColor(255, 90, 80);
    default:                    return QColor(255, 255, 255);
    }
}

/// "m:ss" below one hour, "h:mm:ss" from one hour up.
inline QString formatDuration(int totalSec)
{
    if (totalSec < 0) totalSec = 0;
    const int h = totalSec / 3600;
    const int m = (totalSec % 3600) / 60;
    const int s = totalSec % 60;
    if (h > 0)
        return QStringLiteral("%1:%2:%3").arg(h).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QLatin1Char('0'));
}

} // namespace OverlayZones

#endif // OVERLAYZONES_H
