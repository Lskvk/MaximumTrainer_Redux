#include <QtTest>
#include <QSet>

#include "overlayzones.h"

// Unit tests for the pure helpers behind the in-game WorkoutOverlay.
class TstOverlay : public QObject
{
    Q_OBJECT

private slots:
    void zoneForFtpFraction_data();
    void zoneForFtpFraction();
    void zoneColors_areDistinctAndValid();
    void targetState_data();
    void targetState();
    void formatDuration_data();
    void formatDuration();
};

void TstOverlay::zoneForFtpFraction_data()
{
    QTest::addColumn<double>("fraction");
    QTest::addColumn<int>("zone");

    QTest::newRow("no target")      << 0.0  << 0;
    QTest::newRow("negative")       << -1.0 << 0;
    QTest::newRow("Z1 easy")        << 0.40 << 1;
    QTest::newRow("Z1 upper edge")  << 0.55 << 1;
    QTest::newRow("Z2 lower")       << 0.56 << 2;
    QTest::newRow("Z2 upper edge")  << 0.75 << 2;
    QTest::newRow("Z3")             << 0.85 << 3;
    QTest::newRow("Z3 upper edge")  << 0.90 << 3;
    QTest::newRow("Z4 threshold")   << 1.00 << 4;
    QTest::newRow("Z4 upper edge")  << 1.05 << 4;
    QTest::newRow("Z5")             << 1.15 << 5;
    QTest::newRow("Z5 upper edge")  << 1.20 << 5;
    QTest::newRow("Z6 anaerobic")   << 1.50 << 6;
}

void TstOverlay::zoneForFtpFraction()
{
    QFETCH(double, fraction);
    QFETCH(int, zone);
    QCOMPARE(OverlayZones::zoneForFtpFraction(fraction), zone);
}

void TstOverlay::zoneColors_areDistinctAndValid()
{
    QSet<QRgb> seen;
    for (int zone = 0; zone <= 6; ++zone) {
        const QColor c = OverlayZones::zoneColor(zone);
        QVERIFY(c.isValid());
        seen.insert(c.rgb());
    }
    QCOMPARE(seen.size(), 7);
    QCOMPARE(OverlayZones::zoneColorForFtpFraction(1.0), OverlayZones::zoneColor(4));
}

void TstOverlay::targetState_data()
{
    QTest::addColumn<int>("power");
    QTest::addColumn<int>("target");
    QTest::addColumn<int>("range");
    QTest::addColumn<int>("state");

    using S = OverlayZones::TargetState;
    QTest::newRow("no power")      << -1  << 200 << 10 << int(S::None);
    QTest::newRow("no target")     << 200 << -1  << 10 << int(S::None);
    QTest::newRow("too low")       << 180 << 200 << 10 << int(S::TooLow);
    QTest::newRow("low edge ok")   << 190 << 200 << 10 << int(S::OnTarget);
    QTest::newRow("on target")     << 200 << 200 << 10 << int(S::OnTarget);
    QTest::newRow("high edge ok")  << 210 << 200 << 10 << int(S::OnTarget);
    QTest::newRow("too high")      << 211 << 200 << 10 << int(S::TooHigh);
    QTest::newRow("zero range")    << 201 << 200 << 0  << int(S::TooHigh);
}

void TstOverlay::targetState()
{
    QFETCH(int, power);
    QFETCH(int, target);
    QFETCH(int, range);
    QFETCH(int, state);
    QCOMPARE(int(OverlayZones::targetState(power, target, range)), state);
}

void TstOverlay::formatDuration_data()
{
    QTest::addColumn<int>("seconds");
    QTest::addColumn<QString>("text");

    QTest::newRow("zero")          << 0     << QStringLiteral("0:00");
    QTest::newRow("seconds")       << 9     << QStringLiteral("0:09");
    QTest::newRow("minutes")       << 135   << QStringLiteral("2:15");
    QTest::newRow("under an hour") << 3599  << QStringLiteral("59:59");
    QTest::newRow("one hour")      << 3600  << QStringLiteral("1:00:00");
    QTest::newRow("hours")         << 4933  << QStringLiteral("1:22:13");
    QTest::newRow("negative")      << -5    << QStringLiteral("0:00");
}

void TstOverlay::formatDuration()
{
    QFETCH(int, seconds);
    QFETCH(QString, text);
    QCOMPARE(OverlayZones::formatDuration(seconds), text);
}

QTEST_MAIN(TstOverlay)
#include "tst_overlay.moc"
