/* For definition of the cos_table[] look into cos_table.c */
#ifdef ROCKBOX
#define ILOGCOSTABSIZE 13
#else /* ROCKBOX */
#define ILOGCOSTABSIZE 15
#endif /* ROCKBOX */
#define ICOSTABSIZE (1<<ILOGCOSTABSIZE)
extern t_sample cos_table[];
