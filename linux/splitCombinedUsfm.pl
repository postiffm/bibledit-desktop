#!/usr/bin/perl

# Usage: ./splitCombinedUsfm.pl Luxembourgish.combined.usfm
# Output: one usfm file per book of the Bible contained in the combined file

# Precondition - make sure the file is in Unix format.
# So, if the command 'file Luxembourgish.combined.usfm' shows
# UTF-8 Unicode text, with very long lines, with CRLF line terminators
# then do 'dos2unix Luxembourgish.combined.usfm'
# Now, 'file Luxembourgish.combined.usfm' will show
# UTF-8 Unicode text, with very long lines

$book = "";
open(my $out, '>', "extra.txt");

while ($ln = <>) {
    chomp $ln;
    if ($ln =~ /^\\id (.*)$/) {
	$book = $1;
	close $out;
	$newfilename = $book . ".usfm";
	print STDERR "Processing $book...\n";
	open($out, '>', $newfilename);
    }
    print $out $ln . "\n";
}
