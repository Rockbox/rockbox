#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * API
 */
enum elf_section_type_t
{
    EST_LOAD,
    EST_FILL
};

struct elf_section_t
{
    uint32_t addr;
    uint32_t size;
    enum elf_section_type_t type;
    /* <union> */
    void *section;
    uint32_t pattern;
    /* </union> */
    struct elf_section_t *next;
    /* Internal to elf_write_file */
    uint32_t offset;
};

struct elf_params_t
{
    bool has_start_addr;
    uint32_t start_addr;
    struct elf_section_t *first_section;
    struct elf_section_t *last_section;
};

typedef bool (*elf_read_fn_t)(void *user, uint32_t addr, void *buf, size_t count);
/* write function manages it's own error state */
typedef void (*elf_write_fn_t)(void *user, uint32_t addr, const void *buf, size_t count);
typedef void (*elf_printf_fn_t)(void *user, bool error, const char *fmt, ...);

void elf_init(struct elf_params_t *params);
void elf_add_load_section(struct elf_params_t *params,
    uint32_t load_addr, uint32_t size, const void *section);
void elf_add_fill_section(struct elf_params_t *params,
    uint32_t fill_addr, uint32_t size, uint32_t pattern);
void elf_write_file(struct elf_params_t *params, elf_write_fn_t write, void *user);
bool elf_read_file(struct elf_params_t *params, elf_read_fn_t read, elf_printf_fn_t printf,
    void *user);
bool elf_is_empty(struct elf_params_t *params);
void elf_set_start_addr(struct elf_params_t *params, uint32_t addr);
bool elf_get_start_addr(struct elf_params_t *params, uint32_t *addr);
int elf_get_nr_sections(struct elf_params_t *params);
void elf_release(struct elf_params_t *params);
