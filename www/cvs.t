#define _PAGE_ Accessing source code via CVS
#include "head.t"

<h2>Browsing the repositry</h2>

<p>Just go <a
href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/">here</a>.

<h2>Daily snapshots</h2>

<p>Every night at 6am CET, we build a source tarball and target .mod files
from the latest CVS code.  <a href="daily.shtml">Get them here</a>.

<h2>Downloading (checking out) the source</h2>

<p>You, obviously, need to have <a href="http://www.cvshome.org">CVS</a>
installed to do this.

<p>Here is a complete list of the available modules:

<ul>
<li>apps - the source code to the applications
<li>firmware - the source code to the firmware library
<li>gdb - the gdb stub to use for remote debugging
<li>tools - tools for building the firmware
<li>uisimulator - a user interface simulator for X11
<li>docs - project documentation
<li>www - the web page
</ul>

The examples below use the 'rockbox' module, since that's what most people are
interested in. We have a few other convenient aliases that gets several
modules at once for you:

<ul>
<li> rockbox - gets everything you need to compile and build rockbox for target
<li> rockbox-devel - like 'rockbox' but also includes simulators and gdb code
<li> rockbox-all - gets everything there is in CVS, all modules
<li> website - gets the www and docs modules
</ul>

<h3>Anonymous read-only checkout</h3>

<p>If you are not a registered developer, use this method.
When asked for a password, just press enter:

<p><tt>cvs -d:pserver:anonymous@cvs.rockbox.sourceforge.net:/cvsroot/rockbox login
<br>cvs -z3 -d:pserver:anonymous@cvs.rockbox.sourceforge.net:/cvsroot/rockbox co rockbox</tt>

<p>A "rockbox" directory will be created in your current directory, and all
the directories and source files go there.

<h3>Checkout for developers</h3>

<p>For this, you need to:

<ol>
<li> Have <a href="http://www.openssh.com">SSH</a> installed.
<li> Have a <a href="http://sourceforge.net/account/register.php">SourceForge account</a>
<li> Be a 
<a href="http://sourceforge.net/project/memberlist.php?group_id=44306">registered developer</a>
of the Rockbox project
<li> Log on to your cvs server account once: <tt>ssh <b>username</b>@cvs.rockbox.sourceforge.net</tt> <br>It will disconnect you immediately, but now your account is set up.
</ol>

<p>Then run:

<p><tt>export CVS_RSH=ssh
<br>cvs -z3 -d:ext:<b>username</b>@cvs.rockbox.sourceforge.net:/cvsroot/rockbox co rockbox</tt>

<p>If you are using WinCVS, the procedure is
<a href="http://www.wincvs.org/ssh.html">somewhat different</a>.

<h2>Checking in modifications</h2>

<p>CVS is a "no-reserve" version control system. This means that you work on your local files without first reserving them. Any conflicts with other developers are detected when you check-in, or "commit" as it's called in CVS:

<p><tt>cvs commit <b>filename</b></tt>

<p>This will start an editor and ask you to describe the changes you've made. If you want, you can use the -m command line option to specify the comment right there:

<p><tt>cvs commit -m "This is my change comment" <b>filename</b></tt>

<p><strong>Note:</strong> Before checking in modifications, test-build all targets (player, player-old, recorder, player-sim, recorder-sim) to make sure your changes don't break anything.

<h2>Updating your repository</h2>

<p>Since several people commit to the repository, you will need to periodically
synchronize your local files with the changes made by others.
This operation is called "update":

<p><tt>cvs update -dP</tt>

<p>The <b>-d</b> switch tells update to create any new directories that have been created the repository since last update.
<br>The <b>-P</b> switch tells update to delete files that have been removed in the repository.

<h2>Adding a new file</h2>

<p>Adding a file is very simple:

<p><tt>cvs add <b>filename</b></tt>

<p>If you are adding a binary file, you need to specify the -kb flag:

<p><tt>cvs add -kb <b>filename</b></tt>

<p>These changes, like any other change, has to be committed before they will be visible on the server.

<h2>Querying the status of your files</h2>

<p>Sometimes it is interesting to get a list of the status of your files versus
those on the remote repository. This is called "status":

<p><tt>cvs status</tt>

<p>The output from "status" can be rather verbose. You may want to filter it with grep:

<p><tt>cvs status | grep Status</tt>

<p>To only list files who differ from the server, filter again:

<p><tt>cvs status | grep Status | grep -v Up-to-date</tt>

<h2>Producing a diff of your changes</h2>

<p>If you want to see how your local files differ from the CVS repository,
you can ask CVS to show you:

<p><tt>cvs diff -u [files(s)]</tt>

<p>The <tt>-u</tt> selects the "unified" diff format, which is preferrable
when working with source code.

<h2>What Happens in the Repository?</h2>
<p>
 Subscribe to the rockbox-cvs list to get mails sent to you for every commit
 done to the repostory.
<p>
 To join this list, send a mail to majordomo@cool.haxx.se, with the following
 text in the body (no subject) "subscribe rockbox-cvs".
<p>
 <b>Note</b> that this may cause quite a few mails to get sent during periods
of intense development.

<h2>Getting rid of the password prompts</h2>

<p>Each cvs operation has to be authenticated with ssh. This is normally done
by you entering your password. This gets boring fast.
Instead, you can register your public ssh key with your SourceForge account. This way, your connection is authenticated automatically.

<p><a href="http://sourceforge.net/account/login.php">Log in</a>
to your SourceForge account and go to your
<a href="https://sourceforge.net/account/">account options</a>.
On the bottom of the page, there is a link to
<a href="https://sourceforge.net/account/editsshkeys.php">edit your ssh keys</a>.
Copy the contents of your local <tt>.ssh/identity.pub</tt> or
<tt>.ssh/id_rsa.pub</tt> there.

<p>Like many things on SourceForge, the key change doesn't take effect immediately. You'll have to wait a few hours until some magic batch job kicks in and puts your keys where they should be. Then you can use cvs without entering your password.

<p>If you work from several different computers/accounts, you must add the key for each account you are using.

#include "foot.t"
