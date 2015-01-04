#include "stdbool.h"

#ifndef __BLUETOOTH_HW_H__
#define __BLUETOOTH_HW_H__

void bluetooth_hw_init(void);
void bluetooth_hw_power(bool asserted);
void bluetooth_hw_reset(bool asserted);
void bluetooth_hw_awake(bool asserted);
bool bluetooth_hw_is_powered(void);
bool bluetooth_hw_is_reset(void);
bool bluetooth_hw_is_awake(void);

#endif /* __BLUETOOTH_HW_H__ */
