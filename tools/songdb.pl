#!/usr/bin/perl
#
# Rockbox song database docs:
# http://www.rockbox.org/twiki/bin/view/Main/TagDatabase
#
# MP3::Info by Chris Nandor is included verbatim in this script to make
# it runnable standalone on removable drives. See below.
#

my $db = "rockbox.id3db";
my $dir;
my $strip;
my $add;
my $verbose;
my $help;
my $dirisalbum;
my $dirisalbumname;

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
    elsif($ARGV[0] eq "--add") {
        $add = $ARGV[1];
        shift @ARGV;
        shift @ARGV;
    }				
    elsif($ARGV[0] eq "--verbose") {
        $verbose = 1;
        shift @ARGV;
    }
    elsif($ARGV[0] eq "--dirisalbum") {
    	$dirisalbum = 1;
	shift @ARGV;
    }
    elsif($ARGV[0] eq "--dirisalbumname") {
    	$dirisalbumname = 1;
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

my %lcartists;
my %lcalbums;

my %dir2albumname;

my $dbver = 3;

if(! -d $dir or $help) {
    print "'$dir' is not a directory\n" if ($dir ne "" and ! -d $dir);
    print "songdb --path <dir> [--dirisalbum] [--dirisalbumname] [--db <file>] [--strip <path>] [--add <path>] [--verbose] [--help]\n";
    exit;
}

# return ALL directory entries in the given dir
sub getdir {
    my ($dir) = @_;

    $dir =~ s|/$|| if ($dir ne "/");

    if (opendir(DIR, $dir)) {
        #   my @mp3 = grep { /\.mp3$/ && -f "$dir/$_" } readdir(DIR);
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
        if( /\.mp[23]$/ && -f "$dir/$_" ) {
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
	# um. yes we do.
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

	if ($add ne "") {
	    $path = $add . $path;
	}

        # Only use one case-variation of each album/artist
        if (exists($lcalbums{lc($$id3{'ALBUM'})})) {
            # if another album with different case exists
            # use that case (store it in $$id3{'ALBUM'}
            $$id3{'ALBUM'} = $lcalbums{lc($$id3{'ALBUM'})};
        }
        else {
            # else create a new entry in the hash
            $lcalbums{lc($$id3{'ALBUM'})} = $$id3{'ALBUM'};
        }
        
        if (exists($lcartists{lc($$id3{'ARTIST'})})) {
            $$id3{'ARTIST'} = $lcartists{lc($$id3{'ARTIST'})};
        }
        else {
            $lcartists{lc($$id3{'ARTIST'})} = $$id3{'ARTIST'};
        }

        $entries{$path}= $id3;
        $artists{$id3->{'ARTIST'}}++ if($id3->{'ARTIST'});
        $genres{$id3->{'GENRE'}}++ if($id3->{'GENRE'});
        $years{$id3->{'YEAR'}}++ if($id3->{'YEAR'});

        # fallback names
        $$id3{'ARTIST'} = "<no artist tag>" if ($$id3{'ARTIST'} eq "");
	# Fall back on the directory name (not full path dirname),
	# if no album tag
	if ($dirisalbum) {
	  if($dir2albumname{$dir} eq "") {
	  	$dir2albumname{$dir} = $$id3{'ALBUM'};
	  }
	  elsif($dir2albumname{$dir} ne $$id3{'ALBUM'}) {
	        $dir2albumname{$dir} = (split m[/], $dir)[-1];
	  }
	}
	# if no directory
	if ($dirisalbumname) {
	  $$id3{'ALBUM'} = (split m[/], $dir)[-1] if ($$id3{'ALBUM'} eq "");
	}
	$$id3{'ALBUM'} = "<no album tag>" if ($$id3{'ALBUM'} eq "");
	# fall back on basename of the file if no title tag.
	my $base;
	($base = $f) =~ s/\.\w+$//;
        $$id3{'TITLE'} =  $base if ($$id3{'TITLE'} eq "");

        # Append dirname, to handle multi-artist albums
        $$id3{'DIR'} = $dir;
        my $albumid;
	if ($dirisalbum) {
	  $albumid=$$id3{'DIR'};
	}
	else {
	  $albumid= $id3->{'ALBUM'}."___".$$id3{'DIR'};
	}
	#printf "album id: %s\n", $albumid;

#        if($id3->{'ALBUM'}."___".$id3->{'DIR'} ne "<no album tag>___<no artist tag>") {
            my $num = ++$albums{$albumid};
            if($num > $maxsongperalbum) {
                $maxsongperalbum = $num;
                $longestalbum = $albumid;
            }
            $album2songs{$albumid}{$$id3{TITLE}} = $id3;
	    if($dirisalbum) {
              $artist2albums{$$id3{ARTIST}}{$$id3{DIR}} = $id3;
	    }
	    else {
	      $artist2albums{$$id3{ARTIST}}{$$id3{ALBUM}} = $id3;
	    }
#        }
    }

    if($dirisalbum and $dir2albumname{$dir} eq "") {
       $dir2albumname{$dir} = (split m[/], $dir)[-1];
       printf "%s\n", $dir2albumname{$dir};
    }

    # extractdirs filters out only subdirectories from all given entries
    my @d = extractdirs($dir, @a);

    for $d (sort @d) {
        $dir =~ s|/$||;
        dodir("$dir/$d");
    }
}


dodir($dir);
print "\n";

print "File name table\n" if ($verbose);
for(sort keys %entries) {
    printf("  %s\n", $_) if ($verbose);
    my $l = length($_);
    if($l > $maxfilelen) {
       $maxfilelen = $l;
       $longestfilename = $_;
    }		  
}
$maxfilelen++; # include zero termination byte
while($maxfilelen&3) {
    $maxfilelen++;
}    

my $maxsonglen = 0;
my $sc;
print "\nSong title table\n" if ($verbose);

for(sort {uc($entries{$a}->{'TITLE'}) cmp uc($entries{$b}->{'TITLE'})} keys %entries) {
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
for(sort {uc($a) cmp uc($b)} keys %artists) {
    printf("  %s: %d\n", $_, $i) if ($verbose);
    $artistcount{$_}=$i++;
    my $l = length($_);
    if($l > $maxartistlen) {
        $maxartistlen = $l;
        $longestartist = $_;
    }

    $l = scalar keys %{$artist2albums{$_}};
    if ($l > $maxalbumsperartist) {
        $maxalbumsperartist = $l;
	$longestartistalbum = $_;
    }
}
$maxartistlen++; # include zero termination byte
while($maxartistlen&3) {
    $maxartistlen++;
}

print "\nGenre table\n" if ($verbose);
for(sort keys %genres) {
  my $l = length($_);
  if($l > $maxgenrelen) {
    $maxgenrelen = $l;
    $longestgenrename = $_;
  }
}

$maxgenrelen++; #include zero termination byte
while($maxgenrelen&3) {
    $maxgenrelen++;
}
    

if ($verbose) {
    print "\nYear table\n";
    for(sort keys %years) {
        printf("  %s\n", $_);
    }
}

print "\nAlbum table\n" if ($verbose);
my $maxalbumlen = 0;
my %albumcount;
$i=0;
my @albumssort;
if($dirisalbum) {
 @albumssort = sort {uc($dir2albumname{$a}) cmp uc($dir2albumname{$b})} keys %albums;
}
else {
 @albumssort = sort {uc($a) cmp uc($b)} keys %albums;
}
for(@albumssort) {
    my @moo=split(/___/, $_);
    printf("  %s\n", $moo[0]) if ($verbose);
    $albumcount{$_} = $i++;
    my $l;
    if($dirisalbum) {
     $l = length($dir2albumname{$_});
    }
    else {
     $l = length($moo[0]);
    }
    if($l > $maxalbumlen) {
        $maxalbumlen = $l;
	if($dirisalbum) {
	  $longestalbumname = $dir2albumname{$_};
	}
	else {
	  $longestalbumname = $moo[0];
	}
    }
}
$maxalbumlen++; # include zero termination byte
while($maxalbumlen&3) {
    $maxalbumlen++;
}



sub dumpshort {
    my ($num)=@_;

    #    print "int: $num\n";
    
    print DB pack "n", $num;
}

sub dumpint {
    my ($num)=@_;

#    print "int: $num\n";

    print DB pack "N", $num;
}

if (!scalar keys %entries) {
    print "No songs found. Did you specify the right --path ?\n";
    print "Use the --help parameter to see all options.\n";
    exit;
}

if ($db) {
    my $songentrysize = $maxsonglen + 12 + $maxgenrelen+ 12;
    my $albumentrysize = $maxalbumlen + 4 + $maxsongperalbum*4;
    my $artistentrysize = $maxartistlen + $maxalbumsperartist*4;
    my $fileentrysize = $maxfilelen + 12;

    printf "Number of artists        : %d\n", scalar keys %artists;
    printf "Number of albums         : %d\n", scalar keys %albums;
    printf "Number of songs / files  : %d\n", scalar keys %entries;
    print "Max artist length    : $maxartistlen ($longestartist)\n";
    print "Max album length     : $maxalbumlen ($longestalbumname)\n";
    print "Max song length      : $maxsonglen ($longestsong)\n";
    print "Max songs per album  : $maxsongperalbum ($longestalbum)\n";
    print "Max albums per artist: $maxalbumsperartist ($longestartistalbum)\n";
    print "Max genre length     : $maxgenrelen ($longestgenrename)\n";
    print "Max file length      : $maxfilelen ($longestfilename)\n";
    print "Database version: $dbver\n" if ($verbose);
    print "Song Entry Size : $songentrysize ($maxsonglen + 12 + $maxgenrelen + 4)\n" if ($verbose);
    print "Album Entry Size: $albumentrysize ($maxalbumlen + 4 + $maxsongperalbum * 4)\n" if ($verbose);
    print "Artist Entry Size: $artistentrysize ($maxartistlen + $maxalbumsperartist * 4)\n" if ($verbose);
    print "File Entry Size: $fileentrysize ($maxfilelen + 12)\n" if ($verbose);
    

    open(DB, ">$db") || die "couldn't make $db";
    binmode(DB);
    printf DB "RDB%c", $dbver;
    
    $pathindex = 68; # paths always start at index 68

    $artistindex = $pathindex;

    # set total size of song title table
    $sc = scalar(keys %entries) * $songentrysize;
    my $ac = scalar(keys %albums) * $albumentrysize;
    my $arc = scalar(keys %artists) * $artistentrysize;
    $albumindex = $artistindex + $arc; # arc is size of all artists
    $songindex = $albumindex + $ac; # ac is size of all albums
    my $fileindex = $songindex + $sc; # sc is size of all songs
    
    dumpint($artistindex); # file position index of artist table
    dumpint($albumindex); # file position index of album table
    dumpint($songindex); # file position index of song table
    dumpint($fileindex); # file position index of file table
    dumpint(scalar(keys %artists)); # number of artists
    dumpint(scalar(keys %albums)); # number of albums
    dumpint(scalar(keys %entries)); # number of songs
    dumpint(scalar(keys %entries)); # number of files
    dumpint($maxartistlen); # length of artist name field
    dumpint($maxalbumlen); # length of album name field
    dumpint($maxsonglen); # length of song name field		
    dumpint($maxgenrelen); #length of genre field
    dumpint($maxfilelen); # length of file field
    dumpint($maxsongperalbum); # number of entries in songs-per-album array
    dumpint($maxalbumsperartist); # number of entries in albums-per-artist array
    dumpint(-1); # rundb dirty

    #### TABLE of artists ###
    # name of artist1
    # pointers to albums of artist1

    for (sort {uc($a) cmp uc($b)} keys %artists) {
        my $artist = $_;
        my $str =  $_."\x00" x ($maxartistlen - length($_));
        print DB $str;

        for (sort keys %{$artist2albums{$artist}}) {
            my $id3 = $artist2albums{$artist}{$_};
            my $a;
	    if($dirisalbum) {
	      $a = $albumcount{"$$id3{'DIR'}"} * $albumentrysize;
	    }
	    else {
	      $a = $albumcount{"$$id3{'ALBUM'}___$$id3{'DIR'}"} * $albumentrysize;
	    }
            dumpint($a + $albumindex);
        }

        for (scalar keys %{$artist2albums{$artist}} .. $maxalbumsperartist-1) {
            print DB "\x00\x00\x00\x00";
        }

    }

   ### Build song offset info.
   my $offset = $songindex;
   for(sort {uc($entries{$a}->{'TITLE'}) cmp uc($entries{$b}->{'TITLE'})} keys %entries) {
     my $id3 = $entries{$_};
     $$id3{'songoffset'} = $offset;
     $offset += $songentrysize;
   }
		    

    #### TABLE of albums ###
    # name of album1
    # pointers to artists of album1
    # pointers to songs on album1

    for(@albumssort) {
        my $albumid = $_;
	my @moo=split(/___/, $_);
        my $t;
        my $str;
	if($dirisalbum) {
  	  $t = $dir2albumname{$albumid};
	}
	else {
	  $t = $moo[0];
	}
	$str =  $t."\x00" x ($maxalbumlen - length($t));
	print DB $str;

        my @songlist = keys %{$album2songs{$albumid}};
        my $id3 = $album2songs{$albumid}{$songlist[0]};

	#printf "(d) albumid: %s artist: %s\n",$albumid, $id3->{'ARTIST'};

        my $aoffset = $artistcount{$id3->{'ARTIST'}} * $artistentrysize;
	dumpint($aoffset + $artistindex); # pointer to artist of this album
		
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

    #### Build filename offset info
    my $l=$fileindex;
    my %filenamepos;
    for $f (sort keys %entries) {
        $filenamepos{$f}= $l;
	$l += $fileentrysize;
    }
		
    #### TABLE of songs ###
    # title of song1
    # pointer to artist of song1
    # pointer to album of song1
    # pointer to filename of song1

    for(sort {uc($entries{$a}->{'TITLE'}) cmp uc($entries{$b}->{'TITLE'})} keys %entries) {
        my $f = $_;
        my $id3 = $entries{$f};
        my $t = $id3->{'TITLE'};
	my $g = $id3->{'GENRE'};
        my $str = $t."\x00" x ($maxsonglen- length($t));

        print DB $str; # title
	$str = $g."\x00" x ($maxgenrelen - length($g));

        my $a = $artistcount{$id3->{'ARTIST'}} * $artistentrysize;
        dumpint($a + $artistindex); # pointer to artist of this song

	if($dirisalbum) {
	  $a = $albumcount{"$$id3{DIR}"} * $albumentrysize;
	}
	else {
          $a = $albumcount{"$$id3{ALBUM}___$$id3{DIR}"} * $albumentrysize;
	}
        dumpint($a + $albumindex); # pointer to album of this song

        # pointer to filename of this song
        dumpint($filenamepos{$f});
	print DB $str; #genre
	dumpshort(-1);
	dumpshort($id3->{'YEAR'});
    dumpint(-1);
    dumpshort($id3->{'TRACKNUM'});
    dumpshort(-1);
    }

    #### TABLE of file names ###
    # path1

    for $f (sort keys %entries) {
        my $str = $f."\x00" x ($maxfilelen- length($f));
	my $id3 = $entries{$f}; 
        print DB $str;
	dumpint(0);
        dumpint($id3->{'songoffset'});
        dumpint(-1);
    }

    close(DB);
}

###
### Here follows module MP3::Info Copyright (c) 1998-2004 Chris Nandor
### Modified by Björn Stenberg to remove use of external libraries
###

our(
	@ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS, $VERSION, $REVISION,
	@mp3_genres, %mp3_genres, @winamp_genres, %winamp_genres, $try_harder,
	@t_bitrate, @t_sampling_freq, @frequency_tbl, %v1_tag_fields,
	@v1_tag_names, %v2_tag_names, %v2_to_v1_names, $AUTOLOAD,
	@mp3_info_fields
);

@ISA = 'Exporter';
@EXPORT = qw(
	set_mp3tag get_mp3tag get_mp3info remove_mp3tag
	use_winamp_genres
);
@EXPORT_OK = qw(@mp3_genres %mp3_genres use_mp3_utf8);
%EXPORT_TAGS = (
	genres	=> [qw(@mp3_genres %mp3_genres)],
	utf8	=> [qw(use_mp3_utf8)],
	all	=> [@EXPORT, @EXPORT_OK]
);

# $Id$
($REVISION) = ' $Revision$ ' =~ /\$Revision:\s+([^\s]+)/;
$VERSION = '1.02';

=pod

=head1 NAME

MP3::Info - Manipulate / fetch info from MP3 audio files

=head1 SYNOPSIS

	#!perl -w
	use MP3::Info;
	my $file = 'Pearls_Before_Swine.mp3';
	set_mp3tag($file, 'Pearls Before Swine', q"77's",
		'Sticks and Stones', '1990',
		q"(c) 1990 77's LTD.", 'rock & roll');

	my $tag = get_mp3tag($file) or die "No TAG info";
	$tag->{GENRE} = 'rock';
	set_mp3tag($file, $tag);

	my $info = get_mp3info($file);
	printf "$file length is %d:%d\n", $info->{MM}, $info->{SS};

=cut

{
	my $c = -1;
	# set all lower-case and regular-cased versions of genres as keys
	# with index as value of each key
	%mp3_genres = map {($_, ++$c, lc, $c)} @mp3_genres;

	# do it again for winamp genres
	$c = -1;
	%winamp_genres = map {($_, ++$c, lc, $c)} @winamp_genres;
}

=pod

	my $mp3 = new MP3::Info $file;
	$mp3->title('Perls Before Swine');
	printf "$file length is %s, title is %s\n",
		$mp3->time, $mp3->title;


=head1 DESCRIPTION

=over 4

=item $mp3 = MP3::Info-E<gt>new(FILE)

OOP interface to the rest of the module.  The same keys
available via get_mp3info and get_mp3tag are available
via the returned object (using upper case or lower case;
but note that all-caps "VERSION" will return the module
version, not the MP3 version).

Passing a value to one of the methods will set the value
for that tag in the MP3 file, if applicable.

=cut

sub new {
	my($pack, $file) = @_;

	my $info = get_mp3info($file) or return undef;
	my $tags = get_mp3tag($file) || { map { ($_ => undef) } @v1_tag_names };
	my %self = (
		FILE		=> $file,
		TRY_HARDER	=> 0
	);

	@self{@mp3_info_fields, @v1_tag_names, 'file'} = (
		@{$info}{@mp3_info_fields},
		@{$tags}{@v1_tag_names},
		$file
	);

	return bless \%self, $pack;
}

sub can {
	my $self = shift;
	return $self->SUPER::can(@_) unless ref $self;
	my $name = uc shift;
	return sub { $self->$name(@_) } if exists $self->{$name};
	return undef;
}

sub AUTOLOAD {
	my($self) = @_;
	(my $name = uc $AUTOLOAD) =~ s/^.*://;

	if (exists $self->{$name}) {
		my $sub = exists $v1_tag_fields{$name}
			? sub {
				if (defined $_[1]) {
					$_[0]->{$name} = $_[1];
					set_mp3tag($_[0]->{FILE}, $_[0]);
				}
				return $_[0]->{$name};
			}
			: sub {
				return $_[0]->{$name}
			};

		*{$AUTOLOAD} = $sub;
		goto &$AUTOLOAD;

	} else {
		warn(sprintf "No method '$name' available in package %s.",
			__PACKAGE__);
	}
}

sub DESTROY {

}


=item use_mp3_utf8([STATUS])

Tells MP3::Info to (or not) return TAG info in UTF-8.
TRUE is 1, FALSE is 0.  Default is FALSE.

Will only be able to it on if Unicode::String is available.  ID3v2
tags will be converted to UTF-8 according to the encoding specified
in each tag; ID3v1 tags will be assumed Latin-1 and converted
to UTF-8.

Function returns status (TRUE/FALSE).  If no argument is supplied,
or an unaccepted argument is supplied, function merely returns status.

This function is not exported by default, but may be exported
with the C<:utf8> or C<:all> export tag.

=cut

my $unicode_module = eval { require Unicode::String };
my $UNICODE = 0;

sub use_mp3_utf8 {
	my($val) = @_;
	if ($val == 1) {
		$UNICODE = 1 if $unicode_module;
	} elsif ($val == 0) {
		$UNICODE = 0;
	}
	return $UNICODE;
}

=pod

=item use_winamp_genres()

Puts WinAmp genres into C<@mp3_genres> and C<%mp3_genres>
(adds 68 additional genres to the default list of 80).
This is a separate function because these are non-standard
genres, but they are included because they are widely used.

You can import the data structures with one of:

	use MP3::Info qw(:genres);
	use MP3::Info qw(:DEFAULT :genres);
	use MP3::Info qw(:all);

=cut

sub use_winamp_genres {
	%mp3_genres = %winamp_genres;
	@mp3_genres = @winamp_genres;
	return 1;
}

=pod

=item remove_mp3tag (FILE [, VERSION, BUFFER])

Can remove ID3v1 or ID3v2 tags.  VERSION should be C<1> for ID3v1,
C<2> for ID3v2, and C<ALL> for both.

For ID3v1, removes last 128 bytes from file if those last 128 bytes begin
with the text 'TAG'.  File will be 128 bytes shorter.

For ID3v2, removes ID3v2 tag.  Because an ID3v2 tag is at the
beginning of the file, we rewrite the file after removing the tag data.
The buffer for rewriting the file is 4MB.  BUFFER (in bytes) ca
change the buffer size.

Returns the number of bytes removed, or -1 if no tag removed,
or undef if there is an error.

=cut

sub remove_mp3tag {
	my($file, $version, $buf) = @_;
	my($fh, $return);

	$buf ||= 4096*1024;  # the bigger the faster
	$version ||= 1;

	if (not (defined $file && $file ne '')) {
		$@ = "No file specified";
		return undef;
	}

	if (not -s $file) {
		$@ = "File is empty";
		return undef;
	}

	if (ref $file) { # filehandle passed
		$fh = $file;
	} else {
		$fh = gensym;
		if (not open $fh, "+< $file\0") {
			$@ = "Can't open $file: $!";
			return undef;
		}
	}

	binmode $fh;

	if ($version eq 1 || $version eq 'ALL') {
		seek $fh, -128, 2;
		my $tell = tell $fh;
		if (<$fh> =~ /^TAG/) {
			truncate $fh, $tell or warn "Can't truncate '$file': $!";
			$return += 128;
		}
	}

	if ($version eq 2 || $version eq 'ALL') {
		my $h = _get_v2head($fh);
		if ($h) {
			local $\;
			seek $fh, 0, 2;
			my $eof = tell $fh;
			my $off = $h->{tag_size};

			while ($off < $eof) {
				seek $fh, $off, 0;
				read $fh, my($bytes), $buf;
				seek $fh, $off - $h->{tag_size}, 0;
				print $fh $bytes;
				$off += $buf;
			}

			truncate $fh, $eof - $h->{tag_size}
				or warn "Can't truncate '$file': $!";
			$return += $h->{tag_size};
		}
	}

	_close($file, $fh);

	return $return || -1;
}


=pod

=item set_mp3tag (FILE, TITLE, ARTIST, ALBUM, YEAR, COMMENT, GENRE [, TRACKNUM])

=item set_mp3tag (FILE, $HASHREF)

Adds/changes tag information in an MP3 audio file.  Will clobber
any existing information in file.

Fields are TITLE, ARTIST, ALBUM, YEAR, COMMENT, GENRE.  All fields have
a 30-byte limit, except for YEAR, which has a four-byte limit, and GENRE,
which is one byte in the file.  The GENRE passed in the function is a
case-insensitive text string representing a genre found in C<@mp3_genres>.

Will accept either a list of values, or a hashref of the type
returned by C<get_mp3tag>.

If TRACKNUM is present (for ID3v1.1), then the COMMENT field can only be
28 bytes.

ID3v2 support may come eventually.  Note that if you set a tag on a file
with ID3v2, the set tag will be for ID3v1[.1] only, and if you call
C<get_mp3_tag> on the file, it will show you the (unchanged) ID3v2 tags,
unless you specify ID3v1.

=cut

sub set_mp3tag {
	my($file, $title, $artist, $album, $year, $comment, $genre, $tracknum) = @_;
	my(%info, $oldfh, $ref, $fh);
	local %v1_tag_fields = %v1_tag_fields;

	# set each to '' if undef
	for ($title, $artist, $album, $year, $comment, $tracknum, $genre,
		(@info{@v1_tag_names}))
		{$_ = defined() ? $_ : ''}

	($ref) = (overload::StrVal($title) =~ /^(?:.*\=)?([^=]*)\((?:[^\(]*)\)$/)
		if ref $title;
	# populate data to hashref if hashref is not passed
	if (!$ref) {
		(@info{@v1_tag_names}) =
			($title, $artist, $album, $year, $comment, $tracknum, $genre);

	# put data from hashref into hashref if hashref is passed
	} elsif ($ref eq 'HASH') {
		%info = %$title;

	# return otherwise
	} else {
		warn(<<'EOT');
Usage: set_mp3tag (FILE, TITLE, ARTIST, ALBUM, YEAR, COMMENT, GENRE [, TRACKNUM])
       set_mp3tag (FILE, $HASHREF)
EOT
		return undef;
	}

	if (not (defined $file && $file ne '')) {
		$@ = "No file specified";
		return undef;
	}

	if (not -s $file) {
		$@ = "File is empty";
		return undef;
	}

	# comment field length 28 if ID3v1.1
	$v1_tag_fields{COMMENT} = 28 if $info{TRACKNUM};


	# only if -w is on
	if ($^W) {
		# warn if fields too long
		foreach my $field (keys %v1_tag_fields) {
			$info{$field} = '' unless defined $info{$field};
			if (length($info{$field}) > $v1_tag_fields{$field}) {
				warn "Data too long for field $field: truncated to " .
					 "$v1_tag_fields{$field}";
			}
		}

		if ($info{GENRE}) {
			warn "Genre `$info{GENRE}' does not exist\n"
				unless exists $mp3_genres{$info{GENRE}};
		}
	}

	if ($info{TRACKNUM}) {
		$info{TRACKNUM} =~ s/^(\d+)\/(\d+)$/$1/;
		unless ($info{TRACKNUM} =~ /^\d+$/ &&
			$info{TRACKNUM} > 0 && $info{TRACKNUM} < 256) {
			warn "Tracknum `$info{TRACKNUM}' must be an integer " .
				"from 1 and 255\n" if $^W;
			$info{TRACKNUM} = '';
		}
	}

	if (ref $file) { # filehandle passed
		$fh = $file;
	} else {
		$fh = gensym;
		if (not open $fh, "+< $file\0") {
			$@ = "Can't open $file: $!";
			return undef;
		}
	}

	binmode $fh;
	$oldfh = select $fh;
	seek $fh, -128, 2;
	# go to end of file if no tag, beginning of file if tag
	seek $fh, (<$fh> =~ /^TAG/ ? -128 : 0), 2;

	# get genre value
	$info{GENRE} = $info{GENRE} && exists $mp3_genres{$info{GENRE}} ?
		$mp3_genres{$info{GENRE}} : 255;  # some default genre

	local $\;
	# print TAG to file
	if ($info{TRACKNUM}) {
		print pack "a3a30a30a30a4a28xCC", 'TAG', @info{@v1_tag_names};
	} else {
		print pack "a3a30a30a30a4a30C", 'TAG', @info{@v1_tag_names[0..4, 6]};
	}

	select $oldfh;

	_close($file, $fh);

	return 1;
}

=pod

=item get_mp3tag (FILE [, VERSION, RAW_V2])

Returns hash reference containing tag information in MP3 file.  The keys
returned are the same as those supplied for C<set_mp3tag>, except in the
case of RAW_V2 being set.

If VERSION is C<1>, the information is taken from the ID3v1 tag (if present).
If VERSION is C<2>, the information is taken from the ID3v2 tag (if present).
If VERSION is not supplied, or is false, the ID3v1 tag is read if present, and
then, if present, the ID3v2 tag information will override any existing ID3v1
tag info.

If RAW_V2 is C<1>, the raw ID3v2 tag data is returned, without any manipulation
of text encoding.  The key name is the same as the frame ID (ID to name mappings
are in the global %v2_tag_names).

If RAW_V2 is C<2>, the ID3v2 tag data is returned, manipulating for Unicode if
necessary, etc.  It also takes multiple values for a given key (such as comments)
and puts them in an arrayref.

If the ID3v2 version is older than ID3v2.2.0 or newer than ID3v2.4.0, it will
not be read.

Strings returned will be in Latin-1, unless UTF-8 is specified (L<use_mp3_utf8>),
(unless RAW_V2 is C<1>).

Also returns a TAGVERSION key, containing the ID3 version used for the returned
data (if TAGVERSION argument is C<0>, may contain two versions).

=cut

sub get_mp3tag {
    my($file, $ver, $raw_v2) = @_;
    my($tag, $v1, $v2, $v2h, %info, @array, $fh);
    $raw_v2 ||= 0;
    $ver = !$ver ? 0 : ($ver == 2 || $ver == 1) ? $ver : 0;

    if (not (defined $file && $file ne '')) {
        $@ = "No file specified";
        return undef;
    }

    if (not -s $file) {
        $@ = "File is empty";
        return undef;
    }

    if (ref $file) { # filehandle passed
        $fh = $file;
    } else {
        $fh = gensym;
        if (not open $fh, "< $file\0") {
            $@ = "Can't open $file: $!";
            return undef;
        }
    }

    binmode $fh;

    if ($ver < 2) {
        seek $fh, -128, 2;
        while(defined(my $line = <$fh>)) { $tag .= $line }

        if ($tag =~ /^TAG/) {
            $v1 = 1;
            if (substr($tag, -3, 2) =~ /\000[^\000]/) {
                (undef, @info{@v1_tag_names}) =
                    (unpack('a3a30a30a30a4a28', $tag),
                     ord(substr($tag, -2, 1)),
                     $mp3_genres[ord(substr $tag, -1)]);
                $info{TAGVERSION} = 'ID3v1.1';
            } else {
                (undef, @info{@v1_tag_names[0..4, 6]}) =
                    (unpack('a3a30a30a30a4a30', $tag),
                     $mp3_genres[ord(substr $tag, -1)]);
                $info{TAGVERSION} = 'ID3v1';
            }
            if ($UNICODE) {
                for my $key (keys %info) {
                    next unless $info{$key};
                    my $u = Unicode::String::latin1($info{$key});
                    $info{$key} = $u->utf8;
                }
            }
        } elsif ($ver == 1) {
            _close($file, $fh);
            $@ = "No ID3v1 tag found";
            return undef;
        }
    }

    ($v2, $v2h) = _get_v2tag($fh);

    unless ($v1 || $v2) {
        _close($file, $fh);
        $@ = "No ID3 tag found";
        return undef;
    }

    if (($ver == 0 || $ver == 2) && $v2) {
        if ($raw_v2 == 1 && $ver == 2) {
            %info = %$v2;
            $info{TAGVERSION} = $v2h->{version};
        } else {
            my $hash = $raw_v2 == 2 ? { map { ($_, $_) } keys %v2_tag_names } : \%v2_to_v1_names;
            for my $id (keys %$hash) {
                if (exists $v2->{$id}) {
                    if ($id =~ /^TCON?$/ && $v2->{$id} =~ /^.?\((\d+)\)/) {
                        $info{$hash->{$id}} = $mp3_genres[$1];
                    } else {
                        my $data1 = $v2->{$id};

                        # this is tricky ... if this is an arrayref, we want
                        # to only return one, so we pick the first one.  but
                        # if it is a comment, we pick the first one where the
                        # first charcter after the language is NULL and not an
                        # additional sub-comment, because that is most likely
                        # to be the user-supplied comment

                        if (ref $data1 && !$raw_v2) {
                            if ($id =~ /^COMM?$/) {
                                my($newdata) = grep /^(....\000)/, @{$data1};
                                $data1 = $newdata || $data1->[0];
                            } else {
                                $data1 = $data1->[0];
                            }
                        }

                        $data1 = [ $data1 ] if ! ref $data1;

                        for my $data (@$data1) {
                            $data =~ s/^(.)//; # strip first char (text encoding)
                            my $encoding = $1;
                            my $desc;
                            if ($id =~ /^COM[M ]?$/) {
                                $data =~ s/^(?:...)//;		# strip language
                                $data =~ s/^(.*?)\000+//;	# strip up to first NULL(s),
                                # for sub-comment
                                $desc = $1;
                            }
                            
                            if ($UNICODE) {
                                if ($encoding eq "\001" || $encoding eq "\002") {  # UTF-16, UTF-16BE
                                    my $u = Unicode::String::utf16($data);
                                    $data = $u->utf8;
                                    $data =~ s/^\xEF\xBB\xBF//;	# strip BOM
                                } elsif ($encoding eq "\000") {
                                    my $u = Unicode::String::latin1($data);
                                    $data = $u->utf8;
                                }
                            }
                            
                            if ($raw_v2 == 2 && $desc) {
                                $data = { $desc => $data };
                            }
                            
                            if ($raw_v2 == 2 && exists $info{$hash->{$id}}) {
                                if (ref $info{$hash->{$id}} eq 'ARRAY') {
                                    push @{$info{$hash->{$id}}}, $data;
                                } else {
                                    $info{$hash->{$id}} = [ $info{$hash->{$id}}, $data ];
                                }
                            } else {
                                $info{$hash->{$id}} = $data;
                            }
                        }
                    }
                }
            }
            if ($ver == 0 && $info{TAGVERSION}) {
                $info{TAGVERSION} .= ' / ' . $v2h->{version};
            } else {
                $info{TAGVERSION} = $v2h->{version};
            }
        }
    }

    unless ($raw_v2 && $ver == 2) {
        foreach my $key (keys %info) {
            if (defined $info{$key}) {
                $info{$key} =~ s/\000+.*//g;
                $info{$key} =~ s/\s+$//;
            }
        }
        
        for (@v1_tag_names) {
            $info{$_} = '' unless defined $info{$_};
        }
    }

    if (keys %info && exists $info{GENRE} && ! defined $info{GENRE}) {
        $info{GENRE} = '';
    }

    _close($file, $fh);

    return keys %info ? {%info} : undef;
}

sub _get_v2tag {
    my($fh) = @_;
    my($off, $myseek, $myseek_22, $myseek_23, $v2, $h, $hlen, $num);
    $h = {};

    $v2 = _get_v2head($fh) or return;
    if ($v2->{major_version} < 2) {
        warn "This is $v2->{version}; " .
            "ID3v2 versions older than ID3v2.2.0 not supported\n"
            if $^W;
        return;
    }

    if ($v2->{major_version} == 2) {
        $hlen = 6;
        $num = 3;
    } else {
        $hlen = 10;
        $num = 4;
    }

    $myseek = sub {
        seek $fh, $off, 0;
        read $fh, my($bytes), $hlen;
        return unless $bytes =~ /^([A-Z0-9]{$num})/
            || ($num == 4 && $bytes =~ /^(COM )/);  # stupid iTunes
        my($id, $size) = ($1, $hlen);
        my @bytes = reverse unpack "C$num", substr($bytes, $num, $num);
        for my $i (0 .. ($num - 1)) {
            $size += $bytes[$i] * 256 ** $i;
        }
        return($id, $size);
    };

    $off = $v2->{ext_header_size} + 10;

    while ($off < $v2->{tag_size}) {
        my($id, $size) = &$myseek or last;
        seek $fh, $off + $hlen, 0;
        read $fh, my($bytes), $size - $hlen;
        if (exists $h->{$id}) {
            if (ref $h->{$id} eq 'ARRAY') {
                push @{$h->{$id}}, $bytes;
            } else {
                $h->{$id} = [$h->{$id}, $bytes];
            }
        } else {
            $h->{$id} = $bytes;
        }
        $off += $size;
    }

    return($h, $v2);
}


=pod

=item get_mp3info (FILE)

Returns hash reference containing file information for MP3 file.
This data cannot be changed.  Returned data:

	VERSION		MPEG audio version (1, 2, 2.5)
	LAYER		MPEG layer description (1, 2, 3)
	STEREO		boolean for audio is in stereo

	VBR		boolean for variable bitrate
	BITRATE		bitrate in kbps (average for VBR files)
	FREQUENCY	frequency in kHz
	SIZE		bytes in audio stream

	SECS		total seconds
	MM		minutes
	SS		leftover seconds
	MS		leftover milliseconds
	TIME		time in MM:SS

	COPYRIGHT	boolean for audio is copyrighted
	PADDING		boolean for MP3 frames are padded
	MODE		channel mode (0 = stereo, 1 = joint stereo,
			2 = dual channel, 3 = single channel)
	FRAMES		approximate number of frames
	FRAME_LENGTH	approximate length of a frame
	VBR_SCALE	VBR scale from VBR header

On error, returns nothing and sets C<$@>.

=cut

sub get_mp3info {
    my($file) = @_;
    my($off, $myseek, $byte, $eof, $h, $tot, $fh);

    if (not (defined $file && $file ne '')) {
        $@ = "No file specified";
        return undef;
    }

    if (not -s $file) {
        $@ = "File is empty";
        return undef;
    }

    if (ref $file) { # filehandle passed
        $fh = $file;
    } else {
        $fh = gensym;
        if (not open $fh, "< $file\0") {
            $@ = "Can't open $file: $!";
            return undef;
        }
    }

    $off = 0;
    $tot = 4096;

    $myseek = sub {
        seek $fh, $off, 0;
        read $fh, $byte, 4;
    };

    binmode $fh;
    &$myseek;

    if ($off == 0) {
        if (my $id3v2 = _get_v2head($fh)) {
            $tot += $off += $id3v2->{tag_size};
            &$myseek;
        }
    }

    $h = _get_head($byte);
    until (_is_mp3($h)) {
        $off++;
        &$myseek;
        $h = _get_head($byte);
        if ($off > $tot && !$try_harder) {
            _close($file, $fh);
            $@ = "Couldn't find MP3 header (perhaps set " .
                '$MP3::Info::try_harder and retry)';
            return undef;
        }
    }

    my $vbr = _get_vbr($fh, $h, \$off);

    seek $fh, 0, 2;
    $eof = tell $fh;
    seek $fh, -128, 2;
    $off += 128 if <$fh> =~ /^TAG/ ? 1 : 0;

    _close($file, $fh);

    $h->{size} = $eof - $off;

    return _get_info($h, $vbr);
}

sub _get_info {
    my($h, $vbr) = @_;
    my $i;

    $i->{VERSION}	= $h->{IDR} == 2 ? 2 : $h->{IDR} == 3 ? 1 :
        $h->{IDR} == 0 ? 2.5 : 0;
    $i->{LAYER}	= 4 - $h->{layer};
    $i->{VBR}	= defined $vbr ? 1 : 0;

    $i->{COPYRIGHT}	= $h->{copyright} ? 1 : 0;
    $i->{PADDING}	= $h->{padding_bit} ? 1 : 0;
    $i->{STEREO}	= $h->{mode} == 3 ? 0 : 1;
    $i->{MODE}	= $h->{mode};

    $i->{SIZE}	= $vbr && $vbr->{bytes} ? $vbr->{bytes} : $h->{size};

    my $mfs		= $h->{fs} / ($h->{ID} ? 144000 : 72000);
    $i->{FRAMES}	= int($vbr && $vbr->{frames}
                              ? $vbr->{frames}
                              : $i->{SIZE} / $h->{bitrate} / $mfs
                              );

    if ($vbr) {
        $i->{VBR_SCALE}	= $vbr->{scale} if $vbr->{scale};
        $h->{bitrate}	= $i->{SIZE} / $i->{FRAMES} * $mfs;
        if (not $h->{bitrate}) {
            $@ = "Couldn't determine VBR bitrate";
            return undef;
        }
    }

    $h->{'length'}	= ($i->{SIZE} * 8) / $h->{bitrate} / 10;
    $i->{SECS}	= $h->{'length'} / 100;
    $i->{MM}	= int $i->{SECS} / 60;
    $i->{SS}	= int $i->{SECS} % 60;
    $i->{MS}	= (($i->{SECS} - ($i->{MM} * 60) - $i->{SS}) * 1000);
#	$i->{LF}	= ($i->{MS} / 1000) * ($i->{FRAMES} / $i->{SECS});
#	int($i->{MS} / 100 * 75);  # is this right?
    $i->{TIME}	= sprintf "%.2d:%.2d", @{$i}{'MM', 'SS'};

    $i->{BITRATE}		= int $h->{bitrate};
	# should we just return if ! FRAMES?
    $i->{FRAME_LENGTH}	= int($h->{size} / $i->{FRAMES}) if $i->{FRAMES};
    $i->{FREQUENCY}		= $frequency_tbl[3 * $h->{IDR} + $h->{sampling_freq}];

    return $i;
}

sub _get_head {
    my($byte) = @_;
    my($bytes, $h);

    $bytes = _unpack_head($byte);
    @$h{qw(IDR ID layer protection_bit
           bitrate_index sampling_freq padding_bit private_bit
           mode mode_extension copyright original
           emphasis version_index bytes)} = (
		($bytes>>19)&3, ($bytes>>19)&1, ($bytes>>17)&3, ($bytes>>16)&1,
		($bytes>>12)&15, ($bytes>>10)&3, ($bytes>>9)&1, ($bytes>>8)&1,
		($bytes>>6)&3, ($bytes>>4)&3, ($bytes>>3)&1, ($bytes>>2)&1,
		$bytes&3, ($bytes>>19)&3, $bytes
	);

    $h->{bitrate} = $t_bitrate[$h->{ID}][3 - $h->{layer}][$h->{bitrate_index}];
    $h->{fs} = $t_sampling_freq[$h->{IDR}][$h->{sampling_freq}];

    return $h;
}

sub _is_mp3 {
    my $h = $_[0] or return undef;
    return ! (	# all below must be false
                $h->{bitrate_index} == 0
                ||
                $h->{version_index} == 1
                ||
		($h->{bytes} & 0xFFE00000) != 0xFFE00000
                ||
		!$h->{fs}
                ||
		!$h->{bitrate}
                ||
                $h->{bitrate_index} == 15
                ||
		!$h->{layer}
                ||
                $h->{sampling_freq} == 3
                ||
                $h->{emphasis} == 2
                ||
		!$h->{bitrate_index}
                ||
		($h->{bytes} & 0xFFFF0000) == 0xFFFE0000
                ||
		($h->{ID} == 1 && $h->{layer} == 3 && $h->{protection_bit} == 1)
                ||
		($h->{mode_extension} != 0 && $h->{mode} != 1)
                );
}

sub _get_vbr {
    my($fh, $h, $roff) = @_;
    my($off, $bytes, @bytes, $myseek, %vbr);

    $off = $$roff;
    @_ = ();	# closure confused if we don't do this

    $myseek = sub {
        my $n = $_[0] || 4;
        seek $fh, $off, 0;
        read $fh, $bytes, $n;
        $off += $n;
    };

    $off += 4;

    if ($h->{ID}) {	# MPEG1
        $off += $h->{mode} == 3 ? 17 : 32;
    } else {	# MPEG2
        $off += $h->{mode} == 3 ? 9 : 17;
    }

    &$myseek;
    return unless $bytes eq 'Xing';

    &$myseek;
    $vbr{flags} = _unpack_head($bytes);

    if ($vbr{flags} & 1) {
        &$myseek;
        $vbr{frames} = _unpack_head($bytes);
    }

    if ($vbr{flags} & 2) {
        &$myseek;
        $vbr{bytes} = _unpack_head($bytes);
    }

    if ($vbr{flags} & 4) {
        $myseek->(100);
# Not used right now ...
#		$vbr{toc} = _unpack_head($bytes);
    }

    if ($vbr{flags} & 8) { # (quality ind., 0=best 100=worst)
        &$myseek;
        $vbr{scale} = _unpack_head($bytes);
    } else {
        $vbr{scale} = -1;
    }

    $$roff = $off;
    return \%vbr;
}

sub _get_v2head {
    my $fh = $_[0] or return;
    my($h, $bytes, @bytes);

    # check first three bytes for 'ID3'
    seek $fh, 0, 0;
    read $fh, $bytes, 3;
    return unless $bytes eq 'ID3';

    # get version
    read $fh, $bytes, 2;
    $h->{version} = sprintf "ID3v2.%d.%d",
    @$h{qw[major_version minor_version]} =
        unpack 'c2', $bytes;

    # get flags
    read $fh, $bytes, 1;
    if ($h->{major_version} == 2) {
        @$h{qw[unsync compression]} =
            (unpack 'b8', $bytes)[7, 6];
        $h->{ext_header} = 0;
        $h->{experimental} = 0;
    } else {
        @$h{qw[unsync ext_header experimental]} =
            (unpack 'b8', $bytes)[7, 6, 5];
    }

    # get ID3v2 tag length from bytes 7-10
    $h->{tag_size} = 10;	# include ID3v2 header size
    read $fh, $bytes, 4;
    @bytes = reverse unpack 'C4', $bytes;
    foreach my $i (0 .. 3) {
        # whoaaaaaa nellllllyyyyyy!
        $h->{tag_size} += $bytes[$i] * 128 ** $i;
    }

    # get extended header size
    $h->{ext_header_size} = 0;
    if ($h->{ext_header}) {
        $h->{ext_header_size} += 10;
        read $fh, $bytes, 4;
        @bytes = reverse unpack 'C4', $bytes;
        for my $i (0..3) {
            $h->{ext_header_size} += $bytes[$i] * 256 ** $i;
        }
    }

    return $h;
}

sub _unpack_head {
    unpack('l', pack('L', unpack('N', $_[0])));
}

sub _close {
    my($file, $fh) = @_;
    unless (ref $file) { # filehandle not passed
        close $fh or warn "Problem closing '$file': $!";
    }
}

BEGIN {
    @mp3_genres = (
		'Blues',
		'Classic Rock',
		'Country',
		'Dance',
		'Disco',
		'Funk',
		'Grunge',
		'Hip-Hop',
		'Jazz',
		'Metal',
		'New Age',
		'Oldies',
		'Other',
		'Pop',
		'R&B',
		'Rap',
		'Reggae',
		'Rock',
		'Techno',
		'Industrial',
		'Alternative',
		'Ska',
		'Death Metal',
		'Pranks',
		'Soundtrack',
		'Euro-Techno',
		'Ambient',
		'Trip-Hop',
		'Vocal',
		'Jazz+Funk',
		'Fusion',
		'Trance',
		'Classical',
		'Instrumental',
		'Acid',
		'House',
		'Game',
		'Sound Clip',
		'Gospel',
		'Noise',
		'AlternRock',
		'Bass',
		'Soul',
		'Punk',
		'Space',
		'Meditative',
		'Instrumental Pop',
		'Instrumental Rock',
		'Ethnic',
		'Gothic',
		'Darkwave',
		'Techno-Industrial',
		'Electronic',
		'Pop-Folk',
		'Eurodance',
		'Dream',
		'Southern Rock',
		'Comedy',
		'Cult',
		'Gangsta',
		'Top 40',
		'Christian Rap',
		'Pop/Funk',
		'Jungle',
		'Native American',
		'Cabaret',
		'New Wave',
		'Psychadelic',
		'Rave',
		'Showtunes',
		'Trailer',
		'Lo-Fi',
		'Tribal',
		'Acid Punk',
		'Acid Jazz',
		'Polka',
		'Retro',
		'Musical',
		'Rock & Roll',
		'Hard Rock',
	);

	@winamp_genres = (
		@mp3_genres,
		'Folk',
		'Folk-Rock',
		'National Folk',
		'Swing',
		'Fast Fusion',
		'Bebob',
		'Latin',
		'Revival',
		'Celtic',
		'Bluegrass',
		'Avantgarde',
		'Gothic Rock',
		'Progressive Rock',
		'Psychedelic Rock',
		'Symphonic Rock',
		'Slow Rock',
		'Big Band',
		'Chorus',
		'Easy Listening',
		'Acoustic',
		'Humour',
		'Speech',
		'Chanson',
		'Opera',
		'Chamber Music',
		'Sonata',
		'Symphony',
		'Booty Bass',
		'Primus',
		'Porn Groove',
		'Satire',
		'Slow Jam',
		'Club',
		'Tango',
		'Samba',
		'Folklore',
		'Ballad',
		'Power Ballad',
		'Rhythmic Soul',
		'Freestyle',
		'Duet',
		'Punk Rock',
		'Drum Solo',
		'Acapella',
		'Euro-House',
		'Dance Hall',
		'Goa',
		'Drum & Bass',
		'Club-House',
		'Hardcore',
		'Terror',
		'Indie',
		'BritPop',
		'Negerpunk',
		'Polsk Punk',
		'Beat',
		'Christian Gangsta Rap',
		'Heavy Metal',
		'Black Metal',
		'Crossover',
		'Contemporary Christian',
		'Christian Rock',
		'Merengue',
		'Salsa',
		'Thrash Metal',
		'Anime',
		'JPop',
		'Synthpop',
	);

    @t_bitrate = ([
		[0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256],
		[0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160],
		[0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160]
	],[
		[0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448],
		[0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384],
		[0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320]
	]);

    @t_sampling_freq = (
                        [11025, 12000,  8000],
                        [undef, undef, undef],	# reserved
                        [22050, 24000, 16000],
                        [44100, 48000, 32000]
                        );

    @frequency_tbl = map { $_ ? eval "${_}e-3" : 0 }
		map { @$_ } @t_sampling_freq;

    @mp3_info_fields = qw(
		VERSION
		LAYER
		STEREO
		VBR
		BITRATE
		FREQUENCY
		SIZE
		SECS
		MM
		SS
		MS
		TIME
		COPYRIGHT
		PADDING
		MODE
		FRAMES
		FRAME_LENGTH
		VBR_SCALE
	);

    %v1_tag_fields =
        (TITLE => 30, ARTIST => 30, ALBUM => 30, COMMENT => 30, YEAR => 4);

    @v1_tag_names = qw(TITLE ARTIST ALBUM YEAR COMMENT TRACKNUM GENRE);

    %v2_to_v1_names = (
		# v2.2 tags
		'TT2' => 'TITLE',
		'TP1' => 'ARTIST',
		'TAL' => 'ALBUM',
		'TYE' => 'YEAR',
		'COM' => 'COMMENT',
		'TRK' => 'TRACKNUM',
		'TCO' => 'GENRE', # not clean mapping, but ...
		# v2.3 tags
		'TIT2' => 'TITLE',
		'TPE1' => 'ARTIST',
		'TALB' => 'ALBUM',
		'TYER' => 'YEAR',
		'COMM' => 'COMMENT',
		'TRCK' => 'TRACKNUM',
		'TCON' => 'GENRE',
	);

    %v2_tag_names = (
		# v2.2 tags
		'BUF' => 'Recommended buffer size',
		'CNT' => 'Play counter',
		'COM' => 'Comments',
		'CRA' => 'Audio encryption',
		'CRM' => 'Encrypted meta frame',
		'ETC' => 'Event timing codes',
		'EQU' => 'Equalization',
		'GEO' => 'General encapsulated object',
		'IPL' => 'Involved people list',
		'LNK' => 'Linked information',
		'MCI' => 'Music CD Identifier',
		'MLL' => 'MPEG location lookup table',
		'PIC' => 'Attached picture',
		'POP' => 'Popularimeter',
		'REV' => 'Reverb',
		'RVA' => 'Relative volume adjustment',
		'SLT' => 'Synchronized lyric/text',
		'STC' => 'Synced tempo codes',
		'TAL' => 'Album/Movie/Show title',
		'TBP' => 'BPM (Beats Per Minute)',
		'TCM' => 'Composer',
		'TCO' => 'Content type',
		'TCR' => 'Copyright message',
		'TDA' => 'Date',
		'TDY' => 'Playlist delay',
		'TEN' => 'Encoded by',
		'TFT' => 'File type',
		'TIM' => 'Time',
		'TKE' => 'Initial key',
		'TLA' => 'Language(s)',
		'TLE' => 'Length',
		'TMT' => 'Media type',
		'TOA' => 'Original artist(s)/performer(s)',
		'TOF' => 'Original filename',
		'TOL' => 'Original Lyricist(s)/text writer(s)',
		'TOR' => 'Original release year',
		'TOT' => 'Original album/Movie/Show title',
		'TP1' => 'Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group',
		'TP2' => 'Band/Orchestra/Accompaniment',
		'TP3' => 'Conductor/Performer refinement',
		'TP4' => 'Interpreted, remixed, or otherwise modified by',
		'TPA' => 'Part of a set',
		'TPB' => 'Publisher',
		'TRC' => 'ISRC (International Standard Recording Code)',
		'TRD' => 'Recording dates',
		'TRK' => 'Track number/Position in set',
		'TSI' => 'Size',
		'TSS' => 'Software/hardware and settings used for encoding',
		'TT1' => 'Content group description',
		'TT2' => 'Title/Songname/Content description',
		'TT3' => 'Subtitle/Description refinement',
		'TXT' => 'Lyricist/text writer',
		'TXX' => 'User defined text information frame',
		'TYE' => 'Year',
		'UFI' => 'Unique file identifier',
		'ULT' => 'Unsychronized lyric/text transcription',
		'WAF' => 'Official audio file webpage',
		'WAR' => 'Official artist/performer webpage',
		'WAS' => 'Official audio source webpage',
		'WCM' => 'Commercial information',
		'WCP' => 'Copyright/Legal information',
		'WPB' => 'Publishers official webpage',
		'WXX' => 'User defined URL link frame',

		# v2.3 tags
		'AENC' => 'Audio encryption',
		'APIC' => 'Attached picture',
		'COMM' => 'Comments',
		'COMR' => 'Commercial frame',
		'ENCR' => 'Encryption method registration',
		'EQUA' => 'Equalization',
		'ETCO' => 'Event timing codes',
		'GEOB' => 'General encapsulated object',
		'GRID' => 'Group identification registration',
		'IPLS' => 'Involved people list',
		'LINK' => 'Linked information',
		'MCDI' => 'Music CD identifier',
		'MLLT' => 'MPEG location lookup table',
		'OWNE' => 'Ownership frame',
		'PCNT' => 'Play counter',
		'POPM' => 'Popularimeter',
		'POSS' => 'Position synchronisation frame',
		'PRIV' => 'Private frame',
		'RBUF' => 'Recommended buffer size',
		'RVAD' => 'Relative volume adjustment',
		'RVRB' => 'Reverb',
		'SYLT' => 'Synchronized lyric/text',
		'SYTC' => 'Synchronized tempo codes',
		'TALB' => 'Album/Movie/Show title',
		'TBPM' => 'BPM (beats per minute)',
		'TCOM' => 'Composer',
		'TCON' => 'Content type',
		'TCOP' => 'Copyright message',
		'TDAT' => 'Date',
		'TDLY' => 'Playlist delay',
		'TENC' => 'Encoded by',
		'TEXT' => 'Lyricist/Text writer',
		'TFLT' => 'File type',
		'TIME' => 'Time',
		'TIT1' => 'Content group description',
		'TIT2' => 'Title/songname/content description',
		'TIT3' => 'Subtitle/Description refinement',
		'TKEY' => 'Initial key',
		'TLAN' => 'Language(s)',
		'TLEN' => 'Length',
		'TMED' => 'Media type',
		'TOAL' => 'Original album/movie/show title',
		'TOFN' => 'Original filename',
		'TOLY' => 'Original lyricist(s)/text writer(s)',
		'TOPE' => 'Original artist(s)/performer(s)',
		'TORY' => 'Original release year',
		'TOWN' => 'File owner/licensee',
		'TPE1' => 'Lead performer(s)/Soloist(s)',
		'TPE2' => 'Band/orchestra/accompaniment',
		'TPE3' => 'Conductor/performer refinement',
		'TPE4' => 'Interpreted, remixed, or otherwise modified by',
		'TPOS' => 'Part of a set',
		'TPUB' => 'Publisher',
		'TRCK' => 'Track number/Position in set',
		'TRDA' => 'Recording dates',
		'TRSN' => 'Internet radio station name',
		'TRSO' => 'Internet radio station owner',
		'TSIZ' => 'Size',
		'TSRC' => 'ISRC (international standard recording code)',
		'TSSE' => 'Software/Hardware and settings used for encoding',
		'TXXX' => 'User defined text information frame',
		'TYER' => 'Year',
		'UFID' => 'Unique file identifier',
		'USER' => 'Terms of use',
		'USLT' => 'Unsychronized lyric/text transcription',
		'WCOM' => 'Commercial information',
		'WCOP' => 'Copyright/Legal information',
		'WOAF' => 'Official audio file webpage',
		'WOAR' => 'Official artist/performer webpage',
		'WOAS' => 'Official audio source webpage',
		'WORS' => 'Official internet radio station homepage',
		'WPAY' => 'Payment',
		'WPUB' => 'Publishers official webpage',
		'WXXX' => 'User defined URL link frame',

		# v2.4 additional tags
		# note that we don't restrict tags from 2.3 or 2.4,
		'ASPI' => 'Audio seek point index',
		'EQU2' => 'Equalisation (2)',
		'RVA2' => 'Relative volume adjustment (2)',
		'SEEK' => 'Seek frame',
		'SIGN' => 'Signature frame',
		'TDEN' => 'Encoding time',
		'TDOR' => 'Original release time',
		'TDRC' => 'Recording time',
		'TDRL' => 'Release time',
		'TDTG' => 'Tagging time',
		'TIPL' => 'Involved people list',
		'TMCL' => 'Musician credits list',
		'TMOO' => 'Mood',
		'TPRO' => 'Produced notice',
		'TSOA' => 'Album sort order',
		'TSOP' => 'Performer sort order',
		'TSOT' => 'Title sort order',
		'TSST' => 'Set subtitle',

		# grrrrrrr
		'COM ' => 'Broken iTunes comments',
	);
}

1;

__END__

=pod

=back

=head1 TROUBLESHOOTING

If you find a bug, please send me a patch (see the project page in L<"SEE ALSO">).
If you cannot figure out why it does not work for you, please put the MP3 file in
a place where I can get it (preferably via FTP, or HTTP, or .Mac iDisk) and send me
mail regarding where I can get the file, with a detailed description of the problem.

If I download the file, after debugging the problem I will not keep the MP3 file
if it is not legal for me to have it.  Just let me know if it is legal for me to
keep it or not.


=head1 TODO

=over 4

=item ID3v2 Support

Still need to do more for reading tags, such as using Compress::Zlib to decompress
compressed tags.  But until I see this in use more, I won't bother.  If something
does not work properly with reading, follow the instructions above for
troubleshooting.

ID3v2 I<writing> is coming soon.

=item Get data from scalar

Instead of passing a file spec or filehandle, pass the
data itself.  Would take some work, converting the seeks, etc.

=item Padding bit ?

Do something with padding bit.

=item Test suite

Test suite could use a bit of an overhaul and update.  Patches very welcome.

=over 4

=item *

Revamp getset.t.  Test all the various get_mp3tag args.

=item *

Test Unicode.

=item *

Test OOP API.

=item *

Test error handling, check more for missing files, bad MP3s, etc.

=back

=item Other VBR

Right now, only Xing VBR is supported.

=back


=head1 THANKS

Edward Allen E<lt>allenej@c51844-a.spokn1.wa.home.comE<gt>,
Vittorio Bertola E<lt>v.bertola@vitaminic.comE<gt>,
Michael Blakeley E<lt>mike@blakeley.comE<gt>,
Per Bolmstedt E<lt>tomten@kol14.comE<gt>,
Tony Bowden E<lt>tony@tmtm.comE<gt>,
Tom Brown E<lt>thecap@usa.netE<gt>,
Sergio Camarena E<lt>scamarena@users.sourceforge.netE<gt>,
Chris Dawson E<lt>cdawson@webiphany.comE<gt>,
Luke Drumm E<lt>lukedrumm@mypad.comE<gt>,
Kyle Farrell E<lt>kyle@cantametrix.comE<gt>,
Jeffrey Friedl E<lt>jfriedl@yahoo.comE<gt>,
brian d foy E<lt>comdog@panix.comE<gt>,
Ben Gertzfield E<lt>che@debian.orgE<gt>,
Brian Goodwin E<lt>brian@fuddmain.comE<gt>,
Todd Hanneken E<lt>thanneken@hds.harvard.eduE<gt>,
Todd Harris E<lt>harris@cshl.orgE<gt>,
Woodrow Hill E<lt>asim@mindspring.comE<gt>,
Kee Hinckley E<lt>nazgul@somewhere.comE<gt>,
Roman Hodek E<lt>Roman.Hodek@informatik.uni-erlangen.deE<gt>,
Peter Kovacs E<lt>kovacsp@egr.uri.eduE<gt>,
Johann Lindvall,
Peter Marschall E<lt>peter.marschall@mayn.deE<gt>,
Trond Michelsen E<lt>mike@crusaders.noE<gt>,
Dave O'Neill E<lt>dave@nexus.carleton.caE<gt>,
Christoph Oberauer E<lt>christoph.oberauer@sbg.ac.atE<gt>,
Jake Palmer E<lt>jake.palmer@db.comE<gt>,
Andrew Phillips E<lt>asp@wasteland.orgE<gt>,
David Reuteler E<lt>reuteler@visi.comE<gt>,
John Ruttenberg E<lt>rutt@chezrutt.comE<gt>,
Matthew Sachs E<lt>matthewg@zevils.comE<gt>,
E<lt>scfc_de@users.sf.netE<gt>,
Hermann Schwaerzler E<lt>Hermann.Schwaerzler@uibk.ac.atE<gt>,
Chris Sidi E<lt>sidi@angband.orgE<gt>,
Roland Steinbach E<lt>roland@support-system.comE<gt>,
Stuart E<lt>schneis@users.sourceforge.netE<gt>,
Jeffery Sumler E<lt>jsumler@mediaone.netE<gt>,
Predrag Supurovic E<lt>mpgtools@dv.co.yuE<gt>,
Bogdan Surdu E<lt>tim@go.roE<gt>,
E<lt>tim@tim-landscheidt.deE<gt>,
Pass F. B. Travis E<lt>pftravis@bellsouth.netE<gt>,
Tobias Wagener E<lt>tobias@wagener.nuE<gt>,
Ronan Waide E<lt>waider@stepstone.ieE<gt>,
Andy Waite E<lt>andy@mailroute.comE<gt>,
Ken Williams E<lt>ken@forum.swarthmore.eduE<gt>,
Meng Weng Wong E<lt>mengwong@pobox.comE<gt>.


=head1 AUTHOR AND COPYRIGHT

Chris Nandor E<lt>pudge@pobox.comE<gt>, http://pudge.net/

Copyright (c) 1998-2003 Chris Nandor.  All rights reserved.  This program is
free software; you can redistribute it and/or modify it under the terms
of the Artistic License, distributed with Perl.


=head1 SEE ALSO

=over 4

=item MP3::Info Project Page

	http://projects.pudge.net/

=item mp3tools

	http://www.zevils.com/linux/mp3tools/

=item mpgtools

	http://www.dv.co.yu/mpgscript/mpgtools.htm
	http://www.dv.co.yu/mpgscript/mpeghdr.htm

=item mp3tool

	http://www.dtek.chalmers.se/~d2linjo/mp3/mp3tool.html

=item ID3v2

	http://www.id3.org/

=item Xing Variable Bitrate

	http://www.xingtech.com/support/partner_developer/mp3/vbr_sdk/

=item MP3Ext

	http://rupert.informatik.uni-stuttgart.de/~mutschml/MP3ext/

=item Xmms

	http://www.xmms.org/


=back

=head1 VERSION

v1.02, Sunday, March 2, 2003

=cut
