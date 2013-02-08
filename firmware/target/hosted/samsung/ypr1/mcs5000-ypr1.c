#include <unistd.h>
#include <fcntl.h>
#include "sys/ioctl.h"

#include "mcs5000.h"

/* TODO Settings like hand and sensitivity will be lost when shutting device off!! */

static int mcs5000_dev = -1;

void mcs5000_init(void) {
    mcs5000_dev = open("/dev/r1Touch", O_RDONLY);
}

void mcs5000_close(void) {
    if (mcs5000_dev > 0) {
        close(mcs5000_dev);
    }
}

void mcs5000_power(void) {
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_ON);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_IDLE);
}

void mcs5000_shutdown(void) {
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_FLUSH);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_RESET);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_OFF);
}

void mcs5000_set_hand(HandSetting handSetting) {
    switch (handSetting) {
        case RIGHT_HAND:
            ioctl(mcs5000_dev, DEV_CTRL_TOUCH_RIGHTHAND);
            break;
        case LEFT_HAND:
            ioctl(mcs5000_dev, DEV_CTRL_TOUCH_LEFTHAND);
            break;
    }
}

void mcs5000_set_sensitivity(int level) {
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_SET_SENSE, &level);
}

int mcs5000_read(TsRawData *touchData) {
    return read(mcs5000_dev, touchData, sizeof(TsRawData));
}
