#!/usr/bin/perl

my $basedir = "/home/dast/rockbox-build/daily-build";

my @list=("player", "recorder", "fmrecorder", "recorderv2", "recorder8mb");

for(@list) {
    my $dir = $_;
    opendir(DIR, "$basedir/$dir") or
        die "Can't opendir($basedir/$dir)";
    my @files = sort grep { /^rockbox/ } readdir(DIR);
    closedir DIR;

    for(@files) {
        /(20\d+)/;
        $date{$1}=$1;
    }
}

print "<table class=rockbox>\n";

if (0) {
    print "<tr><th>date</th>";

    for(@list) {
        print "<th>$_</th>";
    }
}

for(reverse sort keys %date) {
    my $d = $_;
    my $nice = $d;
    if($d =~ /(\d\d\d\d)(\d\d)(\d\d)/) {
        $nice = "$1-$2-$3";
    }
    print "</tr>\n<tr><td>$nice</td>";
    
    for(@list) {
        my $n=0;
        my $m = $_;
        print "<td> ";
        # new-style full zip:
        if( -f "daily/$m/rockbox-${m}-${d}.zip") {
            printf "%s<a href=\"daily/$_/rockbox-${m}-${d}.zip\">${m}</a>",
            $n?", ":"";
            $n++;
        }
        print "</td>";
    }
    print "</tr>\n"
}
print "</table>\n";

