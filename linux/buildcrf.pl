#!/usr/bin/perl

# Build Cross Reference Database, (C) Matt Postiff, 2018
# Take a list of verses like the following and create a compact binary
# database from it.

# Input:
#Genesis
#1:1
#2Ki 19:15;
#Rev 4:11
#1:2
#Job 7:12;
#33:4;
#...

# Output:

# Based on a 32-bit word:
#  +--------+--------+--------+--------+
#  |booknum |chapnum | vrsnum | vrsnum2|
#  +--------+--------+--------+--------+
# The vrsnum field accommodate range refs like Exo 10:21-23.
# If a reference is instead Exo 10:21, 23, then we have to
# encode that as a separate reference.
# To specify larger ranges, I use a special encoding:
# book|ch|vs|0xff where the 0xff means "dash," meaning a 
# "complex range operator." vs=0 indicates an entire chapter
# is being cross-referenced.

# In the cross-reference file format, the first listed 
# verse is the verse that is the subject of the
# cross references (Genesis 1:1). Following are 32-bits
# per verse that are cross-references from Genesis 1:1.
# This list is ended by a 0x00000000 32-bits, and then the
# next list starts.

open(my $out, '>:raw', 'bi.crf') or die "Unable to open: $!";

$books{"Genesis"} = 1;
$books{"Exodus"} = 2;
$books{"Leviticus"} = 3;
$books{"Numbers"} = 4;
$books{"Deuteronomy"} = 5;
$books{"Joshua"} = 6;
$books{"Judges"} = 7;
$books{"Ruth"} = 8;
$books{"1 Samuel"} = 9;
$books{"2 Samuel"} = 10;
$books{"1 Kings"} = 11;
$books{"2 Kings"} = 12;
$books{"1 Chronicles"} = 13;
$books{"2 Chronicles"} = 14;
$books{"Ezra"} = 15;
$books{"Nehemiah"} = 16;
$books{"Esther"} = 17;
$books{"Job"} = 18;
$books{"Psalms"} = 19;
$books{"Proverbs"} = 20;
$books{"Ecclesiastes"} = 21;
$books{"Song of Solomon"} = 22;
$books{"Isaiah"} = 23;
$books{"Jeremiah"} = 24;
$books{"Lamentations"} = 25;
$books{"Ezekiel"} = 26;
$books{"Daniel"} = 27;
$books{"Hosea"} = 28;
$books{"Joel"} = 29;
$books{"Amos"} = 30;
$books{"Obadiah"} = 31;
$books{"Jonah"} = 32;
$books{"Micah"} = 33;
$books{"Nahum"} = 34;
$books{"Habakkuk"} = 35;
$books{"Zephaniah"} = 36;
$books{"Haggai"} = 37;
$books{"Zechariah"} = 38;
$books{"Malachi"} = 39;
$books{"Matthew"} = 40;
$books{"Mark"} = 41;
$books{"Luke"} = 42;
$books{"John"} = 43;
$books{"Acts"} = 44;
$books{"Romans"} = 45;
$books{"1 Corinthians"} = 46;
$books{"2 Corinthians"} = 47;
$books{"Galatians"} = 48;
$books{"Ephesians"} = 49;
$books{"Philippians"} = 50;
$books{"Colossians"} = 51;
$books{"1 Thessalonians"} = 52;
$books{"2 Thessalonians"} = 53;
$books{"1 Timothy"} = 54;
$books{"2 Timothy"} = 55;
$books{"Titus"} = 56;
$books{"Philemon"} = 57;
$books{"Hebrews"} = 58;
$books{"James"} = 59;
$books{"1 Peter"} = 60;
$books{"2 Peter"} = 61;
$books{"1 John"} = 62;
$books{"2 John"} = 63;
$books{"3 John"} = 64;
$books{"Jude"} = 65;
$books{"Revelation"} = 66;

