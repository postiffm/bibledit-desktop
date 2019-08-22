#!/usr/bin/perl
use strict;
use warnings;
# Based on readcrf.pl...if that changes, this would have to change
# Read Cross Reference Database and print lists for different
# subsets of the data. This script will take 

# Usage: createLmitedCRF.pl infile
# Example: createdLimitedCRF.pl ../BiblesInternational/bi.crf
# Output is verse ==> cross references
#
# A problem is that if the cross-ref list is empty because
# say we eliminate all OT books, and those were the only
# CRFs for that origin verse, then we end up with a
# bunch of empty lists.
#
# In that case, post-process it with cleanEmptyCRFs.pl infile

print "Creating various cross-reference lists...\n";

my %books;

$books{1} = "Genesis";
$books{2} = "Exodus";
$books{3} = "Leviticus";
$books{4} = "Numbers";
$books{5} = "Deuteronomy";
$books{6} = "Joshua";
$books{7} = "Judges";
$books{8} = "Ruth";
$books{9} = "1 Samuel";
$books{10} = "2 Samuel";
$books{11} = "1 Kings";
$books{12} = "2 Kings";
$books{13} = "1 Chronicles";
$books{14} = "2 Chronicles";
$books{15} = "Ezra";
$books{16} = "Nehemiah";
$books{17} = "Esther";
$books{18} = "Job";
$books{19} = "Psalms";
$books{20} = "Proverbs";
$books{21} = "Ecclesiastes";
$books{22} = "Song of Solomon";
$books{23} = "Isaiah";
$books{24} = "Jeremiah";
$books{25} = "Lamentations";
$books{26} = "Ezekiel";
$books{27} = "Daniel";
$books{28} = "Hosea";
$books{29} = "Joel";
$books{30} = "Amos";
$books{31} = "Obadiah";
$books{32} = "Jonah";
$books{33} = "Micah";
$books{34} = "Nahum";
$books{35} = "Habakkuk";
$books{36} = "Zephaniah";
$books{37} = "Haggai";
$books{38} = "Zechariah";
$books{39} = "Malachi";
$books{40} = "Matthew";
$books{41} = "Mark";
$books{42} = "Luke";
$books{43} = "John";
$books{44} = "Acts";
$books{45} = "Romans";
$books{46} = "1 Corinthians";
$books{47} = "2 Corinthians";
$books{48} = "Galatians";
$books{49} = "Ephesians";
$books{50} = "Philippians";
$books{51} = "Colossians";
$books{52} = "1 Thessalonians";
$books{53} = "2 Thessalonians";
$books{54} = "1 Timothy";
$books{55} = "2 Timothy";
$books{56} = "Titus";
$books{57} = "Philemon";
$books{58} = "Hebrews";
$books{59} = "James";
$books{60} = "1 Peter";
$books{61} = "2 Peter";
$books{62} = "1 John";
$books{63} = "2 John";
$books{64} = "3 John";
$books{65} = "Jude";
$books{66} = "Revelation";

my $infile = shift(@ARGV);
open(my $fh, '<:raw', $infile) or die "Unable to open for input: $!";
my $startNewList = 1;

open(my $crfAll,    ">", "crfAll.txt")    or die "Unable to open for output: $!";
open(my $crfNTOnly, ">", "crfNTOnly.txt") or die "Unable to open for output: $!";
open(my $crfNTPs,   ">", "crfNTPs.txt")   or die "Unable to open for output: $!";
open(my $crfNTPsPro, ">", "crfNTPsPro.txt")   or die "Unable to open for output: $!";

while (1) {
    # Read the encoded data and unpack it
    my $bytes_read = read $fh, my $bytes, 4;
    if ($bytes_read != 4) { last; } # There should always be a multiple of 4 bytes
    my $enc = unpack("L<", $bytes);
    if ($enc == 0) { $startNewList = 1; next; } # end of prior x-ref list
    my $vs2 = $enc & 0xff;
    my $vs = ($enc >> 8) & 0xff;
    my $ch = ($enc >> 16) & 0xff;
    my $bk = ($enc >> 24) & 0xff;

    # Decode and print the data
    #printAll($crfAll, $bk, $ch, $vs, $vs2, $startNewList);
    printRange(0, $crfAll, $bk, $ch, $vs, $vs2, $startNewList, 1, 66, 1, 66);
    printRange(1, $crfNTOnly, $bk, $ch, $vs, $vs2, $startNewList, 40, 66, 40, 66);
    printRange(2, $crfNTPs, $bk, $ch, $vs, $vs2, $startNewList, 40, 66, 19, 19);
    printRange(3, $crfNTPsPro, $bk, $ch, $vs, $vs2, $startNewList, 40, 66, 19, 20);
    
    if ($startNewList == 1) { $startNewList = 0; }
}

close($crfAll);
close($crfNTOnly);
close($crfNTPs);
close($crfNTPsPro);

# This array tells us as we walk through which CRFs we are currently skipping
# because of "out of range" for the particular range set. So 0 is "all CRFs"
# and 1 is NT only, and 2 is NT and Psalms, etc.
# This is a bit complicated, but it let me have a simpler printRange routine.
my @skipList = [0,0,0,0,0,0,0,0,0,0];

sub printRange {
    my ($listidx,$fh,$bk,$ch,$vs,$vs2,$startNewList,$r1,$r2,$r3,$r4) = @_;
    if ($startNewList == 1) {
	$skipList[$listidx] = 0; # reset
    }
    # If for this particular list we are in skip mode, don't do anything
    if ($skipList[$listidx] == 1) {
	return 0;
    }
    if ( (($bk >= $r1) && ($bk <= $r2)) || (($bk >= $r3) && ($bk <= $r4)) ) {
	if ($startNewList == 1) {
	    print {$fh} "\n"; 
	}
      print {$fh} "$books{$bk} $ch:$vs";
      if ($vs2 == 0xff) { print {$fh} "-"; }       # special case: 0xff is a "complex" range marker
      elsif ($vs2 != 0) { print {$fh} "-$vs2 "; }  # otherwise, the second verse is end of a simple range
      else { print {$fh} " "; }
      if ($startNewList == 1) {
	  print {$fh} "==> ";
      }
    }

    # We may wish to use an output format like this for Paratext's sake
    # MRK.4:22 [MAT.10:25 LUK.19:26]

    else {
	# Book is out of range. 2 cases.
	
	# 1. If this reference is the start of a list (a \xo, origin reference)
	# then the rest of its list must be omitted.
	if ($startNewList) { $skipList[$listidx] = 1; }
	if ($listidx == 0) { die "List Index is 0; should never happen for all CRFs."; }
	
	# 2. If on the other hand the reference is in the middle of a list
	# (a \xt, target reference) then we can just skip it itself.
	# Do nothing else
    }
}

# Not using this now
sub printAll {
    my ($fh,$bk,$ch,$vs,$vs2,$startNewList) = @_;
    if ($startNewList == 1) { print {$fh} "\n"; }
    print {$fh} "$books{$bk} $ch:$vs";
    if ($vs2 == 0xff) { print {$fh} "-"; }       # special case: 0xff is a "complex" range marker
    elsif ($vs2 != 0) { print {$fh} "-$vs2 "; }  # otherwise, the second verse is end of a simple range
    else { print {$fh} " "; }
    if ($startNewList == 1) {
	print {$fh} "==> ";
    }
}

#$enc = 0x18041700;
#printf("%08X\n", $enc);
#my $test1 = ($enc >> 24) & 0xff;
#printf("%08X\n", $test1);
#my $test2 = ($enc & 0xff000000) >> 24;
#printf("%08X\n", $test2);
