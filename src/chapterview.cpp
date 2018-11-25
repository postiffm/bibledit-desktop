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
#include "style.h"
//#include "debug.h"

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


bool ChapterView::get_verse_number_at_iterator_internal (GtkTextIter iter, const ustring & verse_marker, ustring& verse_number)
// This function looks at the iterator for the verse number.
// If a verse number is not found, it iterates back till one is found.
// If the iterator can't go back any further, and no verse number was found, it returns false.
// If a verse number is found, it returns true and stores it in parameter "verse_number".
{
  // Look for the v style while iterating backward.
  //DEBUG("3debug_verse_number "+verse_number)
  bool verse_style_found = false;
  do {
    ustring paragraph_style, character_style;
    get_styles_at_iterator(iter, paragraph_style, character_style);
    //DEBUG("Style tags: p="+paragraph_style+" c="+character_style);
    if (character_style == verse_marker) {
      verse_style_found = true;
    }
  } while (!verse_style_found && gtk_text_iter_backward_char(&iter));
  // If the verse style is not in this textbuffer, bail out.
  if (!verse_style_found) {
    return false;
  }
  // The verse number may consist of more than one character.
  // Therefore iterate back to the start of the verse style.
  // For convenience, it iterates back to the start of any style.
  while (!gtk_text_iter_begins_tag (&iter, NULL)) {
    gtk_text_iter_backward_char (&iter);
  }
  // Extract the verse number.
  GtkTextIter enditer = iter;
  gtk_text_iter_forward_chars(&enditer, 10);
  verse_number = gtk_text_iter_get_slice(&iter, &enditer);
  //DEBUG("4debug_verse_number "+verse_number)
  size_t position = verse_number.find(" ");
  position = CLAMP(position, 0, verse_number.length());
  verse_number.erase (position, 10);
  //DEBUG("5debug_verse_number "+verse_number)
  // Indicate that a verse number was found.
  return true;
}

void ChapterView::on_editor_get_widgets_callback (GtkWidget *widget, gpointer user_data)
{
  widget_search_t *search_data = static_cast<widget_search_t*> (user_data);
  vector <GtkWidget*> *widgets = search_data->first;
  GType type = search_data->second;

  if (type == G_TYPE_NONE || G_TYPE_CHECK_INSTANCE_TYPE (widget, type))
    widgets->push_back (widget);
}

/**
 * Retrieves all widgets being children of the container vbox. Only the
 * widgets of type of_type are returned. If of_type is G_TYPE_NONE (default
 * value) then all widgets are returned.
 */
vector <GtkWidget *> ChapterView::editor_get_widgets (GtkWidget * vbox, GType of_type)
{
  vector <GtkWidget *> widgets;
  widget_search_t search_data = make_pair(&widgets, of_type);

  gtk_container_foreach(GTK_CONTAINER (vbox),
                        on_editor_get_widgets_callback, &search_data);

  return widgets;
}

GtkWidget * ChapterView::editor_get_next_textview (const vector <GtkWidget *> &widgets,
                                      GtkWidget * textview)
{
  for (unsigned int i = 0; i < widgets.size(); i++) {
    if (textview == widgets[i])
      if (i < widgets.size() - 1)
        return widgets[i+1];
  }
  return NULL;
}


GtkWidget * ChapterView::editor_get_previous_textview (const vector <GtkWidget *> &widgets,
                                          GtkWidget * textview)
// Gets the textview that precedes the "current" one in the vector.
{
  for (unsigned int i = 0; i < widgets.size(); i++) {
    if (textview == widgets[i])
      if (i)
        return widgets[i-1];
  }
  return NULL;
}


ustring ChapterView::get_verse_number_at_iterator(GtkTextIter iter, const ustring & verse_marker, const ustring & project, GtkWidget * parent_box)
/*
This function returns the verse number at the iterator.
It also takes into account a situation where the cursor is on a heading.
The user expects a heading to belong to the next verse. 
*/
{
  // Get the paragraph style at the iterator, in case it is in a heading.
  ustring paragraph_style_at_cursor;
  {
    ustring dummy;
    get_styles_at_iterator(iter, paragraph_style_at_cursor, dummy);
  }
  
  // Verse-related variables.
  ustring verse_number = "0";
  bool verse_number_found = false;
  GtkWidget * textview = NULL;

  vector <GtkWidget *>
  widgets = editor_get_widgets (parent_box, GTK_TYPE_TEXT_VIEW);

  do {
    // Try to find a verse number in the GtkTextBuffer the "iter" points to.
    verse_number_found = get_verse_number_at_iterator_internal (iter, verse_marker, verse_number);
    // If the verse number was not found, look through the previous GtkTextBuffer.
    if (!verse_number_found) {
      // If the "textview" is not yet set, look for the current one.
      if (textview == NULL) {
        GtkTextBuffer * textbuffer = gtk_text_iter_get_buffer (&iter);
        for (unsigned int i = 0; i < widgets.size(); i++) {
          if (textbuffer == gtk_text_view_get_buffer (GTK_TEXT_VIEW (widgets[i]))) {
            textview = widgets[i];
            break;
          }
        }
      }
      // Look for the previous GtkTextView.
      textview = editor_get_previous_textview (widgets, textview);
      // Start looking at the end of that textview.
      if (textview) {
        GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
        gtk_text_buffer_get_end_iter (textbuffer, &iter);
      }
    }

	//DEBUG("1debug_verse_number "+verse_number)

  } while (!verse_number_found && textview);
  
  // Optional: If the cursor is on a title/heading, increase verse number.
  if (!project.empty()) {
    StyleType type;
    int subtype;
    Style::marker_get_type_and_subtype(project, paragraph_style_at_cursor, type, subtype);
    if (type == stStartsParagraph) {
      switch (subtype) {
        case ptMainTitle:
        case ptSubTitle:
        case ptSectionHeading:
        {
          unsigned int vs = convert_to_int (verse_number);
          vs++;
          verse_number = convert_to_string (vs);
          break;
        }
        case ptNormalParagraph:
        {
          break;
        }
      }
    }
  }
 
  //DEBUG("2debug_verse_number "+verse_number)

  // Return the verse number found.
  return verse_number;
}
