/*
 ** Copyright (©) 2003-2013 Teus Benschop.
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

#include "reference.h"
#include "books.h"
#include "utilities.h"
#include "bible.h"
#include "mapping.h"
#include "tiny_utilities.h"

Reference::Reference()
{
  book = 0;
  chapter = 0;
  verse.clear();
  reftype = blankVerse;
}

Reference::Reference(unsigned int book_in, unsigned int chapter_in, const ustring & verse_in)
{
  book = book_in;
  chapter = chapter_in;
  verse = verse_in;
  if (verse_in.find("-") != ustring::npos) {
    reftype = multiVerse;  
  }
  else if (convert_to_int(verse_in) == 0) {
    reftype = wholeChapter;   
  }
  else { 
    reftype = singleVerse;
  }
}

Reference::Reference(unsigned int book_in, unsigned int chapter_in, unsigned int verse_in)
{
  book = book_in;
  chapter = chapter_in;
  verse = Glib::ustring::format(verse_in);
  reftype = singleVerse;
}

// The following method creates a reference from a bit-encoded
// reference (found in the cross-reference file, or in the Concordance
// internal storage). The layout of the unsigned 32-bit int is:
// +--------+--------+--------+--------+
// |booknum |chapnum | vrsnum | vrsnum2|
// +--------+--------+--------+--------+
// The vrsnum field accommodate range refs like Exo 10:21-23.
// To specify larger ranges, I use a special encoding:
// book|ch|vs|0xff where the 0xff means "dash," meaning a 
// "complex range operator." vs=0 indicates an entire chapter
// is being cross-referenced.
// See linux/buildcrf.pl and readcrf.pl
Reference::Reference (unsigned int encoded)
{
    book = encoded >> 24;
    chapter = (encoded >> 16) & 0xff;
    unsigned int vs1 = (encoded >> 8) & 0xff;
    unsigned int vs2 = encoded & 0xff;
    if (vs2 == 0) { 
      verse = Glib::ustring::format(vs1);
      reftype = singleVerse;
    }
    else if (vs2 == 0xff) {
        // I don't know what to do...it would be easier if I didn't have
        // a ustring for the verse...
        verse = std::to_string(vs1);
        reftype = complexRange;
    }
    else if (vs1 == 0x0) {
      verse = std::to_string(vs1);
      reftype = wholeChapter;
      // It is possible for us to have a cross-reference like 1 Chronicles 24:0-1 Chronicles 26:0,
      // which, for the first verse, is both a complexRange and a wholeChapter. The complexRange
      // takes precedent, and the CrossReferences::write() routine does work OK that way, because
      // 24:0 is a non-verse, so it displays no verse text, and the range is 1 Ch. 24:0.....1 Ch. 26:0
      // which is acceptable for our purposes.
    }
    else {
      verse = std::to_string(vs1) + "-" + std::to_string(vs2);
      reftype = multiVerse;
    }
}

ustring Reference::human_readable(const ustring & language) const
// Gives a reference in a human readable format. If no language is given,
// it uses the default local name of the books.
{
  ustring s;
  if (language.empty()) { s.append(books_id_to_localname(book)); }
  else                  { s.append(books_id_to_name(language, book)); }
  s.append(" ");
  s.append(convert_to_string(chapter));
  s.append(":");
  s.append(verse);
  return s;
}

bool Reference::equals(const Reference & reference)
{
  // See if the references are exactly the same.
  if (book == reference.book) {
    if (chapter == reference.chapter) {
      if (verse == reference.verse) {
        return true;
      } else {
        // See if they overlap, take range and sequence in account.
        vector < unsigned int >me = verse_range_sequence(verse);
        vector < unsigned int >he = verse_range_sequence(reference.verse);
        for (unsigned int m = 0; m < me.size(); m++) {
          for (unsigned int h = 0; h < he.size(); h++) {
            if (me[m] == he[h])
              return true;
          }
        }
      }
    }
  }
  // No match found: The references differ.
  return false;
}

void Reference::assign(const Reference & reference)
{
  book = reference.book;
  chapter = reference.chapter;
  verse = reference.verse;
}

void Reference::assign(unsigned int book_in, unsigned int chapter_in, const ustring & verse_in)
{
  book = book_in;
  chapter = chapter_in;
  verse = verse_in;
}

void Reference::clear(void)
{
  book = 0;
  chapter = 0;
  verse.clear();
}
