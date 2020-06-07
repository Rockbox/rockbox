#!/usr/bin/perl
#
# Rockbox song database docs:
# http://www.rockbox.org/wiki/DataBase
#

use mp3info;
use vorbiscomm;

# configuration settings
my $db = "database";
my $dir;
my $strip;
my $add;
my $verbose;
my $help;
my $dirisalbum;
my $littleendian = 0;
my $dbver = 0x54434806;

# file data
my %entries;

while($ARGV[0]) {
    if($ARGV[0] eq "--path") {
        $dir = $ARGV[1];
        shift @ARGV;
        shift @ARGV;
    }
    elsif($ARGV[0] eq "--db") {
        $db = $ARGV[1];
        shift @ARGV;
        shift @ARGV;
    }
    elsif($ARGV[0] eq "--strip") {
        $strip = $ARGV[1];
        shift @ARGV;
        shift @ARGV;
    }
    elsif($ARGV[0] eq "--add") {
        $add = $ARGV[1];
        shift @ARGV;
        shift @ARGV;
    }				
    elsif($ARGV[0] eq "--dirisalbum") {
        $dirisalbum = 1;
        shift @ARGV;
    }
    elsif($ARGV[0] eq "--littleendian") {
        $littleendian = 1;
        shift @ARGV;
    }
    elsif($ARGV[0] eq "--verbose") {
        $verbose = 1;
        shift @ARGV;
    }
    elsif($ARGV[0] eq "--help" or ($ARGV[0] eq "-h")) {
        $help = 1;
        shift @ARGV;
    }
    else {
        shift @ARGV;
    }
}

if(! -d $dir or $help) {
    print "'$dir' is not a directory\n" if ($dir ne "" and ! -d $dir);
    print <<MOO

songdb --path <dir> [--db <file>] [--strip <path>] [--add <path>] [--dirisalbum] [--littleendian] [--verbose] [--help]

Options:

  --path <dir>   Where your music collection is found
  --db <file>    Prefix for output files. Defaults to database.
  --strip <path> Removes this string from the left of all file names
  --add <path>   Adds this string to the left of all file names
  --dirisalbum   Use dir name as album name if the album name is missing in the
                 tags
  --littleendian Write out data as little endian (for x86 simulators and ARM-
                 based targets such as iPods and iriver H10)
  --verbose      Shows more details while working
  --help         This text  
MOO
;
    exit;
}

sub get_oggtag {
    my $fn = shift;
    my %hash;

    my $ogg = vorbiscomm->new($fn);

    my $h= $ogg->load;

    # Convert this format into the same format used by the id3 parser hash

    foreach my $k ($ogg->comment_tags())
    {
        foreach my $cmmt ($ogg->comment($k))
        {
            my $n;
            if($k =~ /^artist$/i) {
                $n = 'ARTIST';
            }
            elsif($k =~ /^album$/i) {
                $n = 'ALBUM';
            }
            elsif($k =~ /^title$/i) {
                $n = 'TITLE';
            }
            $hash{$n}=$cmmt if($n);
        }
    }

    return \%hash;
}

sub get_ogginfo {
    my $fn = shift;
    my %hash;

    my $ogg = vorbiscomm->new($fn);

    my $h= $ogg->load;

    return $ogg->{'INFO'};
}

# return ALL directory entries in the given dir
sub getdir {
    my ($dir) = @_;

    $dir =~ s|/$|| if ($dir ne "/");

    if (opendir(DIR, $dir)) {
        my @all = readdir(DIR);
        closedir DIR;
        return @all;
    }
    else {
        warn "can't opendir $dir: $!\n";
    }
}

sub extractmp3 {
    my ($dir, @files) = @_;
    my @mp3;
    for(@files) {
        if( (/\.mp[23]$/i || /\.ogg$/i) && -f "$dir/$_" ) {
            push @mp3, $_;
        }
    }
    return @mp3;
}

