/* Declarations for ftl_hooks.c; force-included into every test. */
#ifndef FTL_HOOKS_H
#define FTL_HOOKS_H
#include <stdint.h>
int ftl_vfl_pool_entry(uint32_t bank, uint32_t block);
int ftl_sb_locate(uint32_t sb, uint32_t v, uint32_t *bank, uint32_t *page);
int ftl_locate(uint32_t lpn, uint32_t *bank, uint32_t *page);
void ftl_global_spread(uint32_t *lowest, uint32_t *highest);
uint32_t ftl_pool_snapshot(uint16_t *sb, uint16_t *erasectr, uint32_t max);
void ftl_pool_state(uint32_t *freecount, uint32_t *fresh, uint32_t *nlogs);
int ftl_hook_write_merge(uint32_t first, uint32_t last,
                         const void *(*src)(uint32_t lpn, void *ctx),
                         void *ctx);
_Bool ftl_hook_writeable(void);
void ftl_hook_ctrl(uint16_t *sb);
#include <stddef.h>
int ftl_hook_check(char *msg, size_t len);
void ftl_hook_ctrl_summary(void);
int ftl_hook_settle(void);
int ftl_hook_format(void);
int ftl_hook_open(const char *spec);
int ftl_hook_block_unit(uint32_t block);
int ftl_hook_bad_spare_then_remap(uint32_t bank, uint32_t vblock);
int ftl_hook_pending_erase(uint32_t bank, uint32_t unit, uint32_t vblock,
                           _Bool *moved);
#endif
