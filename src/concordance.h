/*
 ** Copyright (©) 2018-2024 Matt Postiff.
 **  
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **  
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **  
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **  
 */

#ifndef INCLUDED_CONCORDANCE_H
#define INCLUDED_CONCORDANCE_H

#include "ustring.h"
#include <glibmm/iochannel.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include "directories.h"
#include "settings.h"
#include "books.h"
#include "localizedbooks.h"
#include "versifications.h"
#include "versification.h"
#include "mappings.h"
#include "styles.h"
#include "urltransport.h"
#include "vcs.h"
#include "projectutils.h"
#include "usfmtools.h"
#include "usfm-inline-markers.h"
#include "bookdata.h"
#include "htmlwriter2.h"
#include "utilities.h"
#include "biblebookchapterverse.h"

using namespace std;
using namespace Glib;

class Concordance {
 public:
  Concordance(const ustring &_projname);
  ~Concordance();
  void clear(void);
  void writeAlphabeticSortedHtml(HtmlWriter2 &htmlwriter);
  void writeFrequencySortedHtml(HtmlWriter2 &htmlwriter);
  void writeSingleWordListHtml(const ustring &word, HtmlWriter2 &htmlwriter);
  void readExcludedWords(const ustring &filename);
private:
  ustring projname;
  bible bbl;
  // The set of words that we are NOT interested in showing
  unordered_set<string> excludedWords;
  // 1. Unique word ==> count of occurrences
  std::unordered_map<std::string, int, std::hash<std::string>> wordCounts;
  // 2. Unique word ==> vector of integers. These integers are coded to contain the
  // book, chapter, and verse location in 24 bits.
  std::unordered_map<std::string, std::vector<int>, std::hash<std::string>> wordLocations;
  // 3. Unique word ==> count of occurrences. Same info as #1, but sorted automatically.
  std::map<std::string, int> sortedWordCounts;
  void writeVerseLinks(unsigned int num, vector<int> &locations, HtmlWriter2 &htmlwriter);
  void boldface(ustring src, const ustring &boldstr, HtmlWriter2 &htmlwriter);
  void writeVerses(vector<int> &locations, const ustring &word, HtmlWriter2 &htmlwriter);
};

#endif
