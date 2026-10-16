#include <pspmoduleexport.h>
#define NULL ((void *) 0)

extern int module_start;
extern int module_info;
static const unsigned int __syslib_exports[4] __attribute__((section(".rodata.sceResident"))) = {
	0xD632ACDB,
	0xF01D73A7,
	(unsigned int) &module_start,
	(unsigned int) &module_info,
};

extern int plugin_get_header;
static const unsigned int __pluginlib_exports[2] __attribute__((section(".rodata.sceResident"))) = {
	0xF9B12673,
	(unsigned int) &plugin_get_header,
};

const struct _PspLibraryEntry __library_exports[2] __attribute__((section(".lib.ent"), used)) = {
	{ NULL, 0x0000, 0x8000, 4, 1, 1, (unsigned int *) &__syslib_exports },
	{ "pluginlib", 0x0000, 0x0001, 4, 0, 1, (unsigned int *) &__pluginlib_exports },
};
