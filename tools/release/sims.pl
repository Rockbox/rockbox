#!/usr/bin/perl
# This is basically a copy of tools/release/bins.pl, with small changes.
use File::Basename;
use File::Path;
use Cwd;

require "tools/builds.pm";

my ($verbose, $strip, $update, $doonly, $version);
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
    my ($dir, $extra)=@_;
    my $a;

    if(@doonly > 0 && !grep(/^$dir$/, @doonly)) {
        return;
    }

    mkdir "build-$dir";
    chdir "build-$dir";
    print "Build in build-$dir\n" if($verbose);

    # build the target
    $a = buildit($dir, $extra);

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
    my $toplevel = getcwd();
    chdir(dirname($newo));
    $cmd = "zip -9 -r -q \"".basename($newo)."\" "
       . "\"".basename($newo)."\"/rockboxui* "
       . "\"".basename($newo)."\"/UI256.bmp "
       . "\"".basename($newo)."\"/simdisk ";
    print("$cmd\n") if($verbose);
    `$cmd`;
    chdir($toplevel);

    print "remove all contents in $newo\n" if($verbose);
    system("rm -rf $newo");

    return $a;
};

sub buildit {
    my ($dir, $extra)=@_;

    `rm -rf * >/dev/null 2>&1`;

    if ($cross) {
        $simstring = 'a\nw\ns\n\n';
    }
    else {
        $simstring = 's\n';
    }

    my $c = sprintf('printf "%s\n%s%s" | ../tools/configure',
                    $dir, $extra, $simstring);

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make'\n" if($verbose);
    `make 2>/dev/null`;

    print "Run 'make install'\n" if($verbose);
    `make install 2>/dev/null`;
}

for my $b (sort byname keys %builds) {
    if ($builds{$b}{status} >= 2)
    {
        # ipodvideo64mb uses the ipodvideo simulator
        # sansae200r uses the sansae200 simulator
        if ($b ne 'ipodvideo64mb' && $b ne 'sansae200r')
        {
            if ($builds{$b}{ram} ne '')
            {
                # These builds need the ram size sent to configure
                runone($b, $builds{$b}{ram} . '\n');
            }
            else
            {
                runone($b);
            }
        }
    }
}

#The following ports are in the unusable category, but the simulator does build
runone("mini2440");
runone("ondavx747");
runone("ondavx747p");
runone("ondavx777");
runone("sansam200v4");
runone("zenvision");
runone("zenvisionm30gb");
runone("zenvisionm60gb");
runone("creativezenxfi2");
runone("creativezenxfi3");
runone("sonynwze360");
runone("sonynwze370");
runone("creativezenxfi");
runone("creativezen");
runone("creativezenmozaic");
runone("xduoox3");
