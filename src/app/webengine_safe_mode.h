#ifndef WEBENGINE_SAFE_MODE_H
#define WEBENGINE_SAFE_MODE_H

#include <QByteArray>
#include <QString>
#include <QStringList>

class QSettings;

///
/// Recovery for a crash while QtWebEngine starts (#381).  The first
/// QWebEngineProfile call brings up Chromium, including its GPU thread, which
/// runs inside our process: a bad graphics driver or an injected DLL then takes
/// the whole app down before any window appears.
///
/// A pending marker is written to the settings just before that call and
/// removed after it.  If a launch finds the marker still set, the previous
/// launch died there, so this launch (and the following ones) start Chromium
/// with the GPU disabled ("safe graphics").  The choice is remembered so the
/// app does not alternate between crashing and working.
///
/// Overrides: MT_WEBENGINE_SAFE_MODE=1 forces safe graphics on, =0 forces it
/// off (and forgets the remembered choice); the --safe-graphics switch turns it
/// on for one launch.
///
namespace WebEngineSafeMode {

inline constexpr char PendingKey[]   = "startup/webengine_init_pending";
inline constexpr char EnabledKey[]   = "startup/webengine_safe_mode";
inline constexpr char EnvVar[]       = "MT_WEBENGINE_SAFE_MODE";
inline constexpr char Switch[]       = "--safe-graphics";
inline constexpr char ChromiumFlags[] = "--disable-gpu --disable-gpu-compositing";

enum class Reason {
    Off,             // normal start
    PreviousCrash,   // the last launch crashed during WebEngine init
    Remembered,      // an earlier crash turned it on
    CommandLine,     // --safe-graphics
    Environment,     // MT_WEBENGINE_SAFE_MODE=1
    ForcedOff        // MT_WEBENGINE_SAFE_MODE=0
};

struct Decision {
    bool enabled = false;
    Reason reason = Reason::Off;
    bool crashedInSafeMode = false;  // last launch crashed even with the GPU off
};

/// Read the marker and overrides, update the remembered choice in @p settings.
Decision decide(QSettings &settings, const QStringList &args, const QByteArray &envValue);

/// Chromium flags for @p decision appended to @p existingFlags (unchanged when
/// safe graphics is off or the flags are already there).
QByteArray chromiumFlags(const Decision &decision, const QByteArray &existingFlags);

/// decide() on the default settings scope, set QTWEBENGINE_CHROMIUM_FLAGS and
/// log the outcome.  Must run before the first QtWebEngine call.
Decision applyAtStartup(const QStringList &args);

/// Set / clear the pending marker around the QtWebEngine initialisation.
void markInitBegin(QSettings &settings);
void markInitDone(QSettings &settings);

QString reasonName(Reason reason);

} // namespace WebEngineSafeMode

#endif // WEBENGINE_SAFE_MODE_H
