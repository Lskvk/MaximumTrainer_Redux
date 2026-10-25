/*
 * tst_webengine_safe_mode.cpp
 *
 * Qt Test suite for WebEngineSafeMode (#381): a launch that finds the pending
 * marker still set (the previous launch crashed in QtWebEngine init) turns safe
 * graphics on and remembers it; the environment and --safe-graphics override.
 *
 * Every test works on its own INI file in a temporary directory.
 */

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSettings>

#include "webengine_safe_mode.h"

using namespace WebEngineSafeMode;

static const QString V1 = QStringLiteral("0.0.237");
static const QString V2 = QStringLiteral("0.0.238");

class TstWebEngineSafeMode : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void testCleanStart_off();
    void testInitCompleted_clearsMarker();
    void testCrashDuringInit_enablesAndRemembers();
    void testCrashInSafeMode_reported();
    void testEnvZero_forcesOffAndForgets();
    void testEnvOne_forcesOn();
    void testSwitch_enablesForOneLaunch();
    void testChromiumFlags();
    void testChromiumLog_afterCrashOrOnRequest();
    void testWithChromiumLogging();
    void testUpdate_retriesGpu();
    void testCrashAfterUpdate_remembersNewVersion();

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<QSettings> m_settings;
};

void TstWebEngineSafeMode::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    m_settings = std::make_unique<QSettings>(m_dir->filePath(QStringLiteral("s.ini")),
                                             QSettings::IniFormat);
}

void TstWebEngineSafeMode::testCleanStart_off()
{
    const Decision d = decide(*m_settings, {}, {}, V1);
    QVERIFY(!d.enabled);
    QCOMPARE(d.reason, Reason::Off);
    QVERIFY(!d.crashedInSafeMode);
}

void TstWebEngineSafeMode::testInitCompleted_clearsMarker()
{
    markInitBegin(*m_settings);
    QVERIFY(m_settings->value(QLatin1String(PendingKey)).toBool());
    markInitDone(*m_settings);
    QVERIFY(!m_settings->contains(QLatin1String(PendingKey)));
    QVERIFY(!decide(*m_settings, {}, {}, V1).enabled);
}

void TstWebEngineSafeMode::testCrashDuringInit_enablesAndRemembers()
{
    markInitBegin(*m_settings);           // crash: no markInitDone

    Decision d = decide(*m_settings, {}, {}, V1);
    QVERIFY(d.enabled);
    QCOMPARE(d.reason, Reason::PreviousCrash);
    QVERIFY(!d.crashedInSafeMode);

    markInitBegin(*m_settings);           // this launch starts fine
    markInitDone(*m_settings);

    d = decide(*m_settings, {}, {}, V1);      // and the next one keeps safe graphics
    QVERIFY(d.enabled);
    QCOMPARE(d.reason, Reason::Remembered);
}

void TstWebEngineSafeMode::testCrashInSafeMode_reported()
{
    markInitBegin(*m_settings);
    decide(*m_settings, {}, {}, V1);          // safe graphics on
    markInitBegin(*m_settings);           // crashes again

    const Decision d = decide(*m_settings, {}, {}, V1);
    QVERIFY(d.enabled);
    QVERIFY(d.crashedInSafeMode);
}

void TstWebEngineSafeMode::testEnvZero_forcesOffAndForgets()
{
    markInitBegin(*m_settings);
    decide(*m_settings, {}, {}, V1);
    QVERIFY(m_settings->value(QLatin1String(EnabledKey)).toBool());

    const Decision d = decide(*m_settings, { QString::fromLatin1(Switch) }, "0", V1);
    QVERIFY(!d.enabled);
    QCOMPARE(d.reason, Reason::ForcedOff);
    QVERIFY(!m_settings->contains(QLatin1String(EnabledKey)));
}

void TstWebEngineSafeMode::testEnvOne_forcesOn()
{
    const Decision d = decide(*m_settings, {}, "1", V1);
    QVERIFY(d.enabled);
    QCOMPARE(d.reason, Reason::Environment);
    QVERIFY(!m_settings->contains(QLatin1String(EnabledKey)));
}

