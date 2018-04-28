#!/usr/bin/perl

# Build Repeated Phrase File Database, (C) Matt Postiff, 2018
# Take the output of Jon Snoeberger's repeated_phrases.py
# and create a database of Bible references just like the
# cross reference file format, but with a slightly different
# meaning.
#
# Usage: buildrpf.pl ../BiblesInternational/phrases.txt repeated_phrases.rpf
# Input is phrases.txt, output is repeated_phrases.rpf
# phrases.txt is produced by this:
# ./repeated_phrases.py bible.txt phrases.txt 6
# See that script for usage details.

# Input is the repeated phrase followed by a tab followed by a semi-colon
# delimited list of verses where that phrase occurs. We ust want the list
# of verses to be "compressed" into a .rpf database
# a boil breaking forth with blains upon man and upon beast	Exodus_9:9;Exodus_9:10

# Output:
# A sequence of 32-bit words, like this:
# verse1 verse2 verse3 0x00000000 verse1 verse2....
# Based on a 32-bit word:
#  +--------+--------+--------+--------+
#  |booknum |chapnum | vrsnum | vrsnum2|
#  +--------+--------+--------+--------+
#
# See buildcrf.pl for details on the format. The .rpf file is a
# bit simpler because it doesn't have to worry about strange ranges.
#

$infile = shift(@ARGV);
$outfile = shift(@ARGV);
open(my $in, '<', $infile) or die "Unable to open for input: $!";
open(my $out, '>:raw', $outfile) or die "Unable to open for output: $!";

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

$rpCnt = 0; # repeated phrase count
$hwmReps = 0; # high water mark for number of repeats

while ($ln = <$in>) {
    print $ln;
    chomp($ln);
    $ln =~ s/.*\t//; # Eat everything before the tab, and the tab
    my @verses = split(/;/, $ln);
    $reps = scalar @verses;
    if ($reps > $hwmReps) { $hwmReps = $reps; }
    foreach $verse (@verses) {
	print "  ", $verse, "\n";
	$verse =~ /(.*)_([0-9]+):([0-9]+)/;
	$bk = $1;
	$ch = $2;
	$vs = $3;
	$bkNum = $books{$bk};
	#print "  bk=$bk ch=$ch vs=$vs\n";
	if ($bkNum == 0) {
	    die "Cannot find book number for $bk";
	}
	$enc=($bkNum<<24)|($ch<<16)|($vs<<8);
	print $out pack('L<', $enc);
	$rpCnt++;
    }
    # Print a 0x00000000 in order to end this list
    $enc=0;
    print $out pack('L<', $enc);
}

print "Counted $rpCnt repeated phrases.\n";
print "Most times a phrase was repeated: $hwmReps\n";

close($out);
