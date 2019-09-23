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

#ifndef INCLUDED_BIBLEBOOKCHAPTERVERSE_H
#define INCLUDED_BIBLEBOOKCHAPTERVERSE_H

// This file contains the code to store an entire Bible in a
// hierarchical form:
// bible
//   |
//   +--- vector of book *
//                   |
//                   +--- vector of chapter *
//                                    |
//                                    +--- vector of verse *
//                                                    |
//                                                    +--- int vsnum and ustring text
//
// verse_xref is a bit different. Instead of a ustring text, it contains a vector of integer
// data which are specially-encoded cross-references

#include "ustring.h"
#include <glibmm/iochannel.h>
#include <vector>
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

// Forward declarations
class bible;
class chapter;
class verse;

class book {
  public:
	bible *bbl;  // back pointer to the containing bible // book does NOT own bible for garbage collection purposes
	ustring bookname;
    unsigned int booknum;
	vector<chapter *> chapters;
  public:
	book(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
    book();
    virtual ~book();
    void check_chapter_in_range(unsigned int chapnum);
    ustring retrieve_verse(const Reference &ref);
    void byzasciiConvert(ustring &vs);
    virtual void load(void);
};

class book_byz : public book {
public:
  book_byz(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
  void load(void);
private:
  void byzasciiConvert(ustring &vs);
};

class book_sblgnt : public book {
public:
  book_sblgnt(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
  void load(void);
};

class book_sblgntapp : public book {
public:
  book_sblgntapp(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
  void load(void);
};

class book_engmtv : public book {
public:
  book_engmtv(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
  void load(void);
};

class book_leb : public book {
public:
  book_leb(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
  void load(void);
};

class book_netbible : public book {
public:
  book_netbible(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
  void load(void);
};

class book_bixref : public book {
public:
    book_bixref(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
};

class chapter {
  public:
	book *bk;    // back pointer to the containing book; chapter does NOT own book for garbage collection purposes
	int chapnum;     // the chapter number
	map<unsigned int,verse *> verses;
	// project, book, chapter (57 = Philemon, 1 = chapter 1)
  public:
    chapter(book *_bk, int _num);
    ~chapter();
    bool find_verse( uint32_t verseNum );
    bool map_verse( uint32_t verseNum, verse *newVerse );
    void check_verse_in_range(unsigned int vsnum);
	void load(int book, int chapter,
              std::unordered_map<std::string, int, std::hash<std::string>> &wordCounts,      std::unordered_map<std::string, std::vector<int>, std::hash<std::string>> &wordLocations);
    void appendToLastVerse(const ustring &addlText);
    void prependToLastVerse(const ustring &addlText);
    ustring retrieve_verse(const Reference &ref);
};

class verse {
	chapter *ch; // back pointer to the containing chapter
	int vsnum;
	ustring text;
  public:
	verse(chapter *_ch, int _vsnum, ustring _txt);
    virtual ~verse();
    void print(void);
	void addToWordCount(std::unordered_map<std::string, int, std::hash<std::string>> &wordCounts,
        std::unordered_map<std::string, std::vector<int>, std::hash<std::string>> &wordLocations);
    void append(const ustring &addlText);
    void prepend(const ustring &addlText);
    ustring retrieve_verse(const Reference &ref);
};

class verse_xref : public verse {
public:
    verse_xref(chapter *_ch, int _vsnum, ustring _txt);
    ~verse_xref();
    vector <unsigned int> xrefs;
};

class bible {
 public:
  ustring projname;
  vector<book *> books;
  bible(const ustring &_proj, const ustring &_font);
  virtual ~bible();
  void clear(void);
  virtual ustring retrieve_verse(const Reference &ref);
  void font_set(const ustring &_font);
  ustring font_get(void);
  virtual bool validateBookNum(const unsigned int booknum);

protected: // so derived bibles can access it
  void check_book_in_range(unsigned int booknum);
  virtual book *createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
  ustring font;
};

class bible_byz : public bible {
public:
    bible_byz(const ustring &_proj, const ustring &_font);
    bool validateBookNum(const unsigned int booknum);
    book *createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
};

class bible_sblgnt : public bible {
public:
    bible_sblgnt(const ustring &_proj, const ustring &_font);
    bool validateBookNum(const unsigned int booknum);
    book *createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
};

class bible_sblgntapp : public bible {
public:
    bible_sblgntapp(const ustring &_proj, const ustring &_font);
    bool validateBookNum(const unsigned int booknum);
    book *createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
};

class bible_engmtv : public bible {
public:
    bible_engmtv(const ustring &_proj, const ustring &_font);
    bool validateBookNum(const unsigned int booknum);
    book *createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
};

class bible_leb : public bible {
public:
    bible_leb(const ustring &_proj, const ustring &_font);
    bool validateBookNum(const unsigned int booknum);
    book *createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
};

class bible_netbible : public bible {
public:
    bible_netbible(const ustring &_proj, const ustring &_font);
    bool validateBookNum(const unsigned int booknum);
    book *createNewBook(bible *_bbl, const ustring &_bookname, unsigned int _booknum);
};

// To store Bibles International cross-references
// At some point I may make this generic by removing "bi" prefix
class bible_bixref : public bible {
public:
    bible_bixref(const ustring &_proj);
    ~bible_bixref();
    vector<unsigned int> *retrieve_xrefs(const Reference &ref);
};

#endif
