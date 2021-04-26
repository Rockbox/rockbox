/* Standalone version of ucl_nrv2e_decompress_8 from UCL library
 * Copyright (C) 1996-2002 Markus Franz Xaver Johannes Oberhumer
 * All Rights Reserved.
 *
 * See firmware/common/ucl_nrv2e_decompress.c for full copyright notice
 */
#ifndef UCL_NRV2E_DECOMPRESS_H
#define UCL_NRV2E_DECOMPRESS_H

#include <stdint.h>
#include <stdlib.h>

#define UCL_E_OK                 0
#define UCL_E_INPUT_OVERRUN      1
#define UCL_E_OUTPUT_OVERRUN     2
#define UCL_E_LOOKBEHIND_OVERRUN 3
#define UCL_E_INPUT_NOT_CONSUMED 4

extern int ucl_nrv2e_decompress_8(const uint8_t* src, uint32_t src_len,
                                  uint8_t* dst, uint32_t* dst_len);

#endif /* UCL_NRV2E_DECOMPRESS_H */