sub extractdirs {
    my ($dir, @files) = @_;
    $dir =~ s|/$||;
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
    my $hash;
    my $info;

    if($file =~ /\.ogg$/i) {
        $hash = get_oggtag($file);
        $info = get_ogginfo($file);
    }
    else {
        $hash = get_mp3tag($file);
        $info = get_mp3info($file);
        if (defined $$info{'BITRATE'}) {
            $$hash{'BITRATE'} = $$info{'BITRATE'};
        }

        if (defined $$info{'SECS'}) {
            $$hash{'SECS'} = $$info{'SECS'};
        }
    }

    return $hash;
}

sub dodir {
    my ($dir)=@_;

    my %lcartists;
    my %lcalbums;

    print "$dir\n";

    # getdir() returns all entries in the given dir
    my @a = getdir($dir);

    # extractmp3 filters out only the mp3 files from all given entries
    my @m = extractmp3($dir, @a);

    my $f;

    for $f (sort @m) {
        
        my $id3 = singlefile("$dir/$f");

        if (not defined $$id3{'ARTIST'} or $$id3{'ARTIST'} eq "") {
            $$id3{'ARTIST'} = "<Untagged>";
        }

        # Only use one case-variation of each artist
        if (exists($lcartists{lc($$id3{'ARTIST'})})) {
            $$id3{'ARTIST'} = $lcartists{lc($$id3{'ARTIST'})};
        }
        else {
            $lcartists{lc($$id3{'ARTIST'})} = $$id3{'ARTIST'};
        }
        #printf "Artist: %s\n", $$id3{'ARTIST'};

        if (not defined $$id3{'ALBUM'} or $$id3{'ALBUM'} eq "") {
            $$id3{'ALBUM'} = "<Untagged>";
            if ($dirisalbum) {
                $$id3{'ALBUM'} = $dir;
            }
        }

        # Only use one case-variation of each album
        if (exists($lcalbums{lc($$id3{'ALBUM'})})) {
            $$id3{'ALBUM'} = $lcalbums{lc($$id3{'ALBUM'})};
        }
        else {
            $lcalbums{lc($$id3{'ALBUM'})} = $$id3{'ALBUM'};
        }
        #printf "Album: %s\n", $$id3{'ALBUM'};

        if (not defined $$id3{'GENRE'} or $$id3{'GENRE'} eq "") {
            $$id3{'GENRE'} = "<Untagged>";
        }
        #printf "Genre: %s\n", $$id3{'GENRE'};

        if (not defined $$id3{'TITLE'} or $$id3{'TITLE'} eq "") {
            # fall back on basename of the file if no title tag.
            ($$id3{'TITLE'} = $f) =~ s/\.\w+$//;
        }
        #printf "Title: %s\n", $$id3{'TITLE'};

        my $path = "$dir/$f";
        if ($strip ne "" and $path =~ /^$strip(.*)/) {
            $path = $1;
        }

        if ($add ne "") {
            $path = $add . $path;
        }
        #printf "Path: %s\n", $path;

        if (not defined $$id3{'COMPOSER'} or $$id3{'COMPOSER'} eq "") {
            $$id3{'COMPOSER'} = "<Untagged>";
        }
        #printf "Composer: %s\n", $$id3{'COMPOSER'};

        if (not defined $$id3{'YEAR'} or $$id3{'YEAR'} eq "") {
            $$id3{'YEAR'} = "-1";
        }
        #printf "Year: %s\n", $$id3{'YEAR'};

        if (not defined $$id3{'TRACKNUM'} or $$id3{'TRACKNUM'} eq "") {
            $$id3{'TRACKNUM'} = "-1";
        }
        #printf "Track num: %s\n", $$id3{'TRACKNUM'};

        if (not defined $$id3{'BITRATE'} or $$id3{'BITRATE'} eq "") {
            $$id3{'BITRATE'} = "-1";
        }
        #printf "Bitrate: %s\n", $$id3{'BITRATE'};

        if (not defined $$id3{'SECS'} or $$id3{'SECS'} eq "") {
            $$id3{'SECS'} = "-1";
        }
        #printf "Length: %s\n", $$id3{'SECS'};

        $$id3{'PATH'} = $path;
        $entries{$path} = $id3;
    }

    # extractdirs filters out only subdirectories from all given entries
    my @d = extractdirs($dir, @a);
    my $d;

    for $d (sort @d) {
        $dir =~ s|/$||;
        dodir("$dir/$d");
    }
}

