#!/usr/bin/perl

# Very sparse docs:
# http://search.cpan.org/~cnandor/MP3-Info-1.02/Info.pm

# MP3::Info is installed on debian using package 'libmp3-info-perl'

use MP3::Info;

my $dir = $ARGV[0];

if(! -d $dir) {
    print "given argument is not a directory!\n";
    exit;
}

# return ALL directory entries in the given dir
sub getdir {
    my ($dir) = @_;

    opendir(DIR, $dir) || die "can't opendir $dir: $!";
    #   my @mp3 = grep { /\.mp3$/ && -f "$dir/$_" } readdir(DIR);
    my @all = readdir(DIR);
    closedir DIR;
    return @all;
}

sub extractmp3 {
    my ($dir, @files) = @_;
    my @mp3;
    for(@files) {
        if( /\.mp3$/ && -f "$dir/$_" ) {
            push @mp3, $_;
        }
    }
    return @mp3;
}

sub extractdirs {
    my ($dir, @files) = @_;
    my @dirs;
    for(@files) {
        if( -d "$dir/$_" && ($_ !~ /^\.(|\.)$/)) {
            push @dirs, $_;
        }
    }
    return @dirs;
}

sub singlefile {
    my ($file) = @_;

#    print "Check $file\n";

    my $hash = get_mp3tag($file);
  #  my $hash = get_mp3info($file);

#    for(keys %$hash) {
#        print "Info: $_ ".$hash->{$_}."\n";
#    }

    return $hash; # a hash reference
}

sub dodir {
    my ($dir)=@_;

    # getdir() returns all entries in the given dir
    my @a = getdir($dir);

    # extractmp3 filters out only the mp3 files from all given entries
    my @m = extractmp3($dir, @a);

    my $f;

    for $f (sort @m) {
        
        my $id3 = singlefile("$dir/$f");

        printf "Artist: %s\n", $id3->{'ARTIST'};
    }

    # extractdirs filters out only subdirectories from all given entries
    my @d = extractdirs($dir, @a);

    for $d (sort @d) {
        print "Subdir: $d\n";
        dodir("$dir/$d");
    }
}


dodir($dir);
