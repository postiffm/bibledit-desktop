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

#include "libraries.h"
#include "utilities.h"
#include <glib.h>
#include "style.h"
#include "styles.h"
#include "xmlutils.h"
#include "gwrappers.h"
#include "directories.h"
#include "stylesheetutils.h"
#include "constants.h"
#include <glib/gi18n.h>

Style::Style(const ustring & stylesheet, const ustring& marker, bool write)
{
  init(/*stylesheet*/stylesheet, /*usfm marker*/marker, /*write*/write);
}

Style::Style(const ustring& marker)
{
  init(/*stylesheet*/"", /*usfm marker*/marker, /*write*/false);
}

void Style::init(const ustring& stylesheet, const ustring& _marker, bool write)
{
  mystylesheet = stylesheet;
  marker = _marker;
  mywrite = write;
  name = _("Marker");
  info = _("Unified Standard Format Marker");
  type = stInlineText;
  subtype = 0;
  fontsize = 12;
  italic = OFF;
  bold = OFF;
  underline = OFF;
  smallcaps = OFF;
  superscript = false;
  justification = LEFT;
  spacebefore = 0;
  spaceafter = 0;
  leftmargin = 0;
  rightmargin = 0;
  firstlineindent = 0;
  spancolumns = false;
  color = 0;
  print = true;
  userbool1 = false;
  userbool2 = false;
  userbool3 = false;
  userint1 = 0;
  userint2 = 0;
  userint3 = 0;
  userstring1 = "";
  userstring2 = "";
  userstring3 = "";
  // Read style values from the database; this was part of a formerly decprecated method. Watch for issues.
  if (mystylesheet != "") { stylesheet_load_style(mystylesheet, *this); }
}

// Copy another style into this one
Style & Style::operator=(const Style &right)
{
  if (this != &right) { // don't do anything in case of self-assignment
    mystylesheet = right.mystylesheet;
    marker = right.marker;
    mywrite = right.mywrite;
    name = right.name;
    info = right.info;
    type = right.type;
    subtype = right.subtype;
    fontsize = right.fontsize;
    italic = right.italic;
    bold = right.bold;
    underline = right.underline;
    smallcaps = right.smallcaps;
    superscript = right.superscript;
    justification = right.justification;
    spacebefore = right.spacebefore;
    spaceafter = right.spaceafter;
    leftmargin = right.leftmargin;
    rightmargin = right.rightmargin;
    firstlineindent = right.firstlineindent;
    spancolumns = right.spancolumns;
    color = right.color;
    print = right.print;
    userbool1 = right.userbool1;
    userbool2 = right.userbool2;
    userbool3 = right.userbool3;
    userint1 = right.userint1;
    userint2 = right.userint2;
    userint3 = right.userint3;
    userstring1 = right.userstring1;
    userstring2 = right.userstring2;
    userstring3 = right.userstring3;
  }
  return *this;
}

Style::~Style()
{
  // Save style in database.
  if (mywrite) {
    stylesheet_save_style(mystylesheet, *this);
  }
}

bool style_get_plaintext(StyleType type, int subtype)
/*
 Returns the property "plain text" of a certain style when displayed in the Editor.
 Plain text means that this style is to be displayed in the editor using
 plain text.
 type: the type of this style.
 subtype: the subtype of this style.
 */
{
  // Set default value.
  bool plaintext = true;
  // Set value depending on the type of the marker, and the subtype.
  // Only set value if it differs from the default.
  switch (type) {
    case stIdentifier:
    {
      if (subtype == itCommentWithEndmarker)
        plaintext = false;
      break;
    }
    case stNotUsedComment:
    case stNotUsedRunningHeader:
    {
      break;
    }
    case stStartsParagraph:
    case stInlineText:
    case stChapterNumber:
    case stVerseNumber:
    case stFootEndNote:
    case stCrossreference:
    {
      plaintext = false;
      break;
    }
    case stPeripheral:
    {
      break;
    }
    case stPicture:
    {
      break;
    }
    case stPageBreak:
    {
      break;
    }
    case stTableElement:
    {
      break;
    }
    case stWordlistElement:
    {
      plaintext = false;
      break;
    }
  }
  // Return the value.
  return plaintext;
}


