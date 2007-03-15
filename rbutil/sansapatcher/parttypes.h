/* DOS partition types - taken from fdisk */

struct parttype {
        unsigned char type;
        char *name;
};

struct parttype parttypes[] = {
	{0x00, "Empty"},
	{0x01, "FAT12"},
	{0x02, "XENIX root"},
	{0x03, "XENIX usr"},
	{0x04, "FAT16 <32M"},
	{0x05, "Extended"},		/* DOS 3.3+ extended partition */
	{0x06, "FAT16"},		/* DOS 16-bit >=32M */
	{0x07, "HPFS/NTFS"},	/* OS/2 IFS, eg, HPFS or NTFS or QNX */
	{0x08, "AIX"},		/* AIX boot (AIX -- PS/2 port) or SplitDrive */
	{0x09, "AIX bootable"},	/* AIX data or Coherent */
	{0x0a, "OS/2 Boot Manager"},/* OS/2 Boot Manager */
	{0x0b, "W95 FAT32"},
	{0x0c, "W95 FAT32 (LBA)"},/* LBA really is `Extended Int 13h' */
	{0x0e, "W95 FAT16 (LBA)"},
	{0x0f, "W95 Ext'd (LBA)"},
	{0x10, "OPUS"},
	{0x11, "Hidden FAT12"},
	{0x12, "Compaq diagnostics"},
	{0x14, "Hidden FAT16 <32M"},
	{0x16, "Hidden FAT16"},
	{0x17, "Hidden HPFS/NTFS"},
	{0x18, "AST SmartSleep"},
	{0x1b, "Hidden W95 FAT32"},
	{0x1c, "Hidden W95 FAT32 (LBA)"},
	{0x1e, "Hidden W95 FAT16 (LBA)"},
	{0x24, "NEC DOS"},
	{0x39, "Plan 9"},
	{0x3c, "PartitionMagic recovery"},
	{0x40, "Venix 80286"},
	{0x41, "PPC PReP Boot"},
	{0x42, "SFS"},
	{0x4d, "QNX4.x"},
	{0x4e, "QNX4.x 2nd part"},
	{0x4f, "QNX4.x 3rd part"},
	{0x50, "OnTrack DM"},
	{0x51, "OnTrack DM6 Aux1"},	/* (or Novell) */
	{0x52, "CP/M"},		/* CP/M or Microport SysV/AT */
	{0x53, "OnTrack DM6 Aux3"},
	{0x54, "OnTrackDM6"},
	{0x55, "EZ-Drive"},
	{0x56, "Golden Bow"},
	{0x5c, "Priam Edisk"},
	{0x61, "SpeedStor"},
	{0x63, "GNU HURD or SysV"},	/* GNU HURD or Mach or Sys V/386 (such as ISC UNIX) */
	{0x64, "Novell Netware 286"},
	{0x65, "Novell Netware 386"},
	{0x70, "DiskSecure Multi-Boot"},
	{0x75, "PC/IX"},
	{0x80, "Old Minix"},	/* Minix 1.4a and earlier */
	{0x81, "Minix / old Linux"},/* Minix 1.4b and later */
	{0x82, "Linux swap / Solaris"},
	{0x83, "Linux"},
	{0x84, "OS/2 hidden C: drive"},
	{0x85, "Linux extended"},
	{0x86, "NTFS volume set"},
	{0x87, "NTFS volume set"},
	{0x88, "Linux plaintext"},
	{0x8e, "Linux LVM"},
	{0x93, "Amoeba"},
	{0x94, "Amoeba BBT"},	/* (bad block table) */
	{0x9f, "BSD/OS"},		/* BSDI */
	{0xa0, "IBM Thinkpad hibernation"},
	{0xa5, "FreeBSD"},		/* various BSD flavours */
	{0xa6, "OpenBSD"},
	{0xa7, "NeXTSTEP"},
	{0xa8, "Darwin UFS"},
	{0xa9, "NetBSD"},
	{0xab, "Darwin boot"},
	{0xb7, "BSDI fs"},
	{0xb8, "BSDI swap"},
	{0xbb, "Boot Wizard hidden"},
	{0xbe, "Solaris boot"},
	{0xbf, "Solaris"},
	{0xc1, "DRDOS/sec (FAT-12)"},
	{0xc4, "DRDOS/sec (FAT-16 < 32M)"},
	{0xc6, "DRDOS/sec (FAT-16)"},
	{0xc7, "Syrinx"},
	{0xda, "Non-FS data"},
	{0xdb, "CP/M / CTOS / ..."},/* CP/M or Concurrent CP/M or
					   Concurrent DOS or CTOS */
	{0xde, "Dell Utility"},	/* Dell PowerEdge Server utilities */
	{0xdf, "BootIt"},		/* BootIt EMBRM */
	{0xe1, "DOS access"},	/* DOS access or SpeedStor 12-bit FAT
					   extended partition */
	{0xe3, "DOS R/O"},		/* DOS R/O or SpeedStor */
	{0xe4, "SpeedStor"},	/* SpeedStor 16-bit FAT extended
					   partition < 1024 cyl. */
	{0xeb, "BeOS fs"},
	{0xee, "EFI GPT"},		/* Intel EFI GUID Partition Table */
	{0xef, "EFI (FAT-12/16/32)"},/* Intel EFI System Partition */
	{0xf0, "Linux/PA-RISC boot"},/* Linux/PA-RISC boot loader */
	{0xf1, "SpeedStor"},
	{0xf4, "SpeedStor"},	/* SpeedStor large partition */
	{0xf2, "DOS secondary"},	/* DOS 3.3+ secondary */
	{0xfd, "Linux raid autodetect"},/* New (2.2.x) raid partition with
					       autodetect using persistent
					       superblock */
	{0xfe, "LANstep"},		/* SpeedStor >1024 cyl. or LANstep */
	{0xff, "BBT"},		/* Xenix Bad Block Table */
	{ 0, 0 }
};
