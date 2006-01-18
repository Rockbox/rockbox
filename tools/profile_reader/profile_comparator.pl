#!/usr/bin/perl
sub error {
  print("Error: @_\n");
  exit(1);
}
sub usage {
  if (@_) {
    print STDERR ("Error: @_\n");
  }
  print STDERR ("USAGE:\n");
  print STDERR ("$0 file1 file2 [showcalldiff]\n");
  print STDERR
    ("\tfile[12]      output from profile_reader.pl to compare\n");
  print STDERR
    ("\tshowcalldiff  show the percent change in calls instead of ticks\n");
  exit(1);
}
if ($ARGV[0] =~ m/-(h|help|-help)/) {
  usage();
}
if (@ARGV < 2) {
  usage("Requires at least 2 arguments");
}
open(FILE1,shift) || error("Couldn't open file1");
my @file1 = <FILE1>;
close(FILE1);
open(FILE2,shift) || error("Couldn't open file2");
my @file2 = <FILE2>;
close(FILE2);
my $showcalldiff = shift;
my %calls1;
my %calls2;
my @calls = (\%calls1,\%calls2);
my $start = 0;
my @files = (\@file1,\@file2);
my @allcalls = (0,0);
my @allticks = (0,0);
for ( $i=0; $i <= $#files; $i++ ) {
  my $file = $files[$i];
  foreach $line(@$file) {
    chomp($line);
    if ( $line =~ m/By calls/ ) {
      $start = 1;
      next;
    }
    if ( $line =~ m/By ticks/ ) {
      $start = 0;
      last;
    }
    if ( $start == 1) {
      my @line = split(/[[:space:]]+/,$line);
      $allcalls[$i] += $line[1];
      $allticks[$i] += $line[3];
      $calls[$i]{$line[5]} = [($line[1],$line[3])];
    }
  }
}
printf("File one calls: %08ld, ticks: %08ld\n",$allcalls[0],$allticks[0]);
printf("File two calls: %08ld, ticks: %08ld\n",$allcalls[1],$allticks[1]);
printf("Percent change: %+7.2f%%, ticks: %+7.2f%%\n",
        ($allcalls[1]-$allcalls[0])/$allcalls[0]*100,
        ($allticks[1]-$allticks[0])/$allticks[0]*100);
my @allkeys = keys(%calls1);
push(@allkeys,keys(%calls2));
my %u = ();
my @keys = grep {defined} map {
  if (exists $u{$_}) { undef; } else { $u{$_}=undef;$_; }
} @allkeys;
undef %u;
my %byticks;
my %bycalls;
foreach $key(@keys) {
  my $values1 = $calls1{$key};
  my $values2 = $calls2{$key};
  my $calldiff = @$values2[0]-@$values1[0];
  my $totalcalls = @$values2[0]+@$values1[0];
  my $tickdiff = @$values2[1]-@$values1[1];
  my $totalticks = @$values2[1]+@$values1[1];
  my $pdiff;
  my $result;
  if ($showcalldiff) {
    $pdiff = $calldiff/(@$values1[0]>0?@$values1[0]:1)*100;
    $result = sprintf("%+7.2f%%  Calls: %+09d  Symbol: %s$key\n", 
        $pdiff, $calldiff,
        (exists $calls1{$key} && exists $calls2{$key})?"":"LONE ");
  } else {
    $pdiff = $tickdiff/(@$values1[1]>0?@$values1[1]:1)*100;
    $result = sprintf("%+7.2f%%  Ticks: %+09d  Symbol: %s$key\n", 
        $pdiff, $tickdiff, 
        (exists $calls1{$key} && exists $calls2{$key})?"":"LONE ");
  }
  $bycalls{sprintf("%08X$key",$totalcalls)} = $result;
  $byticks{sprintf("%08X$key",$totalticks)} = $result;
}
my @calls = sort(keys(%bycalls));
print("By calls\n");
foreach $call(@calls) {
  print($bycalls{$call});
}
my @ticks = sort(keys(%byticks));
print("By ticks\n");
foreach $tick(@ticks) {
  print($byticks{$tick});
}
