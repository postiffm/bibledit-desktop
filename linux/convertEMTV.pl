#!/usr/bin/perl

# (C) Matt Postiff, 2018

# Convert EMTV from Paul Esposito to Bibleworks (and Bibledit friendly)
# format(s). The text came to me in Word. I exported the NT, Psalms, and
# Proverbs in separate text files, \n delimited. Now I am trying to
# clean them up and format them for Bibleworks. This works well.
#
# Added feature to extract the NT outline and verse addresses for the headings.
# Added feature to write the USFM formatted NT file.
#
# Note that this script is only designed to work on the NT documents.
# The formatting of the Psalms and Proverbs is tougher to handle.

# Usage: ./convertEMTV.pl NewTestament.txt NewTestamentBibleworks.txt NewTestamentOutline.txt emtv.usfm

# Abbreviations for book names that are acceptable to Bibleworks (EMTV => Bibleworks)
$bwabbrevs{"Gen"} = "Gen";
$bwabbrevs{"Exo"} = "Exo";
$bwabbrevs{"Lev"} = "Lev";
$bwabbrevs{"Num"} = "Num";
$bwabbrevs{"Deu"} = "Deu";
$bwabbrevs{"Jos"} = "Jos";
$bwabbrevs{"Jdg"} = "Jdg";
$bwabbrevs{"Rut"} = "Rut";
$bwabbrevs{"1Sa"} = "1Sa";
$bwabbrevs{"2Sa"} = "2Sa";
$bwabbrevs{"1Ki"} = "1Ki";
$bwabbrevs{"2Ki"} = "2Ki";
$bwabbrevs{"1Ch"} = "1Ch";
$bwabbrevs{"2Ch"} = "2Ch";
$bwabbrevs{"Ezr"} = "Ezr";
$bwabbrevs{"Neh"} = "Neh";
$bwabbrevs{"Est"} = "Est";
$bwabbrevs{"Job"} = "Job";
$bwabbrevs{"Psa"} = "Psa";
$bwabbrevs{"Pro"} = "Pro";
$bwabbrevs{"Ecc"} = "Ecc";
$bwabbrevs{"Sol"} = "Sol";
$bwabbrevs{"Isa"} = "Isa";
$bwabbrevs{"Jer"} = "Jer";
$bwabbrevs{"Lam"} = "Lam";
$bwabbrevs{"Eze"} = "Eze";
$bwabbrevs{"Dan"} = "Dan";
$bwabbrevs{"Hos"} = "Hos";
$bwabbrevs{"Joe"} = "Joe";
$bwabbrevs{"Amo"} = "Amo";
$bwabbrevs{"Oba"} = "Oba";
$bwabbrevs{"Jon"} = "Jon";
$bwabbrevs{"Mic"} = "Mic";
$bwabbrevs{"Nah"} = "Nah";
$bwabbrevs{"Hab"} = "Hab";
$bwabbrevs{"Zep"} = "Zep";
$bwabbrevs{"Hag"} = "Hag";
$bwabbrevs{"Zec"} = "Zec";
$bwabbrevs{"Mal"} = "Mal";
$bwabbrevs{"THE BOOK OF MATTHEW"} = "Mat";
$bwabbrevs{"THE GOSPEL OF MARK"} = "Mar";
$bwabbrevs{"THE GOSPEL OF LUKE"} = "Luk";
$bwabbrevs{"THE GOSPEL OF JOHN"} = "Joh";
$bwabbrevs{"THE ACTS OF THE APOSTLES"} = "Act";
$bwabbrevs{"THE BOOK OF ROMANS"} = "Rom";
$bwabbrevs{"FIRST CORINTHIANS"} = "1Co";
$bwabbrevs{"SECOND CORINTHIANS"} = "2Co";
$bwabbrevs{"THE EPISTLE OF PAUL TO THE GALATIANS"} = "Gal";
$bwabbrevs{"THE EPISTLE OF PAUL TO THE EPHESIANS"} = "Eph";
$bwabbrevs{"THE EPISTLE OF PAUL TO THE PHILIPPIANS"} = "Phi";
$bwabbrevs{"THE EPISTLE OF PAUL TO THE COLOSSIANS"} = "Col";
$bwabbrevs{"THE FIRST EPISTLE OF PAUL TO THE THESSALONIANS"} = "1Th";
$bwabbrevs{"THE SECOND EPISTLE OF PAUL TO THE THESSALONIANS"} = "2Th";
$bwabbrevs{"THE FIRST EPISTLE OF PAUL TO TIMOTHY"} = "1Ti";
$bwabbrevs{"THE SECOND EPISTLE OF PAUL TO TIMOTHY"} = "2Ti";
$bwabbrevs{"THE EPISTLE OF PAUL TO TITUS"} = "Tit";
$bwabbrevs{"THE EPISTLE OF PAUL TO PHILEMON"} = "Phm";
$bwabbrevs{"THE EPISTLE TO THE HEBREWS"} = "Heb";
$bwabbrevs{"THE GENERAL EPISTLE OF JAMES"} = "Jam";
$bwabbrevs{"THE FIRST GENERAL EPISTLE OF PETER"} = "1Pe";
$bwabbrevs{"THE SECOND GENERAL EPISTLE OF PETER"} = "2Pe";
$bwabbrevs{"THE FIRST EPISTLE OF JOHN"} = "1Jo";
$bwabbrevs{"THE SECOND EPISTLE OF JOHN"} = "2Jo";
$bwabbrevs{"THE THIRD EPISTLE OF JOHN"} = "3Jo";
$bwabbrevs{"THE GENERAL EPISTLE OF JUDE"} = "Jud";
$bwabbrevs{"THE REVELATION OF JESUS CHRIST"} = "Rev";

