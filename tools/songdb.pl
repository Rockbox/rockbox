#!/usr/bin/perl

# Very sparse docs:
# http://search.cpan.org/~cnandor/MP3-Info-1.02/Info.pm

# MP3::Info is installed on debian using package 'libmp3-info-perl'

# Rockbox song database docs:
# http://www.rockbox.org/twiki/bin/view/Main/TagDatabase

use MP3::Info;

my $db;
my $dir;
my $strip;

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

if(! -d $dir) {
    print "songdb [--db <file>] --path <dir> [--strip <path>]\n";
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

my $maxsongperalbum;

sub dodir {
    my ($dir)=@_;

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

        #printf "Artist: %s\n", $id3->{'ARTIST'};
        $entries{"$dir/$f"}= $id3;
        $filename{$id3}="$dir/$f";
        $artists{$id3->{'ARTIST'}}++ if($id3->{'ARTIST'});
        $genres{$id3->{'GENRE'}}++ if($id3->{'GENRE'});
        $years{$id3->{'YEAR'}}++ if($id3->{'YEAR'});

        $id3->{'FILE'}="$dir/$f"; # store file name

        # prepend Artist name to handle duplicate album names from other
        # artists
        if($id3->{'ALBUM'}) {
            my $num = $albums{$id3->{'ARTIST'}."___".$id3->{'ALBUM'}}++;
            if($num > $maxsongperalbum) {
                $maxsongperalbum = $num;
            }
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

print "File name table\n";
my $fc;
for(sort keys %entries) {
    printf("  %s\n", $_);
    $fc += length($_)+1;
}

my $maxsonglen;
print "\nSong title table\n";
#for(sort {$entries{$a}->{'TITLE'} cmp $entries{$b}->{'TITLE'}} %entries) {
for(sort {$entries{$a}->{'TITLE'} cmp $entries{$b}->{'TITLE'}} keys %entries) {
    printf("  %s\n", $entries{$_}->{'TITLE'} );
    my $l = length($entries{$_}->{'TITLE'});
    if($l > $maxsonglen) {
        $maxsonglen = $l;
    }
}
while($maxsonglen&3) {
    $maxsonglen++;
}

my $maxaristlen;
print "\nArtist table\n";
my $i=0;
my %artistcount;
for(sort keys %artists) {
    printf("  %s\n", $_);
    $artistcount{$_}=$i++;
    my $l = length($_);
    if($l > $maxaristlen) {
        $maxaristlen = $l;
    }
}
while($maxaristlen&3) {
    $maxaristlen++;
}

print "\nGenre table\n";
for(sort keys %genres) {
    printf("  %s\n", $_);
}

print "\nYear table\n";
for(sort keys %years) {
    printf("  %s\n", $_);
}

print "\nAlbum table\n";
my $maxalbumlen;
my %albumcount;
$i=0;
for(sort keys %albums) {
    my @moo=split(/___/, $_);
    printf("  %s\n", $moo[1]);
    $albumcount{$moo[1]} = $i++;
    my $l = length($moo[1]);
    if($l > $maxalbumlen) {
        $maxalbumlen = $l;
    }
}
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

if($db) {
    print STDERR "\nCreating db $db\n";

    print STDERR "Max song length: $maxsonglen\n";
    print STDERR "Max album length: $maxalbumlen\n";
    print STDERR "Database version: $dbver\n";

    open(DB, ">$db") || die "couldn't make $db";
    printf DB "RDB%c", $dbver;

    dumpint(48); # file position index of song table
    dumpint(scalar(keys %entries)); # number of songs
    dumpint($maxsonglen); # length of song name field

    dumpint(48 + $fc); # file position index of album table
    dumpint(scalar(keys %albums)); # number of albums
    dumpint($maxalbumlen); # length of album name field
    dumpint($maxsongperalbum); # number of entries in songs-per-album array

    my $ac = ($maxalbumlen + 8 ) * scalar(keys %albums);

    dumpint(48 + $fc + $ac); # file position index of artist table
    dumpint(scalar(keys %artists)); # number of artists
    dumpint($maxaristlen); # length of artist name field
    dumpint($maxalbumsperartist); # number of entries in albums-per-artist array

    # 12 x 4 = 48 bytes written before this point
    my $l=48;

    #### TABLE of file names ###
    # path1

    my %filenamepos;
    for $f (sort keys %entries) {
        printf DB ("%s\x00", $f);
        $filenamepos{$f}= $l;
        $l += length($f)+1;
    }

    #### TABLE of songs ###
    # title of song1
    # pointer to artist of song1
    # pointer to album of song1
    # pointer to filename of song1

    for(sort {$entries{$a}->{'TITLE'} cmp $entries{$b}->{'TITLE'}} keys %entries) {
        my $f = $_;
        my $id3 = $entries{$f};
        my $t = $id3->{'TITLE'};
        my $str = $t."\x00" x ($maxsonglen- length($t));
        printf DB ("%s\x00", $str); # title

        my $a = $artistcount{$id3->{'ARTIST'}} * $maxaristlen;
        dumpint($a); # pointer to artist of this song

        $a = $albumcount{$id3->{'ALBUM'}} * $maxalbumlen;
        dumpint($a); # pointer to album of this song

        # pointer to filename of this song
        dumpint($filenamepos{$id3->{'FILE'}});
    }

    #### TABLE of albums ###
    # name of album1
    # pointers to artists of album1
    # pointers to songs on album1

    for(sort keys %albums) {
        my @moo=split(/___/, $_);
        my $t = $moo[1];
        my $str =  $t."\x00" x ($maxalbumlen - length($t));
        printf DB ("%s\x00", $str);

        dumpint(14); # pointers to artists of album1
        dumpint(15); # pointers to songs on album1
    }
    
    #### TABLE of artists ###
    # name of artist1
    # pointers to albums of artist1

    for(sort keys %artists) {
        printf DB ("%s\x00", $_);
    }

    close(DB);
}
