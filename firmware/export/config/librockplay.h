#define CONFIG_CODEC SWCODEC
#define CONFIG_PLATFORM PLATFORM_HOSTED
#define CONFIG_STORAGE STORAGE_HOSTFS

#define HAVE_SW_VOLUME_CONTROL
//~ #define PCM_SW_VOLUME_UNBUFFERED /* pcm driver itself is buffered */
/* For app, use fractional-only setup for -79..+0, no large-integer math */
#define PCM_SW_VOLUME_FRACBITS  (16)

#define USB_NONE

#define HAVE_CORELOCK_OBJECT
