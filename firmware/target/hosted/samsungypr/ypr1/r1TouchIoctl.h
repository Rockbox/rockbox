/* This file originates from the linux kernel provided in Samsung's YP-R1 Open
 * Source package (second release, which includes some driver modules sources).
 * 
 * I just operated a cleanup removing TABs (to be compliant to RockBox guidelines) and
 * added a necessary include
 * 
 */

#ifndef __DEV_IOCTL_TS_MELFAS_H__
#define __DEV_IOCTL_TS_MELFAS_H__

typedef struct {
    int count;
    unsigned char addr;
    unsigned char pData[256];
}__attribute__((packed)) TsI2CData;

#define DEV_IOCTL_TS_MAGIC              'X'

#define E_IOCTL_TOUCH_RESET             0
#define E_IOCTL_TOUCH_ON                1
#define E_IOCTL_TOUCH_OFF               2
#define E_IOCTL_TOUCH_FLUSH             3
#define E_IOCTL_TOUCH_SLEEP             4
#define E_IOCTL_TOUCH_WAKE              5
#define E_IOCTL_TOUCH_ENTER_FWUPG_MODE  6
#define E_IOCTL_TOUCH_I2C_READ          7
#define E_IOCTL_TOUCH_I2C_WRITE         8
#define E_IOCTL_TOUCH_RESET_AFTER_FWUPG 9
#define E_IOCTL_TOUCH_RIGHTHAND         10
#define E_IOCTL_TOUCH_LEFTHAND          11
#define E_IOCTL_TOUCH_IDLE              12
#define E_IOCTL_TOUCH_SET_SENSE         13
#define E_IOCTL_TOUCH_GET_VER           14
#define E_IOCTL_TOUCH_SET_REP_RATE      15
#define E_IOCTL_TOUCH_ENABLE_WDOG       16
#define E_IOCTL_TOUCH_DISABLE_WDOG      17

#define DEV_CTRL_TOUCH_RESET                    _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_RESET)
#define DEV_CTRL_TOUCH_ON                       _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_ON)
#define DEV_CTRL_TOUCH_OFF                      _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_OFF)
#define DEV_CTRL_TOUCH_FLUSH                    _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_FLUSH)
#define DEV_CTRL_TOUCH_SLEEP                    _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_SLEEP)
#define DEV_CTRL_TOUCH_WAKE                     _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_WAKE)
#define DEV_CTRL_TOUCH_ENTER_FWUPG_MODE         _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_ENTER_FWUPG_MODE)
#define DEV_CTRL_TOUCH_I2C_READ                 _IOWR(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_I2C_READ, TsI2CData)
#define DEV_CTRL_TOUCH_I2C_WRITE                _IOWR(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_I2C_WRITE, TsI2CData)
#define DEV_CTRL_TOUCH_RESET_AFTER_FWUPG        _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_RESET_AFTER_FWUPG)
#define DEV_CTRL_TOUCH_RIGHTHAND                _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_RIGHTHAND)
#define DEV_CTRL_TOUCH_LEFTHAND                 _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_LEFTHAND)
#define DEV_CTRL_TOUCH_IDLE                     _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_IDLE)
#define DEV_CTRL_TOUCH_SET_SENSE                _IOW(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_SET_SENSE, int)
#define DEV_CTRL_TOUCH_GET_VER                  _IOR(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_GET_VER, int)
#define DEV_CTRL_TOUCH_SET_REP_RATE             _IOW(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_SET_REP_RATE, int)
#define DEV_CTRL_TOUCH_ENABLE_WDOG              _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_ENABLE_WDOG)
#define DEV_CTRL_TOUCH_DISABLE_WDOG             _IO(DEV_IOCTL_TS_MAGIC, E_IOCTL_TOUCH_DISABLE_WDOG)
#define DEV_CTRL_TOUCH_MAX_NR                   18

typedef struct {
    unsigned char inputInfo;
    unsigned char xHigh;
    unsigned char xLow;
    unsigned char yHigh;
    unsigned char yLow;
    unsigned char z;
    unsigned char width;
    unsigned char gesture;
}__attribute__((packed)) TsRawData;

#endif /* __DEV_IOCTL_TS_MELFAS_H__ */
