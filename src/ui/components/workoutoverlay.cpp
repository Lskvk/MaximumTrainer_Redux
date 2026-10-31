#include "workoutoverlay.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QScreen>
#include <QSettings>
#include <QTimer>

#include "workout.h"
#include "interval.h"
#include "util.h"
#include "myconstants.h"
#include "overlayzones.h"
#include "logger.h"

#ifdef Q_OS_WIN
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

namespace {

constexpr int kBaseWidth = 420;
constexpr int kBaseHeight = 170;
constexpr int kMinBaseWidth = 280;
constexpr int kResizeGrip = 10;   // px at the right edge that resize instead of move
const char *kSettingsGroup = "WorkoutOverlay";
#ifdef Q_OS_WIN
constexpr int kToggleHotkeyId = 0x4F56;
#endif

int secondsOf(const QTime &t)
{
    return QTime(0, 0).secsTo(t);
}

} // namespace


WorkoutOverlay::WorkoutOverlay(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowTitle(tr("Workout overlay"));
    setToolTip(tr("Drag to move · drag the right edge to resize · right-click for options"));
    setMouseTracking(true);   // resize cursor over the right edge

    loadSettings();
    registerHotkey();

#ifdef Q_OS_WIN
    // Some games re-assert their own z-order; re-pin ours without ever
    // activating it (QWidget::raise() could steal focus from the game).
    m_topmostTimer = new QTimer(this);
    m_topmostTimer->setInterval(2000);
    connect(m_topmostTimer, &QTimer::timeout, this, [this] {
        if (isVisible())
            SetWindowPos(reinterpret_cast<HWND>(winId()), HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    });
    m_topmostTimer->start();
#endif
}

WorkoutOverlay::~WorkoutOverlay()
{
    unregisterHotkey();
}


void WorkoutOverlay::setWorkout(const Workout &workout)
{
    m_openRide = workout.getWorkoutNameEnum() == Workout::OPEN_RIDE;
    m_blocks.clear();
    m_totalSec = 0;
    if (!m_openRide) {
        for (const Interval &interval : workout.getLstInterval()) {
            const double duration = Util::convertQTimeToSecD(interval.getDurationQTime());
            m_blocks.append(Block{m_totalSec, duration, interval.getFTP_start(), interval.getFTP_end()});
            m_totalSec += duration;
        }
    }
    update();
}

void WorkoutOverlay::setUseMiles(bool useMiles)
{
    m_useMiles = useMiles;
    update();
}

void WorkoutOverlay::setHr(int bpm)
{
    m_hr = bpm;
    update();
}

void WorkoutOverlay::setPower(int watts)
{
    m_power = watts;
    update();
}

void WorkoutOverlay::setCadence(int rpm)
{
    m_cadence = (rpm > 250) ? -1 : rpm;
    update();
}

void WorkoutOverlay::setSpeedKmh(double kmh)
{
    m_speedKmh = kmh;
    update();
}

void WorkoutOverlay::setTarget(int watts, int range)
{
    m_target = watts;
    m_targetRange = range;
    update();
}

void WorkoutOverlay::setTimes(const QTime &elapsed, const QTime &interval, const QTime &remaining)
{
    m_elapsed = elapsed;
    m_interval = interval;
    m_remaining = remaining;
    update();
}

void WorkoutOverlay::setProgress(double workoutSec)
{
    m_progressSec = workoutSec;
    update();
}

void WorkoutOverlay::setStatusText(const QString &text)
{
    m_status = text;
    update();
}


void WorkoutOverlay::toggle()
{
    m_enabled = !isVisible();
    setVisible(m_enabled);
    saveSettings();
}

void WorkoutOverlay::restoreVisibility()
{
    setVisible(m_enabled);
}

void WorkoutOverlay::shutdown()
{
    saveSettings();
    unregisterHotkey();
    if (m_topmostTimer)
        m_topmostTimer->stop();
    hide();
}


void WorkoutOverlay::loadSettings()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    m_opacityPercent = qBound(10, settings.value("opacity", 60).toInt(), 100);
    m_scale = qBound(0.6, settings.value("scale", 1.0).toDouble(), 2.0);
    m_baseWidth = qMax(kMinBaseWidth, settings.value("baseWidth", kBaseWidth).toInt());
    m_enabled = settings.value("enabled", true).toBool();
    const bool hasPos = settings.contains("pos");
    const QPoint savedPos = settings.value("pos").toPoint();
    settings.endGroup();

    applyScale();
    if (hasPos)
        move(savedPos);
    else if (QScreen *screen = QGuiApplication::primaryScreen())
        move(screen->availableGeometry().topRight() + QPoint(-width() - 24, 24));
    keepOnScreen();
}

