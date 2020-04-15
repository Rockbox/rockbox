/*
 *  Simple wrapper for the bspatch entry point.
 */

#ifndef _BSPATCH_H
#define _BSPATCH_H

#ifdef __cplusplus
extern "C" {
#endif

int apply_bspatch(const char *infile, const char *outfile, const char *patchfile);

#ifdef __cplusplus
}
#endif


#endif /* _BSPATCH_H */
