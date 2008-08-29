#ifndef DEFS_H_INCLUDED
#define DEFS_H_INCLUDED

typedef int (*pfdebugf)(const char* fmt,...);
extern pfdebugf dbgf;

#ifdef BUILD_DLL
#   define EXPORT __declspec(dllexport)
#else
#   define EXPORT
#endif

#ifndef MIN
# define MIN(a, b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
# define MAX(a, b) (((a)>(b))?(a):(b))
#endif

#define SWAP_16(x) ((typeof(x))(unsigned short)(((unsigned short)(x) >> 8) | \
                                                ((unsigned short)(x) << 8)))

#define SWAP_32(x) ((typeof(x))(unsigned long)( ((unsigned long)(x) >> 24) | \
                                               (((unsigned long)(x) & 0xff0000ul) >> 8) | \
                                               (((unsigned long)(x) & 0xff00ul) << 8) | \
                                                ((unsigned long)(x) << 24)))

#define PLAYMODES_NUM 5
enum api_playmode {
    API_STATUS_PLAY,
    API_STATUS_STOP,
    API_STATUS_PAUSE,
    API_STATUS_FASTFORWARD,
    API_STATUS_FASTBACKWARD
};

extern enum api_playmode playmodes[PLAYMODES_NUM];
extern const char *playmodeNames[];

#endif // DEFS_H_INCLUDED
