/*
 ** Copyright (©) 2003-2013 Teus Benschop, 2015-2017 Matt Postiff
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


#ifndef INCLUDED_BOOKDATA_H
#define INCLUDED_BOOKDATA_H

#include "ustring.h"

enum BookType {btOldTestament, btNewTestament, btFrontBackMatter, btOtherMaterial, btUnknown, btApocrypha};


typedef struct
{
  //--------------------------------------------------------------
  // Statically filled information
  //--------------------------------------------------------------
  const char *name; // English name.
  const char *osis; // Osis name.
  const char *paratext; // Paratext ID.
  const char *bibleworks; // BibleWorks name, also Biblegateway and Blue Letter Bible
  const char *blueletter; // for blue letter bible (similar to bibleworks but Philippians has a weird abbrev)
  const char *onlinebible; // Online Bible name.
  const char *biblestudytools; // For biblestudytools.com
  unsigned int id; // Bibledit's internal id.
  BookType type; // The type of the book.
  bool onechapter; // The book has one chapter.
  //--------------------------------------------------------------
  // Dynamically filled information (see books.cpp, books_init())
  //--------------------------------------------------------------
  ustring localname; // name of book in local (bridge) language (say French, if LANG=fr_FR.UTF-8 and LANGUAGE=fr_FR:fr)
  ustring localabbrev; // book abbreviation in local (bridge) language
  // The "bridge" language is also called the "majority language" by Bibles International translators. There are
  // three such languages: English, French, and Spanish. But the internationalization feature of Bibledit-Desktop
  // is not limited to those three languages. It can be internationalized so that the interface appears in any language.
} book_record;


unsigned int bookdata_books_count();


#endif
