/***************************************************************************
 * HiBy-hosted: Bluetooth on/off, scan, pair/connect via stock bluez-tools.
 ****************************************************************************/

#include "config.h"
#include "lang.h"
#include "menu.h"
#include "action.h"
#include "splash.h"
#include "kernel.h"
#include "settings.h"
#include "thread.h"
#include "erosqlinux_codec.h"
#include "gui/list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/wait.h>

#define BT_INIT_OK_PATH     "/tmp/bt_init_ok"
#define BT_BRINGUP_TMP      "/tmp/device-bt-bringup.sh"
#define BT_BRINGUP_SD       "/mnt/sd_0/.rockbox/device-bt-bringup.sh"
#define BT_BRINGUP_LOG      "/tmp/rb_bt_bringup.log"
#define BT_SCAN_RESULT_PATH "/tmp/rb_bt_scan.json"
#define BT_LIST_RESULT_PATH "/tmp/rb_bt_list.json"
#define BT_DBUS_SOCK        "/var/run/dbus/system_bus_socket"
#define BT_ACL_PROBE_PATH   "/tmp/rb_bt_acl_ok"
#define BT_UI_LOG_PATH      "/tmp/rb_bt_ui.log"
#define BT_MAX_DEVICES      16
#define BT_NAME_LEN         48
#define BT_MAC_LEN          18
#define BT_AUTO_STACK_SZ    (8192/sizeof(long))

static long bt_auto_stack[BT_AUTO_STACK_SZ];
static unsigned int bt_auto_thread;
static volatile bool bt_auto_run;
static volatile bool bt_connected;
static volatile bool stack_start_issued;
static volatile bool bt_stack_live;
static unsigned int bt_bringup_wait_ticks;

static char bt_status_text[64] = "BT: off";

static int bt_dev_count;
static char bt_dev_mac[BT_MAX_DEVICES][BT_MAC_LEN];
static char bt_dev_name[BT_MAX_DEVICES][BT_NAME_LEN];
static bool bt_dev_paired[BT_MAX_DEVICES];

static void bt_refresh_status_text(void);

static void bt_ui_log(const char *msg)
{
    const char *paths[] = { BT_UI_LOG_PATH,
                            "/mnt/sd_0/.rockbox/rb_bt_ui.log" };
    int i;

    for (i = 0; i < 2; i++) {
        FILE *f = fopen(paths[i], "a");
        if (!f)
            continue;
        fputs(msg, f);
        fputc('\n', f);
        fclose(f);
    }
}

static bool path_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

static bool bt_hci_present(void)
{
    return access("/sys/class/bluetooth/hci0", F_OK) == 0;
}

static int bt_shell(const char *cmd)
{
    char buf[512];
    /* Rockbox must invoke /bin/sh -c; bare "pidof ... >/dev/null" via system()
     * is unreliable on this target. */
    snprintf(buf, sizeof(buf),
             "/bin/sh -c \"PATH=/usr/sbin:/usr/bin:/bin:/sbin %s\"", cmd);
    return system(buf);
}

/* system()/pidof return wait-status words on this target; use /proc instead. */
static bool bt_proc_running(const char *name)
{
    DIR *d = opendir("/proc");
    struct dirent *e;
    char path[128];
    char comm[32];
    FILE *f;
    bool found = false;

    if (!d)
        return false;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] < '1' || e->d_name[0] > '9')
            continue;
        snprintf(path, sizeof(path), "/proc/%s/comm", e->d_name);
        f = fopen(path, "r");
        if (!f)
            continue;
        comm[0] = '\0';
        if (fgets(comm, sizeof(comm), f)) {
            char *nl = strchr(comm, '\n');
            if (nl)
                *nl = '\0';
            if (!strcmp(comm, name))
                found = true;
        }
        fclose(f);
        if (found)
            break;
    }
    closedir(d);
    return found;
}

/* File probe: popen() is unreliable in the Rockbox process on this target. */
static bool bt_query_acl_connected(void)
{
    FILE *f;

    unlink(BT_ACL_PROBE_PATH);
    bt_shell("hcitool con 2>/dev/null | grep ACL >" BT_ACL_PROBE_PATH);
    f = fopen(BT_ACL_PROBE_PATH, "r");
    if (!f)
        return false;
    const int c = fgetc(f);
    fclose(f);
    return c != EOF;
}

