#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QtCore>


class Settings : public QObject
{
    Q_OBJECT

public:
    ~Settings();
    Settings(QObject *parent = 0);
    void saveGeneralSettings();


    /// --------------- Program Settings saved locally
    QString lastLoggedUsername;
    QString lastLoggedKey;


    /// --------------- Program Settings Saved in XML
//    bool forceWorkoutWindowOnTop;
//    bool showIncludedWorkout;
//    bool distanceInKM;
//    QString strava_access_token;
//    bool controlTrainerResistance;


//    ///-------------------- Workout Settings
//    int lastIndexSelectedConfigWorkout;
//    int lastTabSubConfigSelected;

//    //Position Widget in WorkoutDialog (ex: "Timer, "Hr", ...)
//    QString tabDisplay[8];


//    ///0 = Cadence
//    ///1 = Power
//    ///2 = Speed
//    ///3 = Button
//    int startTrigger;
//    int valueCadenceStart;
//    int valuePowerStart;
//    int valueSpeedStart;


//    /// -------------- Display
//    ///-1 = Hide
//    ///0 = Minimalist
//    ///1 = Detailed
//    ///2 = Graph


//    /// Timers
//    bool showIntervalRemaining;
//    bool showWorkoutRemaining;
//    bool showElapsed;
//    int fontSizeTimer;


//    /// Averaging Power (0=None, 1=1sec, 2, 3, 4, 5)
//    int averagingPower;
//    int offsetPower; //adjust watts manually +- 100 Watts

//    //// Main graph, show target and curve
//    bool showSeperatorInterval;
//    bool showGrid;


//    //// -------------------------------- Sounds
//    int soundPlayerVol;  ///0 TO 100
//    bool enableSound;
//    bool soundInterval;
//    bool soundPauseResumeWorkout;
//    bool soundAchievement;
//    bool soundEndWorkout;


};
Q_DECLARE_METATYPE(Settings*)

#endif // SETTINGS_H