bool style_get_paragraph(StyleType type, int subtype)
/*
 Returns true if the combination of the "type" and the"subtype" is
 a paragraph style, as opposed to a character style.

 Note that there is another function, "style_get_starts_new_line_in_editor", 
 which has a slightly different use. Let these two not be confused.

 This function says whether a style is a paragraph style, and the other 
 function says whether a style ought to start a new line in the formatted view.
 */
{
  bool paragraph_style = true;
  // Set value depending on the type of the marker, and the subtype.
  // Only set value if it differs from the default.
  switch (type) {
  case stIdentifier:
    {
      if (subtype == itCommentWithEndmarker)
        paragraph_style = false;
      break;
    }
  case stNotUsedComment:
  case stNotUsedRunningHeader:
    {
      break;
    }
  case stStartsParagraph:
    {
      break;
    }
  case stInlineText:
    {
      paragraph_style = false;
      break;
    }
  case stChapterNumber:
    {
      break;
    }
  case stVerseNumber:
    {
      paragraph_style = false;
      break;
    }
  case stFootEndNote:
    {
      if ((subtype != fentParagraph) && (subtype != fentStandardContent))
        paragraph_style = false;
      break;
    }
  case stCrossreference:
    {
      if (subtype != ctStandardContent)
        paragraph_style = false;
      break;
    }
  case stPeripheral:
    {
      break;
    }
  case stPicture:
    {
      break;
    }
  case stPageBreak:
    {
      break;
    }
  case stTableElement:
    {
      break;
    }
  case stWordlistElement:
    {
      paragraph_style = false;
      break;
    }
  }
  return paragraph_style;
}


bool style_get_starts_new_line_in_editor(StyleType type, int subtype)
// Returns true if the combination of the "type" and the"subtype" starts
// a new line in the formatted view.
{
  // Set default value to starting a new line.
  bool starts_new_line = true;
  // Set value depending on the type of the marker, and the subtype.
  // Only set value if it differs from the default.
  switch (type) {
  case stIdentifier:
    {
      if (subtype == itCommentWithEndmarker)
        starts_new_line = false;
      break;
    }
  case stNotUsedComment:
  case stNotUsedRunningHeader:
    {
      break;
    }
  case stStartsParagraph:
    {
      break;
    }
  case stInlineText:
    {
      starts_new_line = false;
      break;
    }
  case stChapterNumber:
    {
      break;
    }
  case stVerseNumber:
    {
      starts_new_line = false;
      break;
    }
  case stFootEndNote:
    {
      if (subtype != fentParagraph)
        starts_new_line = false;
      break;
    }
  case stCrossreference:
    {
      starts_new_line = false;
      break;
    }
  case stPeripheral:
    {
      break;
    }
  case stPicture:
    {
      break;
    }
  case stPageBreak:
    {
      break;
    }
  case stTableElement:
    {
      break;
    }
  case stWordlistElement:
    {
      starts_new_line = false;
      break;
    }
  }
  // Return the outcome.
  return starts_new_line;
}


bool style_get_starts_new_line_in_usfm(StyleType type, int subtype)
// Returns true if the combination of the "type" and the"subtype" starts
// a new line in the usfm code.
{
  // Set default value to starting a new line.
  bool starts_new_line = true;
  // Set value depending on the type of the marker, and the subtype.
  // Only set value if it differs from the default.
  switch (type) {
  case stIdentifier:
    {
      if (subtype == itCommentWithEndmarker)
        starts_new_line = false;
      break;
    }
  case stNotUsedComment:
  case stNotUsedRunningHeader:
    {
      break;
    }
  case stStartsParagraph:
    {
      break;
    }
  case stInlineText:
    {
      starts_new_line = false;
      break;
    }
  case stChapterNumber:
    {
      break;
    }
  case stVerseNumber:
    {
      break;
    }
  case stFootEndNote:
    {
      starts_new_line = false;
      break;
    }
  case stCrossreference:
    {
      starts_new_line = false;
      break;
    }
  case stPeripheral:
    {
      break;
    }
  case stPicture:
    {
      break;
    }
  case stPageBreak:
    {
      break;
    }
  case stTableElement:
    {
      break;
    }
  case stWordlistElement:
    {
      starts_new_line = false;
      break;
    }
  }
  // Return the outcome.
  return starts_new_line;
}


