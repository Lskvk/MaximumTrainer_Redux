#include "macutils.h"




MacUtils::MacUtils()
{

}

MacUtils::~MacUtils()
{

}


//http://stackoverflow.com/questions/5596319/how-to-programmatically-prevent-a-mac-from-going-to-sleep/5596946#5596946
void MacUtils::disableScreensaver() {


    CFStringRef reasonForActivity = CFSTR("MaximumTrainer Workout");

    IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                                        kIOPMAssertionLevelOn, reasonForActivity, &assertionID);


    if (success == kIOReturnSuccess)
    {
        //success = IOPMAssertionRelease(assertionID);
        ///return disable sucess? todo..
    }

}


void MacUtils::releaseScreensaverLock() {

    IOPMAssertionRelease(assertionID);
}

