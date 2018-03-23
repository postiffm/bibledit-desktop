#!/usr/bin/perl

# Read Cross Reference Database and print

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

open(my $fh, '<:raw', 'bi.crf') or die "Unable to open: $!";
$startNewList = 1;

while (1) {
    my $bytes_read = read $fh, my $bytes, 4;
    if ($bytes_read != 4) { last; } # There should always be a multiple of 4 bytes
    my $enc = unpack("L<", $bytes);
    if ($enc == 0) { print "\n"; $startNewList = 1; next; } # end of prior x-ref list
    my $vs2 = $enc & 0xff;
    my $vs = ($enc >> 8) & 0xff;
    my $ch = ($enc >> 16) & 0xff;
    my $bk = ($enc >> 24) & 0xff;
    print "$books{$bk} $ch:$vs";
    if ($vs2 != 0) { print "-$vs2 "; }
    else { print " "; }
    if ($startNewList == 1) {
	print "==> ";
	$startNewList = 0;
    }
}
#$enc = 0x18041700;
#printf("%08X\n", $enc);
#my $test1 = ($enc >> 24) & 0xff;
#printf("%08X\n", $test1);
#my $test2 = ($enc & 0xff000000) >> 24;
#printf("%08X\n", $test2);