bool style_get_displays_marker(StyleType type, int subtype)
// Returns true if the combination of the "type" and the"subtype" should display
// the marker in the formatted view.
{
  // Set default value to displaying the style.
  bool display_marker = true;
  // Set value depending on the type of the marker, and the subtype.
  // Only set value if it differs from the default.
  switch (type) {
    case stIdentifier:
    {
      if (subtype == itCommentWithEndmarker)
        display_marker = false;
      break;
    }
    case stNotUsedComment:
    case stNotUsedRunningHeader:
    {
      break;
    }
    case stStartsParagraph:
    {
      display_marker = false;
      break;
    }
    case stInlineText:
    {
      display_marker = false;
      break;
    }
    case stChapterNumber:
    {
      display_marker = false;
      break;
    }
    case stVerseNumber:
    {
      display_marker = false;
      break;
    }
    case stFootEndNote:
    {
      display_marker = false;
      break;
    }
    case stCrossreference:
    {
      display_marker = false;
      break;
    }
    case stPeripheral:
    {
      break;
    }
    case stPicture:
    {
      break;
    }
    case stPageBreak:
    {
      break;
    }
    case stTableElement:
    {
      break;
    }
    case stWordlistElement:
    {
      display_marker = false;
      break;
    }
  }
  // Return the outcome.
  return display_marker;

}


bool style_get_starts_character_style(StyleType type, int subtype)
// Returns true if the combination of the "type" and the"subtype" starts
// (or ends, of course) a character style in the formatted view.
{
  // Set default value to not starting a character style.
  bool starts_character_style = false;
  // Set value depending on the type of the marker, and the subtype.
  // Only set value if it differs from the default.
  switch (type) {
  case stIdentifier:
    {
      if (subtype == itCommentWithEndmarker)
        starts_character_style = true;
      break;
    }
  case stNotUsedComment:
  case stNotUsedRunningHeader:
    {
      break;
    }
  case stStartsParagraph:
    {
      break;
    }
  case stInlineText:
    {
      starts_character_style = true;
      break;
    }
  case stChapterNumber:
    {
      break;
    }
  case stVerseNumber:
    {
      break;
    }
  case stFootEndNote:
    {
      if (subtype == fentContentWithEndmarker)
        starts_character_style = true;
      break;
    }
  case stCrossreference:
    {
      if (subtype == ctContentWithEndmarker)
        starts_character_style = true;
      break;
    }
  case stPeripheral:
    {
      break;
    }
  case stPicture:
    {
      break;
    }
  case stPageBreak:
    {
      break;
    }
  case stTableElement:
    {
      break;
    }
  case stWordlistElement:
    {
      starts_character_style = true;
      break;
    }
  }
  // Return the outcome.
  return starts_character_style;
}

bool style_get_starts_verse_number(StyleType type, int subtype)
// Returns true if the combination of the "type" and the"subtype" starts
// a verse number.
{
  // Set default value to not starting a a verse number.
  bool starts_verse_number = false;
  // Set value depending on the type of the marker, and the subtype.
  // Only set value if it differs from the default.
  switch (type) {
  case stIdentifier:
  case stNotUsedComment:
  case stNotUsedRunningHeader:
  case stStartsParagraph:
  case stInlineText:
  case stChapterNumber:
    {
      break;
    }
  case stVerseNumber:
    {
      starts_verse_number = true;
      break;
    }
  case stFootEndNote:
  case stCrossreference:
  case stPeripheral:
  case stPicture:
  case stPageBreak:
  case stTableElement:
  case stWordlistElement:
    {
      break;
    }
  }
  // Return the outcome.
  return starts_verse_number;
}

ustring style_get_verse_marker(const ustring & project)
// Gets the verse marker, normally the "v".
{
  ustring stylesheet = stylesheet_get_actual ();
  extern Styles *styles;
  Usfm *usfm = styles->usfm(stylesheet);
  ustring style = "v";
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    if (style_get_starts_verse_number(usfm->styles[i].type, usfm->styles[i].subtype)) {
      style = usfm->styles[i].marker;
      break;
    }
  }
  return style;
}


