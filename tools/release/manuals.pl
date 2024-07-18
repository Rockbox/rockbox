#!/usr/bin/perl -w
require "./tools/builds.pm";

require "./tools/builds.pm";

my $verbose;
if($ARGV[0] eq "-v") {
    $verbose =1;
    shift @ARGV;
}

my $tag = $ARGV[0];
my $version = $ARGV[1];

my $outdir = "output/manuals";

# made once for all targets
sub runone {
    my ($dir)=@_;
    my $a;

    mkdir "buildm-$dir";
    chdir "buildm-$dir";
    print "Build in buildm-$dir\n" if($verbose);

    # build the manual(s)
    $a = buildit($dir);

    chdir "..";

    my $o="buildm-$dir/rockbox-manual.pdf";
    if (-f $o) {
        my $newo="$outdir/rockbox-$dir-$version.pdf";
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
    } else {
	print "buildm-$dir/rockbox-$dir-$version.pdf not found\n";
    }

    $o="buildm-$dir/rockbox-manual.zip";
    if (-f $o) {
        my $newo="$outdir/rockbox-$dir-$version-html.zip";
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
    }

    print "remove all contents in buildm-$dir\n" if($verbose);
    system("rm -rf buildm-$dir");

    return $a;
};

sub buildit {
    my ($target)=@_;

    `rm -rf * >/dev/null 2>&1`;

    my $c = "../tools/configure --target=$target --type=m --ram=0";

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make manual'\n" if($verbose);
    `make manual VERSION=$version 2>/dev/null`;

    print "Run 'make manual-zip'\n" if($verbose);
    `make manual-zip VERSION=$version 2>/dev/null`;
}

`git checkout $tag`;

# run make in tools first to make sure they're up-to-date
`(cd tools && make ) >/dev/null 2>&1`;

`mkdir -p $outdir`;

for my $b (&manualbuilds) {
    runone($b);
}
