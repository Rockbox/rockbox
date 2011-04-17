#include "elf.h"

void elf_init(struct elf_params_t *params)
{
    params->has_start_addr = false;
    params->start_addr = 0;
    params->first_section = NULL;
    params->last_section = NULL;
}

extern void *xmalloc(size_t s);

static struct elf_section_t *elf_add_section(struct elf_params_t *params)
{
    struct elf_section_t *sec = xmalloc(sizeof(struct elf_section_t));
    if(params->first_section == NULL)
        params->first_section = params->last_section = sec;
    else
    {
        params->last_section->next = sec;
        params->last_section = sec;
    }
    sec->next = NULL;

    return sec;
}

void elf_add_load_section(struct elf_params_t *params,
    uint32_t load_addr, uint32_t size, const void *section)
{
    struct elf_section_t *sec = elf_add_section(params);

    sec->type = EST_LOAD;
    sec->addr = load_addr;
    sec->size = size;
    sec->section = xmalloc(size);
    memcpy(sec->section, section, size);
}

void elf_add_fill_section(struct elf_params_t *params,
    uint32_t fill_addr, uint32_t size, uint32_t pattern)
{
    if(pattern != 0x00)
    {
        printf("oops, non-zero filling, ignore fill section\n");
        return;
    }
    
    struct elf_section_t *sec = elf_add_section(params);
    
    sec->type = EST_FILL;
    sec->addr = fill_addr;
    sec->size = size;
    sec->pattern = pattern;
}

void elf_output(struct elf_params_t *params, elf_write_fn_t write, void *user)
{
    Elf32_Ehdr ehdr;
    uint32_t phnum = 0;
    struct elf_section_t *sec = params->first_section;
    uint32_t offset = 0;
    Elf32_Phdr phdr;
    Elf32_Shdr shdr;
    memset(&ehdr, 0, EI_NIDENT);

    uint32_t bss_strtbl = 0;
    uint32_t text_strtbl = bss_strtbl + strlen(".bss") + 1;
    uint32_t strtbl_size = text_strtbl + strlen(".text") + 1;

    while(sec)
    {
        if(sec->type == EST_LOAD)
        {
            sec->offset = offset;
            offset += sec->size;
        }
        
        phnum++;
        sec = sec->next;
    }

    uint32_t strtbl_offset = offset;

    ehdr.e_ident[EI_MAG0] = ELFMAG0;
    ehdr.e_ident[EI_MAG1] = ELFMAG1;
    ehdr.e_ident[EI_MAG2] = ELFMAG2;
    ehdr.e_ident[EI_MAG3] = ELFMAG3;
    ehdr.e_ident[EI_CLASS] = ELFCLASS32;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;
    ehdr.e_ident[EI_OSABI] = ELFOSABI_NONE;
    ehdr.e_ident[EI_ABIVERSION] = 0;
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_ARM;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = params->start_addr;
    ehdr.e_flags = 0;
    if(params->has_start_addr)
        ehdr.e_flags |= EF_ARM_HASENTRY;
    ehdr.e_ehsize = sizeof ehdr;
    ehdr.e_phentsize = sizeof phdr;
    ehdr.e_phnum = phnum;
    ehdr.e_shentsize = sizeof shdr;
    ehdr.e_shnum = phnum + 2; /* one for section 0 and one for string table */
    ehdr.e_shstrndx = ehdr.e_shnum - 1;
    ehdr.e_phoff = ehdr.e_ehsize;
    ehdr.e_shoff = ehdr.e_ehsize + ehdr.e_phnum * ehdr.e_phentsize;

    write(user, 0, &ehdr, sizeof ehdr);

    uint32_t data_offset = ehdr.e_ehsize + ehdr.e_phnum * ehdr.e_phentsize +
        ehdr.e_shnum * ehdr.e_shentsize;

    sec = params->first_section;
    offset = ehdr.e_phoff;
    while(sec)
    {
        sec->offset += data_offset;
        
        phdr.p_type = PT_LOAD;
        if(sec->type == EST_LOAD)
            phdr.p_offset = sec->offset;
        else
            phdr.p_offset = 0;
        phdr.p_paddr = sec->addr;
        phdr.p_vaddr = phdr.p_paddr; /* assume identity map ? */
        phdr.p_memsz = sec->size;
        if(sec->type == EST_LOAD)
            phdr.p_filesz = phdr.p_memsz;
        else
            phdr.p_filesz = 0;
        phdr.p_flags = PF_X | PF_W | PF_R;
        phdr.p_align = 0;

        write(user, offset, &phdr, sizeof phdr);

        offset += sizeof(Elf32_Phdr);
        sec = sec->next;
    }

    sec = params->first_section;
    offset = ehdr.e_shoff;

    {
        shdr.sh_name = 0;
        shdr.sh_type = SHT_NULL;
        shdr.sh_flags = 0;
        shdr.sh_addr = 0;
        shdr.sh_offset = 0;
        shdr.sh_size = 0;
        shdr.sh_link = SHN_UNDEF;
        shdr.sh_info = 0;
        shdr.sh_addralign = 0;
        shdr.sh_entsize = 0;

        write(user, offset, &shdr, sizeof shdr);

        offset += sizeof(Elf32_Shdr);
    }
    
    while(sec)
    {
        shdr.sh_name = text_strtbl;
        if(sec->type == EST_LOAD)
            shdr.sh_type = SHT_PROGBITS;
        else
            shdr.sh_type = SHT_NOBITS;
        shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        shdr.sh_addr = sec->addr;
        shdr.sh_offset = sec->offset;
        shdr.sh_size = sec->size;
        shdr.sh_link = SHN_UNDEF;
        shdr.sh_info = 0;
        shdr.sh_addralign = 1;
        shdr.sh_entsize = 0;

        write(user, offset, &shdr, sizeof shdr);

        offset += sizeof(Elf32_Shdr);
        sec = sec->next;
    }

    {
        shdr.sh_name = bss_strtbl;
        shdr.sh_type = SHT_STRTAB;
        shdr.sh_flags = 0;
        shdr.sh_addr = 0;
        shdr.sh_offset = strtbl_offset + data_offset;
        shdr.sh_size = strtbl_size;
        shdr.sh_link = SHN_UNDEF;
        shdr.sh_info = 0;
        shdr.sh_addralign = 1;
        shdr.sh_entsize = 0;

        write(user, offset, &shdr, sizeof shdr);

        offset += sizeof(Elf32_Shdr);
    }

    sec = params->first_section;
    while(sec)
    {
        if(sec->type == EST_LOAD)
            write(user, sec->offset, sec->section, sec->size);
        sec = sec->next;
    }

    write(user, strtbl_offset + data_offset, ".bss\0.text\0", strtbl_size);
}

bool elf_is_empty(struct elf_params_t *params)
{
    return params->first_section == NULL;
}

void elf_set_start_addr(struct elf_params_t *params, uint32_t addr)
{
    params->has_start_addr = true;
    params->start_addr = addr;
}

void elf_release(struct elf_params_t *params)
{
    struct elf_section_t *sec, *next_sec;
    sec = params->first_section;
    while(sec)
    {
        next_sec = sec->next;
        if(sec->type == EST_LOAD)
            free(sec->section);
        free(sec);
        sec = next_sec;
    }
    params->first_section = NULL;
    params->last_section = NULL;
}
