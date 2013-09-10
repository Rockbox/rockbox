/* This file originates from the linux kernel provided in Samsung's YP-R1 Open
 * Source package (second release, which includes some driver modules sources).
 *
 * I just operated a cleanup removing TABs (to be compliant to Rockbox guidelines) and
 * added a necessary include
 *
 */

#ifndef __IOCTL_PMU_H__
#define __IOCTL_PMU_H__

#include <sys/ioctl.h>

#define IOCTL_PMU_MAGIC                 'A'

#define E_IOCTL_PMU_GET_BATT_LVL        0
#define E_IOCTL_PMU_GET_CHG_STATUS      1
#define E_IOCTL_PMU_IS_EXT_PWR          2
#define E_IOCTL_PMU_STOP_CHG            3
#define E_IOCTL_PMU_START_CHG           4
#define E_IOCTL_PMU_IS_EXT_PWR_OVP      5
#define E_IOCTL_PMU_LCD_DIM_CTRL        6
#define E_IOCTL_PMU_CORE_CTL_HIGH       7
#define E_IOCTL_PMU_CORE_CTL_LOW        8
#define E_IOCTL_PMU_TSP_USB_PWR_OFF     9

#define IOCTL_PMU_GET_BATT_LVL          _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_GET_BATT_LVL)
#define IOCTL_PMU_GET_CHG_STATUS        _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_GET_CHG_STATUS)
#define IOCTL_PMU_IS_EXT_PWR            _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_IS_EXT_PWR)
#define IOCTL_PMU_STOP_CHG              _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_STOP_CHG)
#define IOCTL_PMU_START_CHG             _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_START_CHG)
#define IOCTL_PMU_IS_EXT_PWR_OVP        _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_IS_EXT_PWR_OVP)
#define IOCTL_PMU_LCD_DIM_CTRL          _IOW(IOCTL_PMU_MAGIC, E_IOCTL_PMU_LCD_DIM_CTRL, int)
#define IOCTL_PMU_CORE_CTL_HIGH         _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_CORE_CTL_HIGH)
#define IOCTL_PMU_CORE_CTL_LOW          _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_CORE_CTL_LOW)
#define IOCTL_PMU_TSP_USB_PWR_OFF       _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_TSP_USB_PWR_OFF)

#define IOCTL_PMU_MAX_NR                (E_IOCTL_PMU_TSP_USB_PWR_OFF+1)

#ifndef __PMU_ENUM__
#define __PMU_ENUM__
enum
{
    EXT_PWR_UNPLUGGED = 0,
    EXT_PWR_PLUGGED,
    EXT_PWR_NOT_OVP,
    EXT_PWR_OVP,
};

enum
{
    PMU_CHARGING = 0,
    PMU_NOT_CHARGING,
    PMU_FULLY_CHARGED,
};

enum
{
    BATT_LVL_OFF = 0,
    BATT_LVL_WARN,
    BATT_LVL_1,
    BATT_LVL_2,
    BATT_LVL_3,
    BATT_LVL_4,
};
#endif    /* __PMU_ENUM__ */

#endif /* __IOCTL_PMU__H__ */