static bool bt_parse_acl_mac(char *mac, size_t mac_len)
{
    FILE *f;
    char line[128];
    const char *p;
    char *sp;

    if (!mac || mac_len < BT_MAC_LEN)
        return false;

    mac[0] = '\0';
    f = fopen(BT_ACL_PROBE_PATH, "r");
    if (!f)
        return false;
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return false;
    }
    fclose(f);

    p = strstr(line, "ACL ");
    if (!p)
        return false;
    p += 4;
    snprintf(mac, mac_len, "%.17s", p);
    sp = strchr(mac, ' ');
    if (sp)
        *sp = '\0';
    return strlen(mac) >= 17;
}

static void erosq_sync_bluetooth_peer(void)
{
    char mac[BT_MAC_LEN];

    if (bt_parse_acl_mac(mac, sizeof(mac)))
        erosq_set_bluetooth_peer(mac);
}

/* Active ACL means the stack is usable even if bt_init_ok / bluez tools lag. */
static bool bt_stack_ready(void)
{
    if (!bt_hci_present())
        return false;
    if (bt_query_acl_connected())
        return true;
    if (!path_exists(BT_INIT_OK_PATH))
        return false;
    if (!path_exists(BT_DBUS_SOCK))
        return false;
    return bt_proc_running("dbus-daemon") && bt_proc_running("bluetoothd")
        && bt_proc_running("bt-media");
}

static bool bt_stack_ready_for_ui(void)
{
    return bt_stack_live || bt_stack_ready();
}

static bool bt_stack_usable_now(void)
{
    return bt_stack_ready();
}

static const char *bt_bringup_script_path(void)
{
    /* SD is the canonical copy (deployed with rockbox.erosq). */
    if (path_exists(BT_BRINGUP_SD))
        return BT_BRINGUP_SD;
    if (path_exists(BT_BRINGUP_TMP))
        return BT_BRINGUP_TMP;
    return NULL;
}

static void bt_launch_bringup(const char *script)
{
    char cmd[384];
    snprintf(cmd, sizeof(cmd),
             "nohup env PATH=/usr/sbin:/usr/bin:/bin:/sbin /bin/sh %s "
             "</dev/null >>" BT_BRINGUP_LOG " 2>&1 &",
             script);
    bt_shell(cmd);
}

static void bt_issue_stack_start(void)
{
    const char *script;

    if (stack_start_issued)
        return;

    /* Do not tear down a stack that is already running (e.g. after adb bringup). */
    if (bt_stack_usable_now()) {
        bt_ui_log("stack already usable, skip bringup");
        stack_start_issued = true;
        if (!path_exists(BT_INIT_OK_PATH))
            bt_shell(": >" BT_INIT_OK_PATH);
        return;
    }

    stack_start_issued = true;
    unlink(BT_INIT_OK_PATH);

    script = bt_bringup_script_path();
    if (script) {
        char note[96];
        int rc;
        snprintf(note, sizeof(note), "bringup launch %s", script);
        bt_ui_log(note);
        bt_launch_bringup(script);
        rc = 0; /* async */
        snprintf(note, sizeof(note), "bringup issued rc=%d", rc);
        bt_ui_log(note);
        return;
    }

    bt_ui_log("bringup: no script, using bt_init");
    bt_shell("nohup /usr/bin/bt_init </dev/null >>" BT_BRINGUP_LOG " 2>&1 &");
}

static void bt_stack_power_on(void)
{
    bt_shell("hciconfig hci0 up 2>/dev/null");
    bt_shell("bt-adapter --set Powered On 2>/dev/null");
    bt_shell("hciconfig hci0 piscan 2>/dev/null");
}

static void bt_stack_stop(void)
{
    erosq_set_bluetooth_route(0);
    bt_shell("/usr/bin/bt_suspend 2>/dev/null");
    unlink(BT_INIT_OK_PATH);
    bt_shell("rm -f /var/run/dbus/system_bus_socket /var/run/messagebus.pid "
             "2>/dev/null");
    bt_connected = false;
    stack_start_issued = false;
}

static void bt_ensure_stack_for_ui(void)
{
    unsigned waited = 0;
    const unsigned tmax = HZ * 90;

    if (!global_settings.bluetooth_enabled)
        return;

    if (!stack_start_issued)
        bt_issue_stack_start();

    while (!bt_stack_ready() && waited < tmax) {
        sleep(HZ / 5);
        waited += HZ / 5;
    }

    if (!bt_stack_ready()) {
        stack_start_issued = false;
        bt_stack_live = false;
        return;
    }

    bt_stack_live = true;
    bt_stack_power_on();
}