$abbrevs{"Gen"} = 1;
$abbrevs{"Exo"} = 2;
$abbrevs{"Lev"} = 3;
$abbrevs{"Num"} = 4;
$abbrevs{"Deu"} = 5;
$abbrevs{"Deut"} = 5;
$abbrevs{"Jos"} = 6;
$abbrevs{"Jdg"} = 7;
$abbrevs{"Rut"} = 8;
$abbrevs{"1Sa"} = 9;
$abbrevs{"2Sa"} = 10;
$abbrevs{"1Ki"} = 11;
$abbrevs{"2Ki"} = 12;
$abbrevs{"1Ch"} = 13;
$abbrevs{"2Ch"} = 14;
$abbrevs{"Ezr"} = 15;
$abbrevs{"Neh"} = 16;
$abbrevs{"Est"} = 17;
$abbrevs{"Job"} = 18;
$abbrevs{"Psa"} = 19;
$abbrevs{"Pro"} = 20;
$abbrevs{"Ecc"} = 21;
$abbrevs{"Sol"} = 22;
$abbrevs{"Isa"} = 23;
$abbrevs{"Jer"} = 24;
$abbrevs{"Lam"} = 25;
$abbrevs{"Eze"} = 26;
$abbrevs{"Dan"} = 27;
$abbrevs{"Hos"} = 28;
$abbrevs{"Joe"} = 29;
$abbrevs{"Amo"} = 30;
$abbrevs{"Oba"} = 31;
$abbrevs{"Jon"} = 32;
$abbrevs{"Mic"} = 33;
$abbrevs{"Nah"} = 34;
$abbrevs{"Hab"} = 35;
$abbrevs{"Zep"} = 36;
$abbrevs{"Hag"} = 37;
$abbrevs{"Zec"} = 38;
$abbrevs{"Mal"} = 39;
$abbrevs{"Mat"} = 40;
$abbrevs{"Mar"} = 41;
$abbrevs{"Luk"} = 42;
$abbrevs{"Joh"} = 43;
$abbrevs{"Act"} = 44;
$abbrevs{"Rom"} = 45;
$abbrevs{"1Co"} = 46;
$abbrevs{"2Co"} = 47;
$abbrevs{"Gal"} = 48;
$abbrevs{"Eph"} = 49;
$abbrevs{"Phi"} = 50;
$abbrevs{"Col"} = 51;
$abbrevs{"1Th"} = 52;
$abbrevs{"2Th"} = 53;
$abbrevs{"1Ti"} = 54;
$abbrevs{"2Ti"} = 55;
$abbrevs{"Tit"} = 56;
$abbrevs{"Phm"} = 57;
$abbrevs{"Heb"} = 58;
$abbrevs{"Jam"} = 59;
$abbrevs{"1Pe"} = 60;
$abbrevs{"2Pe"} = 61;
$abbrevs{"1Jo"} = 62;
$abbrevs{"2Jo"} = 63;
$abbrevs{"3Jo"} = 64;
$abbrevs{"Jud"} = 65;
$abbrevs{"Rev"} = 66;

$currBk = "";
$currBkNum = 0;
$currCh = 0;
$currVs = 0;
$crfCnt = 0;
$bookCnt = 0;
$verseCnt = 0;
$thisCrfCnt = 0;
$lastLineEndsWithSemicolon = 0;

