#!/usr/bin/perl

$basedir = "/home/dast/rockbox-build/daily-build";

sub list {
    $dir = shift @_;

    opendir(DIR, "$basedir/$dir") or
        die "Can't opendir($basedir/$dir)";
    @tarballs = sort grep { /^a/ } readdir(DIR);
    closedir DIR;
    
    print "<ul>\n";
    for ( @tarballs ) {
        print "<li><a href=\"daily/$dir/$_\">$_</a>\n";
    }
    print "</ul>\n";
}

print "<table class=rockbox><tr><th>player</th><th>recorder</th><th>fmrecorder</th></tr>\n";
print "<tr><td>\n";
&list("player");

print "</td><td>\n";
&list("recorder");

print "</td><td><p><b>Note:</b> The FM Recorder version is still experimental and very buggy. It is available here for testing purposes only.\n";
&list("fmrecorder");

print "</td></tr></table>\n";
