#!/usr/bin/perl

# Calculate the highest possible location for an overlay based
# on a reference map file (.refmap)

sub map_scan {
    my ($map) = @_;
    my $ramstart = -1, $ramsize = -1, $startaddr = -1, $endaddr = -1;
    open (MAP, "<$map");
    while (<MAP>) {
        if ($_ =~ /^PLUGIN_RAM +0x([0-9a-f]+) +0x([0-9a-f]+)$/) {
            $ramstart = hex($1);
            $ramsize = hex($2);
        }
        elsif ($_ =~ / +0x([0-9a-f]+) +_?plugin_start_addr = ./) {
            $startaddr = hex($1);
        }
        elsif ($_ =~ / +0x([0-9a-f]+) +_?plugin_end_addr = ./) {
            $endaddr = hex($1);
        }
    }
    close (MAP);
    if ($ramstart < 0 || $ramsize < 0 || $startaddr < 0 || $endaddr < 0
        || $ramstart != $startaddr) {
        printf "Could not analyze map file.\n";
        exit 1;
    }
    return $ramstart + $ramsize - $endaddr;
}

printf map_scan($ARGV[0]) & ~0xf;
