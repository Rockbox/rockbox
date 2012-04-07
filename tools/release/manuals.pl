#!/usr/bin/perl

$version="3.11.1";

require "tools/builds.pm";

my $verbose;
if($ARGV[0] eq "-v") {
    $verbose =1;
    shift @ARGV;
}

my $doonly;
if($ARGV[0]) {
    $doonly = $ARGV[0];
    print "only build $doonly\n" if($verbose);
}

# made once for all targets
sub runone {
    my ($dir)=@_;
    my $a;

    if($doonly && ($doonly ne $dir)) {
        return;
    }

    mkdir "buildm-$dir";
    chdir "buildm-$dir";
    print "Build in buildm-$dir\n" if($verbose);

    # build the manual(s)
    $a = buildit($dir);

    chdir "..";

    my $o="buildm-$dir/manual/rockbox-build.pdf";
    if (-f $o) {
        my $newo="output/rockbox-$dir-$version.pdf";
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
    }

    $o="buildm-$dir/rockbox-manual.zip";
    if (-f $o) {
        my $newo="output/rockbox-$dir-$version-html.zip";
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

    print "Run 'make'\n" if($verbose);
    `make manual VERSION=$version 2>/dev/null`;

    print "Run 'make manual-zip'\n" if($verbose);
    `make manual-zip VERSION=$version 2>/dev/null`;
}

# run make in tools first to make sure they're up-to-date
`(cd tools && make ) >/dev/null 2>&1`;

for my $b (&stablebuilds) {
    next if (length($builds{$b}{configname}) > 0); # no variants

    runone($b);
}
