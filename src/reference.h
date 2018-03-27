/*
** Copyright (©) 2003-2013 Teus Benschop, 2016-2018 Matt Postiff
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

#ifndef INCLUDED_REFERENCE_H
#define INCLUDED_REFERENCE_H

#include "libraries.h"
#include "tiny_utilities.h"

class Reference
{
public:
  enum RefType { blankVerse, singleVerse, multiVerse, complexRange, wholeChapter };
  // Examples of above:
  // blankVerse is a newly intialized Reference
  // singleVerse is like John 3:16
  // multiVerse is like John 3:16-17
  // complexRange is like John 3:36-4:1 (crosses chapters)
  // wholeChapter is like John 3:0, where the zero indicates the entire chapter
  inline RefType getRefType() { return reftype; }
  inline void setRefType(RefType _rt) { reftype = _rt; }
  
private:
  unsigned int book;
  unsigned int chapter;
  ustring verse;  // this is not just a verse integer, but could be 16-18, for example
  RefType reftype;
  
public:
  Reference ();
  Reference (unsigned int book_in, unsigned int chapter_in, const ustring& verse_in);
  Reference (unsigned int book_in, unsigned int chapter_in, unsigned int verse_in);
  Reference (unsigned int encoded);

  inline unsigned int book_get() const { return book; }
  inline unsigned int chapter_get() const { return chapter; }
  inline ustring verse_get() const { return verse; }
  inline unsigned int verse_get_single() const { return convert_to_int(verse); }

  inline void book_set(unsigned int b) { book = b; }
  inline void chapter_set(unsigned int c) { chapter = c; }
  inline void verse_set(const ustring &v) { verse = v; }
  inline void verse_append(ustring ap) { verse.append(ap); }

  ustring human_readable (const ustring& language) const;
  bool equals(const Reference& reference);
  void assign(const Reference& reference);
  void assign(unsigned int book_in, unsigned int chapter_in, const ustring& verse_in);
  void clear(void);
};

#endif
