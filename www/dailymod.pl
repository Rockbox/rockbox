#!/usr/bin/perl

my $basedir = "/home/dast/rockbox-build/daily-build";

my @list=("player", "recorder", "fmrecorder", "recorder8mb");

for(@list) {
    my $dir = $_;
    opendir(DIR, "$basedir/$dir") or
        die "Can't opendir($basedir/$dir)";
    my @files = sort grep { /^rockbox2/ } readdir(DIR);
    closedir DIR;

    for(@files) {
        /(20\d+)/;
        $date{$1}=$1;
    }
}

print "<table class=rockbox><tr><th>date</th>";

for(@list) {
    print "<th>$_</th>";
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
        print "<td> ";
        if( -f "daily/$_/rockbox${d}.mod") {
            print "<a href=\"daily/$_/rockbox${d}.mod\">mod</a>";
            $n++;
        }
        if( -f "daily/$_/rockbox${d}.ajz") {            
            printf "%s<a href=\"daily/$_/rockbox${d}.ajz\">ajz</a>",
            $n?", ":"";
            $n++;
        }
        if( -f "daily/$_/rocks${d}.zip") {
            printf "%s<a href=\"daily/$_/rocks${d}.zip\">rocks</a>",
            $n?", ":"";
            $n++;
        }
        if( -f "daily/$_/rockbox-${d}.zip") {
            printf "%s<a href=\"daily/$_/rockbox-${d}.zip\">full</a>",
            $n?", ":"";
            $n++;
        }
        if( -f "daily/$_/rockbox${d}.ucl") {
            printf "%s<a href=\"daily/$_/rockbox${d}.ucl\">ucl</a>",
            $n?", ":"";
            $n++;
        }
        print "</td>";
    }
    print "</tr>\n"
}
print "</table>\n";

