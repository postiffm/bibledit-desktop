#!/bin/bash

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

# Call scripts/reduceKeyTerms.pl repeatedly to build various keyterm lists

# Location of script
SCR=./reduceKeyTerms.pl
# Key Term file
KTOT=BI_OT_Keyterms.txt
KTNT=BI_NT_Keyterms_rev3.txt

$SCR $KTOT psa pro > BI_OT_Keyterms_PsalmsProverbs.txt 2>>makeKeyTermSets.log
$SCR $KTOT gen exo lev num deu > BI_OT_Keyterms_Pentateuch.txt 2>>makeKeyTermSets.log
$SCR $KTOT jos jud rth 1sa 2sa 1ki 2ki 1ch 2ch ezr neh est > BI_OT_Keyterms_Historical.txt 2>>makeKeyTermSets.log
$SCR $KTOT job psa pro ecc sol > BI_OT_Keyterms_Poetry.txt 2>>makeKeyTermSets.log
$SCR $KTOT isa jer lam eze dan > BI_OT_Keyterms_MajorProphets.txt 2>>makeKeyTermSets.log
$SCR $KTOT hos joe amo oba jon mic nah hab zep hag zec mal > BI_OT_Keyterms_MinorProphets.txt 2>>makeKeyTermSets.log

$SCR $KTNT mat mar luk joh > BI_NT_Keyterms_Gospels.txt 2>>makeKeyTermSets.log
$SCR $KTNT act > BI_NT_Keyterms_Acts.txt 2>>makeKeyTermSets.log
$SCR $KTNT rom 1co 2co gal eph phi col 1th 2th 1ti 2ti tit phm heb > BI_NT_Keyterms_Pauline.txt 2>>makeKeyTermSets.log
$SCR $KTNT jas 1pe 2pe 1jo 2jo 3jo jud > BI_NT_Keyterms_GeneralLetters.txt 2>>makeKeyTermSets.log
$SCR $KTNT rev > BI_NT_Keyterms_Revelation.txt 2>>makeKeyTermSets.log
