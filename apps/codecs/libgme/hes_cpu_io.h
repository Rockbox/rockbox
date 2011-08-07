
#include "hes_emu.h"

#include "blargg_source.h"

int Cpu_read( struct Hes_Emu* this, hes_addr_t addr )
{
	check( addr <= 0xFFFF );
	int result = *Cpu_get_code( &this->cpu, addr );
	if ( this->cpu.mmr [addr >> page_shift] == 0xFF )
		result = Emu_cpu_read( this, addr );
	return result;
}

void Cpu_write( struct Hes_Emu* this, hes_addr_t addr, int data )
{
	check( addr <= 0xFFFF );
	byte* out = this->write_pages [addr >> page_shift];
	addr &= page_size - 1;
	if ( out )
		out [addr] = data;
	else if ( this->cpu.mmr [addr >> page_shift] == 0xFF )
		Emu_cpu_write( this, addr, data );
}

#define CPU_READ_FAST( emu, addr, time, out ) \
	CPU_READ_FAST_( emu, addr, time, out )

#define CPU_READ_FAST_( emu, addr, time, out ) \
{\
	out = READ_PROG( addr );\
	if ( emu->cpu.mmr [addr >> page_shift] == 0xFF )\
	{\
		FLUSH_TIME();\
		out = Emu_cpu_read( emu, addr );\
		CACHE_TIME();\
	}\
}

#define CPU_WRITE_FAST( emu, addr, data, time ) \
	CPU_WRITE_FAST_( emu, addr, data, time )

#define CPU_WRITE_FAST_( emu, addr, data, time ) \
{\
	byte* out = emu->write_pages [addr >> page_shift];\
	addr &= page_size - 1;\
	if ( out )\
	{\
		out [addr] = data;\
	}\
	else if ( emu->cpu.mmr [addr >> page_shift] == 0xFF )\
	{\
		FLUSH_TIME();\
		Emu_cpu_write( emu, addr, data );\
		CACHE_TIME();\
	}\
}

#define CPU_READ( emu, addr, time ) \
	Cpu_read( emu, addr )

#define CPU_WRITE( emu, addr, data, time ) \
	Cpu_write( emu, addr, data )

#define CPU_WRITE_VDP( emu, addr, data, time ) \
	Cpu_write_vdp( emu, addr, data )

#define CPU_SET_MMR( emu, page, bank ) \
	Emu_cpu_set_mmr( emu, page, bank )

#define CPU_DONE( emu, time, result_out ) \
	result_out = Cpu_done( emu )
