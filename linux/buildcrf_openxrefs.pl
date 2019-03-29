#!/usr/bin/perl

# Build Cross Reference Database, (C) Matt Postiff, 2018
# Take a list of verses like the following and create a compact binary
# database from it.
#
# Usage: buildcrf_openxrefs.pl ../BiblesInternational/open_bible_info_crf.txt
# Output by default goes to open_bible_info.crf
# Be careful to not overwrite *.crf accidentally!

# Input:
#From Verse	To Verse	Votes	#www.openbible.info CC-BY 2018-03-19
#Gen.1.1	Job.38.4	51
#Gen.1.1	Ps.121.2	15
#Gen.1.1	Isa.65.17	8

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

# In the cross-reference file format (.crf), the first listed 
# verse is the verse that is the subject of the
# cross references (Genesis 1:1). Following are 32-bits
# per verse that are cross-references from Genesis 1:1.
# This list is ended by a 0x00000000 32-bits, and then the
# next list starts.

open(my $out, '>:raw', 'open_bible_info.crf') or die "Unable to open: $!";

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
$abbrevs{"Exod"} = 2;
$abbrevs{"Lev"} = 3;
$abbrevs{"Num"} = 4;
$abbrevs{"Deu"} = 5;
$abbrevs{"Deut"} = 5;
$abbrevs{"Jos"} = 6;
$abbrevs{"Josh"} = 6;
$abbrevs{"Jdg"} = 7;
$abbrevs{"Judg"} = 7;
$abbrevs{"Rut"} = 8;
$abbrevs{"Ruth"} = 8;
$abbrevs{"1Sa"} = 9;
$abbrevs{"1Sam"} = 9;
$abbrevs{"2Sa"} = 10;
$abbrevs{"2Sam"} = 10;
$abbrevs{"1Ki"} = 11;
$abbrevs{"1Kgs"} = 11;
$abbrevs{"2Ki"} = 12;
$abbrevs{"2Kgs"} = 12;
$abbrevs{"1Ch"} = 13;
$abbrevs{"1Chr"} = 13;
$abbrevs{"2Ch"} = 14;
$abbrevs{"2Chr"} = 14;
$abbrevs{"Ezr"} = 15;
$abbrevs{"Ezra"} = 15;
$abbrevs{"Neh"} = 16;
$abbrevs{"Est"} = 17;
$abbrevs{"Esth"} = 17;
$abbrevs{"Job"} = 18;
$abbrevs{"Psa"} = 19;
$abbrevs{"Ps"} = 19;
$abbrevs{"Pro"} = 20;
$abbrevs{"Prov"} = 20;
$abbrevs{"Ecc"} = 21;
$abbrevs{"Eccl"} = 21;
$abbrevs{"Sol"} = 22;
$abbrevs{"Song"} = 22;
$abbrevs{"Isa"} = 23;
$abbrevs{"Jer"} = 24;
$abbrevs{"Lam"} = 25;
$abbrevs{"Eze"} = 26;
$abbrevs{"Ezek"} = 26;
$abbrevs{"Dan"} = 27;
$abbrevs{"Hos"} = 28;
$abbrevs{"Joe"} = 29;
$abbrevs{"Joel"} = 29;
$abbrevs{"Amo"} = 30;
$abbrevs{"Amos"} = 30;
$abbrevs{"Oba"} = 31;
$abbrevs{"Obad"} = 31;
$abbrevs{"Jon"} = 32;
$abbrevs{"Jonah"} = 32;
$abbrevs{"Mic"} = 33;
$abbrevs{"Nah"} = 34;
$abbrevs{"Hab"} = 35;
$abbrevs{"Zep"} = 36;
$abbrevs{"Zeph"} = 36;
$abbrevs{"Hag"} = 37;
$abbrevs{"Zec"} = 38;
$abbrevs{"Zech"} = 38;
$abbrevs{"Mal"} = 39;
$abbrevs{"Mat"} = 40;
$abbrevs{"Matt"} = 40;
$abbrevs{"Mar"} = 41;
$abbrevs{"Mark"} = 41;
$abbrevs{"Luk"} = 42;
$abbrevs{"Luke"} = 42;
$abbrevs{"Joh"} = 43;
$abbrevs{"John"} = 43;
$abbrevs{"Act"} = 44;
$abbrevs{"Acts"} = 44;
$abbrevs{"Rom"} = 45;
$abbrevs{"1Co"} = 46;
$abbrevs{"1Cor"} = 46;
$abbrevs{"2Co"} = 47;
$abbrevs{"2Cor"} = 47;
$abbrevs{"Gal"} = 48;
$abbrevs{"Eph"} = 49;
$abbrevs{"Phi"} = 50;
$abbrevs{"Phil"} = 50;
$abbrevs{"Col"} = 51;
$abbrevs{"1Th"} = 52;
$abbrevs{"1Thess"} = 52;
$abbrevs{"2Th"} = 53;
$abbrevs{"2Thess"} = 53;
$abbrevs{"1Ti"} = 54;
$abbrevs{"1Tim"} = 54;
$abbrevs{"2Ti"} = 55;
$abbrevs{"2Tim"} = 55;
$abbrevs{"Tit"} = 56;
$abbrevs{"Titus"} = 56;
$abbrevs{"Phm"} = 57;
$abbrevs{"Phlm"} = 57;
$abbrevs{"Heb"} = 58;
$abbrevs{"Jam"} = 59;
$abbrevs{"Jas"} = 59;
$abbrevs{"1Pe"} = 60;
$abbrevs{"1Pet"} = 60;
$abbrevs{"2Pe"} = 61;
$abbrevs{"2Pet"} = 61;
$abbrevs{"1Jo"} = 62;
$abbrevs{"1John"} = 62;
$abbrevs{"2Jo"} = 63;
$abbrevs{"2John"} = 63;
$abbrevs{"3Jo"} = 64;
$abbrevs{"3John"} = 64;
$abbrevs{"Jud"} = 65;
$abbrevs{"Jude"} = 65;
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
$ln = <>; # eat the first line (headers)