bool style_get_starts_footnote(StyleType type, int subtype)
// Returns true if the combination of the "type" and the"subtype" starts
// a footnote.
{
  switch (type) {
    case stIdentifier:
    case stNotUsedComment:
    case stNotUsedRunningHeader:
    case stStartsParagraph:
    case stInlineText:
    case stChapterNumber:
    case stVerseNumber:
    {
      break;
    }
    case stFootEndNote:
    {
      if (subtype == fentFootnote)
        return true;
      break;
    }
    case stCrossreference:
    case stPeripheral:
    case stPicture:
    case stPageBreak:
    case stTableElement:
    case stWordlistElement:
    {
      break;
    }
  }
  return false;
}


bool style_get_starts_endnote(StyleType type, int subtype)
// Returns true if the combination of the "type" and the"subtype" starts
// an endnote.
{
  switch (type) {
    case stIdentifier:
    case stNotUsedComment:
    case stNotUsedRunningHeader:
    case stStartsParagraph:
    case stInlineText:
    case stChapterNumber:
    case stVerseNumber:
    {
      break;
    }
    case stFootEndNote:
    {
      if (subtype == fentEndnote)
        return true;
      break;
    }
    case stCrossreference:
    case stPeripheral:
    case stPicture:
    case stPageBreak:
    case stTableElement:
    case stWordlistElement:
    {
      break;
    }
  }
  return false;
}


bool style_get_starts_crossreference(StyleType type, int subtype)
// Returns true if the combination of the "type" and the"subtype" starts
// a crossreference.
{
  switch (type) {
    case stIdentifier:
    case stNotUsedComment:
    case stNotUsedRunningHeader:
    case stStartsParagraph:
    case stInlineText:
    case stChapterNumber:
    case stVerseNumber:
    case stFootEndNote:
    {
      break;
    }
    case stCrossreference:
    {
      if (subtype == ctCrossreference)
        return true;
      break;
    }
    case stPeripheral:
    case stPicture:
    case stPageBreak:
    case stTableElement:
    case stWordlistElement:
    {
      break;
    }
  }
  return false;
}


bool style_get_starts_note_content(StyleType type, int subtype)
// Returns true if the combination of the "type" and the"subtype" starts
// note content.
{
  bool note_content = false;
  switch (type) {
    case stIdentifier:
    case stNotUsedComment:
    case stNotUsedRunningHeader:
    case stStartsParagraph:
    case stInlineText:
    case stChapterNumber:
    case stVerseNumber:
    {
      break;
    }
    case stFootEndNote:
    {
      if ((subtype == fentContent) || (subtype == fentStandardContent))
        note_content = true;
      break;
    }
    case stCrossreference:
    {
      if ((subtype == ctContent) || (subtype == ctStandardContent))
        note_content = true;
      break;
    }
    case stPeripheral:
    case stPicture:
    case stPageBreak:
    case stTableElement:
    case stWordlistElement:
    {
      break;
    }
  }
  return note_content;
}


ustring style_get_default_note_style(const ustring & project, EditorNoteType type)
{
  ustring stylesheet = stylesheet_get_actual ();
  extern Styles *styles;
  Usfm *usfm = styles->usfm(stylesheet);
  ustring style;
  switch (type) {
    case entFootnote:
    case entEndnote:
    {
      style = "ft";
      break;
    }
    case entCrossreference:
    {
      style = "xt";
      break;
    }
  }
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    switch (type) {
      case entFootnote:
      case entEndnote:
      {
        if (usfm->styles[i].type == stFootEndNote)
          if (usfm->styles[i].subtype == fentStandardContent)
            style = usfm->styles[i].marker;
        break;
      }
      case entCrossreference:
      {
        if (usfm->styles[i].type == stCrossreference)
          if (usfm->styles[i].subtype == ctStandardContent)
            style = usfm->styles[i].marker;
        break;
      }
    }
  }
  return style;
}


ustring style_get_paragraph_note_style(const ustring & project)
// Gets the style that starts a new paragraph in a footnote or endnote.
{
  ustring stylesheet = stylesheet_get_actual ();
  extern Styles *styles;
  Usfm *usfm = styles->usfm(stylesheet);
  ustring style("fp");
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    if (usfm->styles[i].type == stFootEndNote)
      if (usfm->styles[i].subtype == fentParagraph)
        style = usfm->styles[i].marker;
  }
  return style;
}


