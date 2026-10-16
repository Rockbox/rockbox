
#include <pspdebug.h>
#include <pspdisplay.h>
#include <psptypes.h>
#include <pspctrl.h>
#include <pspmodulemgr.h>
#include <pspthreadman.h>
#include <pspiofilemgr.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("bootloader", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(2048);

// #define DEBUG
#ifndef DEBUG
#define printf pspDebugScreenPrintf
#endif

void wait_button_press(int button)
{
    SceCtrlData ctrl;

    while (1) {
        sceCtrlReadBufferPositive(&ctrl, 1);
        if(ctrl.Buttons & button) {
            break;
        }

        sceDisplayWaitVblankStart();
    }
}

int main(void)
{
    pspDebugScreenInit();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(1);

    SceKernelLMOption option;
    SceUID mpid = PSP_MEMORY_PARTITION_USER;

    SceUID id = -1;
    memset(&option, 0, sizeof(option));

    option.size = sizeof(option);
    option.mpidtext = mpid;
    option.mpiddata = mpid;
    option.position = 0;
    option.access = 1;

    /* look for .rockbox folder */
    SceUID handle = sceIoDopen("ms0:/.rockbox");
    if (handle < 0) {
        printf("error: ms0:/.rockbox folder not found.\n");
        goto error;
    }
    sceIoDclose(handle);

    /* look for firmware module */
    handle = sceIoOpen("ms0:/.rockbox/rockbox.prx", PSP_O_RDONLY, 0777);
    if (handle < 0) {
        printf("error: ms0:/.rockbox/rockbox.prx module not found.\n");
        goto error;
    }
    sceIoClose(handle);

    /* launch firmware module */
    id = sceKernelLoadModule("ms0:/.rockbox/rockbox.prx", 0, &option);
    if (id <= 0) {
        printf("error: could not load firmware module (0x%x).\n", id);
        goto error;
    }

    int fd;
    int result = sceKernelStartModule(id, 0, NULL, &fd, NULL);
    if (result <= 0) {
        printf("error: could not launch firmware module (0x%x).\n", result);
        goto error;
    }

    /* sleep until rockbox exits */
    sceKernelSleepThread();

    return 0;
error:
    printf("Press X button to exit.\n");
    wait_button_press(PSP_CTRL_CROSS);
    return -1;
}
