#!/usr/bin/perl

# (C) Matt Postiff, 2018

# Convert EMTV from Paul Esposito to Bibleworks (and Bibledit friendly)
# format(s). The text came to me in Word. I exported the NT, Psalms, and
# Proverbs in separate text files, \n delimited. Now I am trying to
# clean them up and format them for Bibleworks.
#
# Note that this script is only designed to work on the NT documents.
# The formatting of the Psalms and Proverbs is tougher to handle.

# Usage: ./convertEMTV.pl NewTestament.txt NewTestamentBibleworks.txt NewTestamentOutline.txt

# Abbreviations for book names that are acceptable to Bibleworks
$abbrevs{"Gen"} = "Gen";
$abbrevs{"Exo"} = "Exo";
$abbrevs{"Lev"} = "Lev";
$abbrevs{"Num"} = "Num";
$abbrevs{"Deu"} = "Deu";
$abbrevs{"Jos"} = "Jos";
$abbrevs{"Jdg"} = "Jdg";
$abbrevs{"Rut"} = "Rut";
$abbrevs{"1Sa"} = "1Sa";
$abbrevs{"2Sa"} = "2Sa";
$abbrevs{"1Ki"} = "1Ki";
$abbrevs{"2Ki"} = "2Ki";
$abbrevs{"1Ch"} = "1Ch";
$abbrevs{"2Ch"} = "2Ch";
$abbrevs{"Ezr"} = "Ezr";
$abbrevs{"Neh"} = "Neh";
$abbrevs{"Est"} = "Est";
$abbrevs{"Job"} = "Job";
$abbrevs{"Psa"} = "Psa";
$abbrevs{"Pro"} = "Pro";
$abbrevs{"Ecc"} = "Ecc";
$abbrevs{"Sol"} = "Sol";
$abbrevs{"Isa"} = "Isa";
$abbrevs{"Jer"} = "Jer";
$abbrevs{"Lam"} = "Lam";
$abbrevs{"Eze"} = "Eze";
$abbrevs{"Dan"} = "Dan";
$abbrevs{"Hos"} = "Hos";
$abbrevs{"Joe"} = "Joe";
$abbrevs{"Amo"} = "Amo";
$abbrevs{"Oba"} = "Oba";
$abbrevs{"Jon"} = "Jon";
$abbrevs{"Mic"} = "Mic";
$abbrevs{"Nah"} = "Nah";
$abbrevs{"Hab"} = "Hab";
$abbrevs{"Zep"} = "Zep";
$abbrevs{"Hag"} = "Hag";
$abbrevs{"Zec"} = "Zec";
$abbrevs{"Mal"} = "Mal";
$abbrevs{"THE BOOK OF MATTHEW"} = "Mat";
$abbrevs{"THE GOSPEL OF MARK"} = "Mar";
$abbrevs{"THE GOSPEL OF LUKE"} = "Luk";
$abbrevs{"THE GOSPEL OF JOHN"} = "Joh";
$abbrevs{"THE ACTS OF THE APOSTLES"} = "Act";
$abbrevs{"THE BOOK OF ROMANS"} = "Rom";
$abbrevs{"FIRST CORINTHIANS"} = "1Co";
$abbrevs{"SECOND CORINTHIANS"} = "2Co";
$abbrevs{"THE EPISTLE OF PAUL TO THE GALATIANS"} = "Gal";
$abbrevs{"THE EPISTLE OF PAUL TO THE EPHESIANS"} = "Eph";
$abbrevs{"THE EPISTLE OF PAUL TO THE PHILIPPIANS"} = "Phi";
$abbrevs{"THE EPISTLE OF PAUL TO THE COLOSSIANS"} = "Col";
$abbrevs{"THE FIRST EPISTLE OF PAUL TO THE THESSALONIANS"} = "1Th";
$abbrevs{"THE SECOND EPISTLE OF PAUL TO THE THESSALONIANS"} = "2Th";
$abbrevs{"THE FIRST EPISTLE OF PAUL TO TIMOTHY"} = "1Ti";
$abbrevs{"THE SECOND EPISTLE OF PAUL TO TIMOTHY"} = "2Ti";
$abbrevs{"THE EPISTLE OF PAUL TO TITUS"} = "Tit";
$abbrevs{"THE EPISTLE OF PAUL TO PHILEMON"} = "Phm";
$abbrevs{"THE EPISTLE TO THE HEBREWS"} = "Heb";
$abbrevs{"THE GENERAL EPISTLE OF JAMES"} = "Jam";
$abbrevs{"THE FIRST GENERAL EPISTLE OF PETER"} = "1Pe";
$abbrevs{"THE SECOND GENERAL EPISTLE OF PETER"} = "2Pe";
$abbrevs{"THE FIRST EPISTLE OF JOHN"} = "1Jo";
$abbrevs{"THE SECOND EPISTLE OF JOHN"} = "2Jo";
$abbrevs{"THE THIRD EPISTLE OF JOHN"} = "3Jo";
$abbrevs{"THE GENERAL EPISTLE OF JUDE"} = "Jud";
$abbrevs{"THE REVELATION OF JESUS CHRIST"} = "Rev";

my $infile = shift(@ARGV);
my $outfile = shift(@ARGV);
my $outlinefile = shift(@ARGV);

open(my $inf, "<", $infile) or die "Can't open $infile: $!";
open(my $outf, ">", $outfile) or die "Can't open $outfile: $!";
open(my $outlinef, ">", $outlinefile) or die "Can't open $outlinefile: $!";
$needNewLine = 0;

while ($ln = <$inf>) {
    $originalLine = $ln;
    chomp $ln;
    $ln =~ s/^\s+//; # left trim
    $ln =~ s/\s+$//; # right trim
    # First step: categorize all the lines
    if ($ln =~ /^$/) {
	next; # blank link; skip
    }
    elsif (exists $abbrevs{$ln}) {
	    # New Bible book
	    $book = $abbrevs{$ln};
	    $ch = 1;
	    $vs = 0;
	    #print stderr "***$book $ch*2*\n";
    }
    elsif ($ln =~ / CHAPTER ([0-9]+)/) {
	# New Chapter
	$ch = $1;
	#print stderr "***$book $ch*1*\n";
	$vs = 0;
    }
    elsif ($ln =~ /^([0-9]+)\s+(.*)$/) {
	# A verse, but there are some notes like this too. Actually,
	# there were only two notes. See next comment.
	$newvs = $1;
	if ($newvs < $vs) {
	    print stderr "Verse number that is out of sequence: $ln\n";
	    # This helped me to verify that there were only two footnotes in
	    # the entire EMTV. Both appeared at the end of the text file.
	    # One referenced Romans 3:10, the other 1 Peter 4:18. I removed
	    # them and their footnote "callers."
	}
	$vstxt = $2;
	$vs = $newvs;
	if ($needNewLine == 1) { print $outf "\n"; $needNewLine = 0; }
	print $outf "$book $ch:$vs $vstxt";
	$needNewLine = 1;
    }
    elsif ($ln =~ /^To Him/) {
	# A verse portion, like the second line of Revelation 1:5
	# Append to the prior verse
	$vstxt = $ln;
	print $outf " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^To the elect lady/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $outf " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^To those who are called/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $outf " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^To the beloved Gaius/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $outf " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^They are bold,/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $outf " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^See how great a forest/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $outf " $vstxt\n";
	$needNewLine = 0;
    }
    else {
	# I believe this is part of the EMTV Bible outline
	$outline = $ln;
	$nextvs = $vs+1;
	print $outlinef "$book $ch:$nextvs $outline\n";
    }
}