void TstWebEngineSafeMode::testSwitch_enablesForOneLaunch()
{
    const Decision d = decide(*m_settings, { QStringLiteral("app"), QStringLiteral("--SAFE-GRAPHICS") }, {}, V1);
    QVERIFY(d.enabled);
    QCOMPARE(d.reason, Reason::CommandLine);
    QVERIFY(!decide(*m_settings, {}, {}, V1).enabled);
}

void TstWebEngineSafeMode::testChromiumFlags()
{
    Decision on;
    on.enabled = true;
    QCOMPARE(chromiumFlags(on, {}), QByteArray(ChromiumFlags));
    QCOMPARE(chromiumFlags(on, " --foo "), QByteArray("--foo ") + ChromiumFlags);
    QCOMPARE(chromiumFlags(on, "--disable-gpu"), QByteArray("--disable-gpu"));
    QCOMPARE(chromiumFlags(Decision(), "--foo"), QByteArray("--foo"));
}

void TstWebEngineSafeMode::testChromiumLog_afterCrashOrOnRequest()
{
    QVERIFY(!wantsChromiumLog(decide(*m_settings, {}, {}, V1), {}));
    QVERIFY(wantsChromiumLog(decide(*m_settings, {}, {}, V1), " 1 "));

    markInitBegin(*m_settings);
    QVERIFY(wantsChromiumLog(decide(*m_settings, {}, {}, V1), {}));      // previous launch crashed
    markInitDone(*m_settings);
    QVERIFY(!wantsChromiumLog(decide(*m_settings, {}, {}, V1), {}));     // remembered, no new crash

    markInitBegin(*m_settings);
    QVERIFY(wantsChromiumLog(decide(*m_settings, {}, {}, V1), {}));      // crashed in safe mode
}

void TstWebEngineSafeMode::testWithChromiumLogging()
{
    QCOMPARE(withChromiumLogging({}), QByteArray(ChromiumLogFlags));
    QCOMPARE(withChromiumLogging(ChromiumFlags), QByteArray(ChromiumFlags) + ' ' + ChromiumLogFlags);
    QCOMPARE(withChromiumLogging("--enable-logging --v=1"), QByteArray("--enable-logging --v=1"));
}

void TstWebEngineSafeMode::testUpdate_retriesGpu()
{
    markInitBegin(*m_settings);
    QVERIFY(decide(*m_settings, {}, {}, V1).enabled);     // crash in V1: safe graphics
    markInitDone(*m_settings);
    QVERIFY(decide(*m_settings, {}, {}, V1).enabled);     // still V1: remembered

    const Decision d = decide(*m_settings, {}, {}, V2);   // updated: GPU again
    QVERIFY(!d.enabled);
    QVERIFY(d.retriedAfterUpdate);
    QCOMPARE(d.reason, Reason::Off);
    QVERIFY(!m_settings->contains(QLatin1String(EnabledKey)));
    QVERIFY(!decide(*m_settings, {}, {}, V2).enabled);
}

void TstWebEngineSafeMode::testCrashAfterUpdate_remembersNewVersion()
{
    markInitBegin(*m_settings);
    decide(*m_settings, {}, {}, V1);
    markInitDone(*m_settings);

    markInitBegin(*m_settings);                           // V2 retries the GPU and crashes
    const Decision d = decide(*m_settings, {}, {}, V2);
    QVERIFY(d.enabled);
    QCOMPARE(d.reason, Reason::PreviousCrash);
    QVERIFY(!d.crashedInSafeMode);
    QCOMPARE(m_settings->value(QLatin1String(EnabledVersionKey)).toString(), V2);
    markInitDone(*m_settings);
    QCOMPARE(decide(*m_settings, {}, {}, V2).reason, Reason::Remembered);
}

QTEST_GUILESS_MAIN(TstWebEngineSafeMode)
#include "tst_webengine_safe_mode.moc"
