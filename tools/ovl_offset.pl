#!/usr/bin/perl

# Calculate the highest possible location for an overlay based
# on a reference ELF file (.refelf)
# Reads plugin_load_end_addr and plugin_ram_end symbols from the ELF
# via nm, then computes the overlay offset.

sub main {
    # The buflib handle table is a few hundred bytes, just before
    # the plugin buffer. We assume it's never more than 1024 bytes.
    # If this assumption is wrong, overlay loading will fail.
    my $max_handle_table_size = 1024;

    my ($elf) = @_;
    my $ram_end = -1;
    my $load_end = -1;

    open (NM, "nm $elf |") or die "Cannot run nm on $elf: $!\n";
    while (<NM>) {
        if ($_ =~ /^([0-9a-fA-F]+)\s+\S+\s+plugin_ram_end$/) {
            $ram_end = hex($1);
        }
        elsif ($_ =~ /^([0-9a-fA-F]+)\s+\S+\s+plugin_load_end_addr$/) {
            $load_end = hex($1);
        }
    }
    close(NM);

    if ($ram_end < 0 || $load_end < 0) {
        printf STDERR "Could not read symbols from $elf\n";
        printf STDERR "  plugin_ram_end: %s\n", $ram_end < 0 ? "not found" : sprintf("0x%x", $ram_end);
        printf STDERR "  plugin_load_end_addr: %s\n", $load_end < 0 ? "not found" : sprintf("0x%x", $load_end);
        exit 1;
    }

    return ($ram_end - $load_end - $max_handle_table_size) & ~0xf;
}

printf main($ARGV[0]);
