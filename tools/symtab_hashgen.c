#include <endian.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"

/* byteoreder shuffling functions */
uint32_t (*elf2h32)(uint32_t) = NULL;
uint32_t (*h2elf32)(uint32_t) = NULL;

uint16_t (*elf2h16)(uint16_t) = NULL;
uint16_t (*h2elf16)(uint16_t) = NULL;

/* wrappers as acutal byte conversion
 * is implemented as macro
 */
static uint32_t __le32toh(uint32_t val)
{
    return le32toh(val);
}

static uint32_t __be32toh(uint32_t val)
{
    return be32toh(val);
}

static uint32_t __htole32(uint32_t val)
{
    return htole32(val);
}

static uint32_t __htobe32(uint32_t val)
{
    return htobe32(val);
}

static uint16_t __le16toh(uint16_t val)
{
    return le16toh(val);
}

static uint16_t __be16toh(uint16_t val)
{
    return be16toh(val);
}

static uint16_t __htole16(uint16_t val)
{
    return htole16(val);
}

static uint16_t __htobe16(uint16_t val)
{
    return htobe16(val);
}

static uint32_t str_fnv1a(const char *str)
{
    unsigned char *s = (unsigned char *)str;
    uint32_t hval = 0x811c9dc5;

    while (*s)
    {
        hval ^= (uint32_t)*s++;
        hval *= 0x01000193;
    }

    return hval;
}

struct hashtable_t {
    uint32_t addres;
    uint32_t hash;
} __attribute__((packed));

int hash_cmp(const void *a, const void *b)
{
    uint32_t aval = elf2h32(((struct hashtable_t *)a)->hash);
    uint32_t bval = elf2h32(((struct hashtable_t *)b)->hash);

    if (aval > bval)
        return 1;
    else if (aval < bval)
        return -1;
    else
        return 0;
}

int main(int argc, char **argv)
{
    FILE *fp;
    Elf32_Ehdr *ehdr;
    Elf32_Shdr *shdr;
    int i;

    fp = fopen(argv[1], "r+b");

    if (fp == NULL)
       return -1;

    fseek(fp, 0, SEEK_END);
    long elfsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* alloc memory for the file */
    void *elf = malloc(elfsize);

    if (elf == NULL)
    {
        fclose(fp);
        return -2;
    }

    /* slurp file into memory */
    if (fread(elf, 1, elfsize, fp) < elfsize)
    {
        fclose(fp);
        free(elf);
        return -3;
    }

    printf("[INFO]: loading %s, %ld kB\n", argv[1], elfsize/1024);
    ehdr = (Elf32_Ehdr *)elf;

    if (ehdr->e_ident[EI_DATA] == ELFDATA2LSB)
    {
        /* little endian elf */
        elf2h32 = &__le32toh;
        elf2h16 = &__le16toh;
        h2elf32 = &__htole32;
        h2elf16 = &__htole16;
    }
    else
    {
        /* big endian elf */
        elf2h32 = &__be32toh;
        elf2h16 = &__be16toh;
        h2elf32 = &__htobe32;
        h2elf16 = &__htobe16;
    }

    /*  Check ELF header */
    if ((ehdr->e_ident[EI_MAG0] != ELFMAG0) ||
        (ehdr->e_ident[EI_MAG1] != ELFMAG1) ||
        (ehdr->e_ident[EI_MAG2] != ELFMAG2) ||
        (ehdr->e_ident[EI_MAG3] != ELFMAG3))
    {
        fclose(fp);
        free(elf);
        return -4;
    }

    if (elf2h16(ehdr->e_type) == ET_EXEC)
        printf("[INFO]: elf executable\n");
    else if (elf2h16(ehdr->e_type) == ET_REL)
        printf("[INFO]: relocatable elf\n");
    else
    {
        printf("[ERR]: unsuported elf type\n");
        fclose(fp);
        free(elf);
        return -5;
    }

    /* Section headers string table */
    Elf32_Shdr *shstr = (Elf32_Shdr *)(elf + elf2h32(ehdr->e_shoff)) + elf2h16(ehdr->e_shstrndx);

    /* we need to find .symtab_hashtable for patching
     * and .symtab_str for strings
     */
    int hashtable_size = 0;
    struct hashtable_t *hashtable = NULL;
    int stringtable_size = 0;
    char *stringtable = NULL;
    for (i=0; i<(int)elf2h16(ehdr->e_shnum); i++)
    {
        shdr = (Elf32_Shdr *)(elf + elf2h32(ehdr->e_shoff)) + i;
        char *name = (char *)(elf + elf2h32(shstr->sh_offset) + elf2h32(shdr->sh_name));
        //printf("section[%d]: %s\n", i, name);

        if (strcmp(name, ".symtab_hashtable") == 0)
        {
            printf("[INFO]: Found symbol hashtable to patch in section[%d]\n", i);
            hashtable = (struct hashtable_t *)(elf + elf2h32(shdr->sh_offset));
            hashtable_size = elf2h32(shdr->sh_size);
        }
        else if (strcmp(name, ".symtab_str") == 0)
        {
            printf("[INFO]: Found exported symbols string table in section[%d]\n", i);
            stringtable = elf + elf2h32(shdr->sh_offset);
            stringtable_size = elf2h32(shdr->sh_size);
        }
    }

    if (hashtable_size == 0 || hashtable == NULL ||
        stringtable_size == 0 || stringtable == NULL)
    {
        fclose(fp);
        free(elf);
        return -6;
    }

    struct hashtable_t *htp = hashtable;
    char *stp = stringtable;
    while(stringtable_size)
    {
        uint32_t hash = str_fnv1a(stp);
        htp->hash = h2elf32(hash);
        htp++;
        stringtable_size -= strlen(stp) + 1;
        stp += strlen(stp) + 1;
    }

    printf("[INFO]: unsorted hashtable\n");
    printf("        addres      hash        symbol\n");
    htp = hashtable;
    stp = stringtable;
    for (i=0; i<hashtable_size/sizeof(struct hashtable_t); i++)
    {
        printf("        0x%08x  0x%08x  %s\n", htp->addres, htp->hash, stp);
        stp += strlen(stp) + 1;
        htp++;
    }

    printf("[INFO]: sorting hashtable %lu elem\n", hashtable_size/sizeof(struct hashtable_t));
    qsort((void *)hashtable,
          hashtable_size/sizeof(struct hashtable_t),
          sizeof(struct hashtable_t),
          hash_cmp);

    printf("[INFO]: writing elf back\n");
    fseek(fp, 0, SEEK_SET);

    if (fwrite(elf, 1, elfsize, fp) < elfsize)
        printf("[INFO]: elf writeback failed\n");
    
    fclose(fp);
    free(elf);
    return 0;
}
