#include "planobject.h"



PlanObject::PlanObject(QObject *parent) :
    QObject(parent)
{
}


void PlanObject::goToWorkout(QString name) {


    emit signal_goToWorkout(name);

}
