#ifndef __BLUETOOTH_X1000_H__
#define __BLUETOOTH_X1000_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize Bluetooth UART and power on the chip.
 */
void bluetooth_init(void);

/**
 * Power cycle the Bluetooth chip.
 */
void bluetooth_power_on(void);

/**
 * Send a raw HCI command.
 */
bool hci_send_cmd(uint16_t opcode, const uint8_t *params, uint8_t len);

/**
 * Wait for an HCI event.
 */
bool hci_wait_event(uint8_t event_code, uint8_t *buffer, int max_len);

/**
 * Send encoded audio data over Bluetooth.
 */
void bluetooth_send_audio(const uint8_t *data, uint16_t len);

#define MAX_BT_DEVICES 10

/**
 * Scan for nearby Bluetooth devices.
 */
int bluetooth_scan(void);

struct bt_device {
    uint8_t addr[6];
    char name[32];
    uint32_t dev_class;
};

/**
 * Get the number of discovered devices and their details.
 */
int bluetooth_get_results(struct bt_device *devices, int max_count);

/**
 * Connect to a Bluetooth device by address.
 */
bool bluetooth_connect(const uint8_t *addr);

#endif /* __BLUETOOTH_X1000_H__ */
