#ifndef __CONFIG_H	// Include me only once
#define __CONFIG_H

// DEBUGF will print in debug mode:
#ifdef DEBUG
#define DEBUGF(...) fprintf (stderr, __VA_ARGS__)
#define DEBUGT(...) fprintf (stdout, __VA_ARGS__)
#else //DEBUG
#define DEBUGF(...)
#endif //DEBUG


#define OS_LINUX // architecture: LINUX, ROCKBOX, WINDOWS
#define ROCKBOX_LITTLE_ENDIAN // we are intel... little-endian


#ifdef ROCKBOX_LITTLE_ENDIAN
#define BE32(_x_) ((( (_x_) & 0xff000000) >> 24) | \
                   (( (_x_) & 0x00ff0000) >> 8) | \
                   (( (_x_) & 0x0000ff00) << 8) | \
                   (( (_x_) & 0x000000ff) << 24))
#define BE16(_x_) ( (( (_x_) & 0xff00) >> 8) | (( (_x_) & 0x00ff) << 8))
#else
#define BE32(_x_) _x_
#define BE16(_x_) _x_
#endif

#include <stdint.h>

#define ERR_NONE 0	// no error
#define ERR_NOTFOUND -1	// entry not found
#define ERR_MALLOC 1	// memory allocation failed
#define ERR_FILE 2	// file operation failed
#define ERR_INVALID 3	// something is invalid
#define ERR_NO_INPLACE_UPDATE 4	// can't update in this place

#include <assert.h>

#endif
