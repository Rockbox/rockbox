#pragma once
#include "../platform-macros.h"

#if !defined(IAP_LOGF)
#define IAP_LOGF_MUTED
#define IAP_LOGF(...)
#endif
#define print(fmt, ...) IAP_LOGF("%s:%d: " fmt, __func__, __LINE__ __VA_OPT__(, __VA_ARGS__));

#if !defined(IAP_ERRORF)
#define IAP_ERRORF_MUTED
#define IAP_ERRORF(...)
#endif
#define warn(fmt, ...)  IAP_ERRORF("%s:%d: " fmt, __func__, __LINE__ __VA_OPT__(, __VA_ARGS__));

#define check_act(cond, act, ...)                              \
    if(!(cond)) {                                              \
        warn("assertion failed" __VA_OPT__(": " __VA_ARGS__)); \
        act;                                                   \
    }

#define check_ret(cond, ret, ...) check_act(cond, return ret, __VA_ARGS__)

#define array_size(arr) (sizeof(arr) / sizeof(arr[0]))

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)