while ($ln = <>) {
    chomp($ln);
    #print $ln;
    if ($books{$ln} != 0) {  # Example: Genesis, 1 John
	$currBk = $ln;
	$currBkNum = $books{$ln};
	#print "Found book $currBk\n";
	$bookCnt++;
	next;
    }
    if ($ln =~ /^([0-9]+):([0-9]+)$/) { # Example: 1:1, 1:2, 1:3, etc.
	if ($lastLineEndsWithSemicolon == 1) {
	    # 1:5           <-- refers to Genesis 1:5
	    # Psa 65:8;     <-- first cross-ref to 1:5
	    # 74:16         <-- second cross-ref to 1:5, but without special handling, this code thinks it is Gen 74:16
	    # 1:6               which starts a new x-ref list. WRONG! 
	    # Since the prior line ends with semicolon, we believe that the 74:16 goes with it instead of creating a new
	    # x-ref list.
	    $ch = $1;
	    $vs = $2;
	    $enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	    print $out pack('L<', $enc);
	    $crfCnt++;
	    $thisCrfCnt++;
	    $lastLineEndsWithSemicolon = 0; # this should be it
	    next;
	}
	if (($verseCnt != 0) && ($thisCrfCnt == 0)) {
	    print "WARNING: Zero cross refs for $currBk $currCh:$currVs ???\n";
	}
	$currCh = $1;
	$currVs = $2;
	#print "Found $currBk $currCh:$currVs\n";
	# Before printing this verse reference, we have to print 4 bytes of zero to indicate that we have reached the end
	# of a cross-reference list. But not the first four bytes of the file.
	if ($verseCnt > 0) {
	    $enc = 0;
	    print $out pack('L<', $enc);
	    $thisCrfCnt = 0; # reset counter
	}
	$verseCnt++;
	# $enc is the encoded verse reference, as shown above
	$enc=($currBkNum<<24)|($currCh<<16)|($currVs<<8);
	#printf("%08X\n", $enc);
	print $out pack('L<', $enc);
	next;
    }
    if ($ln =~ /^$/) { next; } # Skip blanks
    
    if ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+);?\s*$/) { # Example: 2Ki 19:15; the most common case
	$bk = $1; $bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	#print "Found $bk $ch:$vs\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+);?\s*$/) { # Example: 3:16
	$ch = $1;
	$vs = $2;
	#print "Found $ch:$vs\n";
	# $bkNum carries over from the last iteration of the loop
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+)-([0-9]+);?\s*$/) { # Example: 1:1-2
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	#print "Found $ch:$vs-$vs2\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+),([0-9]+);?\s*$/) { # Example: 1:1,2
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	#print "Found $ch:$vs,$vs2\n";
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+),([0-9]+),([0-9]+);?\s*$/) { # Example: 1:1,2,3
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	#print "Found $ch:$vs,$vs2,$vs3\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: 7:11,16-17;
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	#print "Found $ch:$vs,$vs2-$vs3\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($v2<<8)|$vs3;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+)-([0-9]+),([0-9]+),([0-9]+);?\s*$/) { # Example: 42:2-3,9,10
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	$vs4 = $5;
	#print "Found $ch:$vs-$vs2,$vs3,$vs4\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+),([0-9]+)-([0-9]+),([0-9]+);?\s*$/) { # Example: 5:2,5-6,8
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	$vs4 = $5;
	#print "Found $ch:$vs,$vs2-$vs3,$vs4\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8)|$vs3;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+)-([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: 42:2-3,9-10
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	$vs4 = $5;
	#print "Found $ch:$vs-$vs2,$vs3-$vs4\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8)|$vs4;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+)-([0-9]+),([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: 6:1-2,5,10-14;
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	$vs4 = $5;
	$vs5 = $6;
	#print "Found $ch:$vs-$vs2,$vs3,$vs4-$vs5\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8)|$vs5;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+)-([0-9]+),([0-9]+)-([0-9]+),([0-9]+);?\s*$/) { # Example: 20:27-32,39-41,44;
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	$vs4 = $5;
	$vs5 = $6;
	#print "Found $ch:$vs-$vs2,$vs3-$vs4,$vs5\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8)|$vs4;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs5<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+)-([0-9]+),([0-9]+),([0-9]+),([0-9]+);?\s*$/) { # Example: 22:2-6,9,12,27
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	$vs4 = $5;
	$vs5 = $6;
	#print "Found $ch:$vs-$vs2,$vs3,$vs4,$vs5\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs5<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+),([0-9]+),([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: 27:9,10,28,32-33
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	$vs4 = $5;
	$vs5 = $6;
	#print "Found $ch:$vs,$vs2,$vs3,$vs4-$vs5\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8)|$vs5;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+),([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: 30:4,10,24-25
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	$vs4 = $5;
	#print "Found $ch:$vs,$vs2,$vs3-$vs4\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8)|$vs4;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+),([0-9]+),([0-9]+),([0-9]+);?\s*$/) { # Example: 51:11,31,39,57
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	$vs4 = $5;
	#print "Found $ch:$vs,$vs2,$vs3,$vs4\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([0-9]+):([0-9]+)-([0-9]+),([0-9]+);?\s*$/) { # Example: 1:1-2,3
	$ch = $1;
	$vs = $2;
	$vs2 = $3;
	$vs3 = $4;
	#print "Found $ch:$vs-$vs2, $vs3\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+)-([0-9]+);?\s*$/) { # Example: 2Ki 19:15-17;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	#print "Found $bk $ch:$vs-$vs2\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+),([0-9]+);?\s*$/) { # Example: 1Co 6:9,10;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	#print "Found $bk $ch:$vs-$vs2\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+),([0-9]+),([0-9]+);?\s*$/) { # Example: 2Ki 19:8,11,20;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	#print "Found $bk $ch:$vs,$vs2,$vs3\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+)-([0-9]+),([0-9]+);?\s*$/) { # Example: 2Ki 19:8-11,20;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	#print "Found $bk $ch:$vs,$vs2,$vs3\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+)-([0-9]+),([0-9]+),([0-9]+);?\s*$/) { # Example: 2Ki 19:8-11,20,22;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	#print "Found $bk $ch:$vs,$vs2,$vs3\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: Gen 5:18,21-24;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	#print "Found $bk $ch:$vs,$vs2-$vs3\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8)|$vs3;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+);?\s*$/) { # Example: Eze 40:16,19,26,31,49
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	$vs5 = $7;
	#print "Found $bk $ch:$vs,$vs2,$vs3,$vs4,$vs5\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs5<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+),([0-9]+),([0-9]+),([0-9]+);?\s*$/) { # Example: 2Ki 19:8,11,20,25;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	#print "Found $bk $ch:$vs,$vs2,$vs3,$vs4\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+),([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: Gen 6:3,5,13-14;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	#print "Found $bk $ch:$vs,$vs2,$vs3-$vs4\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8)|$vs4;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+)-([0-9]+),([0-9]+)-([0-9]+),([0-9]+);?\s*$/) { # Example: Eze 34:11-15,23-24,35;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	$vs5 = $7;
	#print "Found $bk $ch:$vs-$vs2,$vs3-$vs4, $vs5\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8)|$vs4;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs5<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+)-([0-9]+),([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: Eze 20:5-6,28,41-44;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	$vs5 = $7;
	#print "Found $bk $ch:$vs-$vs2,$vs3,$vs4-$vs5\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8)|$vs5;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+)-([0-9]+),([0-9]+)-([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: 2Ki 11:1-3,11-12,15-16;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	$vs5 = $7;
	$vs6 = $8;
	#print "Found $bk $ch:$vs-$vs2,$vs3-$vs4,$vs5-$vs6\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8)|$vs4;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs5<<8)|$vs6;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+),([0-9]+)-([0-9]+),([0-9]+),([0-9]+);?\s*$/) { # Example: Rom 8:2,4-6,9,14;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	$vs5 = $7;
	#print "Found $bk $ch:$vs,$vs2-$vs3,$vs4,$vs5\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8)|$vs3;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs5<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+)-([0-9]+),([0-9]+)-([0-9]+);?\s*$/) { # Example: Eze 16:4-22,35-39;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	#print "Found $bk $ch:$vs,$vs2-$vs3,$vs4\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|$vs2;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8)|$vs4;
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+),([0-9]+)-([0-9]+),([0-9]+);?\s*$/) { # Example: Rom 4:1,11-12,16;
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	#print "Found $bk $ch:$vs,$vs2-$vs3,$vs4\n";
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8)|$vs3;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8);
	print $out pack('L<', $enc);
    }
    # Following are tough cases because they cross chapter boundaries or contain ranges that cross chapter boundaries.
    # I am going to use a special encoding
    # book|ch|vs|0xff where the 0xff means "dash," meaning a "complex range operator." vs=0 indicates an entire chapter
    # is being cross-referenced.
    elsif ($ln =~ /^([0-9]+):([0-9]+)-([0-9]+):([0-9]+);?\s*$/) { # Example: 5:14-6:7
	$ch = $2;
	$vs = $3;
	$ch2 = $4;
	$vs2 = $5;
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|0xff;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch2<<16)|($vs2<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+)-([0-9]+);?\s*$/) { # Example: 1Ch 24-26 (3 chapters)
	$bk = $1;$bkNum = $abbrevs{$bk};
	$ch = $2;
	$ch2 = $3;
	#print "Found $bk $ch-$ch2\n";
	$enc=($bkNum<<24)|($ch<<16)|0xff;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch2<<16);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+)-([0-9]+):([0-9]+);?\s*$/) { # Example: Eze 2:8-3:3;
	$bk = $1; $bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$ch2 = $4;
	$vs2 = $5;
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8)|0xff;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch2<<16)|($vs2<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+); ([0-9]+):([0-9]+),([0-9]+);?\s*$/) { # Example: Heb 2:17; 9:12,14;
	$bk = $1; $bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$ch2 = $4;
	$vs2 = $5;
	$vs3 = $6;
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch2<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch2<<16)|($vs3<<8);
	print $out pack('L<', $enc);
    }
    elsif ($ln =~ /^([1-3a-zA-Z ]+) ([0-9]+):([0-9]+),([0-9]+),([0-9]+),([0-9]+)-([0-9]+):([0-9]+);?\s*$/) { # Example: Hag 1:1,3,12,14-2:9;
	$bk = $1; $bkNum = $abbrevs{$bk};
	$ch = $2;
	$vs = $3;
	$vs2 = $4;
	$vs3 = $5;
	$vs4 = $6;
	$ch2 = $7;
	$vs5 = $8;
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs3<<8);
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch<<16)|($vs4<<8)|0xff;
	print $out pack('L<', $enc);
	$enc=($bkNum<<24)|($ch2<<16)|($vs5<<8);
	print $out pack('L<', $enc);
    }
    else {
        print "UNKNOWN>>>$ln<<< \tat $currBk $currCh:$currVs\n";
    }
    $crfCnt++;
    $thisCrfCnt++;

    if ($abbrevs{$bk} == 0) {
	print "UNKNOWN Book abbreviation>>>$bk<<<\n";
    }

    if ($ln =~ /;\s*$/) { $lastLineEndsWithSemicolon = 1; }
    else { $lastLineEndsWithSemicolon = 0; }
}

print "Found $bookCnt books.\n";
print "Counted $crfCnt cross references, many of which are multiple verses.\n";
print "These are references from $verseCnt source verses.\n";

close($out);
