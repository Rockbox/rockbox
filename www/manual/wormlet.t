#define _PAGE_ Manual - Wormlet
#include "head.t"
#include "manual.t"

<h2>Overview</h2>
<p> Wormlet is a multi-user multi-worm game on a multi-threaded multi-functional 
rockbox console. You navigate a humgry little worm. Help 
your worm to find food and to avoid poisoned argh-tiles. The goal 
is to turn your tiny worm into a big worm as long as possible.</p>

<h2>System requirements</h2>
<p>To play Wormlet you need an Archos Jukebox Recorder upgraded with Rockbox open 
source jukebox firmware. Ensure that your version of Rockbox has games complied 
in. Note that not all Rockbox versions contain Wormlet as the games are a 
optional at compile time.</p>
<p>For 2-player games a remote control is not necessary but recommended. If you 
try to hold the Jukebox in the four hands of two players you'll find out why. 
Games with three players are only possible using a remote control.</p>

<h2>Start the game</h2>
<p>Hit F1 on the jukebox to enter the menu. Use the arrow keys to select the 
entry 'Games' and then 'Wormlet'.</p>

<h2>Game Options</h2>
<p>Before the game starts a configuration panel appears. Here you select the 
number of players, the number of worms and the control mode of the game.</p>
<h3>Players</h3>
<p>With the up and down keys of the Jukebox you can adjust the number of 
players that take part in the game. To each player a worm will be assigned.
If there are more worms than players in the game those worms will be 
steered by the Jukebox with artificial stupidity. This enables you to play
against opponents if you lack of friends. If you are smart: maybe you should 
put away Wormlet every now and then to make new friends that can steer the 
worms more intelligently than artificial stupidity. Although: sometimes human
stipidity outstupids artificial stupidity... But that doesn't apply to _your_ 
friends.</p>
<p>By specifying 0 players you enter the couch-potato-mode. All worms are
controlled by artificial stupidity and thus out of control. Grab some popcorn 
and watch the worms fight each other. Maybe you feel like supporting a worm 
of your choice loudly.</p>

<h3>Worms</h3>
Adjust the number of worms that take part in the game. Note that you can have
more worms than players but not more players than worms. Worms without a player
are controlled by artificial stupidity.

<h3>Control</h3>
Using the F1 key you can select the control mode. The available control modes
depend on the number of players taking part in the game. Only in single player 
games the 4 Key control is available. It would be unfair if you had to 
distinguish four keys while your opponent is already busy with only two ....
<table border=1>
  <tr>
    <td>Players</td> 
	<td>Modes</td> 
	<td>Player 1</td>
	<td>Player 2</td>
	<td>Player 3</td>
  </tr>
  <tr>
    <td>0</td>
	<td>Out of control</td>
	<th colspan=3>
	  With no player taking part in the game all worms are out of control and
	  steered by artificial stupidity.
	</th>
  </tr>
  <tr>
    <td rowspan=2>1</td>
	<td>2 Key control</td>
	<td>
	  on Jukebox<br>
	  left: turn worm left<br>
	  right: turn worm right
	</td>
	<td>-</td>
	<td>-</td>
  </tr>
  <tr>
    <td>4 Key control</td>
	<td>
	  on Jukebox<br>
	  left: make worm creep left<br>
	  up: make worm creep up<br>
	  right: make worm creep right<br>
	  down: make worm creep down
	</td>
	<td>-</td>
	<td>-</td>
  </tr>
  <tr>
    <td rowspan=2>2</td>
	<td>Remote Control</td>
	<td>
	  on Jukebox<br>
	  left: turn worm left<br>
	  right: turn worm right<br>
	</td>
	<td>
	  on remote control<br>
	  Volume down: turn worm left</br>
	  Volume up: turn worm right<br>
	</td>
	<td>-</td>
  </tr>
  <tr>
    <td>No rem. control</td>
	<td>
	  on Jukebox<br>
	  left: turn worm left<br>
	  right: turn worm right<br>
	</td>
	<td>
	  on Jukebox<br>
	  F2: turn worm left</br>
	  F3: turn worm right<br>
	</td>
	<td>-</td>
  </tr>
  <tr>
    <td>3</td>
	<td>Remote control</td>
	<td>
	  on Jukebox<br>
	  left: turn worm left<br>
	  right: turn worm right<br>
	</td>
	<td>
	  on remote control<br>
	  Volume down: turn worm left</br>
	  Volume up: turn worm right<br>
	 </td>
	 <td>
	  on Jukebox<br>
	  F2: turn worm left</br>
	  F3: turn worm right<br>
	</td>

  </tr>
</table>
  
<h3>Start the game</h3>
<p>When you have finished selecting the game options you start the game by 
pressing the 'play' or 'on' button. The field is populated with food, 
argh tiles and of course the worms. According to your selectin up to 
three worms appear. All worms start creeping to the right.
The worm of player 1 is the top most worm, player 2 controls the worm in
the middle, the third worm is driven by player 3. </p>

