#define _PAGE_ Daily builds
#include "head.t"

<h2>Source tarballs</h2>

<!--#exec cmd="./dailysrc.pl" -->

<h2>Target builds</h2>

<p>There are three versions of each build:

<!--#exec cmd="./dailymod.pl" -->

#if 0
<p>Note 1: Due to the big difference between the player and recorder models, they support different features.
#endif

<p><b>Note:</b> You must rename the file to "archos.mod" before copying it to the root of your archos.

#include "foot.t"
