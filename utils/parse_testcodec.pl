#!/usr/bin/perl

#parse test codec output files and give wiki formatted results.  


if(scalar(@ARGV) != 2 && scalar(@ARGV) != 1){
	print "Usage: parser_testcodec.pl new_results old_results (compares two results)\n".
	"       parser_testcodec.pl new_results (formats just one result)\n";  
}
 
my %newfile;

#open new benchmark file
open FILE, $ARGV[0];
while ($line = <FILE>){
	chomp $line;
	$filename=$line;
	#print $filename."\n";
	
	$line = <FILE>;
	$line = <FILE>;
	$line =~ m/-\s([0-9\.]*)s/;
	$decodetime = $1;
	
	$line = <FILE>;
	$line = <FILE>;
	$line =~ m/([0-9\.]*)\%/;
	$realtime = $1;
	
	$line = <FILE>;
	$line =~ m/([0-9\.]*)MHz/;
	$mhz=$1;
	#consume blank line
	$line = <FILE>;
	
	#store in hash
	$newfile{$filename} = [$realtime, $mhz, $decodetime];
	
	#| flac_5.flac | 175906 of 175906 | Decode time - 27.74s | File duration - 175.90s | 634.10% realtime | 12.61MHz |
	#print "| $filename | Decode time - $decodetime"."s | $realtime"."% realtime | $mhz"."MHz |\n";
	#print "$filename\t$realtime\t$mhz\n";

	
}	

#open old benchmark file
my %oldfile;
open FILE, $ARGV[1];
while ($line = <FILE>){
	chomp $line;
	$filename=$line;
	#print $filename."\n";
	
	$line = <FILE>;
	$line = <FILE>;
	$line =~ m/-\s([0-9\.]*)s/;
	$decodetime = $1;
	
	$line = <FILE>;
	$line = <FILE>;
	$line =~ m/([0-9\.]*)\%/;
	$realtime = $1;
	
	$line = <FILE>;
	$line =~ m/([0-9\.]*)MHz/;
	$mhz=$1;
		
	#consume blank line
	$line = <FILE>;
	
	#store in hash
	$oldfile{$filename} = [$realtime, $mhz, $decodetime];
	

	
}

my @keylist;

@keylist = sort {$a cmp  $b}  keys(%newfile);
#print for wiki
my $oldkey = "nothing_";
foreach $key (@keylist){
	
	#check if this is a new format and add the table heading
	$oldkey =~ m/([a-z1-9]*)/;
	
	if(!($key =~ m/$1_/i)){
		print "| *MP3*  |||||\n" if($key =~ m/lame/);
		print "| *AAC-LC*  |||||\n" if($key =~ m/nero/);
		print "| *Vorbis*  |||||\n" if($key =~ m/vorbis/);
		print "| *WMA Standard*  |||||\n" if($key =~ m/wma_/);
		print "| *WAVPACK*  |||||\n" if($key =~ m/wv/);
		print "| *Nero AAC-HE*  |||||\n" if($key =~ m/aache/);
		print "| *Apple Lossless*  |||||\n" if($key =~ m/applelossless/);
		print "| *Monkeys Audio*  |||||\n" if($key =~ m/ape/);
		print "| *Musepack*  |||||\n" if($key =~ m/mpc/);
		print "| *FLAC*  |||||\n" if($key =~ m/flac/);
		print "| *Cook (RA)*  |||||\n" if($key =~ m/cook/);
		print "| *AC3 (A52)*  |||||\n" if($key =~ m/a52/);
		print "| *atrac3*  |||||\n" if($key =~ m/atrac3/);
		print "| *True Audio*  |||||\n" if($key =~ m/true/);
		print "| *MP2*  |||||\n" if($key =~ m/toolame/);
		#potiential future rockbox codecs
		print "| *atrac*  |||||\n" if($key =~ m/atrac1/);
		print "| *WMA Professional*  |||||\n" if($key =~ m/wmapro/);
		print "| *WMA Lossless*  |||||\n" if($key =~ m/wmal/);
		
	}
	
	if(defined($oldfile{$key})){
		$str=sprintf("%1.2f",($oldfile{$key}->[1]-$newfile{$key}->[1])/$oldfile{$key}->[1]*100+100	);
		print "| $key | $newfile{$key}->[0]"."% realtime | Decode time - $newfile{$key}->[2]s | ".sprintf("%2.2f",$newfile{$key}->[1])."MHz | ".$str."%|\n";
	}elsif(scalar(@ARGV) ==2){
		print "| $key | $newfile{$key}->[0]"."% realtime | Decode time - $newfile{$key}->[2]s | $newfile{$key}->[1]"."MHz | - |\n";
	} else{
		
		print "| $key | ". $newfile{$key}->[0]."% realtime | Decode time - $newfile{$key}->[2]s | ".sprintf("%2.2f",$newfile{$key}->[1])."MHz |\n";
	}
	$oldkey=$key;
}