#!/usr/bin/perl

# Very sparse docs:
# http://search.cpan.org/~cnandor/MP3-Info-1.02/Info.pm

# MP3::Info is installed on debian using package 'libmp3-info-perl'

# Rockbox song database docs:
# http://www.rockbox.org/twiki/bin/view/Main/TagDatabase

use MP3::Info;

my $db = "rockbox.id3db";
my $dir = ".";
my $strip;
my $verbose;
my $help;

while($ARGV[0]) {
    if($ARGV[0] eq "--db") {
        $db = $ARGV[1];
        shift @ARGV;
        shift @ARGV;
    }
    elsif($ARGV[0] eq "--path") {
        $dir = $ARGV[1];
        shift @ARGV;
        shift @ARGV;
    }
    elsif($ARGV[0] eq "--strip") {
        $strip = $ARGV[1];
        shift @ARGV;
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
my %entries;
my %genres;
my %albums;
my %years;
my %filename;

my $dbver = 1;

if(! -d $dir or $help) {
    print "'$dir' is not a directory\n" if (! -d $dir);
    print "songdb [--db <file>] [--path <dir>] [--strip <path>] [--verbose] [--help]\n";
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

my $maxsongperalbum;

sub dodir {
    my ($dir)=@_;

    print "$dir\n";

    # getdir() returns all entries in the given dir
    my @a = getdir($dir);

    # extractmp3 filters out only the mp3 files from all given entries
    my @m = extractmp3($dir, @a);

    my $f;

    for $f (sort @m) {
        
        my $id3 = singlefile("$dir/$f");

        # ARTIST
        # COMMENT
        # ALBUM
        # TITLE
        # GENRE
        # TRACKNUM
        # YEAR

        # don't index songs without tags
        if (not defined $$id3{'ARTIST'} and
            not defined $$id3{'ALBUM'} and
            not defined $$id3{'TITLE'})
        {
            next;
        }

        #printf "Artist: %s\n", $id3->{'ARTIST'};
        my $path = "$dir/$f";
        if ($strip ne "" and $path =~ /^$strip(.*)/) {
            $path = $1;
        }

        $entries{$path}= $id3;
        $artists{$id3->{'ARTIST'}}++ if($id3->{'ARTIST'});
        $genres{$id3->{'GENRE'}}++ if($id3->{'GENRE'});
        $years{$id3->{'YEAR'}}++ if($id3->{'YEAR'});

        # fallback names
        $$id3{'ARTIST'} = "<no artist tag>" if ($$id3{'ARTIST'} eq "");
        $$id3{'ALBUM'} = "<no album tag>" if ($$id3{'ALBUM'} eq "");
        $$id3{'TITLE'} = "<no title tag>" if ($$id3{'TITLE'} eq "");

        # prepend Artist name to handle duplicate album names from other
        # artists
        my $albumid = $id3->{'ALBUM'}."___".$id3->{'ARTIST'};
        if($albumid ne "<no album tag>___<no artist tag>") {
            my $num = ++$albums{$albumid};
            if($num > $maxsongperalbum) {
                $maxsongperalbum = $num;
                $longestalbum = $albumid;
            }
            $album2songs{$albumid}{$$id3{TITLE}} = $id3;
            $artist2albums{$$id3{ARTIST}}{$$id3{ALBUM}} = $id3;
        }
    }

    # extractdirs filters out only subdirectories from all given entries
    my @d = extractdirs($dir, @a);

    for $d (sort @d) {
        #print "Subdir: $d\n";
        dodir("$dir/$d");
    }
}


dodir($dir);
print "\n";

print "File name table\n" if ($verbose);
my $fc;
for(sort keys %entries) {
    printf("  %s\n", $_) if ($verbose);
    $fc += length($_)+1;
}

my $maxsonglen = 0;
my $sc;
print "\nSong title table\n" if ($verbose);

for(sort {$entries{$a}->{'TITLE'} cmp $entries{$b}->{'TITLE'}} keys %entries) {
    printf("  %s\n", $entries{$_}->{'TITLE'} ) if ($verbose);
    my $l = length($entries{$_}->{'TITLE'});
    if($l > $maxsonglen) {
        $maxsonglen = $l;
        $longestsong = $entries{$_}->{'TITLE'};
    }
}
$maxsonglen++; # include zero termination byte
while($maxsonglen&3) {
    $maxsonglen++;
}

my $maxartistlen = 0;
print "\nArtist table\n" if ($verbose);
my $i=0;
my %artistcount;
for(sort keys %artists) {
    printf("  %s\n", $_) if ($verbose);
    $artistcount{$_}=$i++;
    my $l = length($_);
    if($l > $maxartistlen) {
        $maxartistlen = $l;
        $longestartist = $_;
    }

    $l = scalar keys %{$artist2albums{$_}};
    if ($l > $maxalbumsperartist) {
        $maxalbumsperartist = $l;
    }
}
$maxartistlen++; # include zero termination byte
while($maxartistlen&3) {
    $maxartistlen++;
}

if ($verbose) {
    print "\nGenre table\n";
    for(sort keys %genres) {
        printf("  %s\n", $_);
    }

    print "\nYear table\n";
    for(sort keys %years) {
        printf("  %s\n", $_);
    }
}

print "\nAlbum table\n" if ($verbose);
my $maxalbumlen = 0;
my %albumcount;
$i=0;
for(sort keys %albums) {
    my @moo=split(/___/, $_);
    printf("  %s\n", $moo[0]) if ($verbose);
    $albumcount{$_} = $i++;
    my $l = length($moo[0]);
    if($l > $maxalbumlen) {
        $maxalbumlen = $l;
        $longestalbumname = $moo[0];
    }
}
$maxalbumlen++; # include zero termination byte
while($maxalbumlen&3) {
    $maxalbumlen++;
}



sub dumpint {
    my ($num)=@_;

#    print STDERR "int: $num\n";

    printf DB ("%c%c%c%c",
               $num>>24,
               ($num&0xff0000)>>16,
               ($num&0xff00)>>8,
               ($num&0xff));
}

if (!scalar keys %entries) {
    print "No songs found. Did you specify the right --path ?\n";
    print "Use the --help parameter to see all options.\n";
    exit;
}

if ($db) {
    my $songentrysize = $maxsonglen + 12;
    my $albumentrysize = $maxalbumlen + 4 + $maxsongperalbum*4;
    my $artistentrysize = $maxartistlen + $maxalbumsperartist*4;

    printf STDERR "Number of artists  : %d\n", scalar keys %artists;
    printf STDERR "Number of albums   : %d\n", scalar keys %albums;
    printf STDERR "Number of songs    : %d\n", scalar keys %entries;
    print STDERR "Max artist length  : $maxartistlen ($longestartist)\n";
    print STDERR "Max album length   : $maxalbumlen ($longestalbumname)\n";
    print STDERR "Max song length    : $maxsonglen ($longestsong)\n";
    print STDERR "Max songs per album: $maxsongperalbum ($longestalbum)\n";
    print STDERR "Database version: $dbver\n" if ($verbose);

    open(DB, ">$db") || die "couldn't make $db";
    printf DB "RDB%c", $dbver;
    
    $pathindex = 48; # paths always start at index 48

    $songindex = $pathindex + $fc; # fc is size of all paths
    $songindex++ while ($songindex & 3); # align to 32 bits

    dumpint($songindex); # file position index of song table
    dumpint(scalar(keys %entries)); # number of songs
    dumpint($maxsonglen); # length of song name field

    # set total size of song title table
    $sc = scalar(keys %entries) * $songentrysize;
    
    $albumindex = $songindex + $sc; # sc is size of all songs
    dumpint($albumindex); # file position index of album table
    dumpint(scalar(keys %albums)); # number of albums
    dumpint($maxalbumlen); # length of album name field
    dumpint($maxsongperalbum); # number of entries in songs-per-album array

    my $ac = scalar(keys %albums) * $albumentrysize;

    $artistindex = $albumindex + $ac; # ac is size of all albums
    dumpint($artistindex); # file position index of artist table
    dumpint(scalar(keys %artists)); # number of artists
    dumpint($maxartistlen); # length of artist name field
    dumpint($maxalbumsperartist); # number of entries in albums-per-artist array

    my $l=0;

    #### TABLE of file names ###
    # path1

    my %filenamepos;
    for $f (sort keys %entries) {
        printf DB ("%s\x00", $f);
        $filenamepos{$f}= $l;
        $l += length($f)+1;
    }
    while ($l & 3) {
        print DB "\x00";
        $l++;
    }

    #### TABLE of songs ###
    # title of song1
    # pointer to artist of song1
    # pointer to album of song1
    # pointer to filename of song1

    my $offset = $songindex;
    for(sort {$entries{$a}->{'TITLE'} cmp $entries{$b}->{'TITLE'}} keys %entries) {
        my $f = $_;
        my $id3 = $entries{$f};
        my $t = $id3->{'TITLE'};
        my $str = $t."\x00" x ($maxsonglen- length($t));

        print DB $str; # title

        my $a = $artistcount{$id3->{'ARTIST'}} * $artistentrysize;
        dumpint($a + $artistindex); # pointer to artist of this song

        $a = $albumcount{"$$id3{ALBUM}___$$id3{ARTIST}"} * $albumentrysize;
        dumpint($a + $albumindex); # pointer to album of this song

        # pointer to filename of this song
        dumpint($filenamepos{$f} + $pathindex);

        $$id3{'songoffset'} = $offset;
        $offset += $songentrysize;
    }

    #### TABLE of albums ###
    # name of album1
    # pointers to artists of album1
    # pointers to songs on album1

    for(sort keys %albums) {
        my $albumid = $_;
        my @moo=split(/___/, $_);
        my $t = $moo[0];
        my $str =  $t."\x00" x ($maxalbumlen - length($t));
        print DB $str;

        my $aoffset = $artistcount{$moo[0]} * $artistentrysize;
        dumpint($aoffset + $artistindex); # pointer to artist of this album

        my @songlist = keys %{$album2songs{$albumid}};
        my $id3 = $album2songs{$albumid}{$songlist[0]};
        if (defined $id3->{'TRACKNUM'}) {
            @songlist = sort {
                $album2songs{$albumid}{$a}->{'TRACKNUM'} <=>
                    $album2songs{$albumid}{$b}->{'TRACKNUM'}
            } @songlist;
        }
        else {
            @songlist = sort @songlist;
        }

        for (@songlist) {
            my $id3 = $album2songs{$albumid}{$_};
            dumpint($$id3{'songoffset'});
        }

        for (scalar keys %{$album2songs{$albumid}} .. $maxsongperalbum-1) {
            print DB "\x00\x00\x00\x00";
        }
    }
    
    #### TABLE of artists ###
    # name of artist1
    # pointers to albums of artist1

    for (sort keys %artists) {
        my $artist = $_;
        my $str =  $_."\x00" x ($maxartistlen - length($_));
        print DB $str;

        for (sort keys %{$artist2albums{$artist}}) {
            my $id3 = $artist2albums{$artist}{$_};
            my $a = $albumcount{"$$id3{'ALBUM'}___$$id3{'ARTIST'}"} * $albumentrysize;
            dumpint($a + $albumindex);
        }

        for (scalar keys %{$artist2albums{$artist}} .. $maxalbumsperartist-1) {
            print DB "\x00\x00\x00\x00";
        }

    }

    close(DB);
}
