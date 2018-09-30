#!/usr/bin/perl

# (C) Matt Postiff, 2018
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

# Take the given Keyterm file and fix the key term numbers. For example,
# BI1000
# BI1001
# BI1001a
# ...
# should be
# BI1000
# BI1001
# BI1002
# ...
# The 'a' numbers are replaced.
# 
# Example usage:
# fixKeyTermNumbering.pl BI_NT_Keyterms_rev5.txt

my $filenm = shift(@ARGV);

# Lowercase all the other argv's
foreach my $arg (@ARGV) {
	$arg = lc $arg;
	#print "Looking for book $arg\n";
	push (@books, $arg);
}

$numKeyTermsDeleted = 0;
$numKeyTermsKept = 0;

open(my $fh, $filenm)
  or die "Could not open file '$filenm' $!";
  
$line = <$fh>; # slurp up the first line (BI TN Keyterms... title)
print $line; # and reproduce it on the output

$ktNumber = 9999; # because the first will be 10000

while ($line = <$fh>) {
    if ($line =~ /^\[keyterm\]/) {
	$ktNumber++;
	print $line;
    }
    elsif ($line =~ /^\s*BI[0-9]+\s/) {
	$line =~ s/^BI[0-9]+\s/BI$ktNumber /;
	print $line;
    }
    else {
	$line =~ s/^\s+(.+)/$1/;  # Remove leading spaces from lines that have stuff
	# Trying not to remove blank lines.
	print $line;
    }
}
