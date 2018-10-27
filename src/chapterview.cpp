/*
 ** Copyright (©) 2016 Matt Postiff.
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

// Skeletal implementation

#include "chapterview.h"
#include "reference.h"
#include "spelling.h"
#include "usfm.h"

ChapterView::ChapterView()
{
  current_reference.clear();
}

ChapterView::~ChapterView() 
{
  // Don't need to do anything in this base class
}

// So...the story here is that this routine is used by spelling.cpp in both USFMview and Editor2. It needs to be here
// so that both can use it...I think...and spelling check doesn't need to know what kind of view it is running on.
void ChapterView::get_styles_at_iterator(GtkTextIter iter, ustring & paragraph_style, ustring & character_style)
{
  // Get the applicable styles.
  // This is done by getting the names of the styles at the iterator.
  // The first named tag is the paragraph style, 
  // and the second named one is the character style.
  paragraph_style.clear();
  character_style.clear();
  GSList *tags = NULL, *tagp = NULL;
  tags = gtk_text_iter_get_tags(&iter);
  for (tagp = tags; tagp != NULL; tagp = tagp->next) {
    GtkTextTag *tag = (GtkTextTag *) tagp->data;
    gchar *strval;
    g_object_get(G_OBJECT(tag), "name", &strval, NULL);
    if (strval) {
      if (strlen(strval)) {
        // Skip the tag for a misspelled word.
        if (strcmp(strval, spelling_tag_name()) != 0) {
          // First store the paragraph style, then the character style.
          // This works because the editing code takes care when applying the tags,
          // to first apply the paragraph style, and then the character style.
          if (paragraph_style.empty()) {
            paragraph_style = strval;
          } else {
            character_style = strval;
          }
        }
      }
      g_free(strval);
    }
  }
  if (tags) {
    g_slist_free(tags);
  }
}

bool ChapterView::iterator_includes_note_caller (GtkTextIter iter)
// Check whether the iter points right after a note caller.
{
  if (!gtk_text_iter_backward_char (&iter)) {
    return false;
  }
  ustring paragraph_style, character_style;
  get_styles_at_iterator(iter, paragraph_style, character_style);
  if (character_style.find (note_starting_style ()) == string::npos)
    return false;
  return true;
}

bool ChapterView::move_end_iterator_before_note_caller_and_validate (GtkTextIter startiter, GtkTextIter enditer, GtkTextIter & moved_enditer)
// If the iterator is after a note caller, it moves the iterator till it is before the note caller.
// Return true if the two iterators contain some text.
{
  // Initialize the iterator that migth be moved.
  moved_enditer = enditer;
  // Finite loop prevention.
  unsigned int finite_loop = 0;
  // Keep moving the iterator for as long as it contains a note caller.
  while (iterator_includes_note_caller (moved_enditer) && finite_loop < 10) {
    gtk_text_iter_backward_char (&moved_enditer);
    finite_loop++;
  }
  // Check whether the end iterator is bigger than the start iterator.
  return (gtk_text_iter_compare (&moved_enditer, &startiter) > 0);
}
