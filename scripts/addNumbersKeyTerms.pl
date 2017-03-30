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

# Take the rev 1 BI_NT_Keyterms.txt and assign a unique number to each keyterm.
# Was:
# [keyterm]
# 3956*   ALL, ALL (KINDS OF), ALL (THINGS)
# Should be:
# [keyterm]
# BI10000 3956*   ALL, ALL (KINDS OF), ALL (THINGS)
# I start with BI10000 because the OT starts with BI00000

$num = 10000;
$sawKeyTermKeyword = 0;

while ($line = <>) {
    #$line =~ s/\r//g; # remove \r (^M, DOS CR)
    
	if ($line =~ /^\[keyterm\]/) {
	    $sawKeyTermKeyword = 1;
		print $line;  # reproduce this line on the output
	}
    elsif ($line =~ /^[0-9]{4}/) {
		if ($sawKeyTermKeyword == 1) {
			printf("BI%5d ", $num);
			print $line;
			$num++;
			$sawKeyTermKeyword = 0;
			}
		elsif ($sawKeyTermKeyword == 0) {
			print $line;  # reproduce this line on the output
			#print "\n\n\n***Something funny about the above line so I am not treating this one\n\n\n";
			# This case is when you have a "reference" that looks like this:
			# 4190, pp. 36,37
			# or like this:
			# 4567 "Satan" be1ow.
			# which are the only two cases in teh BI_NT_Keyterms_rev2.txt file that this is a problem.
		}
    }
	else {
		$sawKeyTermKeyword = 0;
		print $line;  # reproduce this line on the output
	}
}
