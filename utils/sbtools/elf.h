#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Definitions
 *   taken from elf.h linux header
 *   based on ELF specification
 *   based on ARM ELF specification
 */
typedef uint16_t Elf32_Half;

typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Section;

#define EI_NIDENT   16

typedef struct
{
    unsigned char   e_ident[EI_NIDENT]; /* Magic number and other info */
    Elf32_Half      e_type; /* Object file type */
    Elf32_Half      e_machine; /* Architecture */
    Elf32_Word      e_version; /* Object file version */
    Elf32_Addr      e_entry; /* Entry point virtual address */
    Elf32_Off       e_phoff; /* Program header table file offset */
    Elf32_Off       e_shoff; /* Section header table file offset */
    Elf32_Word      e_flags; /* Processor-specific flags */
    Elf32_Half      e_ehsize; /* ELF header size in bytes */
    Elf32_Half      e_phentsize; /* Program header table entry size */
    Elf32_Half      e_phnum; /* Program header table entry count */
    Elf32_Half      e_shentsize; /* Section header table entry size */
    Elf32_Half      e_shnum; /* Section header table entry count */
    Elf32_Half      e_shstrndx; /* Section header string table index */
}Elf32_Ehdr;

#define EI_MAG0     0 /* File identification byte 0 index */
#define ELFMAG0     0x7f /* Magic number byte 0 */

#define EI_MAG1     1 /* File identification byte 1 index */
#define ELFMAG1     'E' /* Magic number byte 1 */

#define EI_MAG2     2 /* File identification byte 2 index */
#define ELFMAG2     'L' /* Magic number byte 2 */

#define EI_MAG3     3 /* File identification byte 3 index */
#define ELFMAG3     'F' /* Magic number byte 3 */

#define EI_CLASS    4 /* File class byte index */
#define ELFCLASS32  1 /* 32-bit objects */

#define EI_DATA     5 /* Data encoding byte index */
#define ELFDATA2LSB 1 /* 2's complement, little endian */

#define EI_VERSION  6 /* File version byte index, Value must be EV_CURRENT */

#define EI_OSABI    7 /* OS ABI identification */
#define ELFOSABI_NONE       0 /* UNIX System V ABI */
#define ELFOSABI_ARM_AEABI  64 /* ARM EABI */
#define ELFOSABI_ARM        97 /* ARM */

#define EI_ABIVERSION   8 /* ABI version */

#define EI_PAD      9 /* Byte index of padding bytes */

#define ET_EXEC     2 /* Executable file */

#define EM_ARM      40 /* ARM */

#define EV_CURRENT  1 /* Current version */

#define EF_ARM_HASENTRY     0x00000002

#define SHN_UNDEF   0 /* Undefined section */

typedef struct
{
    Elf32_Word  sh_name; /* Section name (string tbl index) */
    Elf32_Word  sh_type; /* Section type */
    Elf32_Word  sh_flags; /* Section flags */
    Elf32_Addr  sh_addr; /* Section virtual addr at execution */
    Elf32_Off   sh_offset; /* Section file offset */
    Elf32_Word  sh_size; /* Section size in bytes */
    Elf32_Word  sh_link; /* Link to another section */
    Elf32_Word  sh_info; /* Additional section information */
    Elf32_Word  sh_addralign; /* Section alignment */
    Elf32_Word  sh_entsize; /* Entry size if section holds table */
}Elf32_Shdr;

#define SHT_NULL            0 /* Section header table entry unused */
#define SHT_PROGBITS        1 /* Program data */
#define SHT_SYMTAB          2 /* Symbol table */
#define SHT_STRTAB          3 /* String table */
#define SHT_RELA            4 /* Relocation entries with addends */
#define SHT_HASH            5 /* Symbol hash table */
#define SHT_DYNAMIC         6 /* Dynamic linking information */
#define SHT_NOTE            7 /* Notes */
#define SHT_NOBITS          8 /* Program space with no data (bss) */
#define SHT_REL             9 /* Relocation entries, no addends */
#define SHT_SHLIB           10 /* Reserved */
#define SHT_DYNSYM          11 /* Dynamic linker symbol table */
#define SHT_INIT_ARRAY      14 /* Array of constructors */
#define SHT_FINI_ARRAY      15 /* Array of destructors */
#define SHT_PREINIT_ARRAY   16 /* Array of pre-constructors */
#define SHT_GROUP           17 /* Section group */
#define SHT_SYMTAB_SHNDX    18 /* Extended section indeces */
#define	SHT_NUM             19 /* Number of defined types.  */

#define SHF_WRITE       (1 << 0) /* Writable */
#define SHF_ALLOC       (1 << 1) /* Occupies memory during execution */
#define SHF_EXECINSTR   (1 << 2) /* Executable */
#define SHF_MERGE       (1 << 4) /* Might be merged */
#define SHF_STRINGS     (1 << 5) /* Contains nul-terminated strings */

typedef struct
{
    Elf32_Word  p_type; /* Segment type */
    Elf32_Off   p_offset; /* Segment file offset */
    Elf32_Addr  p_vaddr; /* Segment virtual address */
    Elf32_Addr  p_paddr; /* Segment physical address */
    Elf32_Word  p_filesz; /* Segment size in file */
    Elf32_Word  p_memsz; /* Segment size in memory */
    Elf32_Word  p_flags; /* Segment flags */
    Elf32_Word  p_align; /* Segment alignment */
}Elf32_Phdr;

#define PT_LOAD     1 /* Loadable program segment */

#define PF_X    (1 << 0) /* Segment is executable */
#define PF_W    (1 << 1) /* Segment is writable */
#define PF_R    (1 << 2) /* Segment is readable */

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
    /* Internal to elf_output */
    uint32_t offset;
};

struct elf_params_t
{
    bool has_start_addr;
    uint32_t start_addr;
    struct elf_section_t *first_section;
    struct elf_section_t *last_section;
};

typedef void (*elf_write_fn_t)(void *user, uint32_t addr, const void *buf, size_t count);

void elf_init(struct elf_params_t *params);
void elf_add_load_section(struct elf_params_t *params,
    uint32_t load_addr, uint32_t size, const void *section);
void elf_add_fill_section(struct elf_params_t *params,
    uint32_t fill_addr, uint32_t size, uint32_t pattern);
void elf_output(struct elf_params_t *params, elf_write_fn_t write, void *user);
bool elf_is_empty(struct elf_params_t *params);
void elf_set_start_addr(struct elf_params_t *params, uint32_t addr);
void elf_release(struct elf_params_t *params);