void WorkoutOverlay::saveSettings() const
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue("opacity", m_opacityPercent);
    settings.setValue("scale", m_scale);
    settings.setValue("baseWidth", m_baseWidth);
    settings.setValue("enabled", m_enabled);
    settings.setValue("pos", pos());
    settings.endGroup();
}

void WorkoutOverlay::applyScale()
{
    setFixedSize(qRound(m_baseWidth * m_scale), qRound(kBaseHeight * m_scale));
    update();
}

// A saved position can point at a monitor that is no longer connected, and a
// wider/larger overlay can spill past the edge of the one it is on.
void WorkoutOverlay::keepOnScreen()
{
    QScreen *screen = QGuiApplication::screenAt(frameGeometry().center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    const QRect avail = screen->availableGeometry();
    QRect r = frameGeometry();
    if (!avail.intersects(r))
        r.moveTopLeft(avail.topRight() + QPoint(-r.width() - 24, 24));
    r.moveLeft(qBound(avail.left(), r.left(), qMax(avail.left(), avail.right() - r.width() + 1)));
    r.moveTop(qBound(avail.top(), r.top(), qMax(avail.top(), avail.bottom() - r.height() + 1)));
    move(r.topLeft());
}

bool WorkoutOverlay::inResizeGrip(const QPoint &pos) const
{
    return pos.x() >= width() - qRound(kResizeGrip * m_scale);
}


void WorkoutOverlay::registerHotkey()
{
#ifdef Q_OS_WIN
    m_hotkeyRegistered = RegisterHotKey(reinterpret_cast<HWND>(winId()), kToggleHotkeyId,
                                        MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'O') != FALSE;
    if (!m_hotkeyRegistered)
        LOG_WARN("WorkoutOverlay", QStringLiteral("Ctrl+Alt+O is taken by another app; use F9 in the workout window instead"));
#endif
}

void WorkoutOverlay::unregisterHotkey()
{
#ifdef Q_OS_WIN
    if (m_hotkeyRegistered)
        UnregisterHotKey(reinterpret_cast<HWND>(winId()), kToggleHotkeyId);
#endif
    m_hotkeyRegistered = false;
}

bool WorkoutOverlay::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        const MSG *msg = static_cast<const MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == static_cast<WPARAM>(kToggleHotkeyId)) {
            toggle();
            if (result)
                *result = 0;
            return true;
        }
    }
#endif
    return QWidget::nativeEvent(eventType, message, result);
}


void WorkoutOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (inResizeGrip(event->position().toPoint())) {
            m_resizing = true;
            m_resizeStartX = event->globalPosition().toPoint().x();
            m_resizeStartWidth = width();
        } else {
            m_dragging = true;
            m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void WorkoutOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (m_resizing && (event->buttons() & Qt::LeftButton)) {
        const int newWidth = m_resizeStartWidth + event->globalPosition().toPoint().x() - m_resizeStartX;
        m_baseWidth = qMax(kMinBaseWidth, qRound(newWidth / m_scale));
        applyScale();
        event->accept();
        return;
    }
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }
    setCursor(inResizeGrip(event->position().toPoint()) ? Qt::SizeHorCursor : Qt::ArrowCursor);
    QWidget::mouseMoveEvent(event);
}

void WorkoutOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if ((m_dragging || m_resizing) && event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_resizing = false;
        keepOnScreen();
        saveSettings();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void WorkoutOverlay::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QMenu *opacityMenu = menu.addMenu(tr("Background opacity"));
    for (int pct : {25, 40, 60, 80, 95}) {
        QAction *action = opacityMenu->addAction(QStringLiteral("%1 %").arg(pct));
        action->setCheckable(true);
        action->setChecked(pct == m_opacityPercent);
        connect(action, &QAction::triggered, this, [this, pct] {
            m_opacityPercent = pct;
            update();
            saveSettings();
        });
    }

    QMenu *sizeMenu = menu.addMenu(tr("Size"));
    const QList<QPair<QString, double>> sizes = {
        {tr("Small"), 0.8}, {tr("Medium"), 1.0}, {tr("Large"), 1.3}, {tr("Extra large"), 1.6}};
    for (const auto &size : sizes) {
        QAction *action = sizeMenu->addAction(size.first);
        action->setCheckable(true);
        action->setChecked(qFuzzyCompare(size.second, m_scale));
        const double scale = size.second;
        connect(action, &QAction::triggered, this, [this, scale] {
            m_scale = scale;
            applyScale();
            keepOnScreen();
            saveSettings();
        });
    }

    QMenu *widthMenu = menu.addMenu(tr("Width (or drag the right edge)"));
    const QList<QPair<QString, int>> widths = {
        {tr("Narrow"), 320}, {tr("Normal"), kBaseWidth}, {tr("Wide"), 560}, {tr("Extra wide"), 720}};
    for (const auto &preset : widths) {
        QAction *action = widthMenu->addAction(preset.first);
        action->setCheckable(true);
        action->setChecked(preset.second == m_baseWidth);
        const int baseWidth = preset.second;
        connect(action, &QAction::triggered, this, [this, baseWidth] {
            m_baseWidth = baseWidth;
            applyScale();
            keepOnScreen();
            saveSettings();
        });
    }

    menu.addSeparator();
    QAction *hideAction = menu.addAction(tr("Hide  (F9 / Ctrl+Alt+O)"));
    connect(hideAction, &QAction::triggered, this, &WorkoutOverlay::toggle);

    // The menus must stay above the overlay itself: mark them topmost and pause
    // the periodic re-pin, which would otherwise push the overlay over them.
    for (QMenu *m : {&menu, opacityMenu, sizeMenu, widthMenu})
        m->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    if (m_topmostTimer)
        m_topmostTimer->stop();
    menu.exec(event->globalPos());
    if (m_topmostTimer)
        m_topmostTimer->start();
}


void WorkoutOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const double s = m_scale;
    const QColor textColor(255, 255, 255);
    const QColor captionColor(170, 176, 186);

    p.setPen(QPen(QColor(255, 255, 255, 45), 1));
    p.setBrush(QColor(15, 17, 21, m_opacityPercent * 255 / 100));
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10 * s, 10 * s);

    // Resize grip hint on the right edge.
    p.setPen(QPen(QColor(255, 255, 255, 70), qMax(1.0, 1.5 * s)));
    for (int i = 0; i < 2; ++i) {
        const double gx = width() - (4 + 3 * i) * s;
        p.drawLine(QPointF(gx, height() / 2.0 - 8 * s), QPointF(gx, height() / 2.0 + 8 * s));
    }

    const QFont baseFont = font();
    auto useFont = [&](double px, bool bold) {
        QFont f = baseFont;
        f.setPixelSize(qMax(1, qRound(px * s)));
        f.setBold(bold);
        p.setFont(f);
    };
    auto textWidth = [&](const QString &text) {
        return QFontMetricsF(p.font()).horizontalAdvance(text);
    };
    // Value in a big font followed by its unit in a small caption font.
    auto drawValueUnit = [&](const QRectF &row, const QString &value, const QString &unit,
                             double valuePx, double unitPx, const QColor &valueColor, bool alignRight) {
        useFont(valuePx, true);
        const double valueW = textWidth(value);
        useFont(unitPx, false);
        const double unitW = textWidth(unit);
        const double gap = 4 * s;
        const double x = alignRight ? row.right() - valueW - gap - unitW : row.left();

        useFont(valuePx, true);
        p.setPen(valueColor);
        p.drawText(QRectF(x, row.top(), valueW + 1, row.height()), Qt::AlignLeft | Qt::AlignVCenter, value);
        useFont(unitPx, false);
        p.setPen(captionColor);
        p.drawText(QRectF(x + valueW + gap, row.top(), unitW + 1, row.height() - 3 * s),
                   Qt::AlignLeft | Qt::AlignBottom, unit);
    };

    const double pad = 12 * s;
    const double w = width() - 2 * pad;
    double y = pad;
    const QString dash = QStringLiteral("–");

    // Row 1: elapsed · interval left (or status badge) · total left
    const QRectF row1(pad, y, w, 20 * s);
    useFont(17, true);
    p.setPen(textColor);
    p.drawText(row1, Qt::AlignLeft | Qt::AlignVCenter, OverlayZones::formatDuration(secondsOf(m_elapsed)));
    if (!m_openRide) {
        p.setPen(captionColor);
        p.drawText(row1, Qt::AlignRight | Qt::AlignVCenter,
                   QStringLiteral("-%1").arg(OverlayZones::formatDuration(secondsOf(m_remaining))));
    }
    if (!m_status.isEmpty()) {
        useFont(13, true);
        const double badgeW = textWidth(m_status) + 14 * s;
        const QRectF badge(row1.center().x() - badgeW / 2, row1.top(), badgeW, row1.height());
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 159, 10));
        p.drawRoundedRect(badge, badge.height() / 2, badge.height() / 2);
        p.setPen(QColor(20, 20, 20));
        p.drawText(badge, Qt::AlignCenter, m_status);
    } else if (!m_openRide) {
        p.setPen(textColor);
        p.drawText(row1, Qt::AlignHCenter | Qt::AlignVCenter,
                   QStringLiteral("▸ %1").arg(OverlayZones::formatDuration(secondsOf(m_interval))));
    }
    y += row1.height() + 6 * s;

    // Row 2: power (coloured against target) · heart rate
    const QRectF row2(pad, y, w, 42 * s);
    const bool hasTarget = !m_openRide && m_target > 0;
    const OverlayZones::TargetState state =
        hasTarget ? OverlayZones::targetState(m_power, m_target, m_targetRange) : OverlayZones::TargetState::None;
    drawValueUnit(row2, m_power >= 0 ? QString::number(m_power) : dash,
                  hasTarget ? tr("W / %1").arg(m_target) : tr("W"),
                  36, 14, OverlayZones::targetColor(state), false);

    useFont(28, true);
    const QString hrText = m_hr >= 0 ? QString::number(m_hr) : dash;
    const double hrW = textWidth(hrText);
    p.setPen(textColor);
    p.drawText(row2, Qt::AlignRight | Qt::AlignVCenter, hrText);
    useFont(20, false);
    const QString heart = QStringLiteral("♥");
    p.setPen(QColor(255, 69, 58));
    p.drawText(QRectF(row2.left(), row2.top(), w - hrW - 5 * s, row2.height()),
               Qt::AlignRight | Qt::AlignVCenter, heart);
    y += row2.height() + 4 * s;

    // Row 3: cadence · speed
    const QRectF row3(pad, y, w, 24 * s);
    drawValueUnit(row3, m_cadence >= 0 ? QString::number(m_cadence) : dash, tr("rpm"),
                  19, 13, textColor, false);
    QString speedText = dash;
    if (m_speedKmh >= 0) {
        const double speed = m_useMiles ? m_speedKmh * constants::GLOBAL_CONST_CONVERT_KMH_TO_MILES : m_speedKmh;
        speedText = QString::number(speed, 'f', 1);
    }
    drawValueUnit(row3, speedText, m_useMiles ? tr("mph") : tr("km/h"), 19, 13, textColor, true);
    y += row3.height() + 8 * s;

    // Row 4: program timeline
    const QRectF row4(pad, y, w, height() - y - pad);
    if (m_blocks.isEmpty() || m_totalSec <= 0) {
        useFont(12, false);
        p.setPen(captionColor);
        p.drawText(row4, Qt::AlignCenter, tr("Free ride"));
    } else {
        paintTimeline(p, row4);
    }
}

