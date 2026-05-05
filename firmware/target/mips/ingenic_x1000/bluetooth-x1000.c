#include "bluetooth-x1000.h"
#include "system.h"
#include "kernel.h"
#include "gpio-x1000.h"
#include "clk-x1000.h"
#include "x1000/uart.h"
#include "logf.h"
#include <string.h>

#include "file.h"
#include <stdio.h>
#include <string.h>

#define BT_PWR          GPIO_PC(22)
#define BT_STACK_SIZE   4096

static long bt_thread_stack[BT_STACK_SIZE/sizeof(long)];

#define FW_FILENAME "/.rockbox/bcm43438.fw"
static struct event_queue bt_queue;

static uint16_t bt_conn_handle = 0;
static uint16_t bt_sig_cid = 0;
static uint16_t bt_media_cid = 0;
static int bt_state = 0; // 0: Idle, 1: Connecting, 2: Connected, 3: Streaming

static struct bt_device discovered_devices[MAX_BT_DEVICES];
static int num_discovered = 0;

static void l2cap_send(uint16_t cid, const uint8_t *data, uint16_t len);

static void bluetooth_thread(void);
static int hci_read_packet(uint8_t *type, uint8_t *code, uint8_t *buffer, int max_len);

static void uart0_putc(uint8_t c)
{
    long start = current_tick;
    while(!(REG_UART_ULSR(0) & BM_UART_ULSR_TDRQ)) {
        if (TIME_AFTER(current_tick, start + HZ/10)) return; // 100ms timeout
        yield();
    }
    REG_UART_UTHR(0) = c;
}

static int uart0_getc(int timeout_ms)
{
    long timeout = current_tick + (timeout_ms * HZ / 1000);
    if (timeout_ms > 0 && timeout == current_tick) timeout++; // ensure at least 1 tick

    while (TIME_BEFORE(current_tick, timeout)) {
        if (REG_UART_ULSR(0) & BM_UART_ULSR_DRY) {
            return REG_UART_URBR(0);
        }
    }
    return -1;
}

void uart0_init(uint32_t baud)
{
    jz_writef(CPM_CLKGR, UART0(0));
    gpio_set_function(GPIO_PC(17), GPIOF_DEVICE(0));
    gpio_set_function(GPIO_PC(18), GPIOF_DEVICE(0));
    gpio_set_function(GPIO_PC(19), GPIOF_DEVICE(0));
    gpio_set_function(GPIO_PC(20), GPIOF_DEVICE(0));
    
    REG_UART_ULCR(0) = 0x83;
    uint32_t div = clk_get(X1000_CLK_SCLK_A) / (16 * baud);
    REG_UART_UDLLR(0) = div & 0xff;
    REG_UART_UDLHR(0) = (div >> 8) & 0xff;
    REG_UART_ULCR(0) = 0x03;
    REG_UART_UFCR(0) = 0x07;
    REG_UART_UMCR(0) = 0x22;
}

void bluetooth_power_on(void)
{
    gpio_set_level(BT_PWR, 0);
    sleep(HZ/10);
    gpio_set_level(BT_PWR, 1);
    sleep(HZ/2);
}

bool hci_send_cmd(uint16_t opcode, const uint8_t *params, uint8_t len)
{
    uart0_putc(0x01);
    uart0_putc(opcode & 0xff);
    uart0_putc(opcode >> 8);
    uart0_putc(len);
    for(uint8_t i = 0; i < len; i++)
        uart0_putc(params[i]);
    return true;
}



