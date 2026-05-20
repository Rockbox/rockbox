#include "config.h"
#include "lang.h"
#include "menu.h"
#include "settings.h"
#include "bluetooth-x1000.h"
#include "splash.h"
#include "action.h"
#include "list.h"

static int do_scan(void)
{
    struct bt_device devices[MAX_BT_DEVICES];
    int count;

    bluetooth_scan();
    splash(HZ * 8, "Scanning...");
    
    count = bluetooth_get_results(devices, MAX_BT_DEVICES);
    if (count == 0) {
        splash(HZ * 2, "No devices found");
    } else {
        /* Simple list of results for now */
        char msg[128];
        snprintf(msg, sizeof(msg), "Found %d devices. See log.", count);
        splash(HZ * 2, msg);
    }
    return 0;
}

static int do_connect(void)
{
    splash(0, "Connecting...");
    /* Placeholder address */
    uint8_t addr[6] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
    if (bluetooth_connect(addr))
        splash(HZ, "Connected");
    else
        splash(HZ, "Failed");
    return 0;
}

MENUITEM_SETTING(bluetooth_enabled, &global_settings.bluetooth_enabled, NULL);
MENUITEM_FUNCTION(scan_item, 0, ID2P(LANG_BLUETOOTH_SCAN), do_scan, NULL, Icon_NOICON);
MENUITEM_FUNCTION(connect_item, 0, ID2P(LANG_BLUETOOTH_PAIR), do_connect, NULL, Icon_NOICON);

MAKE_MENU(bluetooth_menu, ID2P(LANG_BLUETOOTH), 0, Icon_NOICON,
            &bluetooth_enabled, &scan_item, &connect_item);
