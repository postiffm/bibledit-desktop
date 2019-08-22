#!/usr/bin/perl
use strict;
use warnings;

#require "./bibletools.pl";

# The idea of this script is to take a list of Bible references
# and encode it into an 11-digit format. The format is:

# 0 - <bk code 2 digit 1-66> - <ch code 3 digit code> - <vs code 3 digit code> - 00

# Example: Genesis

my %books;
my %abbrevs;

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
$books{"Psalm"} = 19;
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

my $ln;

while ($ln = <>) {
    # Assuming reading one ref per line
    my @parts = split(/[\s:]/, $ln);
    #print $parts[0], "**",  $parts[1], "**", $parts[2], "**\n";
    my $bk = $parts[0];
    my $ch = $parts[1];
    my $vs = $parts[2];
    my $bknum = $books{$bk};
    printf("0%02d%03d%03d00\n", $bknum, $ch, $vs);

    # Parse

    # Write encoded output
    
}
