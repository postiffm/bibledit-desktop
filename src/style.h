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


#ifndef INCLUDED_STYLE_H
#define INCLUDED_STYLE_H


#include "libraries.h"
#include <glib.h> 

enum EditorNoteType {entFootnote, entEndnote, entCrossreference};

enum StyleType {
  stIdentifier, stNotUsedComment, stNotUsedRunningHeader, stStartsParagraph, stInlineText, 
  stChapterNumber, stVerseNumber, stFootEndNote, stCrossreference, stPeripheral,
  stPicture, stPageBreak, stTableElement, stWordlistElement
};
enum IdentifierType {
  itBook, itEncoding, itComment, itRunningHeader, 
  itLongTOC, itShortTOC, itBookAbbrev, 
  itChapterLabel, itPublishedChapterMarker, itCommentWithEndmarker
};
enum FootEndNoteType {
  fentFootnote, fentEndnote, fentStandardContent, fentContent, fentContentWithEndmarker, fentParagraph
};
enum CrossreferenceType {
  ctCrossreference, ctStandardContent, ctContent, ctContentWithEndmarker
};
enum NoteNumberingType {
  nntNumerical, nntAlphabetical, nntUserDefined
};
enum NoteNumberingRestartType {
  nnrtNever, nnrtBook, nnrtChapter
};
enum EndnotePositionType {
  eptAfterBook, eptAfterEverything, eptAtMarker
};
enum ParagraphType {
  ptMainTitle, ptSubTitle, ptSectionHeading, ptNormalParagraph
};
enum PeripheralType {
  ptPublication, ptTableOfContents, ptPreface, ptIntroduction, ptGlossary,
  ptConcordance, ptIndex, ptMapIndex, ptCover, ptSpine
};
enum TableElementType {
  tetRow, tetHeading, tetCell
};
enum WordListElementType {
  wltWordlistGlossaryDictionary, wltHebrewWordlistEntry, wltGreekWordlistEntry, 
  wltSubjectIndexEntry
};

// Note: A "style" is associated with a usfm "marker."
// A usfm marker has a print style associated with it (indent, font size, etc.)
class Style
{
public:
  Style (const ustring& stylesheet, const ustring& _marker, bool write);
  Style (const ustring& _marker);
  // TO DO: write an explicit copy constructor
  Style & operator=(const Style &right);
  ~Style ();
  ustring marker;
  ustring name;
  ustring info;
  StyleType type;
  int subtype;
  double fontsize;
  ustring italic;
  ustring bold;
  ustring underline;
  ustring smallcaps;
  bool superscript;
  ustring justification;
  double spacebefore;
  double spaceafter;
  double leftmargin;
  double rightmargin;
  double firstlineindent;
  bool spancolumns;
  unsigned int color;
  bool print;
  bool userbool1;
  bool userbool2;
  bool userbool3;
  int userint1;
  int userint2;
  int userint3;
  ustring userstring1;
  ustring userstring2;
  ustring userstring3;
private:
  // TO DO: Eventually I believe I can get rid of these two fields.
  ustring mystylesheet;
  bool mywrite;
  void init(const ustring& stylesheet, const ustring& style, bool write);

public:
    // These methods are very simple and need no Style object to function
    // properly. I just stuffed them into this namespace because they 
    // fit better here. Prior to this, all were called style_... which
    // indicated to me that they belonged with tey style code.
    // TO DO: We could debate whether some of these (or all) belong in the 
    // stylesheet or styles.cpp file. Probably all that access the global
    // stylesheet and styles should be moved elsewhere.
    static bool    get_plaintext(StyleType type, int subtype);
    static bool    get_paragraph(StyleType type, int subtype);
    static bool    get_starts_new_line_in_editor(StyleType type, int subtype);
    static bool    get_starts_new_line_in_usfm(StyleType type, int subtype);
    static bool    get_displays_marker(StyleType type, int subtype);
    static bool    get_starts_character_style(StyleType type, int subtype);
    static bool    get_starts_verse_number(StyleType type, int subtype);
    static ustring get_verse_marker(const ustring& project);
    static bool    get_starts_footnote(StyleType type, int subtype);
    static bool    get_starts_endnote(StyleType type, int subtype);
    static bool    get_starts_crossreference(StyleType type, int subtype);
    static bool    get_starts_note_content(StyleType type, int subtype);
    static ustring get_default_note_style(const ustring& project, EditorNoteType type);
    static ustring get_paragraph_note_style(const ustring& project);
    static bool    get_starts_table_row(StyleType type, int subtype);
    static ustring get_table_row_marker(const ustring& project);
    static bool    get_starts_table_cell(StyleType type, int subtype);
    static ustring get_table_cell_marker(const ustring& project, int column);
    static void    marker_get_type_and_subtype(const ustring & project, const ustring & marker, StyleType & type, int &subtype);
};

#endif