static void bt_dev_clear(void)
{
    bt_dev_count = 0;
    memset(bt_dev_mac, 0, sizeof(bt_dev_mac));
    memset(bt_dev_name, 0, sizeof(bt_dev_name));
    memset(bt_dev_paired, 0, sizeof(bt_dev_paired));
}

static void bt_dev_add(const char *mac, const char *name, bool paired)
{
    int i;

    if (!mac || !mac[0] || bt_dev_count >= BT_MAX_DEVICES)
        return;

    for (i = 0; i < bt_dev_count; i++) {
        if (!strcasecmp(bt_dev_mac[i], mac))
        {
            if (name && name[0])
                snprintf(bt_dev_name[i], BT_NAME_LEN, "%s", name);
            if (paired)
                bt_dev_paired[i] = true;
            return;
        }
    }

    snprintf(bt_dev_mac[bt_dev_count], BT_MAC_LEN, "%s", mac);
    if (name && name[0])
        snprintf(bt_dev_name[bt_dev_count], BT_NAME_LEN, "%s", name);
    else
        snprintf(bt_dev_name[bt_dev_count], BT_NAME_LEN, "%s", mac);
    bt_dev_paired[bt_dev_count] = paired;
    bt_dev_count++;
}

static const char *bt_json_value(const char *line, const char *key)
{
    char pattern[32];
    const char *p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(line, pattern);
    if (!p)
        return NULL;
    p = strchr(p, ':');
    if (!p)
        return NULL;
    p++;
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '"')
        return p + 1;
  return p;
}

static void bt_parse_device_json(const char *path, bool list_mode)
{
    FILE *f;
    char line[256];
    char mac[BT_MAC_LEN];
    char name[BT_NAME_LEN];
    const char *v;
    bool paired = list_mode;

    bt_dev_clear();
    f = fopen(path, "r");
    if (!f)
        return;

    mac[0] = name[0] = '\0';
    while (fgets(line, sizeof(line), f)) {
        if ((v = bt_json_value(line, "MAC"))) {
            snprintf(mac, sizeof(mac), "%.17s", v);
            /* trim at closing quote */
            char *q = strchr(mac, '"');
            if (q)
                *q = '\0';
        }
        if ((v = bt_json_value(line, "Name"))) {
            snprintf(name, sizeof(name), "%.47s", v);
            char *q = strchr(name, '"');
            if (q)
                *q = '\0';
        }
        if ((v = bt_json_value(line, "Alias")) && !name[0]) {
            snprintf(name, sizeof(name), "%.47s", v);
            char *q = strchr(name, '"');
            if (q)
                *q = '\0';
        }
        if (list_mode) {
            if (strstr(line, "\"Paired\"") && strstr(line, "true"))
                paired = true;
            if (strstr(line, "\"Paired\"") && strstr(line, "1"))
                paired = true;
        }
        if (strchr(line, '}') && mac[0]) {
            bt_dev_add(mac, name[0] ? name : mac, paired);
            mac[0] = name[0] = '\0';
            paired = list_mode;
        }
    }
    fclose(f);
}

static int bt_run_scan(void)
{
    unlink(BT_SCAN_RESULT_PATH);
    bt_shell("bt-adapter -d -p " BT_SCAN_RESULT_PATH " 2>/dev/null &");
    sleep(HZ * 10);
    bt_shell("bt-adapter -s 2>/dev/null");
    sleep(HZ / 2);
    bt_parse_device_json(BT_SCAN_RESULT_PATH, false);
    return bt_dev_count;
}

static int bt_run_list_paired(void)
{
    unlink(BT_LIST_RESULT_PATH);
    bt_shell("bt-device -l -p " BT_LIST_RESULT_PATH " 2>/dev/null");
    sleep(HZ);
    bt_parse_device_json(BT_LIST_RESULT_PATH, true);
    return bt_dev_count;
}

static int bt_connect_device(const char *mac)
{
    char cmd[96];

    snprintf(cmd, sizeof(cmd),
             "bt-device -c %s 2>/dev/null; bt-connect -c %s 2>/dev/null",
             mac, mac);
    bt_shell(cmd);
    snprintf(cmd, sizeof(cmd), "bt-device --set %s Trusted true 2>/dev/null", mac);
    bt_shell(cmd);
    sleep(HZ * 2);
    return bt_query_acl_connected();
}

