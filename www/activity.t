#define YELLOW "#ffffa3"
#define GREEN  "#80db72"
#define RED    "#ffadad"

#define STATUS(_col_,_a_,_b_,_c_,_d_) \
<tr bgcolor=_col_><td>_a_</td><td>_b_</td><td>_c_</td><td>_d_</td></tr>

<table align="right">
<tr><th colspan=2>Color codes:</th></tr>
<tr><td bgcolor=GREEN>&nbsp; &nbsp;</td><td> Working code exists</td></tr>
<tr><td bgcolor=YELLOW>&nbsp; &nbsp;</td><td> Development in progress</td></tr>
<tr><td bgcolor=RED>&nbsp; &nbsp;</td><td> Undermanned. Help needed.</td></tr>
</table>

<table>
<tr bgcolor="#cccccc"><th>Module</th><th>File(s) in CVS</h><th>Current person</th><th>Status</th></tr><tr>
STATUS(YELLOW,ATA driver,firmware/drivers/ata.c,Björn,Written; untested)
STATUS(GREEN,I<sup>2</sup>C driver,firmware/drivers/i2c.c,Linus,Works)
STATUS(YELLOW,MAS driver,firmware/drivers/mas.c,Linus,Embryo written)
STATUS(RED,Fat32 filesystem,firmware/drivers/fat.c,Björn & Linus,Fat16 code exists)
STATUS(YELLOW,Key handling,firmware/drivers/button.c,-,Written; untested)
STATUS(YELLOW,LCD driver,firmware/drivers/lcd.c,Björn,Written; untested)
STATUS(GREEN,LED driver,firmware/drivers/led.c,-,Written)
STATUS(YELLOW,CPU setup,firmware/system.c,Linus,In progress)
STATUS(GREEN,GDB stub,gdb/,Linus,Works)
STATUS(YELLOW,Scheduler,firmware/thread.c,Linus,Final touches)
STATUS(GREEN,List,firmware/common/list.c,Linus,Works)
STATUS(YELLOW,Playlist handling,-,Stuart,Planning)
STATUS(RED,Newlib port,-,-,-)
STATUS(GREEN,ID3 parser,firmware/i3d.c,Daniel,Works)
STATUS(YELLOW,X11 simulator,uisimulator/,Daniel,Progressing)
STATUS(YELLOW,Win32 simulator,uisimulator/win32,Felix,Progressing)
STATUS(GREEN,FAQ,www/docs/FAQ,Rob,Existing)
STATUS(RED,Directory browser UI,-,-,-)
STATUS(RED,Application...,-,-,-)
</tr></table>
<i><small>Updated __DATE__</small></i>


