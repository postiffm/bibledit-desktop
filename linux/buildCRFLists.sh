#!/bin/sh

./createLimitedCRF.pl ../BiblesInternational/bi.crf
./cleanEmptyCRFs.pl crfNTOnly.txt
./cleanEmptyCRFs.pl crfNTPs.txt
./cleanEmptyCRFs.pl crfNTPsPro.txt