dodir($dir);
print "\n";

sub dumpshort {
    my ($num)=@_;

    #    print "int: $num\n";

    if ($littleendian) {
        print DB pack "v", $num;
    }
    else {    
        print DB pack "n", $num;
    }
}

sub dumpint {
    my ($num)=@_;

#    print "int: $num\n";

    if ($littleendian) {
        print DB pack "V", $num;
    }
    else {    
        print DB pack "N", $num;
    }
}

sub dump_tag_string {
    my ($s, $index) = @_;

    my $strlen = length($s)+1;
    my $padding = $strlen%4;
    if ($padding > 0) {
        $padding = 4 - $padding;
        $strlen += $padding;
    }

    dumpshort($strlen);
    dumpshort($index);
    print DB $s."\0";

    for (my $i = 0; $i < $padding; $i++) {
	    print DB "X";
    }
}

sub dump_tag_header {
    my ($entry_count) = @_;

    my $size = tell(DB) - 12;
    seek(DB, 0, 0);
    
    dumpint($dbver);
    dumpint($size);
    dumpint($entry_count);
}

sub openfile {
    my ($f) = @_;
    open(DB, "> $f") || die "couldn't open $f";
    binmode(DB);
}

sub create_tagcache_index_file {
    my ($index, $key, $unique) = @_;

    my $num = 0;
    my $prev = "";
    my $offset = 12;

    openfile $db ."_".$index.".tcd";
    dump_tag_header(0);

    for(sort {uc($entries{$a}->{$key}) cmp uc($entries{$b}->{$key})} keys %entries) {
        if (!$unique || !($entries{$_}->{$key} eq $prev)) {
            my $index;

            $num++;
            $prev = $entries{$_}->{$key};
            $offset = tell(DB);
            printf("  %s\n", $prev) if ($verbose);
            
            if ($unique) {
                $index = 0xFFFF;
            }
            else {
                $index = $entries{$_}->{'INDEX'};
            }
            dump_tag_string($prev, $index);
        }
        $entries{$_}->{$key."_OFFSET"} = $offset;
    }

    dump_tag_header($num);
    close(DB);
}

if (!scalar keys %entries) {
    print "No songs found. Did you specify the right --path ?\n";
    print "Use the --help parameter to see all options.\n";
    exit;
}

my $i = 0;
for (sort keys %entries) {
    $entries{$_}->{'INDEX'} = $i;
    $i++;
}

if ($db) {
    # tagcache index files
    create_tagcache_index_file(0, 'ARTIST', 1);
    create_tagcache_index_file(1, 'ALBUM', 1);
    create_tagcache_index_file(2, 'GENRE', 1);
    create_tagcache_index_file(3, 'TITLE', 0);
    create_tagcache_index_file(4, 'PATH', 0);
    create_tagcache_index_file(5, 'COMPOSER', 1);

    # Master index file
    openfile $db ."_idx.tcd";
    dump_tag_header(0);

    # current serial
    dumpint(0);

    for (sort keys %entries) {
        dumpint($entries{$_}->{'ARTIST_OFFSET'});
        dumpint($entries{$_}->{'ALBUM_OFFSET'});
        dumpint($entries{$_}->{'GENRE_OFFSET'});
        dumpint($entries{$_}->{'TITLE_OFFSET'});
        dumpint($entries{$_}->{'PATH_OFFSET'});
        dumpint($entries{$_}->{'COMPOSER_OFFSET'});
        dumpint($entries{$_}->{'YEAR'});
        dumpint($entries{$_}->{'TRACKNUM'});
        dumpint($entries{$_}->{'BITRATE'});
        dumpint($entries{$_}->{'SECS'});
        # play count
        dumpint(0);
        # play time
        dumpint(0);
        # last played
        dumpint(0);
        # status flag
        dumpint(0);
    }

    dump_tag_header(scalar keys %entries);
    close(DB);
}
