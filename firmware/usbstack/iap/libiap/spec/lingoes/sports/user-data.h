#pragma once
#include <stdint.h>

/* [1] P.341 4.9.13 Command 0x88: GetUserData */

enum IAPSportsUserDataType {
    IAPSportsUserDataType_UnitSystem       = 0x00,
    IAPSportsUserDataType_Name             = 0x01,
    IAPSportsUserDataType_Gender           = 0x02,
    IAPSportsUserDataType_Weight           = 0x03,
    IAPSportsUserDataType_Age              = 0x04,
    IAPSportsUserDataType_WorkoutRecording = 0x05,
};

struct IAPGetUserDataPayload {
    uint8_t type; /* IAPSportsUserDataType */
} __attribute__((packed));

struct IAPRetUserDataPayload {
    uint8_t type; /* IAPSportsUserDataType */
    uint8_t data[];
} __attribute__((packed));

enum IAPSportsUnitSystem {
    IAPSportsUnitSystem_None     = 0x00,
    IAPSportsUnitSystem_Imperial = 0x01,
    IAPSportsUnitSystem_Metric   = 0x02,
};

struct IAPRetUserDataUnitSystemPayload {
    uint8_t type;                  /* = IAPSportsUserDataType_UnitSystem */
    uint8_t preferred_unit_system; /* IAPSportsUnitSystem */
} __attribute__((packed));

struct IAPRetUserDataNamePayload {
    uint8_t type; /* = IAPSportsUserDataType_Name */
    char    name[];
} __attribute__((packed));

enum IAPSportsGender {
    IAPSportsGender_None   = 0x00,
    IAPSportsGender_Female = 0x01,
    IAPSportsGender_Male   = 0x02,
};

struct IAPRetUserDataGenderPayload {
    uint8_t type;   /* = IAPSportsUserDataType_Gender */
    uint8_t gender; /* IAPSportsGender */
} __attribute__((packed));

struct IAPRetUserDataWeightPayload {
    uint8_t  type; /* = IAPSportsUserDataType_Weight */
    uint16_t weight;
} __attribute__((packed));

struct IAPRetUserDataAgePayload {
    uint8_t type; /* = IAPSportsUserDataType_Age */
    uint8_t age;
} __attribute__((packed));

enum IAPSportsWorkoutRecordingPreferences {
    IAPSportsWorkoutRecordingPreferences_None   = 0x00,
    IAPSportsWorkoutRecordingPreferences_Never  = 0x01,
    IAPSportsWorkoutRecordingPreferences_Ask    = 0x02,
    IAPSportsWorkoutRecordingPreferences_Always = 0x03,
};

struct IAPRetUserDataWorkoutRecordingPayload {
    uint8_t type;       /* = IAPSportsUserDataType_WorkoutRecording */
    uint8_t preference; /* IAPSportsWorkoutRecordingPreferences */
} __attribute__((packed));

/* IAPSetUserDataPayload = IAPRetUserDataPayload */