int hci_read_packet(uint8_t *type, uint8_t *code, uint8_t *buffer, int max_len)
{
    int c = uart0_getc(100);
    if(c < 0) return -1;
    *type = (uint8_t)c;
    
    if(*type == 0x04) { // HCI Event
        int ev_code = uart0_getc(100);
        if(ev_code < 0) return -1;
        *code = (uint8_t)ev_code;
        
        int len = uart0_getc(100);
        if(len < 0) return -1;
        
        for(int i = 0; i < len; i++) {
            int b = uart0_getc(100);
            if(b < 0) return -1;
            if(i < max_len) buffer[i] = (uint8_t)b;
        }
        return len;
    } else if (*type == 0x02) { // HCI ACL Data
        uart0_getc(100); // handle low
        uart0_getc(100); // handle high
        int l1 = uart0_getc(100);
        int l2 = uart0_getc(100);
        if (l1 < 0 || l2 < 0) return -1;
        int len = l1 | (l2 << 8);
        for(int i = 0; i < len; i++) {
            int b = uart0_getc(100);
            if(b < 0) return -1;
            if(i < max_len) buffer[i] = (uint8_t)b;
        }
        *code = 0; // Not used for ACL
        return len;
    }
    return 0;
}
bool hci_wait_event(uint8_t event_code, uint8_t *buffer, int max_len)
{
    uint8_t type, code;
    long start = current_tick;
    while (TIME_BEFORE(current_tick, start + HZ*2)) {
        int len = hci_read_packet(&type, &code, buffer, max_len);
        if (len > 0 && type == 0x04 && code == event_code) return true;
        if (len < 0) return false;
        yield();
    }
    return false;
}
static bool upload_firmware(void)
{
    int fd;
    uint8_t ev[16];
    uint8_t cmd_buf[256];

    fd = open(FW_FILENAME, O_RDONLY);
    if (fd < 0) {
        logf("BT: FW file not found: %s", FW_FILENAME);
        return false;
    }

    logf("BT: Starting FW upload from file");
    while (1) {
        /* Read 3-byte header: opcode(2), len(1) */
        if (read(fd, cmd_buf, 3) != 3) break;

        uint16_t opcode = cmd_buf[0] | (cmd_buf[1] << 8);
        uint8_t len = cmd_buf[2];

        /* Read command body */
        if (read(fd, &cmd_buf[3], len) != len) {
            logf("BT: FW file truncated");
            close(fd);
            return false;
        }

        hci_send_cmd(opcode, &cmd_buf[3], len);

        /* Wait for Command Complete */
        if(!hci_wait_event(0x0e, ev, sizeof(ev))) {
            logf("BT: FW upload fail at opcode %04x", opcode);
            close(fd);
            return false;
        }
    }
    
    close(fd);
    logf("BT: FW upload success");
    return true;
}

static void bluetooth_thread(void)
{
    uint8_t buffer[256];
    uint8_t type, code;
    while (1) {
        /* Poll for HCI events and ACL data */
        int len = hci_read_packet(&type, &code, buffer, sizeof(buffer));
        if (len > 0) {
            logf("BT: Recv type %02x, code %02x, len %d", type, code, len);
            if (type == 0x04) { // HCI Event
                if (code == 0x02 || code == 0x22) { // Inquiry Result or Inquiry Result with RSSI
                    uint8_t num_responses = buffer[0];
                    int entry_size = (code == 0x02) ? 14 : 15;
                    for (int i = 0; i < num_responses; i++) {
                        if (num_discovered < MAX_BT_DEVICES) {
                            memcpy(discovered_devices[num_discovered].addr, &buffer[1 + i*entry_size], 6);
                            snprintf(discovered_devices[num_discovered].name, 32, "Device %02x:%02x:%02x",
                                     buffer[1+i*entry_size+3], buffer[1+i*entry_size+4], buffer[1+i*entry_size+5]);
                            num_discovered++;
                            logf("BT: Found device %02x:%02x:%02x", buffer[1+i*entry_size+3], buffer[1+i*entry_size+4], buffer[1+i*entry_size+5]);
                        }
                    }
                } else if (code == 0x03) { // Connection Complete
                    bt_conn_handle = buffer[1] | (buffer[2] << 8);
                    bt_state = 1; // Connected at HCI level
                    /* Send L2CAP Connection Request (PSM=0x0019 AVDTP Signaling) */
                    uint8_t req[] = { 0x02, 0x01, 0x04, 0x00, 0x19, 0x00, 0x01, 0x00 };
                    l2cap_send(0x0001, req, sizeof(req));
                }
            } else if (type == 0x02) { // ACL Data
                uint16_t cid = buffer[2] | (buffer[3] << 8);
                if (cid == 0x0001) { // L2CAP Signaling CID
                    uint8_t sig_code = buffer[4];
                    if (sig_code == 0x03) { // L2CAP Connection Response
                        uint16_t scid = buffer[10] | (buffer[11] << 8);
                        bt_sig_cid = scid;
                        /* AVDTP Discover command */
                        uint8_t avdtp_disc[] = { 0x00, 0x01 };
                        l2cap_send(bt_sig_cid, avdtp_disc, sizeof(avdtp_disc));
                        bt_state = 2; // Negotiating
                    }
                } else if (cid == bt_sig_cid) {
                    /* Handle AVDTP signaling responses */
                    uint8_t avdtp_sig = buffer[5];
                    if (avdtp_sig == 0x01) { // Discover Response
                        /* Placeholder: Pick first SEP and Set Configuration */
                        uint8_t seid = buffer[6] >> 2;
                        uint8_t avdtp_set[] = { 0x10, 0x03, seid << 2, 0x00, 
                                                0x01, 0x00, // Service Category 1: Media Transport
                                                0x07, 0x06, 0x00, 0x00, 0xff, 0xff, 0x02, 0x35 // Media Codec (SBC)
                                              };
                        l2cap_send(bt_sig_cid, avdtp_set, sizeof(avdtp_set));
                        bt_state = 3;
                    }
                }
            }
        }
        sleep(HZ/50);
    }
}

