#!/usr/bin/perl

# This takes input from USFM and extracts the refs and finds all the
# ways that books are referred to. This helps us to find errors in
# \toc3 entries so that Scripture App Builder can know the correct
# references. An unintended side-effect is that it also helps find
# inconsistent ways that books are referenced (1 Sam versus I Sam,
# for example), and wrong spacings. 

# Do a grep first to extract just the lines we want, and run
# like this:
# grep '\\r' *.usfm | ~/bibledit-desktop/linux/distillRefs.pl
# The grep allows us to be a little sloppy below, not checking for
# blank lines or lines with no interesting USFM like \r or \rq...\rq*

# First use for Rito NT. 
# Then adapted for Quechua which had a slightly different reference
# style (periods after book names, and some other peculiarities.

while ($ln = <>) {
    # Strip leading filename (if using grep, it is like 40_Matthew.usfm)
    $ln =~ s/^.+\.usfm://;
    #print $ln;
    # Strip leading \r, leading and trailing \rq...\rq*, parentheses
    $ln =~ s/.*\\r //;
    $ln =~ s/.*\\rq //;
    $ln =~ s/\\rq.*$//;
    $ln =~ s/\(//g;
    $ln =~ s/\)//g;
    $ln =~ s/\n$//g;
    $ln =~ s/\r$//g;
    @verses = split('[,;0123456789] ', $ln);
    foreach $verse (@verses) {
	#print "  ==>", $verse;
	# The verses are in the format Tekikaga 20:13 (Rito NT)
	# So I need to extract the book name, and leave the rest.
	$verse =~ /(.+)\s+[0-9]/;
	# This will carry over to future loop iterations for like Jn. 1:1-2, 14
	$book = $1;
	#print "  ==>", $book, "\n";
	$books{$book}++; # record

	if ($book =~ /^\s+/) {
	    print $ln, " has a book with a space at the beginning of the name\n";
	}

	if ($book eq "") {
	    print $ln, " has an empty book\n";
	    print "Previous line: ", $lastln, "\n\n";
	}
    }
    $lastln = $ln;
    #print "\n";
}

printf("%-15s %s\n", "Book", "Occurrences");
printf("%-15s %s\n", "--------------", "-----------");
foreach $book (keys %books) {
    printf("%-15s %d\n", $book, $books{$book});
}
