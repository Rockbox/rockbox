#!/usr/bin/perl

# Calculate the highest possible location for an overlay based
# on a reference map file (.refmap)

sub map_scan {
    # The buflib handle table is a few hundred bytes, just before
    # the plugin buffer. We assume it's never more than 1024 bytes.
    # If this assumption is wrong, overlay loading will fail.
    my $max_handle_table_size = 1024;
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
    return $ramstart + $ramsize - $endaddr - $max_handle_table_size;
}

printf map_scan($ARGV[0]) & ~0xf;
