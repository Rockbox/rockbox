#!/usr/bin/perl
use Switch;
use List::Util 'shuffle'; # standard from Perl 5.8 and later

my $tempfile = "multigcc.out";
my @params;
my @files;
my $list = \@params;

# parse command line arguments
for my $a (@ARGV) {
    if ($a eq "--") {
        $list = \@files;
        next;
    }

    push @{$list}, $a;
}

exit if (not @files);

my $command = join " ", @params;

# shuffle the file list to spread the load as evenly as we can
@files = shuffle(@files);  

# count number of cores
my $cores;
switch($^O) {
    case "darwin" {
        chomp($cores = `sysctl -n hw.ncpu`);
        $cores = 1 if ($?);
    }
    case "solaris" {
        $cores = scalar grep /on-line/i, `psrinfo`;
        $cores = 1 if ($?);
    }
    else {
        if (open CPUINFO, "</proc/cpuinfo") {
            $cores = scalar grep /^processor/i, <CPUINFO>;
            close CPUINFO;
        }
        else {
            $cores = 1;
        }
    }
}

# don't run empty children
if (scalar @files <= $cores)
{
    $cores = 1;
}

# fork children
my @pids;
my $slice = int((scalar @files + $cores) / $cores);
for my $i (0 .. $cores-1)
{
    my $pid = fork;
    if ($pid)
    {
        # mother
        $pids[$i] = $pid;
    }
    else
    {
        # get my slice of the files
        my @list = @files[$i * $slice .. $i * $slice + $slice - 1];

        # run command
        system("$command @list > $tempfile.$$");

        exit;
    }
}

for my $i (0 .. $cores - 1)
{
    # wait for child to complete
    waitpid $pids[$i], 0;

    # read & print result
    if (open F, "<$tempfile.$pids[$i]")
    {
        print <F>;
        close F;
        unlink "$tempfile.$pids[$i]";
    }
}