# Abbreviations acceptable to USFM (Bibleworks => USFM)
$usfmabbrevs{"Gen"} = "GEN";
$usfmabbrevs{"Exo"} = "EXO";
$usfmabbrevs{"Lev"} = "LEV";
$usfmabbrevs{"Num"} = "NUM";
$usfmabbrevs{"Deu"} = "DEU";
$usfmabbrevs{"Jos"} = "JOS";
$usfmabbrevs{"Jdg"} = "JDG";
$usfmabbrevs{"Rut"} = "RUT";
$usfmabbrevs{"1Sa"} = "1SA";
$usfmabbrevs{"2Sa"} = "2SA";
$usfmabbrevs{"1Ki"} = "1KI";
$usfmabbrevs{"2Ki"} = "2KI";
$usfmabbrevs{"1Ch"} = "1CH";
$usfmabbrevs{"2Ch"} = "2CH";
$usfmabbrevs{"Ezr"} = "EZR";
$usfmabbrevs{"Neh"} = "NEH";
$usfmabbrevs{"Est"} = "EST";
$usfmabbrevs{"Job"} = "JOB";
$usfmabbrevs{"Psa"} = "PSA";
$usfmabbrevs{"Pro"} = "PRO";
$usfmabbrevs{"Ecc"} = "ECC";
$usfmabbrevs{"Sol"} = "SNG";
$usfmabbrevs{"Isa"} = "ISA";
$usfmabbrevs{"Jer"} = "JER";
$usfmabbrevs{"Lam"} = "LAM";
$usfmabbrevs{"Eze"} = "EZK";
$usfmabbrevs{"Dan"} = "DAN";
$usfmabbrevs{"Hos"} = "HOS";
$usfmabbrevs{"Joe"} = "JOL";
$usfmabbrevs{"Amo"} = "AMO";
$usfmabbrevs{"Oba"} = "OBA";
$usfmabbrevs{"Jon"} = "JON";
$usfmabbrevs{"Mic"} = "MIC";
$usfmabbrevs{"Nah"} = "NAM";
$usfmabbrevs{"Hab"} = "HAB";
$usfmabbrevs{"Zep"} = "ZEP";
$usfmabbrevs{"Hag"} = "HAG";
$usfmabbrevs{"Zec"} = "ZEC";
$usfmabbrevs{"Mal"} = "MAL";
$usfmabbrevs{"Mat"} = "MAT";
$usfmabbrevs{"Mar"} = "MRK";
$usfmabbrevs{"Luk"} = "LUK";
$usfmabbrevs{"Joh"} = "JHN";
$usfmabbrevs{"Act"} = "ACT";
$usfmabbrevs{"Rom"} = "ROM";
$usfmabbrevs{"1Co"} = "1CO";
$usfmabbrevs{"2Co"} = "2CO";
$usfmabbrevs{"Gal"} = "GAL";
$usfmabbrevs{"Eph"} = "EPH";
$usfmabbrevs{"Phi"} = "PHP";
$usfmabbrevs{"Col"} = "COL";
$usfmabbrevs{"1Th"} = "1TH";
$usfmabbrevs{"2Th"} = "2TH";
$usfmabbrevs{"1Ti"} = "1TI";
$usfmabbrevs{"2Ti"} = "2TI";
$usfmabbrevs{"Tit"} = "TIT";
$usfmabbrevs{"Phm"} = "PHM";
$usfmabbrevs{"Heb"} = "HEB";
$usfmabbrevs{"Jam"} = "JAS";
$usfmabbrevs{"1Pe"} = "1PE";
$usfmabbrevs{"2Pe"} = "2PE";
$usfmabbrevs{"1Jo"} = "1JN";
$usfmabbrevs{"2Jo"} = "2JN";
$usfmabbrevs{"3Jo"} = "3JN";
$usfmabbrevs{"Jud"} = "JUD";
$usfmabbrevs{"Rev"} = "REV";

