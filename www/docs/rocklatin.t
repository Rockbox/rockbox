#define _PAGE_ Rocklatin
#include "head.t"

<h1>Rocklatin1</h1>

Only for developers...

<h2>Background</h2>
The Archos player comes in two models. One with old LCD and one with new LCD.
(Differences can be seen below). You can't find any difference more than the
LCD, therefor we run the same code on both models. The original software
contains two different mappings from ASCII-character to hardware-LCD. Because
some characters doesn't exist in both hardwares all national characters
are left out in both hardware.
<p>The old LCD can have 4 software defined characters, and the new LCD can
have 8 software defined characters.
<table border=1><tr>
<td>HW layout of old LCD:<br><img width=272 height=272 src="lcd_old_hw.gif"></td>
<td>HW layout of new LCD:<br><img width=272 height=272 src="lcd_new_hw.gif"></td>
</tr></table>

<h2>What is Rocklatin1</h2>
Rocklatin1 is based on Winlatin1 (which is identical to Latin1 but some
extra characters). All characters presented in any HW-LCD (i.e. old LCD)
is mapped in Rocklatin1 and some extra characters we find good to use.

<table border=1><tr>
<td>Rocklatin1 of old LCD:<br><img width=272 height=272 src="lcd_old.gif"></td>
<td>Rocklatin1 of new LCD:<br><img width=272 height=272 src="lcd_new.gif"></td>
</tr></table>
The red characters are characters not defined in the HW-LCD. These characters
are mapped by the software to a software defined character (0-4/8) whenever 
they are used.

<h2>But what if...</h2>
...all software defined characters are taken?<br>
Well, then a substitute character will be used for that character.

<table border=1><tr>
<td>Substitute of old LCD:<br><img width=272 height=272 src="lcd_old_subst.gif"></td>
<td>Substitute of new LCD:<br><img width=272 height=272 src="lcd_new_subst.gif"></td>
</tr></table>
The red characters shows where a substitution is made.

<p>
All Rocklatin1 characters between 0x00 and 0x1f are hardcoded to be prioritized.
That means that if a national character is displayed at LCD and an icon
(0x18-0x1f) is to be shown, the character with highest Rocklatin1 value will
be switched to a substitute character.

<h2>Accessing hardware</h2>
The Rockbox software can access a HW-LCD-character by doing a lcd_putc(0x100-0x1ff). That would of course make it 100% hardware depended (=not good).
<p>
The Rockbox software can also define 23 own patterns, even though hardware only
allows 4 or 8. The software should of course not try to display more than 4
or 8 of such characters. This code example shows how to define a pattern:
<pre>
{
  unsigned char pattern[]={ 0x0a, 0x00, 0x00, 0x0c,
                            0x04, 0x04, 0x0e};
  unsigned char handle;

  handle=lcd_get_locked_pattern();
  lcd_define_pattern(handle, pattern);

  lcd_putc(x, y, handle);

  ...

  lcd_unlock_pattern(handle);
}
</pre>
The handle is very likely to be between 0x01 to 0x17, which in software will
be handled as a prioritized character (even higher than the icons).

<h2>Some notes</h2>
<ul>
<li>Displaying the same rocklatin-mapped-character many times at the LCD
only occupies one HW-LCD-mapped character.
<li>If a substitute character is used, the "should-be" character will never
be shown (no flickering screen) until the character is moved or scrolled.
<li>Characters already displayed are only substituted if a prioritized
character is to be displayed.
<li>The software maps the characters circular in order to minimize the
likelyhood to remap the same character very often.
<li>The gifs above is generated with the tool "generate_rocklatin".
<li>Rocklatin character 0x92 is defined as the "cursor" character.
<li>Rocklatin character 0x93-0x95 is only used for substitution (a substitute
character must be a Rocklatin character).
<li>Implementation and design by Kjell Ericson and Mats Lidell (for questions).
<li>Rocklatin1 is based on Winlatin1 because the old LCD happened to have 7
of the Winlatin1-extra characters (no need to remap/remove those).
<li>If you find any characters identical in old and new HW-LCD that aren't
mapped (and really are useful) you can tell us.
</ul>

#include "foot.t"
