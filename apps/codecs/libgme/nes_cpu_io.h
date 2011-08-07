
#include "nsf_emu.h"

#ifndef NSF_EMU_APU_ONLY
	#include "nes_namco_apu.h"
	#include "nes_fds_apu.h"
	#include "nes_mmc5_apu.h"
#endif

#include "blargg_source.h"

int Cpu_read( struct Nsf_Emu* this, nes_addr_t addr )
{
	int result = this->cpu.low_mem [addr & 0x7FF];
	if ( addr & 0xE000 )
	{
		result = *Cpu_get_code( &this->cpu, addr );
		if ( addr < sram_addr ) 
		{
			if ( addr == status_addr )
				result = Apu_read_status( &this->apu, Cpu_time( &this->cpu ) );
			else 
			{
				#ifndef NSF_EMU_APU_ONLY
					if ( namco_enabled( this ) && addr == namco_data_reg_addr )
						return Namco_read_data( &this->namco );
			
					if ( fds_enabled( this ) && (unsigned) (addr - fds_io_addr) < fds_io_size )
						return Fds_read( &this->fds, Cpu_time( &this->cpu ), addr );
			
					if ( mmc5_enabled( this ) ) {
						int i = addr - 0x5C00;
						if ( (unsigned) i < mmc5_exram_size )
							return this->mmc5.exram [i];
		
						int m = addr - 0x5205;
						if ( (unsigned) m < 2 )
							return (this->mmc5_mul [0] * this->mmc5_mul [1]) >> (m * 8) & 0xFF;
					}
				#endif
				result = addr >> 8; // simulate open bus
			}
		}
	}
	
	/* if ( addr != 0x2002 )
		debug_printf( "Read unmapped $%.4X\n", (unsigned) addr ); */
	
	return result;
}

void Cpu_write( struct Nsf_Emu* this, nes_addr_t addr, int data )
{	
	int offset = addr - sram_addr;
	if ( (unsigned) offset < sram_size )
	{
		this->sram [offset] = data;
	}
	else
	{
		// after sram because cpu handles most low_ram accesses internally already
		int temp = addr & (low_ram_size-1); // also handles wrap-around
		if ( !(addr & 0xE000) )
		{
			this->cpu.low_mem [temp] = data;
		}
		else
		{
			int bank = addr - banks_addr;
			if ( (unsigned) bank < bank_count )
			{
				Write_bank( this, bank, data );
			}
			else if ( (unsigned) (addr - start_addr) <= end_addr - start_addr )
			{
				Apu_write_register(  &this->apu, Cpu_time( &this->cpu ), addr, data );
			}
			else
			{
			#ifndef NSF_EMU_APU_ONLY
				// 0x8000-0xDFFF is writable
				int i = addr - 0x8000;
				if ( fds_enabled( this ) && (unsigned) i < fdsram_size )
					fdsram( this ) [i] = data;
				else
			#endif
				Cpu_write_misc( this, addr, data );
			}
		}
	}
}

#define CPU_READ( emu, addr, time )         Cpu_read( emu, addr )
#define CPU_WRITE( emu, addr, data, time )  Cpu_write( emu, addr, data )
