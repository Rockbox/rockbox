#define _PAGE_ How To Work With Patches
#include "head.t"
<p>
 When we speak of 'patches' in the Rockbox project, we mean a set of changes
 to one or more source files.

<h2>Tools Of The Trade</h2>
<p>
Use the tools 'diff' and 'patch'. Preferably the GNU versions. They're readily
available for all imaginable platforms.
<p>
Try one of these:
<ul>
<li> <a href="http://www.fsf.org/software/patch/patch.html">http://www.fsf.org/software/patch/patch.html</a>
<li> <a href="http://www.gnu.org/directory/diffutils.html">http://www.gnu.org/directory/diffutils.html</a>
<li> <a href="http://gnuwin32.sourceforge.net/packages/patch.htm">http://gnuwin32.sourceforge.net/packages/patch.htm</a> - patch for Windows
<li> <a href="http://gnuwin32.sourceforge.net/packages/diffutils.htm">http://gnuwin32.sourceforge.net/packages/diffutils.htm</a> - diff for Windows
</ul>

<h2>Creating A Patch</h2>
<p>
 We generate diffs (often called patches) using 'diff' in a manner similar to
this:
<pre>
  diff -u oldfile newfile > patch
</pre>
<p>
 People who have checked out code with CVS can do diffs using cvs like this:
<pre>
  cvs diff -u file > patch
</pre>
<p>
 'diff' can also be used on a whole directory etc to generate one file with
changes done to multiple:
<pre>
  diff -u olddir newdir > patch 
</pre>
<p>
 The -u option means the output is using the 'unified diff' format. Older
 diff programs don't have that, and then -c (for 'context diff') is OK.

<h2>Submitting A Patch</h2>

<p>All patches that are meant for inclusion in the sources should follow the
format listed on the <a href="contributing.html">Contributing to Rockbox</a>
page, and be posted to the <a
href="http://sourceforge.net/tracker/?group_id=44306&atid=439120">patch
tracker</a>.  Patches sent to the mailing list are quickly lost in the traffic
of the list itself.

<p>
 Please keep in mind that not all submitted patches will be accepted.

<h2>Applying A Patch</h2>
<p>
 Applying a 'patch' (output from diff -u) is done with the 'patch' tool:
<pre>
  patch < patchfile
</pre>
<p>
 patch knows that the patchfile is a set of changes on one or more files, and
will do those to your local files. If your files have changed too much for the
patch to work, it will save the sections of the patch that aren't possible to
apply in a file called "filename.rej" (filename being the name of the file for
which the failing section was intended for). Then you must take care of them
manually.

<p>
 If there is path information in the patchfile that you want to cut off
 from the left, tell patch how many directory levels to cut off to find the
 names in your file system:
<pre>
  patch -p0 < patchfile
  patch -p1 < patchfile
  patch -p2 < patchfile
</pre>
 ... each example line removes one extra level of dir info from the left.
<p>
 You can use the --dry-run option to patch to make sure that the patch applies
clean. It doesn't actually apply the patch, only prints what would happen if
you run it.
<p>
 You can remove a patch again from the sources by doing the reverse action of
a specific patch. You do this with the -R (or --reverse) options, such as:
<pre>
  patch -p1 -R < patchfile
</pre>

#include "foot.t"
