#!/usr/bin/perl

use File::Basename;

$cgi = basename $0;

print "Content-Type: text/html\n\n";

$image = $ARGV[0];

$image =~ /img(\d+)/;

print "<html><head><title>Rockbox - Photo $1</title></head>\n";
print "<body bgcolor=black text=white link=white vlink=gray>\n";

if ($image eq "" ) {
    print "<p>No image specified\n";
    exit;
}

print "<h1>&nbsp;</h1><div align=center><img src=$image>\n";

# compressed image
if ( $image =~ /img(\d+).jpg/ ) {
    $num = $1;
    for $i ( 1 .. 10 ) {
	$prev = sprintf("%04d",$num-$i);
	last if ( -f "img$prev.jpg" ); 
    }
    if ( $prev == $num-10 ) {
	undef $prev;
    }

    for $i ( 1 .. 20 ) { 
	$next = sprintf("%04d",$num+$i);
	print "<!-- Trying $next -->\n";
	last if ( -f "img$next.jpg" );
    }
    if ( $next == $num+20 ) {
	undef $next;
    }

    if ( -f "bildtext.txt" ) {
	open FILE, "<bildtext.txt";
	@txt = <FILE>;
	close FILE;
	@match = grep /^$num: /, @txt;
	if ( $match[0] =~ /^$num: (.*)/ ) {
	    print "<p><i>$1</i>\n";
	}
    }


    print "<p>\n";
    print "<a href=$cgi?img$prev.jpg>&lt; Previous</a>  &nbsp; \n" if ( $prev );
    print "<a href=.>Index</a>\n";
    if ( -f "IMG_$num.JPG" ) {
        $size = int( (stat("IMG_$num.JPG"))[7] / 1024 );
	print " &nbsp; <a href=IMG_$num.JPG>Fullsize ($size kB)</a>\n";
    }

    print " &nbsp; <a href=$cgi?img$next.jpg>Next &gt;</a>\n" if ( $next );

}

# showing fullsize already
elsif ( $image =~ /IMG_(\d+).JPG/ ) {
    $num = $1;
    for $i ( 1 .. 10 ) {
	$prev = sprintf("%04d",$num-$i);
	last if ( -f "IMG_$prev.JPG" ); 
    }
    if ( $prev == $num-10 ) {
	undef $prev;
    }

    for $i ( 1 .. 20 ) { 
	$next = sprintf("%04d",$num+$i);
	print "<!-- Trying $next -->\n";
	last if ( -f "IMG_$next.JPG" );
    }
    if ( $next == $num+20 ) {
	undef $next;
    }

    print "<p>\n";
    print "<a href=$cgi?IMG_$prev.JPG>&lt; Previous</a>  &nbsp; \n" if ( $prev );
    print "<a href=.>Index</a>\n";
    if ( -f "img$num.jpg" ) {
        $size = int( (stat("img$num.jpg"))[7] / 1024 );
	print " &nbsp; <a href=img$num.jpg>Small ($size kB)</a>\n";
    }

    print " &nbsp; <a href=$cgi?IMG_$next.JPG>Next &gt;</a>\n" if ( $next );
}
print "</div></body></html>\n";