while ($ln = <>) {
    chomp($ln);
    print $ln;
    if ($ln =~ /^$/) { next; } # Skip blanks
    # Following is a regex that matches like Gen.1.1 or 1Sa.1.1
    $refpattern = qr/([1-3a-zA-Z]+)\.([0-9]+)\.([0-9]+)/;
    # Example: Gen.1.1	Job.38.4	51
    if ($ln =~ /^$refpattern\t$refpattern\t/) {
	$bk1 = $1;
	$ch1 = $2;
	$vs1 = $3;
	$bk2 = $4;
	$ch2 = $5;
	$vs2 = $6;
	print "\tFound $bk1 $ch1:$vs1 ==> $bk2 $ch2:$vs2\n";
	# Before printing this verse reference, we have to print 4 bytes of zero to indicate that we have reached the end
	# of a cross-reference list. But not the first four bytes of the file.
	if (($bk1 != $currBk) || ($ch1 != $currCh) || ($vs1 != $currVs)) {
	    # We need to make a change-over to a new "parent" verse. To mark that, we put an 0x0 at
	    # the end of the previous list...except if this is the first verse, there is no previous
	    # list, so we don't put a 0x0 first.n
	    if ($verseCnt != 0) {
		$enc = 0;
		print $out pack('L<', $enc);
	    }
	    $thisCrfCnt = 0; # reset counter
	    $currBk = $bk1;
	    $currBkNum = $abbrevs{$bk1};
	    if ($currBkNum == 0) {
		print "UNKNOWN Book abbreviation>>>$bk1<<<\n";
	    }
	    $currCh = $ch1;
	    $currVs = $vs1;
	    $enc=($currBkNum<<24)|($currCh<<16)|($currVs<<8);
	    print $out pack('L<', $enc);
	}
	$bk2Num = $abbrevs{$bk2};
	if ($bk2Num == 0) {
	    print "UNKNOWN Book abbreviation>>>$bk2<<<\n";
	}
	$enc=($bk2Num<<24)|($ch2<<16)|($vs2<<8);
	print $out pack('L<', $enc);
	$verseCnt++;
    }
    # Example: Gen.1.1	Ps.89.11-Ps.89.12
    elsif ($ln =~ /^$refpattern\t$refpattern-$refpattern\t/) {
	$bk1 = $1;
	$ch1 = $2;
	$vs1 = $3;
	$bk2 = $4;
	$ch2 = $5;
	$vs2 = $6;
	$bk3 = $7;
	$ch3 = $8;
	$vs3 = $9;
	# Repeated code...not good, but quick
	print "\tFound $bk1 $ch1:$vs1 ==> $bk2 $ch2:$vs2 - $bk3 $ch3:$vs3 \n";
	# Before printing this verse reference, we have to print 4 bytes of zero to indicate that we have reached the end
	# of a cross-reference list. But not the first four bytes of the file.
	if (($verseCnt != 0) && (($bk1 != $currBk) || ($ch1 != currCh) || ($vs1 != currVs))) {
	    $enc = 0;
	    print $out pack('L<', $enc);
	    $thisCrfCnt = 0; # reset counter
	    $currBk = $bk1;
	    $currBkNum = $abbrevs{$bk1};
	    if ($currBkNum == 0) {
		print "UNKNOWN Book abbreviation>>>$bk1<<<\n";
	    }
	    $currCh = $ch1;
	    $currVs = $vs1;
	    $enc=($currBkNum<<24)|($currCh<<16)|($currVs<<8);
	    print $out pack('L<', $enc);
	}
	$bk2Num = $abbrevs{$bk2};
	if ($bk2Num == 0) {
	    print "UNKNOWN Book abbreviation>>>$bk2<<<\n";
	}
	$bk3Num = $abbrevs{$bk3};
	if ($bk3Num == 0) {
	    print "UNKNOWN Book abbreviation>>>$bk3<<<\n";
	}
	if (($bk2 == $bk3) && ($ch2 == $ch3)) {
	    # Fine...we can encode with the short encoding
	    $enc=($bk2Num<<24)|($ch2<<16)|($vs2<<8)|$vs3;
	    print $out pack('L<', $enc);
	    $verseCnt++;	
	}
	else {
	    # We have to encode with the longer encoding because it is like Gen.1.31-Gen.2.1 where
	    # it crosses a chapter boundary.
	    $enc=($bk2Num<<24)|($ch2<<16)|($vs2<<8)|0xff;
	    print $out pack('L<', $enc);
	    # Usually $bk3Num == $bk2Num...but sometimes the cross-ref crosses a book boundary.
	    $enc=($bk3Num<<24)|($ch3<<16)|($vs3<<8)|0xff;
	    print $out pack('L<', $enc);
	    $verseCnt++;	
	}
    }
    else {
        print "UNKNOWN>>>$ln<<<\n";
    }
    $crfCnt++;
    $thisCrfCnt++;

    if ($ln =~ /;\s*$/) { $lastLineEndsWithSemicolon = 1; }
    else { $lastLineEndsWithSemicolon = 0; }
}

print "Found $bookCnt books.\n";
print "Counted $crfCnt cross references, many of which are multiple verses.\n";
print "These are references from $verseCnt source verses.\n";

close($out);