static const char *bt_device_list_name(int selected_item, void *data,
                                       char *buffer, size_t buffer_len)
{
    (void)data;
    if (selected_item < 0 || selected_item >= bt_dev_count)
        return "";
    snprintf(buffer, buffer_len, "%s", bt_dev_name[selected_item]);
    return buffer;
}

static int bt_device_list_action(const char *title)
{
    struct gui_synclist lists;
    int action = ACTION_NONE;
    int result = 0;

    if (bt_dev_count <= 0) {
        splash(HZ * 2, "No devices found");
        return 0;
    }

    gui_synclist_init(&lists, bt_device_list_name, NULL, false, 1, NULL);
    gui_synclist_set_title(&lists, (unsigned char *)title, NOICON);
    gui_synclist_set_nb_items(&lists, bt_dev_count);
    gui_synclist_draw(&lists);

    while (result == 0) {
        gui_synclist_draw(&lists);
        list_do_action(CONTEXT_STD, TIMEOUT_BLOCK, &lists, &action);
        switch (action) {
        case ACTION_STD_OK:
            splash(HZ, "Connecting...");
            if (bt_connect_device(bt_dev_mac[lists.selected_item])) {
                bt_connected = true;
                erosq_set_bluetooth_peer(bt_dev_mac[lists.selected_item]);
                erosq_set_bluetooth_route(1);
                bt_refresh_status_text();
                splash(HZ * 2, "Connected");
            } else {
                splash(HZ * 3, "Connect failed");
            }
            result = 1;
            break;
        case ACTION_STD_CANCEL:
            result = 1;
            break;
        default:
            break;
        }
    }
    return result;
}

static int bt_scan_devices_callback(void)
{
    if (!global_settings.bluetooth_enabled) {
        splash(HZ * 2, "Turn Bluetooth on first");
        return ACTION_NONE;
    }
    bt_ensure_stack_for_ui();
    if (!bt_stack_ready()) {
        splash(HZ * 3, "BT stack not ready");
        return ACTION_NONE;
    }
    splash(HZ, "Scanning...");
    if (bt_run_scan() <= 0) {
        splash(HZ * 3, "No devices found");
        return ACTION_NONE;
    }
    bt_device_list_action("Scan results");
    bt_refresh_status_text();
    return ACTION_NONE;
}

static int bt_paired_devices_callback(void)
{
    if (!global_settings.bluetooth_enabled) {
        splash(HZ * 2, "Turn Bluetooth on first");
        return ACTION_NONE;
    }
    bt_ensure_stack_for_ui();
    if (!bt_stack_ready()) {
        splash(HZ * 3, "BT stack not ready");
        return ACTION_NONE;
    }
    splash(HZ, "Loading paired...");
    if (bt_run_list_paired() <= 0) {
        splash(HZ * 3, "No paired devices");
        return ACTION_NONE;
    }
    bt_device_list_action("Paired devices");
    bt_refresh_status_text();
    return ACTION_NONE;
}

static int bt_disconnect_callback(void)
{
    bt_shell("bt-connect -d 2>/dev/null; bt-device -d 2>/dev/null");
    erosq_set_bluetooth_route(0);
    bt_connected = false;
    bt_refresh_status_text();
    splash(HZ * 2, "Disconnected");
    return ACTION_NONE;
}

static void bt_auto_thread_fn(void)
{
    bool was_connected = false;
    bool was_stack_up = false;
    bool stack_up = false;

    while (bt_auto_run) {
        if (!global_settings.bluetooth_enabled) {
            if (stack_up) {
                bt_stack_stop();
                stack_up = false;
            }
            bt_bringup_wait_ticks = 0;
            sleep(HZ);
            continue;
        }

        if (!stack_up) {
            bt_issue_stack_start();
            stack_up = bt_stack_ready();
            if (stack_up) {
                bt_stack_power_on();
                bt_bringup_wait_ticks = 0;
            } else {
                bt_bringup_wait_ticks++;
                /* Retry bringup if the background script never finished. */
                if (bt_bringup_wait_ticks > (unsigned)(HZ * 90)) {
                    stack_start_issued = false;
                    bt_bringup_wait_ticks = 0;
                }
                sleep(HZ / 2);
            }
        }

        bt_stack_live = stack_up;
        if (stack_up != was_stack_up) {
            bt_ui_log(stack_up ? "auto: stack up" : "auto: stack down");
            bt_refresh_status_text();
        }
        was_stack_up = stack_up;

        const bool connected = stack_up && bt_query_acl_connected();
        bt_connected = connected;

        if (connected != was_connected) {
            if (connected) {
                erosq_sync_bluetooth_peer();
                erosq_set_bluetooth_route(1);
                erosq_apply_bluetooth_route();
            } else {
                erosq_set_bluetooth_route(0);
            }
            bt_refresh_status_text();
        } else if (connected && !erosq_bluetooth_route_applied()) {
            erosq_sync_bluetooth_peer();
            erosq_apply_bluetooth_route();
        }
        was_connected = connected;

        sleep(stack_up ? (connected ? HZ / 4 : HZ / 2) : HZ / 2);
    }

    if (stack_up)
        bt_stack_stop();
}