bool style_get_starts_table_row(StyleType type, int subtype)
{
  bool starts_row = false;
  switch (type) {
  case stIdentifier:
  case stNotUsedComment:
  case stNotUsedRunningHeader:
  case stStartsParagraph:
  case stInlineText:
  case stChapterNumber:
  case stVerseNumber:
  case stFootEndNote:
  case stCrossreference:
  case stPeripheral:
  case stPicture:
  case stPageBreak:
    {
      break;
    }
  case stTableElement:
    {
      if (subtype == tetRow)
        starts_row = true;
      break;
    }
  case stWordlistElement:
    {
      break;
    }
  }
  return starts_row;
}

ustring style_get_table_row_marker(const ustring & project)
// Get the marker that starts a new row in a table.
{
  ustring stylesheet = stylesheet_get_actual ();
  extern Styles *styles;
  Usfm *usfm = styles->usfm(stylesheet);
  ustring style = "tr";
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    if (style_get_starts_table_row(usfm->styles[i].type, usfm->styles[i].subtype)) {
      style = usfm->styles[i].marker;
      break;
    }
  }
  return style;
}

bool style_get_starts_table_cell(StyleType type, int subtype)
{
  bool starts_cell = false;
  switch (type) {
  case stIdentifier:
  case stNotUsedComment:
  case stNotUsedRunningHeader:
  case stStartsParagraph:
  case stInlineText:
  case stChapterNumber:
  case stVerseNumber:
  case stFootEndNote:
  case stCrossreference:
  case stPeripheral:
  case stPicture:
  case stPageBreak:
    {
      break;
    }
  case stTableElement:
    {
      if (subtype != tetRow)
        starts_cell = true;
      break;
    }
  case stWordlistElement:
    {
      break;
    }
  }
  return starts_cell;
}

#include "tiny_utilities.h"

ustring style_get_table_cell_marker(const ustring & project, int column)
// Get the marker that starts a cell in a table in "column".
// Column starts with 1 for the first column.
{
  ustring stylesheet = stylesheet_get_actual ();
  extern Styles *styles;
  Usfm *usfm = styles->usfm(stylesheet);
  ustring style = "tc" + convert_to_string(column);
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    if (style_get_starts_table_cell(usfm->styles[i].type, usfm->styles[i].subtype)) {
      if (usfm->styles[i].subtype == tetCell) {
        if (usfm->styles[i].userint1 == column) {
          style = usfm->styles[i].marker;
          break;
        }
      }
    }
  }
  return style;
}

EditorNoteType note_type_get(const ustring & project, const ustring & marker)
// Gets the type of the note, e.g. a footnote.
{
  EditorNoteType notetype = entFootnote;
  ustring stylesheet = stylesheet_get_actual ();
  extern Styles *styles;
  Usfm *usfm = styles->usfm(stylesheet);
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    if (usfm->styles[i].marker == marker) {
      if (usfm->styles[i].type == stFootEndNote) {
        if (usfm->styles[i].subtype == fentFootnote) {
          notetype = entFootnote;
        }
        if (usfm->styles[i].subtype == fentEndnote) {
          notetype = entFootnote;
        }
      }
      if (usfm->styles[i].type == stCrossreference) {
        notetype = entCrossreference;
      }
    }
  }
  return notetype;
}

NoteNumberingType note_numbering_type_get(const ustring & project, const ustring & marker)
/*
 Gets the numbering type of a note, for example, the numbering could be numerical
 or alphabetical.
 */
{
  NoteNumberingType numbering = nntNumerical;
  ustring stylesheet = stylesheet_get_actual ();
  extern Styles *styles;
  Usfm *usfm = styles->usfm(stylesheet);
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    if (usfm->styles[i].marker == marker) {
      numbering = (NoteNumberingType) usfm->styles[i].userint1;
    }
  }
  return numbering;
}


ustring note_numbering_user_sequence_get(const ustring & project, const ustring & marker)
// Gets the sequence of characters from which the note caller should be taken.
{
  ustring sequence;
  ustring stylesheet = stylesheet_get_actual ();
  extern Styles *styles;
  Usfm *usfm = styles->usfm(stylesheet);
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    if (usfm->styles[i].marker == marker) {
      sequence = usfm->styles[i].userstring1;
    }
  }
  return sequence;
}
