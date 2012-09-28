#ifndef __ASSEMBLER__
struct symtab_item_t
{
   void *addr;
   const char *name;
};

#define EXPORT_SYMBOL(sym) \
   extern typeof(sym) sym; \
   static const char __strtab_##sym[] __attribute__((section("__symtab_str" "." #sym), aligned(1))) = #sym; \
   static const struct symtab_item_t __symtab_##sym __attribute__((section("__symtab" "." #sym), used))     \
   = { (void *)&sym, __strtab_##sym }

#else
.macro gas_export_symbol sym
    .section __symtab_str.\sym
__strtab_\sym:
    .asciz "\sym"
    .section __symtab.\sym
    .word \sym
    .word __strtab_\sym
.endm
#define EXPORT_SYMBOL(sym) gas_export_symbol sym
#endif
