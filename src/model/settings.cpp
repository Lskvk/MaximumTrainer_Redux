#include "settings.h"

#include <QDebug>
#include <QSettings>


Settings::~Settings() {
    qDebug() << "destructor Settings;";
}


/// Constructor - load settings
Settings::Settings(QObject *parent) : QObject(parent)  {


    QSettings settings;


    settings.beginGroup("SettingsGeneral");
    lastLoggedUsername = settings.value("lastLoggedUsername", "").toString();
    lastLoggedKey = settings.value("lastLoggedKey", "").toString();
    // Keys of the removed remembered-usernames list; "username"+i was pointer
    // arithmetic, hence the truncated names.
    for (const char *staleKey : {"rememberMyPassword", "username", "sername", "ername", "rname", "name"})
        settings.remove(staleKey);
    settings.endGroup();


}


void Settings::saveGeneralSettings() {

    QSettings settings;

    qDebug() << "saveGeneralSettings!";


    settings.beginGroup("SettingsGeneral");
    settings.setValue("lastLoggedUsername", lastLoggedUsername);
    settings.setValue("lastLoggedKey", lastLoggedKey);
    settings.endGroup();


}

