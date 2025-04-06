#!/usr/bin/perl
require "./tools/builds.pm";

my $verbose;
if($ARGV[0] eq "-v") {
    $verbose =1;
    shift @ARGV;
}

my $tag = $ARGV[0];
my $version = $ARGV[1];

my $outdir = "output/bins";

my $cpus = `nproc`;

# made once for all targets
sub runone {
    my ($dir, $confnum, $extra)=@_;
    my $a;

    mkdir "buildb-$dir";
    chdir "buildb-$dir";
    print "Build in buildb-$dir\n" if($verbose);

    # build the manual(s)
    $a = buildit($dir, $confnum, $extra);

    chdir "..";

    my $o="buildb-$dir/rockbox.zip";
    my $map="buildb-$dir/rockbox-maps.zip";
    my $elf="buildb-$dir/rockbox-elfs.zip";
    if (-f $o) {
        my $newo="$outdir/rockbox-$dir-$version.zip";
        my $newmap="$outdir/rockbox-$dir-$version-maps.zip";
        my $newelf="$outdir/rockbox-$dir-$version-elfs.zip";
        system("mkdir -p $outdir");
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
        system("mv $map $newmap");
        print "moved $map to $newmap\n" if($verbose);
        system("mv $elf $newelf");
        print "moved $elf to $newelf\n" if($verbose);
    }

    print "remove all contents in build-$dir\n" if($verbose);
    system("rm -rf buildb-$dir");

    return $a;
};

sub fonts {
    my ($dir, $confnum, $newl)=@_;
    my $a;

    mkdir "buildf-$dir";
    chdir "buildf-$dir";
    print "Build fonts in buildf-$dir\n" if($verbose);

    # build the fonts
    $a = buildfonts($dir, $confnum, $newl);

    chdir "..";

    my $o="buildf-$dir/rockbox-fonts.zip";
    if (-f $o) {
        my $newo="$outdir/rockbox-fonts-$version.zip";
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
    }

    print "remove all contents in buildb-$dir\n" if($verbose);
    system("rm -rf buildf-$dir");

    return $a;
};

sub buildit {
    my ($target, $confnum, $extra)=@_;

    `rm -rf * >/dev/null 2>&1`;

    my $ram = $extra ? $extra : -1;
    my $c = "../tools/configure --type=n --target=$confnum --ram=$ram";

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make'\n" if($verbose);
    `make -j$cpus VERSION=$version`;

    print "Run 'make zip'\n" if($verbose);
    `make zip VERSION=$version`;

    print "Run 'make mapzip'\n" if($verbose);
    `make mapzip VERSION=$version`;

    print "Run 'make elfzip'\n" if($verbose);
    `make elfzip VERSION=$version`;
}

sub buildfonts {
    my ($target, $confnum, $newl)=@_;

    `rm -rf * >/dev/null 2>&1`;

    my $ram = $extra ? $extra : -1;
    my $c = "../tools/configure --type=n --target=$confnum --ram=$ram";

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make fontzip'\n" if($verbose);
    `make fontzip NODEPS=1`;
}

`git checkout $tag`;

# run make in tools first to make sure they're up-to-date
print "cd tools && make\n" if($verbose);
`(cd tools && make ) >/dev/null`;

for my $b (&stablebuilds) {
    my $configname = $builds{$b}{configname} ? $builds{$b}{configname} : $b;
    runone($b, $configname, $builds{$b}{ram});
}

fonts("fonts", "iaudiox5");
