#!/usr/bin/perl

sub getdir {
    my ($some_dir, $files)=@_;
    opendir(DIR, $some_dir) || die "can't opendir $some_dir: $!";
    my @all = grep { /$files$/ && -f "$some_dir/$_" } readdir(DIR);
    closedir DIR;
    return @all;
}

my @all=getdir(".", "\.c");
my @pluginhead=getdir("lib", "\.h");

for(@pluginhead) {
    $plug{$_}=$_;
}

my %head2lib=('Tremor' => 'libTremor');

my $s;
foreach $s (sort @all) {

    my $plib=0;
    my $codec;
    open(F, "<$s");
    while(<F>) {
        if($_ =~ /^ *\#include [\"<]([^\"]+)[\">]/) {
            my $f = $1;
            if($plug{$f}) {
                $plib=1;
            }
            if($f =~ /codecs\/([^\/]+)/) {
                $codec=$1;
                my $d = $head2lib{$codec};
                if($d) {
                    $codec = $d;
                }
            }
        }
    }

    #print "$s uses $plib and $codec\n";

    $s =~ s/\.c//;
    printf("\$(OBJDIR)/$s.elf: \$(OBJDIR)/$s.o \$(LINKFILE)%s%s\n\t\$(ELFIT)\n\n",
           $plib?" \$(OBJDIR)/libplugin.a":"",
           $codec?" \$(OBJDIR)/$codec.a":"");
}
