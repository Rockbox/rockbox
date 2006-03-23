#!/usr/bin/perl

sub error {
    print STDERR ("Error: @_\n");
    exit(1);
}

sub warning {
    print STDERR ("Warning: @_\n");
}

# string (filename.map)
# return hash(string:hash(string:number))
sub read_map {
    open(MAP_FILE,$_[0]) || error("Couldn't open a map $_[0]");
    my %retval;
    while (<MAP_FILE>) {
        chomp;
        my @parts = split(/[[:space:]]+/);
        if (@parts != 5) {
            next;
        }
        if ($parts[1] =~ m/\.(text|data|rodata|bss|icode|idata|irodata|ibss)/) {
            my $region = $parts[1];
            my $number = $parts[2];
            @parts = split(/\//,$parts[4]);
            @parts = split(/[\(\)]/,$parts[$#parts]);
            my $library = $retval{$parts[0]};
            my %library =  %$library;
            my $object = $parts[$#parts];
            $library{$object . $region} = $number;
            $retval{$parts[0]} = \%library;
        }
    }
    close(MAP_FILE);
    return %retval;
}

# string (filename.[ao]), hash(string:number)
# return hash(number:string)
sub read_library {
    open(OBJECT_FILE,"objdump -t $_[0] |") || 
        error("Couldn't pipe objdump for $_[0]");
    my $library = $_[1];
    my %library = %$library;
    my %retval;
    my $object;
    while (<OBJECT_FILE>) {
        chomp;
        my @parts = split(/[[:space:]]+/);
        if ($parts[0] =~ m/:$/) {
            $object = $parts[0];
            $object =~ s/:$//;
            next;
        }
        if (@parts != 6) {
            next;
        }
        if ($parts[0] eq "") {
            next;
        }
        if ($parts[3] eq $parts[5]) {
            next;
        }
        if ($parts[3] =~ m/\.(text|data|rodata|bss|icode|idata|irodata|ibss)/) {
            my $region = $parts[3];
            my $symbolOffset = hex("0x" . $parts[0]);
            my $sectionOffset = hex($library{$object . $region});
            my $location = $symbolOffset + $sectionOffset;
            $retval{$location} = $parts[5] . "(" . $object . ")";
        }
    }
    close(OBJECT_FILE);
    return %retval;
}

# string (0xFFFFFFFF), hash(number:string)
# return string
sub get_name {
    my $location = hex($_[0]);
    my $offsets = $_[1];
    my %offsets = %$offsets;
    if (exists $offsets{$location}) {
        return $offsets{$location};
    } else {
        my $retval = $_[0];
        $retval =~ y/[A-Z]/[a-z]/;
        warning("No symbol found for $retval");
        return $retval;
    }
}

# string (filename), hash(number:string)
# return array(array(number,number,string))
sub create_list {
    open(PROFILE_FILE,$_[0]) || 
        error("Could not open profile file: $profile_file");
    my $offsets = $_[1];
    my $started = 0;
    my %pfds;
    while (<PROFILE_FILE>) {
        if ($started == 0) {
            if (m/^0x/) {
                $started = 1;
            } else {
                next;
            }
        }
        my @parts = split(/[[:space:]]+/);
        if ($parts[0] =~ m/^0x/) {
            my $callName = get_name($parts[0],$offsets);
            my $calls = $parts[1];
            my $ticks = $parts[2];
            my @pfd = ($calls,$ticks,$callName);
            if (exists $pfds{$callName}) {
                my $old_pfd = $pfds{$callName};
                $pfd[0]+=@$old_pfd[0];
                $pfd[1]+=@$old_pfd[1];
            }
            $pfds{$callName} = \@pfd;
        } else {
            last;
        }

    }
    close(PROFILE_FILE);
#    print("FUNCTIONS\tTOTAL_CALLS\tTOTAL_TICKS\n");
#    printf("     %4d\t   %8d\t   %8d\n",$pfds,$totalCalls,$totalTicks);
    return values(%pfds);
}

# array(array(number,number,string)), number (sort element)
sub print_sorted {
    my $pfds = $_[0];
    my @pfds = @$pfds;
    my $sort_index = $_[1];
    my $percent = $_[2];
    my %elements;
    my $totalCalls = 0;
    my $totalTicks = 0;
    $pfds = 0;
    foreach $element(@pfds) {
        $elements{@$element[$sort_index] . @$element[2]} = $element;
        $pfds++;
        $totalCalls += @$element[0];
        $totalTicks += @$element[1];
    }
    my @keys = sort(keys(%elements));
    print("FUNCTIONS\tTOTAL_CALLS\tTOTAL_TICKS\n");
    printf("     %4d\t   %8d\t   %8d\n",$pfds,$totalCalls,$totalTicks);
    foreach $key(@keys) {
        my $element = $elements{$key};
        if ($percent) {
            printf("Calls: %7.2f%% Ticks: %7.2f%% Symbol: %s\n",
                    @$element[0]/$totalCalls*100,
                    @$element[1]/$totalTicks*100,
                    @$element[2]);
        } else {
            printf("Calls: %08d Ticks: %08d Symbol: %s\n",
                    @$element);
        }
    }
}

# merges two hashes
sub merge_hashes {
    my $hash1 = $_[0];
    my $hash2 = $_[1];
    return (%$hash1,%$hash2);
}

sub usage {
    if (@_) {
        print STDERR ("Error: @_\n");
    }
    print STDERR ("USAGE:\n");
    print STDERR ("$0 profile.out map obj[...] [map obj[...]...] sort[...]\n");
    print STDERR 
        ("\tprofile.out  output from the profiler, extension is .out\n");
    print STDERR 
        ("\tmap          map file, extension is .map\n");
    print STDERR
        ("\tobj          library or object file, extension is .a or .o\n");
    print STDERR
        ("\tformat       0-2[_p] 0: by calls, 1: by ticks, 2: by name\n");
    print STDERR
        ("\t             _p shows percents instead of counts\n");
    print STDERR ("NOTES:\n");
    print STDERR
        ("\tmaps and objects come in sets, one map then many objects\n");
    exit(1);
}


if ($ARGV[0] =~ m/-(h|help|-help)/) {
    usage();
}
if (@ARGV < 2) {
    usage("Requires at least 2 arguments");
}
if ($ARGV[0] !~ m/\.out$/) {
    usage("Profile file must end in .out");
}
my $i = 1;
my %symbols;
{
    my %map;
    for (; $i < @ARGV; $i++) {
        my $file = $ARGV[$i];
        if ($file =~ m/\.map$/) {
            %map = read_map($file);
        } elsif ($file =~ m/\.[ao]$/) {
            if (!%map) {
                usage("No map file found before first object file");
            }
            my @parts = split(/\//,$file);
            my %new_symbols = read_library($file,$map{$parts[$#parts]});
            %symbols = merge_hashes(\%symbols,\%new_symbols);
        } else {
            last;
        }
    }
}
if (!%symbols) {
    warning("No symbols found");
}
my @pfds = create_list($ARGV[0],\%symbols);
for (; $i < @ARGV; $i++) {
    print_sorted(\@pfds,split("_",$ARGV[$i]));
}
