#ifndef WORKOUTOVERLAY_H
#define WORKOUTOVERLAY_H

#include <QWidget>
#include <QTime>
#include <QVector>
#include <QPoint>

class Workout;
class QPainter;
class QTimer;

/// Small translucent always-on-top summary of the running workout, meant to
/// sit over a (borderless-windowed) game. It must be parented to the
/// WorkoutDialog: that dialog is application-modal, so a parentless window
/// would not receive mouse input.
class WorkoutOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit WorkoutOverlay(QWidget *parent);
    ~WorkoutOverlay() override;

    void setWorkout(const Workout &workout);
    void setUseMiles(bool useMiles);

    void setHr(int bpm);
    void setPower(int watts);
    void setCadence(int rpm);
    void setSpeedKmh(double kmh);
    void setTarget(int watts, int range);

    void setTimes(const QTime &elapsed, const QTime &interval, const QTime &remaining);
    void setProgress(double workoutSec);
    /// Short badge text ("PAUSED", "READY", …); empty while riding.
    void setStatusText(const QString &text);

    /// Toggle visibility and remember the choice for the next workout.
    void toggle();
    /// Show the overlay if the user left it enabled last time.
    void restoreVisibility();
    void saveSettings() const;
    /// Called when the workout window closes: save, release the hotkey, hide.
    void shutdown();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    struct Block {
        double startSec;
        double durationSec;
        double ftpStart;   // fraction of FTP, <= 0 = no target
        double ftpEnd;
    };

    void loadSettings();
    void applyScale();
    void keepOnScreen();
    void registerHotkey();
    void unregisterHotkey();
    void paintTimeline(QPainter &p, const QRectF &rect) const;

    QVector<Block> m_blocks;
    double m_totalSec = 0;
    double m_progressSec = 0;
    bool m_openRide = false;
    bool m_useMiles = false;

    int m_hr = -1;
    int m_power = -1;
    int m_cadence = -1;
    double m_speedKmh = -1;
    int m_target = -1;
    int m_targetRange = 0;

    QTime m_elapsed = QTime(0, 0);
    QTime m_interval = QTime(0, 0);
    QTime m_remaining = QTime(0, 0);
    QString m_status;

    int m_opacityPercent = 60;
    double m_scale = 1.0;
    bool m_enabled = true;

    bool m_dragging = false;
    QPoint m_dragOffset;

    bool m_hotkeyRegistered = false;
    QTimer *m_topmostTimer = nullptr;
};

#endif // WORKOUTOVERLAY_H
