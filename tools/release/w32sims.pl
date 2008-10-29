#!/usr/bin/perl
# This is basically a copy of tools/release/bins.pl, with small changes
# You need to set up $PATH to include the bin dir of a crosscompiled SDL
# installation, and have mingw32 compilers installed. Everything else should
# Just Work[tm].

my $verbose, $update, $doonly, $version;
while (scalar @ARGV > 0) {
    if ($ARGV[0] eq "-h") {
        print $ARGV[0]."\n";
        print <<MOO
Usage: w32sims [-v] [-u] [-r VERSION] [buildonly]
       -v Verbose output
       -u Run svn up before building
       -r Use the specified version string for filenames (defaults to SVN
          revision)
MOO
;
        exit 1;
    }
    elsif ($ARGV[0] eq "-v") {
        $verbose =1;
    }
    elsif ($ARGV[0] eq "-u") {
        $update =1;
    }
    elsif ($ARGV[0] eq "-r") {
        $version =$ARGV[1];
        shift @ARGV;
    }
    else {
        $doonly =$ARGV[0];
        # This will only be printed if the -v flag comes earler
        print "only build $doonly\n" if($verbose);
    }
    shift @ARGV;
}

if($update) {
    # svn update!
    system("svn -q up");
}

$rev = `svnversion`;
chomp $rev;
print "rev $rev\n" if($verbose);

if (!defined($version)) {
    $version = sprintf("r%d", $rev);
}

# made once for all targets
sub runone {
    my ($dir, $confnum, $extra)=@_;
    my $a;

    if($doonly && ($doonly ne $dir)) {
        return;
    }

    mkdir "build-$dir";
    chdir "build-$dir";
    print "Build in build-$dir\n" if($verbose);

    # build the target
    $a = buildit($dir, $confnum, $extra);

    chdir "..";

    print "Zip up the sim and associated files\n" if ($verbose);
    system("mv build-$dir rockbox-$dir-w32-sim");
    `zip -9 -r -q rockbox-$dir-w32-sim/rockbox.zip rockbox-$dir-w32-sim/{rockboxui.exe,UI256.bmp,SDL.dll,archos}`;

    my $o="rockbox-$dir-w32-sim/rockbox.zip";
    if (-f $o) {
        my $newo="output/rockbox-$dir-$version-w32-sim.zip";
        system("mkdir -p output");
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
    }

    print "remove all contents in rockbox-$dir-w32-sim\n" if($verbose);
    system("rm -rf rockbox-$dir-w32-sim");

    return $a;
};

sub buildit {
    my ($target, $confnum, $extra)=@_;

    `rm -rf * >/dev/null 2>&1`;

    my $c = sprintf('printf "%s\n%ss\n" | ../tools/configure',
                    $confnum, $extra);

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make'\n" if($verbose);
    `make -j 2>/dev/null`;

    print "Run 'make install'\n" if($verbose);
    `make install 2>/dev/null`;

    print "Find and copy SDL.dll\n" if ($verbose);
    `cp \`dirname \\\`which sdl-config\\\`\`/SDL.dll ./`;

}

# run make in tools first to make sure they're up-to-date
print "cd tools && make\n" if($verbose);
`(cd tools && make ) >/dev/null 2>&1`;

runone("player", "player", '\n');
runone("recorder", "recorder", '\n');
runone("recorder8mb", "recorder", '8\n');
runone("fmrecorder", "fmrecorder", '\n');
runone("fmrecorder8mb", "fmrecorder", '8\n');
runone("recorderv2", "recorderv2", '\n');
runone("ondiosp", "ondiosp", '\n');
runone("ondiofm", "ondiofm", '\n');
runone("h100", "h100");
runone("h120", "h120");
runone("h300", "h300");
runone("ipodcolor", "ipodcolor");
runone("ipodnano", "ipodnano");
runone("ipod4gray", "ipod4g");
runone("ipodvideo", "ipodvideo", '32\n');
runone("ipodvideo64mb", "ipodvideo", '64\n');
runone("ipod3g", "ipod3g");
runone("ipod1g2g", "ipod1g2g");
runone("iaudiox5", "x5");
runone("iaudiom5", "m5");
runone("iaudiom3", "m3");
runone("ipodmini1g", "ipodmini");
runone("ipodmini2g", "ipodmini2g");
runone("h10", "h10");
runone("h10_5gb", "h10_5gb");
runone("gigabeatf", "gigabeatf");
runone("sansae200", "e200");
runone("sansac200", "c200");
#runone("mrobe500", "mrobe500");
runone("mrobe100", "mrobe100");
runone("cowond2", "cowond2");
