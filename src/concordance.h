/*
 ** Copyright (©) 2018- Matt Postiff.
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

#include <glibmm/ustring.h>
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
#include "options.h"
#include "htmlwriter2.h"

using namespace std;
using namespace Glib;

// Forward declarations
class bible;
class chapter;
class verse;

class book {
  public:
	bible *bbl;  // back pointer to the containing bible
	ustring bookname;
    int booknum;
	vector<chapter> chapters;
  public:
	book(bible *_bbl, const ustring &_bookname, int _booknum);
};

class chapter {
  public:
	book *bk;    // back pointer to the containing book
	int chapnum;     // the chapter number
	vector<verse> verses;
	// project, book, chapter (57 = Philemon, 1 = chapter 1)
  public:
    chapter(book *_bk, int _num);
	void load(int book, int chapter,
              std::unordered_map<std::string, int, std::hash<std::string>> &wordCounts,      std::unordered_map<std::string, std::vector<int>, std::hash<std::string>> &wordLocations);
};

class verse {
	chapter *ch; // back pointer to the containing chapter
	int vsnum;
	ustring text;
  public:
	verse(chapter *_ch, int _vsnum, ustring _txt);
    void print(void);
	void addToWordCount(std::unordered_map<std::string, int, std::hash<std::string>> &wordCounts,
        std::unordered_map<std::string, std::vector<int>, std::hash<std::string>> &wordLocations);
};

class bible {
 public:
  ustring projname;
  vector<book> books; 
  bible(const ustring &_proj);
};

class concordance {
 public:
  concordance(const ustring &_projname, HtmlWriter2 &htmlwriter);
  void sortedByWords(HtmlWriter2 &htmlwriter);
  void readExcludedWords(const ustring &filename);
private:
  ustring projname;
  // The set of words that we are NOT interested in showing
  unordered_set<string> excludedWords;
  // 1. Unique word ==> count of occurrences
  std::unordered_map<std::string, int, std::hash<std::string>> wordCounts;
  // 2. Unique word ==> vector of integers. These integers are coded to contain the
  // book, chapter, and verse location in 24 bits.
  std::unordered_map<std::string, std::vector<int>, std::hash<std::string>> wordLocations;
  // 3. Unique word ==> count of occurrences. Same info as #1, but sorted automatically.
  std::map<std::string, int> sortedWordCounts;
};
