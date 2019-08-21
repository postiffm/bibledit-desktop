#!/usr/bin/perl
use strict;
use warnings;

my $infile = shift(@ARGV);
my $newinfile = $infile . ".orig";

print "Cleaning $infile...\n";

rename($infile, $newinfile);

open(my $inf, "<", $newinfile);
open(my $outf, ">", $infile); # $infile is now an output file!

while (<$inf>) {

    if (/==>\s*$/) { next; }
    else {
	print {$outf} $_;
    }

}
