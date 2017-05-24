#!/usr/bin/perl

# (C) Matt Postiff, 2017
#  
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#  
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# Take the given Keyterm file and an "adjustment file" from
# the command line and apply the adjustments throughout the key
# term file.
# Example usage:
# fixKeyTerms.pl BI_OT_Keyterms_rev3.txt adjOT_Keyterms1.txt
#
# The adjustment file has multiple lines of this format:
# gen 32 +1
# This means that we should take every verse reference we find
# to Genesis 32:x and make it 32:x+1.
# exo 2 -1
# By contrast, we should take Exodus 2:x and make it Exodus 2:x-1.

my $filenm = shift(@ARGV);
my $adjfile = shift(@ARGV);
my $refMode = 0;

# Lowercase all the other argv's
foreach my $arg (@ARGV) {
	$arg = lc $arg;
	#print "Looking for book $arg\n";
	push (@books, $arg);
}

$numKeyTermsDeleted = 0;
$numKeyTermsKept = 0;

sub printKeyTerm {
	my $output1 = $_[0];
	my $output2 = $_[1];
	my $output3 = $_[2];
	my @output4 = @{$_[3]};
	my $arrSize = @output4;
	if ($arrSize == 0) {
		#print "This keyterm has no references: $output2";
		$numKeyTermsDeleted++;
	}
	else {
		$numKeyTermsKept++;
		print $output1;
		print $output2;
		print $output3;
		foreach my $ref (@output4) {
			print $ref;
		}
	}
}

#print "Processing $filenm\n";
open(my $fh, $filenm)
  or die "Could not open file '$filenm' $!";
  
$line = <$fh>; # slurp up the first line (BI OT Keyterms... title)
print $line; # and reproduce it on the output

my $output1, $output2, $output3, @output4;

while ($line = <$fh>) {
	if ($line =~ /^\[keyterm\]/) {
		if ($refMode) {
			# Dump output and pop back to the top level of the parser
			printKeyTerm($output1, $output2, $output3, \@output4);
			$output1 = ""; $output2 = ""; $output3 = ""; @output4 = ();
		}
		$output1 = $line;  # reproduce this line on the output
		$refMode = 0;
	}
	elsif (!$refMode && ($line =~ /^[\s]*BI[0-9]{5}/)) {
	    $output2 = $line; # reproduce this line on output
		#print $output2;
	}
	elsif (!$refMode && ($line =~ /^\[references\]/)) {
		$output3 = $line; # reproduce this line on output
		$refMode = 1; # put us into the second level of the parser
	}
	elsif ($refMode) {
		# This line is a Bible reference.
		# We need to weed out those that are not in the books that the user wants.
		# Read in another reference and store it if it is a good one
		$line =~ /([\w]{3})/; $thisbk = lc $1;
		#print "Found ref for $thisbk in $line";
		foreach my $book (@books) {
			# Both $book and $thisbk have been "lowercased" 
			# with lc to make matching easier.
			if ($book eq $thisbk) {
				# Found a match, don't have to look further in my book list
				push (@output4, $line);
				#print "Found a match in $thisbk\n";
				last;
			}
		}
	}
}

printKeyTerm($output1, $output2, $output3, \@output4);

print STDERR "There were originally a total of ", $numKeyTermsDeleted + $numKeyTermsKept, " key terms.\n";
print STDERR "You requested only references to @books.\n";
print STDERR "This led to removal of $numKeyTermsDeleted key terms.\n";
print STDERR "You have kept $numKeyTermsKept key terms.\n";
