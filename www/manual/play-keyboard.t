#define _PAGE_ Manual - Player keyboard
#include "head.t"
#include "manual.t"

    <p>
      The keyboard allows you to edit text. The first line holds the
      text being edited and the second line is a combined menu line
      where you both can select characters for insertion and the
      operations; backspace, delete, accept and abort.
    </p>

    <p>
      You move between the lines using the normal LEFT and RIGHT
      buttons. An arrow to the left indicate what line is selected. The
      second line will scroll. This is like like in most menus except
      that the first line will always show the text being edited.
    </p>

    <h2>Button bindings and functionality</h2>

    <table class=buttontable>
      <tr><th>Line</th><th>Function and key bindings</th>
	<tr><td>First</td><td>Text - Use UP and DOWN to move the cursor</td></tr>
	<tr><td>Second</td>
	  <td>
	    <table>
	      <tr>
		<td>Characters</td><td>Chars selectable for
		input. With ON insert the char between the
		arrows. Select another character by using UP and
		DOWN. Finally change the character subset with
		MENU. There are three subsets to choose from:
		capital letters, small letters and others.</td>
	      </tr>
	      <tr>
		<td>Backspace</td><td>Use UP to delete the char to the
		left the cursor.</td>
	      </tr>
	      <tr><td>Delete</td><td>Use UP to delete char under the
		  cursor</td>
	      </tr>
	      <tr><td>Accept</td><td>UP accepts the text and returns
		  to the application</td>
	      </tr>
	      <tr><td>Abort</td><td>UP returns to the application
		  without any change</td>
	      </tr>
	    </table>
	  </td>
	</tr>
    </table>

    <h2>Example</h2>

    <p>Supposed we want edit the text "file.mp3" and change it to
    "fileName.mp3". Maybe as a part of doing a rename operation. This
    is how the screen will look:</p>

    <img src="play-keyboard-initial.png">

    <p>Note how the arrow points to the first line. This means that UP
    and DOWN will move the cursor on the first line so that you can
    select where to insert a char or where to delete chars.</p>

    <p>Now we press DOWN three times to move the cursor so it will be
    above the dot. The we move down to the second line by pressing
    RIGHT. Finally we select the character N by pressing UP a few
    times. You will se that the second line will scroll to the left
    placing a new character between the arrows each time you press
    UP. We continue doing this until we have N between the arrows. The
    screen will look something like this then.</p>

    <img src="play-keyboard-Nselected.png">

    <p>We are now ready to insert out first character. Press ON and N
    will be inserted at the place of the cursor.</p>
    
    <p>To get to the small letters we press MENU, move to 'a' by using
    UP and DOWN, insert it by pressing ON.</p>

    <img src="play-keyboard-Naentered.png">

    <p>Finally to feed our new text back to the application requesting
    this we move further down by pressing RIGHT. This will scroll the
    second line vertically, move away the character insertion line and
    another menu line will be visible. We continue pressing RIGHT until
    the menu choice Accept is visible.</p>

    <img src="play-keyboard-accept.png">

    <p>We acknowledge this by pressing UP. We are done. We will now be
    taken back to where we were before entering the keyboard.</p>

#include "foot.t"
