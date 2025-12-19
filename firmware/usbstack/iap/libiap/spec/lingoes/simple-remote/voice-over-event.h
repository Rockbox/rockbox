#pragma once
#include <stdint.h>

enum IAPVoiceOverEventType {
    IAPVoiceOverEventType_MovePointer         = 0x00,
    IAPVoiceOverEventType_MoveToFirst         = 0x01,
    IAPVoiceOverEventType_MoveToLast          = 0x02,
    IAPVoiceOverEventType_MoveToNext          = 0x03,
    IAPVoiceOverEventType_MoveToPrev          = 0x04,
    IAPVoiceOverEventType_ScrollLeftPage      = 0x05,
    IAPVoiceOverEventType_ScrollRightPage     = 0x06,
    IAPVoiceOverEventType_ScrollUpPage        = 0x07,
    IAPVoiceOverEventType_ScrollDownPage      = 0x08,
    IAPVoiceOverEventType_ScrollToPoint       = 0x09,
    IAPVoiceOverEventType_SendTextToInput     = 0x0A,
    IAPVoiceOverEventType_Cut                 = 0x0B,
    IAPVoiceOverEventType_Copy                = 0x0C,
    IAPVoiceOverEventType_Paste               = 0x0D,
    IAPVoiceOverEventType_Home                = 0x0E,
    IAPVoiceOverEventType_Touch               = 0x0F,
    IAPVoiceOverEventType_DisplayScale        = 0x10,
    IAPVoiceOverEventType_CenterDisplay       = 0x11,
    IAPVoiceOverEventType_PauseSpeak          = 0x12,
    IAPVoiceOverEventType_ResumeSpeak         = 0x13,
    IAPVoiceOverEventType_ReadTextFromCurrent = 0x14,
    IAPVoiceOverEventType_ReadTextFromTop     = 0x15,
    IAPVoiceOverEventType_SendTextToSpeech    = 0x16,
    IAPVoiceOverEventType_Escape              = 0x17,
};

struct IAPVoiceOverEventPayload {
    uint8_t type; /* IAPVoiceOverEventType */
    uint8_t data[];
} __attribute__((packed));

struct IAPVoiceOverEventMovePointerPayload {
    uint8_t  type; /* = IAPVoiceOverEventType_MovePointer */
    uint16_t x;
    uint16_t y;
} __attribute__((packed));

struct IAPVoiceOverEventScrollToPointPayload {
    uint8_t  type; /* = IAPVoiceOverEventType_ScrollToPoint */
    uint16_t x;
    uint16_t y;
} __attribute__((packed));

struct IAPVoiceOverEventSendTextToInputPayload {
    uint8_t  type; /* = IAPVoiceOverEventType_SendTextToInput */
    uint16_t current_section_index;
    uint16_t last_section_index;
    char     text[];
} __attribute__((packed));

enum IAPVoiceOverEventTouchEventType {
    IAPVoiceOverEventTouchEventType_Begin      = 0x00,
    IAPVoiceOverEventTouchEventType_Moved      = 0x01,
    IAPVoiceOverEventTouchEventType_Stationary = 0x02,
    IAPVoiceOverEventTouchEventType_Ended      = 0x03,
    IAPVoiceOverEventTouchEventType_Canceled   = 0x04,
};

struct IAPVoiceOverEventTouchPayload {
    uint8_t type; /* = IAPVoiceOverEventType_Touch */
    uint8_t reserved[4];
    uint8_t touch_event_type; /* IAPVoiceOverEventTouchEventType */
} __attribute__((packed));

struct IAPVoiceOverEventDisplayScalePayload {
    uint8_t  type; /* = IAPVoiceOverEventType_DisplayScale */
    uint16_t scale_factor;
} __attribute__((packed));

struct IAPVoiceOverEventCenterDisplayPayload {
    uint8_t  type; /* = IAPVoiceOverEventType_CenterDisplay */
    uint16_t x;
    uint16_t y;
} __attribute__((packed));

struct IAPVoiceOverEventSendTextToSpeechPayload {
    uint8_t  type; /* = IAPVoiceOverEventType_SendTextToSpeech */
    uint16_t current_section_index;
    uint16_t last_section_index;
    char     text[];
} __attribute__((packed));
