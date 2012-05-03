#ifndef RBCODECCONFIG_H_INCLUDED
#define RBCODECCONFIG_H_INCLUDED

#include "config.h"

#ifndef __ASSEMBLER__

/* NULL, offsetof, size_t */
#include <stddef.h>

/* ssize_t, off_t, open, close, read, lseek, SEEK_SET, SEEK_CUR, SEEK_END,
 * O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, MAX_PATH, filesize */
#include "file.h"

/* {,u}int{8,16,32,64}_t, , intptr_t, uintptr_t, bool, true, false, swap16,
 * swap32, hto{be,le}{16,32}, {be,le}toh{16,32}, ROCKBOX_{BIG,LITTLE}_ENDIAN,
 * {,U}INT{8,16,32,64}_{MIN,MAX} */
#include "system.h"

/* Structure to record some info during processing call */
struct dsp_loop_context
{
    long last_yield;
#ifdef CPU_COLDFIRE
    unsigned long old_macsr;
#endif
};

static inline void dsp_process_start(struct dsp_loop_context *ctx)
{
    /* At least perform one yield before starting */
    ctx->last_yield = current_tick;
    yield();
#if defined(CPU_COLDFIRE)
    /* set emac unit for dsp processing, and save old macsr, we're running in
       codec thread context at this point, so can't clobber it */
    ctx->old_macsr = coldfire_get_macsr();
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
#endif
}

static inline void dsp_process_loop(struct dsp_loop_context *ctx)
{
    /* Yield at least once each tick */
    long tick = current_tick;
    if (TIME_AFTER(tick, ctx->last_yield))
    {
        ctx->last_yield = tick;
        yield();
    }
}

static inline void dsp_process_end(struct dsp_loop_context *ctx)
{
#if defined(CPU_COLDFIRE)
    /* set old macsr again */
    coldfire_set_macsr(ctx->old_macsr);
#endif
    (void)ctx;
}

#define DSP_PROCESS_START() \
    struct dsp_loop_context __ctx; \
    dsp_process_start(&__ctx)

#define DSP_PROCESS_LOOP() \
    dsp_process_loop(&__ctx)

#define DSP_PROCESS_END() \
    dsp_process_end(&__ctx)

#endif

#endif
