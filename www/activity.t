#define YELLOW "#ffffa3"
#define GREEN  "#80db72"
#define RED    "#ffadad"

#define STATUS(_col_,_mod_,_file_,_href_,_person_,_status_) \
<tr bgcolor=_col_><td>_mod_</td><td><a href=_href_>_file_</a></td><td>_person_</td><td>_status_</td></tr>

<table align="right">
<tr><th colspan=2>Color codes:</th></tr>
<tr><td bgcolor=GREEN>&nbsp; &nbsp;</td><td> Working code exists</td></tr>
<tr><td bgcolor=YELLOW>&nbsp; &nbsp;</td><td> Development in progress</td></tr>
<tr><td bgcolor=RED>&nbsp; &nbsp;</td><td> Undermanned. Help needed.</td></tr>
</table>

<table cellspacing=0 cellpadding=2 border=1>
<tr bgcolor="#cccccc"><th>Module</th><th>File(s) in CVS</h><th>Current person</th><th>Status</th></tr><tr>
STATUS(GREEN,I<sup>2</sup>C driver,firmware/drivers/i2c.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/drivers/i2c.c", Linus,Works)
STATUS(GREEN,LED driver,firmware/drivers/led.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/drivers/led.c",-,Written)
STATUS(GREEN,GDB stub,gdb/,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/gdb/", Linus,Works)
STATUS(GREEN,List,firmware/common/list.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/common/lists.c", Linus,Works)
STATUS(GREEN,ID3 parser,firmware/i3d.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/id3.c", Daniel,Works)
STATUS(GREEN,FAQ,www/docs/FAQ,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/www/docs/FAQ",Rob,Existing)
STATUS(GREEN,Fat32 filesystem,firmware/drivers/fat.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/drivers/fat.c", Björn,Works)
STATUS(GREEN,Tetris,uisimulator/tetris.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/uisimulator/tetris.c", &nbsp;, Written)
STATUS(GREEN,MAS driver,firmware/drivers/mas.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/drivers/mas.c", Linus,Works)
STATUS(GREEN,ATA driver,firmware/drivers/ata.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/drivers/ata.c", Björn,Works)
STATUS(GREEN,Scheduler,firmware/thread.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/thread.c", Linus,Works)
STATUS(YELLOW,Key handling,firmware/drivers/button.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/drivers/button.c", -,Written; untested)
STATUS(YELLOW,LCD driver,firmware/drivers/lcd.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/drivers/lcd.c", Björn,Written; untested)
STATUS(YELLOW,CPU setup,firmware/system.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/system.c", Linus,In progress)
STATUS(YELLOW,Playlist handling,firmware/playlist.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/firmware/playlist.c", Wavey, Progressing)
STATUS(YELLOW,X11 simulator,uisimulator/x11, "http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/uisimulator/x11/", Daniel,Progressing)
STATUS(YELLOW,Win32 simulator,uisimulator/win32/,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/uisimulator/win32/", Felix,Progressing)
STATUS(RED,Boot loader (rolo),-,"", -,-)
STATUS(RED,Directory browser UI,uisimulator/tree.c,"", Daniel,Progressing)
STATUS(RED,Application...,uisimulator/app.c,"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/uisimulator/app.c",-,Embryo)
STATUS(RED,API docs,-,"",-, Header files exist)
STATUS(RED,New DSP algorithms,-,"",-, -)
</tr></table>
<i><small>Updated __DATE__</small></i>