my $infile = shift(@ARGV);
my $bibleworksfile = shift(@ARGV);
my $outlinefile = shift(@ARGV);
my $usfmfile = shift(@ARGV);

open(my $inf, "<", $infile) or die "Can't open $infile: $!";
open(my $bibleworks, ">", $bibleworksfile) or die "Can't open $bibleworksfile: $!";
open(my $outlinef, ">", $outlinefile) or die "Can't open $outlinefile: $!";
open(my $usfm, ">", $usfmfile) or die "Can't open $usfmfile: $!";
binmode($usfm, ":utf8"); # Bibledit import requires Unicode usfm

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
    elsif (exists $bwabbrevs{$ln}) {
	    # New Bible book
	    $book = $bwabbrevs{$ln};
	    $ch = 1;
	    $vs = 0;
	    if ($needNewLine == 1) { print $bibleworks "\n"; print $usfm "\n"; $needNewLine = 0; }
	    print $usfm "\\id ", $usfmabbrevs{$book}, "\n";
	    print $usfm "\\mt1 ", $ln, "\n";
	    # In the case of Paul Esposito's text, he does not label
	    # chapter 1, so I have to add that in myself at the start
	    # of every new chapter.
	    $ch = 1;
	    print $usfm "\\c $ch\n";
	    print $usfm "\\p\n";
	    # In the case of ACTS, and ONLY Acts, Mr. Esposito did not
	    # open the book with a section heading. This meant that \p
	    # was not output after a \s1, changing the font context to
	    # a paragraph font (?). Anyway, this also happened after some
	    # "internal" chapters, so I just put a \p after every \c
    }
    elsif ($ln =~ / CHAPTER ([0-9]+)/) {
	# New Chapter
	$ch = $1;
	if ($needNewLine == 1) { print $bibleworks "\n"; print $usfm "\n"; $needNewLine = 0; }
	print $usfm "\\c $ch\n";
	print $usfm "\\p\n";
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
	if ($needNewLine == 1) { print $bibleworks "\n"; print $usfm "\n"; $needNewLine = 0; }
	print $bibleworks "$book $ch:$vs $vstxt";
	$vstxt = fixUnicode($vstxt);
	print $usfm "\\v $vs $vstxt";
	$needNewLine = 1;
    }
    #
    # Following are all special cases of verses that were split across
    # lines in the EMTV text file (exported from Word).
    #
    elsif ($ln =~ /^To Him/) {
	# A verse portion, like the second line of Revelation 1:5
	# Append to the prior verse
	$vstxt = $ln;
	print $bibleworks " $vstxt\n";
	$vstxt = fixUnicode($vstxt);
	print $usfm " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^To the elect lady/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $bibleworks " $vstxt\n";
	$vstxt = fixUnicode($vstxt);
	print $usfm " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^To those who are called/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $bibleworks " $vstxt\n";
	$vstxt = fixUnicode($vstxt);
	print $usfm " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^To the beloved Gaius/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $bibleworks " $vstxt\n";
	$vstxt = fixUnicode($vstxt);
	print $usfm " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^They are bold,/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $bibleworks " $vstxt\n";
	$vstxt = fixUnicode($vstxt);
	print $usfm " $vstxt\n";
	$needNewLine = 0;
    }
    elsif ($ln =~ /^See how great a forest/) {
	# Append to the prior verse
	$vstxt = $ln;
	print $bibleworks " $vstxt\n";
	$vstxt = fixUnicode($vstxt);
	print $usfm " $vstxt\n";
	$needNewLine = 0;
    }
    else {
	# I believe this is part of the EMTV Bible outline
	$outline = $ln;
	$nextvs = $vs+1;
	print $outlinef "$book $ch:$nextvs $outline\n";
	if ($needNewLine == 1) { print $bibleworks "\n"; print $usfm "\n"; $needNewLine = 0; }
	$outline = fixUnicode($outline);
	print $usfm "\\s1 $outline\n";
	print $usfm "\\p\n";
    }
}

sub fixUnicode {
    my $vstxt = shift(@_);

    $vstxt =~ s/\N{U+0091}/\N{U+2018}/g;
    $vstxt =~ s/\N{U+0092}/\N{U+2019}/g;
    $vstxt =~ s/\N{U+0093}/\N{U+201C}/g;
    $vstxt =~ s/\N{U+0094}/\N{U+201D}/g;
    $vstxt =~ s/\N{U+0097}/\N{U+2014}/g;

    #if ($vstxt =~ /([\N{U+0080}-\N{U+0098}])/) { # (\P{ASCII})???
    #    $specialChar = $1;
    #    printf stderr "Special character %08X\n", ord($specialChar);
    #}
    
    return $vstxt;     
}
