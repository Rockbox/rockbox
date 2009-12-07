#!/usr/bin/perl
# This is basically a copy of tools/release/bins.pl, with small changes.
use File::Basename;
use File::Path;
use Cwd;

require "../builds.pm";

my $verbose, $strip, $update, $doonly, $version;
my @doonly;

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time());
$year+=1900;
$mon+=1;
$verbose=0;

my $filename = "rockbox-sim-<target>-<version>";

while (scalar @ARGV > 0) {
    if ($ARGV[0] eq "-h") {
        print $ARGV[0]."\n";
        print <<MOO
Usage: w32sims [-v] [-u] [-s] [-w] [-r VERSION] [-f filename] [buildonly]
       -v Verbose output
       -u Run svn up before building
       -r Use the specified version string for filenames (defaults to SVN
          revision)
       -s Strip binaries before zipping them up.
       -w Crosscompile for Windows (requires mingw32)
       -f Filename format string (without extension). This can include a
          filepath (relative or absolute) May include the following special
          strings:
            <version> - Replaced by the revision (or version name if -r is
                        used)
            <target>  - Target shortname
            <YYYY>    - Year (4 digits)
            <MM>      - Month (2 digits)
            <DD>      - Day of month (2 digits - 0-padded)
            <HH>      - Hour of day (2 digits, 0-padded, 00-23)
            <mm>      - Minute (2 digits, 0-padded)
          The default filename is rockbox-sim-<target>-<version>
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
    elsif ($ARGV[0] eq "-s") {
        $strip =1;
    }
    elsif ($ARGV[0] eq "-w") {
        $cross =1;
    }
    elsif ($ARGV[0] eq "-r") {
        $version =$ARGV[1];
        shift @ARGV;
    }
    elsif ($ARGV[0] eq "-f") {
        $filename = $ARGV[1];
        shift @ARGV;
    }
    else {
        push(@doonly,$ARGV[0]);
    }
    shift @ARGV;
}

if($update) {
    # svn update!
    system("svn -q up");
}

$test = `sdl-config --libs`;
if ($test eq "") {
    printf("You don't appear to have sdl-config\n");
    die();
}

$rev = `tools/version.sh .`;
chomp $rev;
print "rev $rev\n" if($verbose);

if (@doonly) {
    printf("Build only %s\n", join(', ', @doonly)) if($verbose);
}

if (!defined($version)) {
    $version = $rev;
}

# made once for all targets
sub runone {
    my ($dir)=@_;
    my $a;

    if(@doonly > 0 && !grep(/^$dir$/, @doonly)) {
        return;
    }

    mkdir "build-$dir";
    chdir "build-$dir";
    print "Build in build-$dir\n" if($verbose);

    # build the target
    $a = buildit($dir);

    # Do not continue if the rockboxui executable is not created. This will
    #    prevent a good build getting overwritten by a bad build when
    #    uploaded to the web server.
    unless ( (-e "rockboxui") || (-e "rockboxui.exe") ) {
        print "No rockboxui, clean up and return\n" if($verbose);
        chdir "..";
        system("rm -rf build-$dir");
        return $a;
    }

    if ($strip) {
        print "Stripping binaries\n" if ($verbose);
        # find \( -name "*.exe" -o -name "*.rock" -o -name "*.codec" \) -exec ls -l "{}" ";"
        open(MAKE, "Makefile");
        my $AS=(grep(/^export AS=/, <MAKE>))[0];
        chomp($AS);
        (my $striptool = $AS) =~ s/^export AS=(.*)as$/$1strip/;
        
        $cmd = "find \\( -name 'rockboxui*' -o -iname '*dll' -o -name '*.rock' -o -name '*.codec' \\) -exec $striptool '{}' ';'";
        print("$cmd\n") if ($verbose);
        `$cmd`;
        close(MAKE);
    }

    chdir "..";

    my $newo=$filename;

    $newo =~ s/<version>/$version/g;
    $newo =~ s/<target>/$dir/g;
    $newo =~ s/<YYYY>/$year/g;
    $newo =~ s/<MM>/$mon/g;
    $newo =~ s/<DD>/$mday/g;
    $newo =~ s/<HH>/$hour/g;
    $newo =~ s/<mm>/$min/g;


    print "Zip up the sim and associated files\n" if ($verbose);
    mkpath(dirname($newo));
    system("mv build-$dir $newo");
    if ($cross) {
        print "Find and copy SDL.dll\n" if ($verbose);
        open(MAKE, "$newo/Makefile");
        my $GCCOPTS=(grep(/^export GCCOPTS=/, <MAKE>))[0];
        chomp($GCCOPTS);
        (my $sdldll = $GCCOPTS) =~ s/^export GCCOPTS=.*-I([^ ]+)\/include\/SDL.*$/$1\/bin\/SDL.dll/;
        print "Found $sdldll\n" if ($verbose);
        `cp $sdldll ./$newo/`;
        close(MAKE);
    }
    my $toplevel = getcwd();
    chdir(dirname($newo));
    $cmd = "zip -9 -r -q \"".basename($newo)."\" "
       . "\"".basename($newo)."\"/rockboxui* "
       . "\"".basename($newo)."\"/UI256.bmp "
       . "\"".basename($newo)."\"/SDL.dll "
       . "\"".basename($newo)."\"/simdisk ";
    print("$cmd\n") if($verbose);
    `$cmd`;
    chdir($toplevel);

    print "remove all contents in $newo\n" if($verbose);
    system("rm -rf $newo");

    return $a;
};

sub buildit {
    my ($target, $confnum, $extra)=@_;

    `rm -rf * >/dev/null 2>&1`;

    if ($cross) {
        $simstring = 'a\nw\ns\n\n';
    }
    else {
        $simstring = 's\n';
    }

    my $c = sprintf('printf "%s\n%s%s" | ../tools/configure',
                    $confnum, $extra, $simstring);

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make'\n" if($verbose);
    `make 2>/dev/null`;

    print "Run 'make install'\n" if($verbose);
    `make install 2>/dev/null`;
}

runone("player", "player", '\n');
runone("recorder", "recorder", '\n');
#runone("recorder8mb", "recorder", '8\n');
runone("fmrecorder", "fmrecorder", '\n');
#runone("fmrecorder8mb", "fmrecorder", '8\n');
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
#runone("ipodvideo64mb", "ipodvideo", '64\n');
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
runone("gigabeats", "gigabeats");
runone("sansae200", "e200");
runone("sansae200v2", "e200v2");
runone("sansac200", "c200");
runone("mrobe500", "mrobe500");
runone("mrobe100", "mrobe100");
runone("cowond2", "cowond2");
runone("clip", "clip");
runone("zvm30gb", "creativezvm30gb");
runone("zvm60gb", "creativezvm60gb");
runone("zenvision", "creativezenvision");
runone("hdd1630", "hdd1630");
runone("fuze", "fuze");
runone("m200v4", "m200v4");
runone("sa9200", "sa9200");
runone("sansac200v2", "c200v2");
runone("yh820", "yh820");
runone("yh920", "yh920");
runone("yh925", "yh925");
runone("ondavx747", "ondavx747");
runone("ondavx747p", "ondavx747p");
runone("ondavx777", "ondavx777");
#runone("ifp7xx", "ifp7xx");
runone("lyremini2440", "mini2440");
