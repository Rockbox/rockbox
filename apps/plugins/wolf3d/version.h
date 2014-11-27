#ifndef _VERSION_H_
#define _VERSION_H_

#ifndef VERSIONALREADYCHOSEN              // used for batch compiling

/* Defines used for different versions */

//#define SPEAR
//#define SPEARDEMO
#define UPLOAD
#define GOODTIMES
#define CARMACIZED
//#define APOGEE_1_0
//#define APOGEE_1_1
//#define APOGEE_1_2

/*
    Wolf3d Full v1.1 Apogee (with ReadThis)   - define CARMACIZED and APOGEE_1_1
    Wolf3d Full v1.4 Apogee (with ReadThis)   - define CARMACIZED
    Wolf3d Full v1.4 GT/ID/Activision         - define CARMACIZED and GOODTIMES
    Wolf3d Shareware v1.0                     - define UPLOAD and APOGEE_1_0
    Wolf3d Shareware v1.1                     - define CARMACIZED and UPLOAD and APOGEE_1_1
    Wolf3d Shareware v1.2                     - define CARMACIZED and UPLOAD and APOGEE_1_2
    Wolf3d Shareware v1.4                     - define CARMACIZED and UPLOAD
    Spear of Destiny Full and Mission Disks   - define CARMACIZED and SPEAR
                                                (and GOODTIMES for no FormGen quiz)
    Spear of Destiny Demo                     - define CARMACIZED and SPEAR and SPEARDEMO
*/

#endif

//#define FIXRAINSNOWLEAKS    // Enables leaking ceilings fix (by Adam Biser, only needed if maps with rain/snow and ceilings exist)

#define DEBUGKEYS             // Comment this out to compile without the Tab debug keys
#define ARTSEXTERN
#define DEMOSEXTERN
#define PLAYDEMOLIKEORIGINAL  // When playing or recording demos, several bug fixes do not take
                              // effect to let the original demos work as in the original Wolf3D v1.4
                              // (actually better, as the second demo rarely worked)

#endif
