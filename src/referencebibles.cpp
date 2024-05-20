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

#include "referencebibles.h"
#include <glib/gi18n.h>
#include "progresswindow.h"
#include "directories.h"
#include <locale>
#include "gtkwrappers.h"

// A small set of Bibles is available as a separate package:
// Following assumes you have installed bibledit locally instead of in system directories
// cd ~/bibledit/share/bibledit-desktop
// git clone git@github.com:postiffm/bibles.git
//   or
// git clone https://github.com/postiffm/bibles.git
// These are shown in the Analysis window as an aid to the translator.
// Goal is to have BYZ (unaccented) first, then maybe NET, SBLGNT, ASV, English Majority Text (EMTV),
// Second goal is to include another tab for translator notes from the NET or other
// resources.

ReferenceBibles::ReferenceBibles() : bibles()
{
  // Bibles
  bibles.push_back( new bible_byz("BYZ", "Symbol") );    // Byzantine Majority Text, Pierpont/Robinson
  bibles.push_back( new bible_sblgnt("SBL", "Symbol") ); // SBL Greek NT
  bibles.push_back( new bible_engmtv("EMT", "Times") );  // English Majority Text Version, Paul Esposito
  bibles.push_back( new bible_leb("LEB", "Times") );     // Lexham English Bible
  bibles.push_back( new bible_netbible("NET", "Times") );     // Netbible English Bible

  // Apparatus
  apparatus.push_back( new bible_sblgntapp("SBL", "Symbol") ); // SBL Greek NT
}

ReferenceBibles::~ReferenceBibles()
{
  uint8_t i;

  for (i=0; i < bibles.size(); i++) {
    delete bibles.at(i);
  }
  bibles.clear();

  for (i=0; i < apparatus.size(); i++) {
    delete apparatus.at(i);
  }
  apparatus.clear();
}

void ReferenceBibles::write_bibles(const Reference &ref,  HtmlWriter2 &htmlwriter)
{
    ProgressWindow progresswindow (_("Loading reference Bibles"), false);
    progresswindow.set_iterate (0, 1, bibles.size());
    for (auto it : bibles) {
        progresswindow.iterate();
        bible *b = it;
        if (b == NULL) { break; }
        ustring verse;
        try {
          verse = b->retrieve_verse(ref);
        }
        catch (std::runtime_error &e) {
          htmlwriter.paragraph_open();
          verse.append(e.what());
          htmlwriter.paragraph_close();
        }
        //  I assume the following post-condition to the above statement:
        //  the verse is loaded into the bible,  book,  chapter,  and the
        //  the verse contains only the text of the verse,  no x:y prefix
        if (!b->validateBookNum(ref.book_get()) || verse.empty()) {
            // If the book is not valid in this Bible (say it is only NT, no OT), or
            // the verse happens to be emptyp for some reason, do not print anything
//            htmlwriter.paragraph_open();
//            verse.append(_("<empty>"));
//            htmlwriter.paragraph_close();
        }
        else {
              replace_text(verse, "\n", " ");
              htmlwriter.paragraph_open();
              htmlwriter.text_add(b->projname + " " + books_id_to_localname(ref.book_get()) + " " + std::to_string(ref.chapter_get()) + ":" + ref.verse_get());
              htmlwriter.font_open(b->font_get().c_str());
              htmlwriter.text_add(verse);
              htmlwriter.font_close();
              htmlwriter.paragraph_close();
            }

      }
}

void ReferenceBibles::write_apparatus(const Reference &ref,  HtmlWriter2 &htmlwriter)
{
    ProgressWindow progresswindow (_("Loading reference Apparatus"), false);
    progresswindow.set_iterate (0, 1, apparatus.size());
    for (auto it : apparatus) {
        progresswindow.iterate();
        bible *b = it;
        if (b == NULL) { break; }
        ustring verse;
        try {
          verse = b->retrieve_verse(ref);
        }
        catch (std::runtime_error &e) {
          htmlwriter.paragraph_open();
          verse.append(e.what());
          htmlwriter.paragraph_close();
        }
        //  I assume the following post-condition to the above statement:
        //  the verse is loaded into the bible,  book,  chapter,  and the
        //  the verse contains only the text of the verse,  no x:y prefix
        if (!b->validateBookNum(ref.book_get()) || verse.empty()) {
            // If the book is not valid in this Bible (say it is only NT, no OT), or
            // the verse happens to be emptyp for some reason, do not print anything
//            htmlwriter.paragraph_open();
//            verse.append(_("<empty>"));
//            htmlwriter.paragraph_close();
        }
        else {
              replace_text(verse, "\n", " ");
              htmlwriter.paragraph_open();
              htmlwriter.text_add(b->projname + " " + books_id_to_localname(ref.book_get()) + " " + std::to_string(ref.chapter_get()) + ":" + ref.verse_get());
              htmlwriter.font_open(b->font_get().c_str());
              htmlwriter.text_add(verse);
              htmlwriter.font_close();
              htmlwriter.paragraph_close();
            }

      }
}
