#!/usr/bin/perl

$codecs="apps/codecs";

opendir(DIR, $codecs) || die "can't opendir $some_dir: $!";
my @maps = sort grep { /\.map/ && -f "$codecs/$_" } readdir(DIR);
closedir DIR;

print "Codec             IRAM IBSS    BSS    Text \n";


for my $m (@maps) {
    my ($iram, $ibss, $bss, $text)=scanmap($m);    
    printf("%-15s: %5d %5d %6d %6d\n",
           $m, $iram, $ibss, $bss, $text);
}

sub scanmap {
    my ($file)=@_;

    open(F, "<$codecs/$file");

    while(<F>) {
        if(/[ \t]*0x([0-9a-f]+) *_plugin_start_addr/) {
            #print "CODEC START: $1\n";
            $codec = hex($1);
        }
        elsif(/[ \t]*0x([0-9a-f]+) *plugin_bss_start/) {
            #print "CODEC BSS START: $1\n";
            $codecbss = hex($1);
        }
        elsif(/[ \t]*0x([0-9a-f]+) *_plugin_end_addr/) {
            #print "CODEC END: $1\n";
            $bss = (hex($1) - $codecbss);
            $codec = (hex($1) - $codec - $bss);
        }
        elsif(/[ \t]*0x([0-9a-f]+) *iramstart/) {
            #print "IRAM START: $1\n";
            $iram = hex($1);
        }
        elsif(/[ \t]*0x([0-9a-f]+) *iramend/) {
            #print "IRAM END: $1\n";
            $iram = (hex($1) - $iram);
        }
        elsif(/[ \t]*0x([0-9a-f]+) *iedata/) {
            #print "IBSS START: $1\n";
            $ibss = hex($1);
        }
        elsif(/[ \t]*0x([0-9a-f]+) *iend/) {
            #print "IBSS END: $1\n";
            $ibss = (hex($1) - $ibss);
        }
    }

    close(F);

    return ($iram, $ibss, $bss, $codec);
}
