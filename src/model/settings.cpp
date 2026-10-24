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