static void bt_auto_start(void)
{
    if (!bt_auto_thread) {
        bt_auto_run = true;
        bt_auto_thread = create_thread(bt_auto_thread_fn, bt_auto_stack,
                                       sizeof(bt_auto_stack), 0, "bt_auto"
                                       IF_PRIO(, PRIORITY_USER_INTERFACE));
    } else {
        bt_auto_run = true;
    }
}

static void bt_auto_stop(void)
{
    bt_auto_run = false;
    bt_bringup_wait_ticks = 0;
    bt_stack_live = false;
    erosq_set_bluetooth_route(0);
    bt_stack_stop();
}

static int bt_enable_callback(int action,
                              const struct menu_item_ex *this_item,
                              struct gui_synclist *this_list)
{
    (void)this_item;
    (void)this_list;

    if (action == ACTION_EXIT_MENUITEM) {
        if (global_settings.bluetooth_enabled)
            bt_auto_start();
        else
            bt_auto_stop();
        bt_refresh_status_text();
        splash(HZ * 2, global_settings.bluetooth_enabled ?
                         "Bluetooth on" : "Bluetooth off");
    }
    return action;
}

static void bt_refresh_status_text(void)
{
    if (!global_settings.bluetooth_enabled) {
        snprintf(bt_status_text, sizeof(bt_status_text), "BT: off");
        return;
    }
    if (bt_connected)
        snprintf(bt_status_text, sizeof(bt_status_text), "BT: on, connected");
    else if (bt_stack_ready_for_ui())
        snprintf(bt_status_text, sizeof(bt_status_text), "BT: on, waiting");
    else if (stack_start_issued)
        snprintf(bt_status_text, sizeof(bt_status_text), "BT: starting...");
    else
        snprintf(bt_status_text, sizeof(bt_status_text), "BT: bringup retry");
}

static int bluetooth_hosted_menu_callback(int action,
                                          const struct menu_item_ex *item,
                                          struct gui_synclist *list)
{
    static int last_redraw_tick;

    (void)item;
    (void)list;

    bt_refresh_status_text();

    /* Status line does not auto-update; redraw while bring-up runs. */
    if (action == ACTION_ENTER_MENUITEM
        || TIME_BEFORE(last_redraw_tick + HZ / 2, current_tick)) {
        last_redraw_tick = current_tick;
        return ACTION_REDRAW;
    }
    return action;
}

MENUITEM_SETTING(bt_hosted_enable_item, &global_settings.bluetooth_enabled,
                 bt_enable_callback);
MENUITEM_RETURNVALUE(bt_hosted_status_item, (unsigned char *)bt_status_text,
                     0, NULL, Icon_NOICON);
MENUITEM_FUNCTION(bt_scan_item, 0, (unsigned char *)"Scan for devices",
                  bt_scan_devices_callback, NULL, Icon_NOICON);
MENUITEM_FUNCTION(bt_paired_item, 0, (unsigned char *)"Paired devices",
                  bt_paired_devices_callback, NULL, Icon_NOICON);
MENUITEM_FUNCTION(bt_disconnect_item, 0, (unsigned char *)"Disconnect",
                  bt_disconnect_callback, NULL, Icon_NOICON);

MAKE_MENU(bluetooth_hosted_menu, ID2P(LANG_BLUETOOTH),
          bluetooth_hosted_menu_callback, Icon_NOICON,
          &bt_hosted_enable_item,
          &bt_hosted_status_item,
          &bt_scan_item,
          &bt_paired_item,
          &bt_disconnect_item);

void bluetooth_hosted_boot_init(void)
{
    if (global_settings.bluetooth_enabled)
        bt_auto_start();
}