void bluetooth_init(void)
{
    uint8_t ev[16];
    logf("BT: Initializing...");
    uart0_init(115200);
    bluetooth_power_on();

    /* 1. HCI Reset */
    logf("BT: Resetting chip...");
    hci_send_cmd(0x0c03, NULL, 0);
    if(!hci_wait_event(0x0e, ev, sizeof(ev))) {
        logf("BT: Reset fail");
        return;
    }

    /* 2. Upload Firmware Patch */
    logf("BT: Preparing for FW upload...");
    hci_send_cmd(0xfc2e, NULL, 0);
    if(!hci_wait_event(0x0e, ev, sizeof(ev))) {
        logf("BT: FW prepare fail");
        return;
    }
    
    if(!upload_firmware()) return;
    sleep(HZ/10);

    /* 3. Set Baudrate to 1Mbps */
    logf("BT: Setting baudrate to 1M...");
    uint8_t br_params[] = { 0x00, 0x00, 0x40, 0x42, 0x0f, 0x00 };
    hci_send_cmd(0xfc18, br_params, sizeof(br_params));
    if(!hci_wait_event(0x0e, ev, sizeof(ev))) {
        logf("BT: Baudrate set fail");
        return;
    }
    
    uart0_init(1000000);
    
    /* 4. Final Reset */
    logf("BT: Final reset...");
    hci_send_cmd(0x0c03, NULL, 0);
    hci_wait_event(0x0e, ev, sizeof(ev));

    logf("BT: Initialization complete");
    queue_init(&bt_queue, true);
    create_thread(bluetooth_thread, bt_thread_stack, sizeof(bt_thread_stack),
                  0, "bluetooth"
                  IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));
}

void bluetooth_send_audio(const uint8_t *data, uint16_t len)
{
    if (bt_media_cid == 0) return;
    l2cap_send(bt_media_cid, data, len);
}

int bluetooth_get_results(struct bt_device *devices, int max_count)
{
    int count = (num_discovered < max_count) ? num_discovered : max_count;
    memcpy(devices, discovered_devices, count * sizeof(struct bt_device));
    return count;
}

int bluetooth_scan(void)
{
    /* Clear old results */
    num_discovered = 0;
    /* Send HCI Inquiry */
    uint8_t inquiry_params[] = { 0x33, 0x8b, 0x9e, 0x05, 0x00 }; // LAP, length, num_responses
    hci_send_cmd(0x0401, inquiry_params, sizeof(inquiry_params));
    return 0; // Scanning started
}

bool bluetooth_connect(const uint8_t *addr)
{
    /* Send HCI Create Connection */
    uint8_t conn_params[13];
    memcpy(conn_params, addr, 6);
    conn_params[6] = 0x18; // Packet Type
    conn_params[7] = 0xcc;
    conn_params[8] = 0x01; // Page Scan Repetition Mode
    conn_params[9] = 0x00; // Reserved
    conn_params[10] = 0x00; // Clock Offset
    conn_params[11] = 0x00;
    conn_params[12] = 0x01; // Role Switch
    
    hci_send_cmd(0x0405, conn_params, sizeof(conn_params));
    return true;
}

static void l2cap_send(uint16_t cid, const uint8_t *data, uint16_t len)
{
    if (bt_conn_handle == 0) return;
    uart0_putc(0x02);
    uart0_putc(bt_conn_handle & 0xff);
    uart0_putc((bt_conn_handle >> 8) | 0x20);
    uint16_t total_len = len + 4;
    uart0_putc(total_len & 0xff);
    uart0_putc(total_len >> 8);
    uart0_putc(len & 0xff);
    uart0_putc(len >> 8);
    uart0_putc(cid & 0xff);
    uart0_putc(cid >> 8);
    for(uint16_t i = 0; i < len; i++) uart0_putc(data[i]);
}