<h2>The game</h2>
<p>Use the control keys of your worm to navigate around obstacles and find 
food. Worms can not stop creeping besides when dead. Dead worms are no fun.
Be careful as your worm will try to eat anything that you steer it 
across. It doesn't distinguish wether it's eatable or not.</p>

<h3>Food</h3>
<p>The small quadratical hollow pieces are food. By creeping your worm over a
food tile you make it eat. After you ate your worm grows. Each time a piece
of food has been eaten a new piece of food will pop up somewhere. 
Unfortunately for each new food that appears two new argh pieces will appear,
too.</p>

<h3>Argh</h3>
You surely wondered what the heaven an argh might be. An argh is a 
black quadratical poisoned piece - slightly bigger than food - that makes a 
worm say "Argh!" when creeping against. A worm that tried to eat an argh is 
dead. Thus you must avoid eating argh under any circumstances. Arghs have 
the annoying tendency to accumulate.

<h3>Worms</h3>
Thou shall not eat worms. Neither other worms nor yourself. Eating worms is
plasphemic canibalism, not healthy and causes instant death. And it doesn't 
help you anyway: you can't hurt the other worm by biting it. It will go 
on creeping happily and eat all the food you left on the table.

<h3>Walls</h3>
Don't creep against the walls. Walls are not eatable. Creeping a worm against
a wall causes it a headache it doesn't survive.

<h3>Game over</h3>
The game is over when all worms are dead. The longest worm wins the game.

<h3>Pause the game</h3>
Press the play key to pause the game. Hit play again to resume the game.

<h3>Stop the game</h3>
There are two ways to stop a running game. 
<ul>
<li>If you want to quit Wormlet entirely
simply hit the off button. The game will stop immediately and you will return
to the game menu. 
<li>If you want to stop the game and still see the screen hit the on button. This
freezes the game. If you hit the on button again a new game starts with the same
configuration. To return to the games menu you can hit the stop button. 
A stopped game can not be resumed.
</ul>

<h2>The score board</h2>
On the right side of the game field you can see the score board. For each worm
it displays its status and its length. The top most entry displays the state of
worm 1, the second worm 3 and the third worm 3. When a worm dies it's entry on the
score board turns black. 

<h3>Len:</h3>
Here the current length of the worm is displayed. When a worm is on food it 
grows by one pixel for each step it creeps. 

<h3>Hungry</h3>
That's the normal state of a worm. Worms are always hungry and want to eat. 
It's good to have a hungry worm since it means that your worm is alive. But
it's better to get your worm growing.

<h3>Growing</h3>
When your worm has eaten a piece of food it starts growing. For each step
it creeps it can grow by one pixel. One piece of food lasts for 7 steps.
After your worm has crept 7 steps the food is used up. If you encounter 
another piece of food while growing don't hesitate to eat it. It will increase
your growing state for another 7 steps.

<h3>Crashed</h3>
This indicats that you crashed your poor worm against a wall. That was evil. 
Go and find something hard and smash it against your forehead so you feel
what you did to your worm. And don't use your Jukebox. You might damage it and
with its soft blue rubber edges that wouldn't hurt enough anyway.

<h3>Argh</h3>
If your score board entry displays "Argh" it means your worm is dead because of
trying to eat an argh. Until we can make the worm say "Argh!" it's your 
job to say "Argh!" aloud.

<h3>Wormed</h3>
Your worm tried to eat another worm or even itself. That's why it's dead now. 
Maybe your opponent has managed to build a trap with his worm. Try to
do the same with him in the next game.

<h2>Hints</h2>
<ul>
<li>During the first games you will be busy with controlling your worm. Try to 
avoid other worms and creep far away from them. Wait until they curled up
themselves and collect the food afterwards. Don't bother if the other worms
grow longer than yours - you can catch up after they've died.
<li>When you are more experienced watch the tactics of other worms. Especially 
those worms controlled by artificial stupidity head straight for the nearest
piece of food. Let the other worm have it's next piece of food and head for
the food it would probably want next. Try to put yourself between the opponent 
and that food. From now on you can 'control' the other worm by blocking it. 
You could trap it
by making a 1 pixel wide U-turn. You also could move from food to food and make 
sure you keep between your opponent and the food. So you can always reach it 
before your opponent.
<li>While playing the game the Jukebox still can play music. For single player 
game use any music you like. For berzerk games with 2 players use hard rock and 
for 3 player games use heavy metal or X-Phobie 
(<a href="http://www.x-phobie.de">http://www.x-phobie.de</a>).
<li>Play fair and don't kick your opponent on the big toe or poke him in the eye. 
That's wouldn't be bad manners.
</ul>
 
#include "foot.t"
