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
};
Q_DECLARE_METATYPE(Settings*)

#endif // SETTINGS_H