void WorkoutOverlay::paintTimeline(QPainter &p, const QRectF &rect) const
{
    const double s = m_scale;
    p.save();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 25));
    p.drawRoundedRect(rect, 3 * s, 3 * s);

    double maxFraction = 1.2;
    for (const Block &b : m_blocks)
        maxFraction = qMax(maxFraction, qMax(b.ftpStart, b.ftpEnd));
    maxFraction = qMin(maxFraction, 1.6);

    auto yFor = [&](double fraction) {
        const double h = fraction <= 0 ? 0.08 : qMin(fraction, maxFraction) / maxFraction;
        return rect.bottom() - h * rect.height();
    };
    auto xFor = [&](double sec) {
        return rect.left() + qBound(0.0, sec / m_totalSec, 1.0) * rect.width();
    };

    p.setPen(QPen(QColor(0, 0, 0, 90), 1));
    for (const Block &b : m_blocks) {
        const double x0 = xFor(b.startSec);
        const double x1 = xFor(b.startSec + b.durationSec);
        const QPolygonF poly({QPointF(x0, rect.bottom()), QPointF(x0, yFor(b.ftpStart)),
                              QPointF(x1, yFor(b.ftpEnd)), QPointF(x1, rect.bottom())});
        p.setBrush(OverlayZones::zoneColorForFtpFraction((b.ftpStart + b.ftpEnd) / 2));
        p.drawPolygon(poly);
    }

    const double cursorX = xFor(m_progressSec);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 130));
    p.drawRect(QRectF(rect.left(), rect.top(), cursorX - rect.left(), rect.height()));

    p.setPen(QPen(Qt::white, qMax(1.0, 2 * s)));
    p.drawLine(QPointF(cursorX, rect.top() - 2 * s), QPointF(cursorX, rect.bottom()));

    p.restore();
}
