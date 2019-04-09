/* Copyright (©) 2018 Matt Postiff, 2003-2013 Teus Benschop.
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

#include "utilities.h"
#include <glib.h>
#include "editor3.h"
#include "usfm.h"
#include <gdk/gdkkeysyms.h>
#include "projectutils.h"
#include "settings.h"
#include "screen.h"
#include "referenceutils.h"
#include "books.h"
#include "gwrappers.h"
#include "biblenotes.h"
#include "color.h"
#include "bible.h"
#include "stylesheetutils.h"
#include "styles.h"
#include "usfmtools.h"
#include "directories.h"
#include "notecaller.h"
#include "gtkwrappers.h"
#include "keyboard.h"
#include "tiny_utilities.h"
#include "git.h"
#include "progresswindow.h"
#include "merge_utils.h"
#include <glib/gi18n.h>
#include "debug.h"

// Rewrite of editor2, in a single textview (for the most part)

/*


Text editor with a formatted view and undo and redo.
Care was taken not to embed child widgets in the GtkTextView,
because this could give crashes in Gtk under certain circumstances.


The following elements of text have their own textview and textbuffer:
- every paragraph
- every cell of a table.
- every note.

While the user edits text, signal handlers are connected so it can be tracked what is being edited.
The signal handlers call routines that make EditorActions out of this, and optionally correct the text being typed.
The EditorActions, when played back, will have the same effect as if the user typed.
If Undo is done, then the last EditorAction is undone. It can also be Redone.


The following things need to be tested after a change was made to the Editor object:
* USFM \id GEN needs to be displayed with the full \id line.
* USFM \p starts a new paragraph.
* USFM \v works fine for verse markup.
* USFM \add does the character style.
* If USFM text is pasted through the clipboard, this should load properly.
* If a new line is entered, even in the middle of existing text, the editor should display this.
* If a new line is entered in a character style, the existing character style should go to the next paragraph.
* If plain text with new lines is pasted through the clipboard, the editor should display the new lines
* When typing text where a character style starts, it should take this character style.
* When typing text right after where a character style ends, it should take this character style.
* 
*/


Editor3::Editor3(GtkWidget * vbox_in, const ustring & project_in)
{
  // current_reference_set initializes itself via its constructor
  // Save and initialize variables.
  project = project_in;
  do_not_process_child_anchors_being_deleted = false;
  texttagtable = NULL;
  previous_hand_cursor = false;
  highlight = NULL;
  editable = false;
  event_id_show_quick_references = 0;
  signal_if_verse_changed_event_id = 0;
  keystrokeNum = 0;
  keyStrokeNum_paragraph_crossing_processed = 0;
  verse_restarts_paragraph = false;
  focused_textview = NULL;
  disregard_text_buffer_signals = 0;
  textbuffer_delete_range_was_fired = false;
  verse_tracking_on = false;
  editor_actions_size_at_no_save = false;
  font_size_multiplier = 1;
  highlight_timeout_event_id = 0;
  
  // Create data that is needed for any of the possible formatted views.
  create_or_update_formatting_data();

  // Spelling checker.
  spellingchecker = new SpellingChecker(this, texttagtable);
  g_signal_connect((gpointer) spellingchecker->check_signal, "clicked", G_CALLBACK(on_button_spelling_recheck_clicked), gpointer(this));
  load_dictionaries();

  // The formatted editor GUI is structured like this:
  // vbox_client -> vbox = vbox_in -> scrolledwindow -> viewport -> vbox_viewport -> vbox_paragraphs -> textview w/ textbuffer model
  //                                                                              -> hseparator
  //                                                                              -> vbox_notes      -> notetextview w/ notetextbuffer model
  //                               -> vbox_parking_lot
  // This seems far too complex. But we have to have the viewport and vbox_viewport to 
  // contain the three separate portions of the text. I tried every possible combination without the viewport,
  // but we have to manually add it, at least in GTK2.
  
  // The basic GUI, which actually is empty until text will be loaded in it.
  scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolledwindow);
  gtk_box_pack_start(GTK_BOX(vbox_in), scrolledwindow, true, true, 0);
  // TODO: Fix next line to turn off horizontal scrolling if needed
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), viewport);

  vbox_viewport = gtk_vbox_new (false, 0);
  gtk_widget_show(vbox_viewport);
  gtk_container_add (GTK_CONTAINER (viewport), vbox_viewport);

  // Box to store the standard paragraphs.
  vbox_paragraphs = gtk_vbox_new (false, 0);
  gtk_widget_show(vbox_paragraphs);
  gtk_box_pack_start(GTK_BOX(vbox_viewport), vbox_paragraphs, false, false, 0);
  
  textview = NULL;
  textbuffer = NULL;
  
  last_focused_widget = vbox_paragraphs;

  // The separator between text and notes.
  hseparator = gtk_hseparator_new ();
  gtk_widget_show(hseparator);
  gtk_box_pack_start(GTK_BOX(vbox_viewport), hseparator, false, false, 0);

  // Box to store the notes.
  vbox_notes = gtk_vbox_new (false, 0);
  gtk_widget_show(vbox_notes);
  gtk_box_pack_start(GTK_BOX(vbox_viewport), vbox_notes, false, false, 0);

  notetextview = NULL;
  notetextbuffer = NULL;
  
  // Create the invisible parking lot where GtkTextViews get parked while not in use.
  vbox_parking_lot = gtk_vbox_new (false, 0);
  gtk_box_pack_start(GTK_BOX(vbox_in), vbox_parking_lot, false, false, 0);

  // Buttons to give signals.
  new_verse_signal = gtk_button_new();
  new_styles_signal = gtk_button_new();
  word_double_clicked_signal = gtk_button_new();
  reload_signal = gtk_button_new();
  changed_signal = gtk_button_new();
  quick_references_button = gtk_button_new();
  spelling_checked_signal = gtk_button_new ();
  new_widget_signal = gtk_button_new ();

  // Initialize a couple of event ids.
  textview_move_cursor_id = 0;
  textview_grab_focus_event_id = 0;
  spelling_timeout_event_id = 0;
  textview_button_press_event_id = 0;

  // Styles are used to uniquely identify footnotes/endnotes/etc. This 
  // var keeps track of the last one used. This was a static in editor_aids.cpp
  // but that meant it was shared between editor windows, which caused very strange
  // bugs with footnotes being duplicated or lost. postiffm 6/30/2016.
  note_style_num = 0;

  go_to_new_reference_highlight = false; // 3/22/2016 MAP
  
  // Tag for highlighting search words and current verse.
  // For convenience the GtkTextBuffer function is called. 
  // This adds the tag to the GtkTextTagTable, making it available
  // to any other text buffer that uses the same text tag table.
  {
    GtkTextBuffer * textbuffer = gtk_text_buffer_new (texttagtable);
    reference_tag = gtk_text_buffer_create_tag(textbuffer, NULL, "background", "khaki", NULL);
    verse_highlight_tag = gtk_text_buffer_create_tag(textbuffer, NULL, "background", "yellow", NULL);
    g_object_unref (textbuffer);
  }

  currHighlightedVerse = "0";
  
  // Automatic saving of the file, periodically (every minute)
  save_timeout_event_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 60000, GSourceFunc(on_save_timeout), gpointer(this), NULL);
}


Editor3::~Editor3()
{
  // Verse tracking off.
  switch_verse_tracking_off ();

  // Save the chapter.
  chapter_save();

  // Destroy a couple of timeout sources.
  gw_destroy_source(textview_move_cursor_id);
  gw_destroy_source(textview_grab_focus_event_id);
  gw_destroy_source(save_timeout_event_id);
  gw_destroy_source(highlight_timeout_event_id);
  gw_destroy_source(spelling_timeout_event_id);
  gw_destroy_source(event_id_show_quick_references);
  gw_destroy_source(signal_if_verse_changed_event_id);
  gw_destroy_source(textview_button_press_event_id);

  // Delete speller.
  delete spellingchecker;

  // Destroy the signalling buttons.
  gtk_widget_destroy(new_verse_signal);           new_verse_signal = NULL;
  gtk_widget_destroy(new_styles_signal);          new_styles_signal = NULL;
  gtk_widget_destroy(word_double_clicked_signal); word_double_clicked_signal = NULL;
  gtk_widget_destroy(reload_signal);              reload_signal = NULL;
  gtk_widget_destroy(changed_signal);             changed_signal = NULL;
  gtk_widget_destroy(quick_references_button);    quick_references_button = NULL;
  gtk_widget_destroy(spelling_checked_signal);    spelling_checked_signal = NULL;
  gtk_widget_destroy(new_widget_signal);          new_widget_signal = NULL;

  // Destroy the texttag tables.
  g_object_unref(texttagtable);
  
  g_object_unref(textview);        textview = NULL;     textbuffer = NULL;
  g_object_unref(notetextview);    notetextview = NULL; notetextbuffer = NULL;
  focused_textview = NULL;

  // Destroy possible highlight object.
  if (highlight) { 
    delete highlight;
    highlight = NULL;
  }
    
  // Destroy the editor actions.
  // This will also destroy any GtkTextViews these actions created.
  clear_and_destroy_editor_actions (actions_done);
  clear_and_destroy_editor_actions (actions_undone);

  // Destroy remainder of text area.
  gtk_widget_destroy(scrolledwindow); // does this destroy the viewport and its children also?
}

void Editor3::chapter_load(const Reference &ref)
// Loads a chapter with the number ref.chapter_get()
{
  DEBUG("book="+std::to_string(ref.book_get())+" ch="+std::to_string(ref.chapter_get())+" v="+ref.verse_get())
  // Clear the stacks of actions done and redoable.
  clear_and_destroy_editor_actions (actions_done);
  clear_and_destroy_editor_actions (actions_undone);

  // Switch verse tracking off.
  switch_verse_tracking_off ();

  // Save reference
  current_reference = ref;

  // Settings.
  extern Settings *settings;
  ProjectConfiguration *projectconfig = settings->projectconfig(project);

  // Reset note style number
  note_style_num = 0;
  
  // Load text in memory and cache it for later use.
  loaded_chapter_lines = project_retrieve_chapter(project, current_reference.book_get(), current_reference.chapter_get());

  // Whether chapter is editable.
  editable = true;
  if (loaded_chapter_lines.empty()) {
    editable = false;
  }
  if (!projectconfig->editable_get()) {
    editable = false;
  }

  // Get rid of possible previous widgets with their data.
  gtk_container_foreach(GTK_CONTAINER(vbox_paragraphs), on_container_tree_callback_destroy, gpointer(this));
  DEBUG("W1 Destroyed supposedly all prior widgets")
  // Make one long line containing the whole chapter.
  // This is done so as to exclude any possibility that the editor does not
  // properly load a possible chapter that has line-breaks at unexpected places.
  // Add one space to the end so as to accomodate situation such as that the 
  // last marker is "\toc". Without that extra space it won't see this marker,
  // because the formatter looks for "\toc ". The space will solve the issue.
  ustring line;
  for (unsigned int i = 0; i < loaded_chapter_lines.size(); i++) {
    if (!line.empty()) { line.append(" "); }
    line.append(loaded_chapter_lines[i]);
  }
  line.append(" ");

  // Load in editor.
  text_load (line, "", false);
  DEBUG("W3 just did text_load") 
  // Clean up extra spaces before the insertion points in all the
  // newly created textbuffers.
  for (unsigned int i = 0; i < actions_done.size(); i++) {
    EditorAction * action = actions_done[i];
    if ((action->type == eatCreateParagraph) ||
        (action->type == eatCreateNoteParagraph)) {
      EditorActionCreateParagraph * paragraph = static_cast <EditorActionCreateParagraph *> (action);
      EditorActionDeleteText * trim_action = paragraph_delete_last_character_if_space (paragraph);
      if (trim_action) {
        apply_editor_action (trim_action);
      }
    }
  }
  DEBUG("W4 Cleaned up extra spaces")
  // Insert the chapter load boundary.
  apply_editor_action (new EditorAction (this, eatLoadChapterBoundary));
  DEBUG("W5 Inserted chapter load boundary")
  // Place cursor at the start and scroll it onto the screen.
  current_verse_number = current_reference.verse_get();
  currHighlightedVerse = "0"; // this means "the proper verse has not yet been highlighted"
  //vector <GtkWidget *> textviews = editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW);
  highlightCurrVerse(textview); // now it is highlighted
  scroll_to_insertion_point_on_screen(textview);

  // Store size of actions buffer so we know whether the chapter changed.
  editor_actions_size_at_no_save = actions_done.size();
}

bool Editor3::text_starts_paragraph (ustring& line, StyleType type, int subtype, size_t marker_pos, size_t marker_length, bool is_opener, bool marker_found)
{
  if (marker_found && (marker_pos == 0) && is_opener) {
      if (Style::get_starts_new_line_in_editor(type, subtype)) {
          line.erase(0, marker_length);
          return true;
      }
  }
  return false;
}


bool Editor3::text_starts_verse (ustring& line, StyleType type, int subtype, size_t marker_pos, size_t marker_length, bool is_opener, bool marker_found)
{
  if (marker_found && (marker_pos == 0) && is_opener) {
      if (Style::get_starts_verse_number(type, subtype)) {
          line.erase (0, marker_length);
          return true;
      }
  }
  return false;  
}

bool Editor3::editor_starts_character_style(ustring & line, ustring & character_style, const ustring & marker_text, size_t marker_pos, size_t marker_length, bool is_opener, bool marker_found)
{
    if (marker_found && (marker_pos == 0) && (is_opener)) {
        StyleType type;
        int subtype;
        Style::marker_get_type_and_subtype(project, marker_text, type, subtype);
        if (Style::get_starts_character_style(type, subtype)) {
            character_style = marker_text;
            line.erase(0, marker_length);
            return true;
        }
    }
    return false;
}


bool Editor3::editor_ends_character_style(ustring & line, ustring & character_style, const ustring & marker_text, size_t marker_pos, size_t marker_length, bool is_opener, bool marker_found)
{
    if (marker_found && (marker_pos == 0) && (is_opener)) {
        StyleType type;
        int subtype;
        Style::marker_get_type_and_subtype(project, marker_text, type, subtype);
        if (Style::get_starts_character_style(type, subtype)) {
            character_style.clear();
            line.erase(0, marker_length);
            return true;
        }
    }
    return false;
}


bool Editor3::text_starts_note_raw(ustring & line, ustring & character_style, const ustring & marker_text, size_t marker_pos, size_t marker_length, bool is_opener, bool marker_found, ustring& raw_note)
// This function determines whether the text starts a footnote, an endnote, or a crossreference.
{
    if (marker_found && (marker_pos == 0) && (is_opener)) {
        StyleType type;
        int subtype;
        Style::marker_get_type_and_subtype(project, marker_text, type, subtype);
        if (Style::get_starts_footnote(type, subtype) || Style::get_starts_endnote(type, subtype) || Style::get_starts_crossreference(type, subtype)) {
            // Proceed if the endmarker is in the text too.
            ustring endmarker = usfm_get_full_closing_marker(marker_text);
            size_t endmarkerpos = line.find(endmarker);
            if (endmarkerpos != string::npos) {
                // Get raw note text and erase it from the input buffer.
                raw_note = line.substr(marker_length, endmarkerpos - endmarker.length());
                line.erase(0, endmarkerpos + endmarker.length());
                // The information was processed: return true
                return true;
            }
        }
    }
    return false;
}

// Add text to the end of a GtkTextBuffer and return iterators pointing to start
// and end of it.
void Editor3::gtk_text_buffer_append_with_markers (GtkTextBuffer *textbuffer, const ustring& text, GtkTextIter &startiter, GtkTextIter &enditer)
{
    gtk_text_buffer_get_end_iter(textbuffer, &startiter);
    gint startiter_offset = gtk_text_iter_get_offset (&startiter);
    gtk_text_buffer_insert (textbuffer, &startiter, text.c_str(), -1);
    // startiter has been invalidated and now points to the end of the inserted text.
    // Therefore it is the new enditer.
    enditer = startiter;
    gtk_text_buffer_get_iter_at_offset (textbuffer, &startiter, startiter_offset);
}

// Same as above, but don't pass or update start/end markers.
void Editor3::gtk_text_buffer_append (GtkTextBuffer *textbuffer, const ustring& text)
{
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(textbuffer, &iter);
    gtk_text_buffer_insert (textbuffer, &iter, text.c_str(), -1); 
}

GtkWidget *Editor3::create_text_view(GtkWidget *parent_vbox)
{
    // Yes, these tromp on Editor3::textview and ::textbuffer vars of 
    // the same name. But that's OK. The Editor3:: version of these
    // are supposed to be assigned by the caller, since this helper
    // method can produce any number of textviews (Bible text and 
    // footnote areas, but there could be others).
    GtkWidget *textview;
    GtkTextBuffer *textbuffer;
    
    // The textbuffer is the data store or "model." It uses the text tag table.
    textbuffer = gtk_text_buffer_new(texttagtable);
    
    // The text view is the "view" for editing.
    textview = gtk_text_view_new_with_buffer(textbuffer);
    gtk_widget_show(textview);
    
    // Add textview to the GUI.
    gtk_box_pack_start(GTK_BOX(parent_vbox), textview, false, false, 0);
    
    // Set up our textview defaults.
    gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_wrap_mode   (GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
    gtk_text_view_set_editable    (GTK_TEXT_VIEW(textview), editable);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW(textview), 5);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), 5);
    gtk_text_view_set_indent      (GTK_TEXT_VIEW(textview), 10);
    gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(textview), 5);
    
    // Set font
    set_font_textview (textview);
    
    return textview;
}

#define DEBUG_STYLES(NUM) { ustring charstyles; for (auto style : v_character_style) { charstyles.append(style); charstyles.append(" "); }\
  DEBUG("TL" #NUM ": STYLES: p=" + paragraph_style + " c=" + charstyles + " TEXT=" + text.substr(0, 25) + "..."); }

void Editor3::text_load (ustring text, ustring character_style, bool note_mode)
// This loads text into the formatted editor.
// text: text to load.
// character_style: character style to start with.
// note_mode: If true, it is limited to formatting notes only.
{
  // Clean new lines
  replace_text (text, "\n", " ");
  DEBUG("W2 text="+text)
  
  //------------------------------------------------------------------------------
  // The character_style is the starting character style. But then there is the
  // possibility of nested character styles. So we have to keep track of that.
  //------------------------------------------------------------------------------
  vector <ustring> v_character_style;
  if (character_style != "") { v_character_style.push_back(character_style); }
  
  bool start = true;
  
  //------------------------------------------------------------------------------
  // Create textview/textbuffer for the Bible text
  //------------------------------------------------------------------------------
  textview = create_text_view(vbox_paragraphs);
  textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
  focused_textview = GTK_TEXT_VIEW(textview);

  //------------------------------------------------------------------------------
  // Create textview/textbuffer for the footnotes/endnotes/xrefs
  //------------------------------------------------------------------------------
  notetextview = create_text_view(vbox_notes);
  notetextbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(notetextview));
 
  //------------------------------------------------------------------------------
  // Load the text into the editor chunk by chunk. Chunks are delimited by 
  // USFM markers.
  //------------------------------------------------------------------------------
  ustring marker_text;
  size_t marker_pos;
  size_t marker_length;
  bool is_opener = false;
  bool marker_found;
  ustring paragraph_style = "";
  while (!text.empty()) {
    marker_found = usfm_search_marker(text, marker_text, marker_pos, marker_length, is_opener);
    if (marker_found) { DEBUG("TL1: " + marker_text); } else { DEBUG("TL1: No marker: " + text); }
    
    StyleType type;
    int subtype;
    Style::marker_get_type_and_subtype(project, marker_text, type, subtype);
    
    //------------------------------------------------------------------------------
    // The next chunk of text is just text, up to the next marker,
    // or the end of the chapter.
    //------------------------------------------------------------------------------
    if (marker_pos != 0 || !marker_found) { 
      // Text between markers is added to the view/model.
      ustring text_betw_markers = text.substr(0, marker_pos);
      GtkTextIter startiter, enditer;
      gtk_text_buffer_append_with_markers(textbuffer, text_betw_markers, startiter, enditer);
      gtk_text_buffer_apply_tag_by_name (textbuffer, paragraph_style.c_str(), &startiter, &enditer);
      for (auto style : v_character_style) {
        gtk_text_buffer_apply_tag_by_name (textbuffer, style.c_str(), &startiter, &enditer);
      }
      text.erase(0, marker_pos);
      DEBUG("Added text="+text_betw_markers);
      DEBUG_STYLES(7);
    }
    //------------------------------------------------------------------------------
    // We found a new opening USFM marker at the start of the remaining text. Process
    // it, depending on the particular type to guide our specific actions.
    //------------------------------------------------------------------------------
    else if (marker_found && (marker_pos == 0) && is_opener) {
        DEBUG_STYLES(2);
        if (Style::get_displays_marker(type, subtype)) {
            GtkTextIter startiter, enditer;
            gtk_text_buffer_append_with_markers(textbuffer, usfm_get_full_opening_marker (marker_text), startiter, enditer);
            gtk_text_buffer_apply_tag_by_name (textbuffer, paragraph_style.c_str(), &startiter, &enditer);      
        }
        if (Style::get_starts_new_line_in_editor(type, subtype) && !start) {
            gtk_text_buffer_append(textbuffer, "\n"); // but don't add newline if just at start of buffer
        }
        
        if (Style::get_starts_nested_character_style(project, marker_text)) {
            // If this is a nested style, add it to the active list of styles
            ustring marker_text_wo_plus = marker_text.substr(1,ustring::npos);
            v_character_style.push_back(marker_text_wo_plus);
            text.erase(0, marker_length);
            DEBUG_STYLES(6);
        }
        else if (Style::get_starts_character_style(type, subtype)) {
            // If this is NOT a nested style, clear the active list of styles and add it as new one
            v_character_style.clear();
            v_character_style.push_back(marker_text);
            text.erase(0, marker_length);
            DEBUG_STYLES(3);
        }
        else if (Style::get_paragraph(type, subtype)) { // starts a paragraph style; maybe need rename
            paragraph_style = marker_text;
            text.erase(0, marker_length);
            DEBUG_STYLES(4);
        }
        else if (Style::get_starts_verse_number(type, subtype)) {
            // Grab the verse number and process the whole thing \v_#, since it is a special case in USFM
            text.erase(0, marker_length); // eat \v_
            size_t spaceaftervnum = text.find_first_of(" ");
            ustring versenum = text.substr(0, spaceaftervnum);
            GtkTextIter startiter, enditer;
            gtk_text_buffer_append_with_markers(textbuffer, versenum, startiter, enditer);
            gtk_text_buffer_apply_tag_by_name (textbuffer, paragraph_style.c_str(), &startiter, &enditer);
            gtk_text_buffer_apply_tag_by_name (textbuffer, "v", &startiter, &enditer);
            text.erase(0, spaceaftervnum); // eat #
        }
        else if (Style::get_starts_footnote(type, subtype) || Style::get_starts_endnote(type, subtype) || Style::get_starts_crossreference(type, subtype)) {
            // Proceed if the endmarker is in the text too.
            ustring endmarker = usfm_get_full_closing_marker(marker_text);
            size_t endmarkerpos = text.find(endmarker);
            if (endmarkerpos != string::npos) {
                // Get raw note text and erase it from the input buffer.
                ustring raw_note = text.substr(marker_length, endmarkerpos - endmarker.length()); // is this a correct calculation? MAP 11/6/2018
                // Erase the entire footnote from the body of the text
                text.erase(0, endmarkerpos + endmarker.length());
                
                // Insert a superscript "note caller" in the body of the text.
                EditorNoteType note_type = note_type_get (project, marker_text);
                
                ustring caller_in_text, caller_style;
                {
                    get_next_note_caller_and_style (note_type, caller_in_text, caller_style, false);
                    Style style ("", caller_style, false);
                    style.superscript = true;
                    create_or_update_text_style(&style, false, false, font_size_multiplier);
                    GtkTextIter startiter, enditer;
                    gtk_text_buffer_append_with_markers(textbuffer, caller_in_text, startiter, enditer);
                    gtk_text_buffer_apply_tag_by_name (textbuffer, caller_style.c_str(), &startiter, &enditer);
                }
                // Add the foot/endnote/x-ref text to the notetextbuffer
                gtk_text_buffer_append(notetextbuffer, caller_in_text + "\t");
                gtk_text_buffer_append(notetextbuffer, raw_note);
                gtk_text_buffer_append(notetextbuffer, "\n");
            }
            else {
              gtkw_dialog_error(scrolledwindow, "Footnote without endmarker is unexpected");   
            }
        }
        else {
            // Erase just the marker that we found
            text.erase(0, marker_length);
        }
    }
    //------------------------------------------------------------------------------
    // Closing USFM marker...style is already marked as a tag in the textview.
    //------------------------------------------------------------------------------
    else if (marker_found && (marker_pos == 0) && !is_opener) {
        v_character_style.pop_back();
        text.erase(0, marker_length);
        DEBUG_STYLES(5);
    }
    else {
      ustring snippet = text.substr(0, 25);
      gtkw_dialog_error(scrolledwindow, "Unexpected USFM in text_load: " + snippet + "...");
    }

    start = false;
  }

  textviewbuffer_create_actions (textbuffer, textview); // setup callbacks, fonts, etc. only after text is ready
  
  // Update gui.
  if (!note_mode) {
    signal_if_styles_changed();
    signal_if_verse_changed();
  }

  // Trigger a spelling check.
  if (!note_mode) {
    spelling_trigger();
  }
}

ustring Editor3::chapter_get_ustring()
{
  GtkTextIter startiter, enditer;
  gtk_text_buffer_get_start_iter(textbuffer, &startiter);
  gtk_text_buffer_get_end_iter(textbuffer, &enditer);
  ustring chapter_text = usfm_get_text(textbuffer, startiter, enditer);
  replace_text(chapter_text, "  ", " "); // a minimal cleanup
  return chapter_text;
}

void Editor3::chapter_save()
// Handles saving the chapter.
{
  // Set variables.
  reload_chapter_number = current_reference.chapter_get();

  // If the text is not editable, bail out.
  if (!editable) { return; }

  // If the text was not changed, bail out.
  //if (editor_actions_size_at_no_save == actions_done.size()) { return; }
  // TODO: Fix the above functionality

  // If the project is empty, bail out.
  if (project.empty()) { return; }

  // Get the USFM text.
  ustring chaptertext = chapter_get_ustring();
  //DEBUG("chapter="+chaptertext)
  // Flags for use below.
  bool reload = false;
  bool save_action_is_over = false;
  bool check_chapter_cache = true;

  // If the chapter text is completely empty, 
  // that means that the user has deleted everything.
  // This is interpreted as to mean that the user wishes to delete this chapter.
  // After asking for confirmation, delete the chapter.
  if (chaptertext.empty()) {
    if (gtkw_dialog_question(NULL, _("The chapter is empty.\nDo you wish to delete this chapter?"), GTK_RESPONSE_YES) == GTK_RESPONSE_YES) {
      project_remove_chapter(project, current_reference.book_get(), current_reference.chapter_get());
      save_action_is_over = true;
      reload = true;
      if (current_reference.chapter_get() > 0) { reload_chapter_number = current_reference.chapter_get() - 1; }
    }
  }

  DEBUG("Saving chapter")
  
  // If the text has not yet been dealt with, save it.  
  if (!save_action_is_over) {
    // Clean it up a bit and divide it into lines.
    ParseLine parseline(trim(chaptertext));
    // Add verse information.
    CategorizeChapterVerse ccv(parseline.lines);
    /*
       We have noticed people editing Bibledit's data directly. 
       If this was done with OpenOffice, and then saving it as text again, 
       three special bytes were added right at the beginning of the file, 
       making the \c x marker not being parsed correctly. 
       It would then look as if this is chapter 0.
       In addition to this, the user could have edited the chapter number.
       If a change in the chapter number is detected, ask the user what to do.
     */
    unsigned int chapter = current_reference.chapter_get();
    unsigned int chapter_in_text = chapter;
    for (unsigned int i = 0; i < ccv.chapter.size(); i++) {
      if (ccv.chapter[i] != chapter) {
        chapter_in_text = ccv.chapter[i];
      }
    }
    // Ask what to do if the chapter number in the text differs from the 
    // chapter that has been loaded.
    if (chapter_in_text != chapter) {
      unsigned int confirmed_chapter_number;
      ustring message;
      message = _("Chapter ") + convert_to_string(chapter) + _(" was loaded, but it appears that the chapter number has been changed to ") + convert_to_string(chapter_in_text) + _(".\nDo you wish to save the text as a different chapter, that is, as chapter ") + convert_to_string(chapter_in_text) + "?";
      if (gtkw_dialog_question(NULL, message.c_str()) == GTK_RESPONSE_YES) {
        confirmed_chapter_number = chapter_in_text;
        reload = true;
        reload_chapter_number = chapter_in_text;
        check_chapter_cache = false;
      } else {
        confirmed_chapter_number = chapter;
      }
      for (unsigned int i = 0; i < ccv.chapter.size(); i++) {
        ccv.chapter[i] = confirmed_chapter_number;
      }
      // Check whether the new chapter number already exists.
      if (confirmed_chapter_number != chapter) {
        vector < unsigned int >chapters = project_get_chapters(project, current_reference.book_get());
        set < unsigned int >chapter_set(chapters.begin(), chapters.end());
        if (chapter_set.find(confirmed_chapter_number) != chapter_set.end()) {
          message = _("The new chapter number already exists\nDo you wish to overwrite it?");
          if (gtkw_dialog_question(NULL, message.c_str()) == GTK_RESPONSE_NO) {
            gtkw_dialog_info(NULL, _("The changes have been discarded"));
            save_action_is_over = true;
            reload = true;
          }
        }
      }
    }

    // Check the chapter cache with the version on disk. 
    // The chapter cache contains the chapter data when it was loaded in the editor.
    // Normally cache and disk are the same.
    // In case of collaboration, the text on disk may differ from the text in the chapter cache.
    if ((!save_action_is_over) && check_chapter_cache) {
      // The code is disabled, because it at times reverts changes entered in the editor.
      //vector <ustring> file_data = project_retrieve_chapter (project, book, chapter);
      //if (loaded_chapter_lines != file_data) {
        //merge_editor_and_file (loaded_chapter_lines, parseline.lines, project, book, chapter);
        //save_action_is_over = true;
        //reload = true;
      //}
    }

    // Store chapter.
    if (!save_action_is_over) {
      project_store_chapter(project, current_reference.book_get(), ccv);
      save_action_is_over = true;
    }
  }

  // Store size of editor actions. Based on this it knows next time whether to save the chapter.
  editor_actions_size_at_no_save = actions_done.size();

  // Possible reload signal.
  if (reload) {
    gtk_button_clicked(GTK_BUTTON(reload_signal));
  }
}


ustring Editor3::text_get_selection()
// Retrieves the selected text from the textview that has the focus, 
// and returns it as USFM code.
{
  ustring text;
  GtkTextIter startiter, enditer;
  gtk_text_buffer_get_selection_bounds(textbuffer, &startiter, &enditer);
  text = usfm_get_text(textbuffer, startiter, enditer);
  return text;
}


void Editor3::insert_text(const ustring &text)
// This inserts plain or USFM text at the cursor location of the focused textview.
// If text is selected, this is erased first.
{
  // If the text is not editable, bail out.
  if (!editable) { return; }

  // Erase selected text.
  gtk_text_buffer_delete_selection(textbuffer, true, editable);
  // Insert the text at the cursor.
  gtk_text_buffer_insert_at_cursor (textbuffer, text.c_str(), -1);
}


void Editor3::show_quick_references()
// Starts the process to show the quick references.
// A delaying routine is used to make the program more responsive.
// That is, the quick references are not shown at each change,
// but only shortly after. 
// Without this pasting a long text in the footnote takes a lot of time.
{
  gw_destroy_source(event_id_show_quick_references);
  event_id_show_quick_references = g_timeout_add_full(G_PRIORITY_DEFAULT, 200, GSourceFunc(show_quick_references_timeout), gpointer(this), NULL);
}


bool Editor3::show_quick_references_timeout(gpointer user_data)
{
  ((Editor3 *) user_data)->show_quick_references_execute();
  return false;
}


void Editor3::show_quick_references_execute()
// Takes the text of the references in the note that has the cursor,
// and shows that text in the quick reference area.
{
  // Clear the event id.
  event_id_show_quick_references = 0;
  
  // If we're not in a note, bail out.
  if (last_focused_type() != etvtNote) { return; }

  // Get the text of the focused note.
  GtkTextIter startiter, enditer;
  gtk_text_buffer_get_start_iter(notetextbuffer, &startiter);
  gtk_text_buffer_get_end_iter(notetextbuffer, &enditer);
  gchar *txt = gtk_text_buffer_get_text(notetextbuffer, &startiter, &enditer, true);
  ustring note_text = txt;
  g_free(txt); // Postiff: plug memory leak because above allocates mem, then ustring copies from it

  // Get the language of the project.
  extern Settings *settings;
  ProjectConfiguration *projectconfig = settings->projectconfig(project);
  ustring language = projectconfig->language_get();

  // Extract references.
  ReferencesScanner refscanner(language, current_reference.book_get(), note_text);

  // If there are no references, bail out.
  if (refscanner.references.empty()) {
    return;
  }

  // Make the references available and fire a signal.
  quick_references = refscanner.references;
  gtk_button_clicked(GTK_BUTTON(quick_references_button));
}


void Editor3::on_textview_move_cursor(GtkTextView * textview, GtkMovementStep step, gint count, gboolean extend_selection, gpointer user_data)
{
  ((Editor3 *) user_data)->textview_move_cursor(textview, step, count);
}

// TODO: According to https://developer.gnome.org/gtk3/stable/GtkTextView.html#GtkTextView-move-cursor,
// "applications should not connect" to the move-cursor signal. But it is an integral component
// of how things are done in Bibledit as of 4/25/2016.
void Editor3::textview_move_cursor(GtkTextView * textview, GtkMovementStep step, gint count)
{
  //DEBUG("Signal move_cursor step="+std::to_string(int(step))+" count="+std::to_string(int(count)))
  //ustring debug_verse_number = verse_number_get();
  //DEBUG("debug_verse_number "+debug_verse_number)

  // Clear the character style that was going to be applied when the user starts typing.
  character_style_on_start_typing.clear();
  // Keep postponing the actual handler if a new cursor movement was detected before the previous one was processed.
  gw_destroy_source(textview_move_cursor_id);
  textview_move_cursor_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, GSourceFunc(on_textview_move_cursor_delayed), gpointer(this), NULL);
  // Act on paragraph crossing.
//  paragraph_crossing_act (step, count);
  //vector <GtkWidget *> textviews = editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW);
  scroll_to_insertion_point_on_screen(GTK_WIDGET(textview));
  highlightCurrVerse(GTK_WIDGET(textview));
}

bool Editor3::on_textview_move_cursor_delayed(gpointer user_data)
{
  ((Editor3 *) user_data)->textview_move_cursor_delayed();
  return false;
}


void Editor3::textview_move_cursor_delayed()
// Handle cursor movement.
{
  //DEBUG("Signal move_cursor_delayed")
  //ustring debug_verse_number = verse_number_get();
  //DEBUG("debug_verse_number "+debug_verse_number)
  textview_move_cursor_id = 0;
  signal_if_styles_changed();
  DEBUG("2 Calling signal_if_verse_changed");
  signal_if_verse_changed(); // for SOME cursor moves the verse will change; for no regular letter keystrokes
}

ustring Editor3::verse_number_get()
// Returns the verse number of the insertion point.
{
  // Default verse number.
  ustring number = "1"; // was "0"
  if (focused_textview) {
    // Get an iterator at the cursor location of the focused textview.
    GtkTextIter iter;
    GtkTextBuffer * focused_textbuffer = gtk_text_view_get_buffer (focused_textview);
    gtk_text_buffer_get_iter_at_mark(focused_textbuffer, &iter, gtk_text_buffer_get_insert(focused_textbuffer));
    // Get verse number.
    number = get_verse_number_at_iterator(iter, Style::get_verse_marker(project), project);
    //DEBUG("current_verse_number="+current_verse_number+" and returning number="+number)
  }
  return number;
}

ustring Editor3::get_verse_number_at_iterator(GtkTextIter iter, const ustring & verse_marker, const ustring & project)
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
//  GtkWidget * textview = NULL;
// TODO: Go carefully more through this logic
  verse_number_found = get_verse_number_at_iterator_internal (iter, verse_marker, verse_number);

#ifdef OLDSTUFF
  
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
#endif
  
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


bool Editor3::get_iterator_at_verse_number (const ustring& verse_number, const ustring& verse_marker, GtkTextIter & iter, GtkWidget *& textview, bool deep_search)
// This returns the iterator and textview where "verse_number" starts.
// Returns true if the verse was found, else false.
{
    // Search from start of buffer.
    gtk_text_buffer_get_start_iter (textbuffer, &iter);
    // Verse 0 or empty: beginning of chapter.
    if ((verse_number == "0") || (verse_number.empty())) {
      return true;
    }
    // Go through the buffer and find out about the verse.
    do {
      ustring paragraph_style, character_style, verse_at_iter;
      get_styles_at_iterator(iter, paragraph_style, character_style);
      if (character_style == verse_marker) {
        verse_at_iter = get_verse_number_at_iterator(iter, verse_marker, "");
        if (verse_number == verse_at_iter) {
          return true;
        }
      }
      // Optionally do a deep search: whether the verse is in a sequence or range of verses.
      if (deep_search && !verse_at_iter.empty()) {
        unsigned int verse_int = convert_to_int(verse_at_iter);
        vector <unsigned int> combined_verses = verse_range_sequence(verse_number);
        for (unsigned int i2 = 0; i2 < combined_verses.size(); i2++) {
          if (verse_int == combined_verses[i2]) {
            return true;
          }
        }
      }
    } while (gtk_text_iter_forward_char(&iter));
  
  // If we haven't done the deep search yet, do it now.
  if (!deep_search) {
    if (get_iterator_at_verse_number (verse_number, verse_marker, iter, textview, true)) {
      return true;
    }
  }
  // Verse was not found.
  return false;
}

void Editor3::on_textview_grab_focus(GtkWidget * widget, gpointer user_data)
{
  ((Editor3 *) user_data)->textview_grab_focus(widget);
}


void Editor3::textview_grab_focus(GtkWidget * widget)
{
  //DEBUG("Signal grab_focus")
  // Store the paragraph action that created the widget

  focused_textview = GTK_TEXT_VIEW(widget); // on switch from text to note window, probably need to swap this.
  // In Editor3, following should be true, since there are only two textviews
  assert( (focused_textview == GTK_TEXT_VIEW(textview)) || (focused_textview == GTK_TEXT_VIEW(notetextview)) );  

  // Clear the character style that was going to be applied when the user starts typing.
  character_style_on_start_typing.clear();
  // Further processing of the focus grab is done with a delay. Some of these events will be "ignored" if multiple 
  // changes of focus happen within a very short time, so that only the last one should be done.
  gw_destroy_source(textview_grab_focus_event_id);
  textview_grab_focus_event_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 10, GSourceFunc(on_textview_grab_focus_delayed), gpointer(this), NULL);
}

bool Editor3::on_textview_grab_focus_delayed(gpointer data)
{
  ((Editor3 *) data)->textview_grab_focus_delayed();
  return false;
}


void Editor3::textview_grab_focus_delayed() // Todo
/*
 If the user clicks in the editor window, 
 and straight after that the position of the cursor is requested, 
 we get the position where the cursor was prior to clicking. 
 This delayed handler solves that.
 */
{
  //DEBUG("Signal grab_focus_delayed")
  textview_grab_focus_event_id = 0;
  signal_if_styles_changed();
  DEBUG("3 Calling signal_if_verse_changed");
  signal_if_verse_changed();
  show_quick_references();
}


void Editor3::undo()
{
  // If edits were made, the last action on the stack is the OneActionBoundary.
  // Undo the actions on the stack, and stop at the second OneActionBoundary.
  unsigned int one_action_boundaries_encountered = 0;
  while (can_undo() && (one_action_boundaries_encountered <= 1)) {
    EditorAction * action = actions_done[actions_done.size() - 1];
    if (action->type == eatOneActionBoundary) {
      one_action_boundaries_encountered++;
    }
    if (one_action_boundaries_encountered <= 1) {
      apply_editor_action (action, eaaUndo);
    }
  }
}


void Editor3::redo()
{
  // Redo the actions on the stack till we encounter the OneActionBoundary.
  unsigned int one_action_boundaries_encountered = 0;
  while (can_redo() && (one_action_boundaries_encountered == 0)) {
    EditorAction * action = actions_undone [actions_undone.size() - 1];
    if (action->type == eatOneActionBoundary) {
      one_action_boundaries_encountered++;
    }
    apply_editor_action (action, eaaRedo);
  }
}


bool Editor3::can_undo()
{
  bool undoable_actions_available = false;
  if (!actions_done.empty()) {
    EditorAction * last_action = actions_done[actions_done.size() - 1];
    undoable_actions_available = (last_action->type != eatLoadChapterBoundary);
  }
  return undoable_actions_available;
}


bool Editor3::can_redo()
{
  return !actions_undone.empty();
}


void Editor3::font_set()
{
  vector <GtkWidget *> textviews = editor_get_widgets (vbox_paragraphs);
  for (unsigned int i = 0; i < textviews.size(); i++) {
    set_font_textview (textviews[i]);
  }
}


void Editor3::set_font_textview (GtkWidget * textview)
{
  // Set font.
  PangoFontDescription *font_desc = NULL;
  extern Settings *settings;
  ProjectConfiguration *projectconfig = settings->projectconfig(project);
  if (!projectconfig->editor_font_default_get()) {
    font_desc = pango_font_description_from_string(projectconfig->editor_font_name_get().c_str());
  }
  gtk_widget_modify_font(textview, font_desc);
  if (font_desc)
    pango_font_description_free(font_desc);

  // Set the colours.
  if (projectconfig->editor_default_color_get()) {
    color_widget_default(textview);
  } else {
    color_widget_set(textview, projectconfig->editor_normal_text_color_get(), projectconfig->editor_background_color_get(), projectconfig->editor_selected_text_color_get(), projectconfig->editor_selection_color_get());
  }

  // Set predominant text direction.
  if (projectconfig->right_to_left_get()) {
    gtk_widget_set_direction(textview, GTK_TEXT_DIR_RTL);
  } else {
    gtk_widget_set_direction(textview, GTK_TEXT_DIR_LTR);
  }
}


bool Editor3::on_save_timeout(gpointer data)
{
  return ((Editor3 *) data)->save_timeout();
}


bool Editor3::save_timeout()
{
  chapter_save();
  return true;
}


gboolean Editor3::on_motion_notify_event(GtkWidget * textview, GdkEventMotion * event, gpointer user_data)
{
  return ((Editor3 *) user_data)->motion_notify_event(textview, event);
}


gboolean Editor3::motion_notify_event(GtkWidget * textview, GdkEventMotion * event)
// Update the cursor image if the pointer moved. 
{
  gint x, y;
  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(textview), GTK_TEXT_WINDOW_WIDGET, gint(event->x), gint(event->y), &x, &y);
  GtkTextIter iter;
  gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(textview), &iter, x, y);
  ustring paragraph_style, character_style;
  get_styles_at_iterator(iter, paragraph_style, character_style);
  bool hand_cursor = character_style.find (note_starting_style ()) != string::npos;
  if (hand_cursor != previous_hand_cursor) {
    GdkCursor *cursor;
    if (hand_cursor) {
      cursor = gdk_cursor_new(GDK_HAND2);
    } else {
      cursor = gdk_cursor_new(GDK_XTERM);
    }
    gdk_window_set_cursor(gtk_text_view_get_window(GTK_TEXT_VIEW(textview), GTK_TEXT_WINDOW_TEXT), cursor);
    gdk_cursor_unref (cursor);
  }
  previous_hand_cursor = hand_cursor;
  gdk_window_get_pointer (gtk_widget_get_window (textview), NULL, NULL, NULL);
  return false;
}


gboolean Editor3::on_textview_button_press_event(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
  return ((Editor3 *) user_data)->textview_button_press_event(widget, event);
}


gboolean Editor3::textview_button_press_event(GtkWidget * widget, GdkEventButton * event)
{
  // See whether the user clicked on a note caller.
  if (event->type == GDK_BUTTON_PRESS) {
    // Get iterator at clicking location.
    GtkTextIter iter;
    gint x, y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_WIDGET, gint(event->x), gint(event->y), &x, &y);
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(widget), &iter, x, y);
    // Check whether this is a note caller.
    ustring paragraph_style, character_style;
    get_styles_at_iterator(iter, paragraph_style, character_style);
    if (character_style.find (note_starting_style ()) != string::npos) {
      // Focus the note paragraph that has this identifier.
      EditorActionCreateNoteParagraph * note_paragraph = note2paragraph_action (character_style);
      if (note_paragraph) {
        give_focus (note_paragraph->textview);
      }
      // Do not propagate the button press event.
      return true;
    }
  }

  // Process this event with a delay.
  gw_destroy_source(textview_button_press_event_id);
  textview_button_press_event_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, GSourceFunc(on_textview_button_press_delayed), gpointer(this), NULL);

  // Double-clicking sends the word to Toolbox.
  if (event->type == GDK_2BUTTON_PRESS) {

    // Get the textbuffer.
    GtkTextBuffer *textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));

    // Get the word.
    GtkTextIter enditer;
    GtkTextIter startiter;
    gtk_text_buffer_get_iter_at_mark(textbuffer, &enditer, gtk_text_buffer_get_insert(textbuffer));
    if (!gtk_text_iter_ends_word(&enditer))
      gtk_text_iter_forward_word_end(&enditer);
    startiter = enditer;
    gtk_text_iter_backward_word_start(&startiter);
    // Do not include note markers.
    GtkTextIter moved_enditer;
    if (move_end_iterator_before_note_caller_and_validate (startiter, enditer, moved_enditer)) {
      gchar *txt = gtk_text_buffer_get_text(textbuffer, &startiter, &moved_enditer, false);
      word_double_clicked_text = txt;
      g_free(txt); // Postiff: plug memory leak
  
      // Signal to have it sent to Toolbox.
      gtk_button_clicked(GTK_BUTTON(word_double_clicked_signal));
    }
  }

  // Propagate the event.
  return false;
}

void Editor3::create_or_update_formatting_data()
// Create and fill the text tag table for all the formatted views.
// If already there, update it.
{
  DEBUG("Called")
  // If there is no text tag table, create a new one.
  if (!texttagtable) {
    texttagtable = gtk_text_tag_table_new();
  }

  // Get the stylesheet.
  ustring stylesheet = stylesheet_get_actual ();

  // Get the font size multiplication factor.
  PangoFontDescription *font_desc = NULL;
  extern Settings *settings;
  ProjectConfiguration *projectconfig = settings->projectconfig(project);
  if (!projectconfig->editor_font_default_get()) {
    font_desc = pango_font_description_from_string(projectconfig->editor_font_name_get().c_str());
    gint fontsize = pango_font_description_get_size(font_desc) / PANGO_SCALE;
    font_size_multiplier = (double)fontsize / 12;
    pango_font_description_free(font_desc);
  }
  // Create the unknown style.
  {
    Style style("", unknown_style(), false);
    create_or_update_text_style(&style, false, true, font_size_multiplier);
  }

  // Get all the Styles.
  extern Styles *styles;
  Usfm *usfm = styles->usfm(stylesheet);

  // Handle the known styles.
  // The priorities of the character styles should be higher than those of the 
  // paragraph styles. 
  // Therefore paragraph styles are created first, then the other ones.
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    // Get the properties of this style.
    bool paragraphstyle = Style::get_paragraph(usfm->styles[i].type, usfm->styles[i].subtype);
    bool plaintext = Style::get_plaintext(usfm->styles[i].type, usfm->styles[i].subtype);
    // Create a text style, only paragraph styles.
    if (paragraphstyle)
      create_or_update_text_style(&(usfm->styles[i]), true, plaintext, font_size_multiplier);
  }
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    // Get the properties of this style.
    bool paragraphstyle = Style::get_paragraph(usfm->styles[i].type, usfm->styles[i].subtype);
    bool plaintext = Style::get_plaintext(usfm->styles[i].type, usfm->styles[i].subtype);
    // Create a text style, the non-paragraph styles.
    if (!paragraphstyle)
      create_or_update_text_style(&(usfm->styles[i]), false, plaintext, font_size_multiplier);
  }

  // Special handling for the verse style, whether it restarts the paragraph.
  for (unsigned int i = 0; i < usfm->styles.size(); i++) {
    if (usfm->styles[i].type == stVerseNumber) {
      verse_restarts_paragraph = usfm->styles[i].userbool1;
    }
  }
}


void Editor3::create_or_update_text_style(Style * style, bool paragraph, bool plaintext, double font_multiplier)
// This creates or updates a GtkTextTag with the data stored in "style".
// The fontsize of the style is calculated by the value as stored in "style", and multiplied by "font_multiplier".
{
  // DEBUG("Called")
  // Take the existing tag, or create a new one and add it to the tagtable.
  GtkTextTag *tag = gtk_text_tag_table_lookup(texttagtable, style->marker.c_str());
  if (!tag) {
    tag = gtk_text_tag_new(style->marker.c_str());
    gtk_text_tag_table_add(texttagtable, tag);
    g_object_unref(tag);
  }
  // Optionally handle plain-text style and return.
  if (plaintext) {
    GValue gvalue = G_VALUE_INIT;
    g_value_init(&gvalue, G_TYPE_STRING);
    int fontsize = (int)(12 * font_multiplier);
    ustring font = "Courier " + convert_to_string(fontsize);
    g_value_set_string(&gvalue, font.c_str());
    g_object_set_property(G_OBJECT(tag), "font", &gvalue);
    g_value_unset(&gvalue);
    return;
  }
  // Fontsize.
  if (paragraph) {
    GValue gvalue = G_VALUE_INIT;
    g_value_init(&gvalue, G_TYPE_DOUBLE);
    double fontsize = style->fontsize * font_multiplier;
    g_value_set_double(&gvalue, fontsize);
    g_object_set_property(G_OBJECT(tag), "size-points", &gvalue);
    g_value_unset(&gvalue);
  }
  // Italic, bold, underline, smallcaps can be ON, OFF, INHERIT, or TOGGLE.
  // Right now, INHERIT is taken as OFF, and TOGGLE is interpreted as ON.
  // Improvements might be needed.
  {
    PangoStyle pangostyle = PANGO_STYLE_NORMAL;
    if ((style->italic == ON) || (style->italic == TOGGLE))
      pangostyle = PANGO_STYLE_ITALIC;
    GValue gvalue = G_VALUE_INIT;
    g_value_init(&gvalue, PANGO_TYPE_STYLE);
    g_value_set_enum(&gvalue, pangostyle);
    g_object_set_property(G_OBJECT(tag), "style", &gvalue);
    g_value_unset(&gvalue);
  }
  {
    PangoWeight pangoweight = PANGO_WEIGHT_NORMAL;
    if ((style->bold == ON) || (style->bold == TOGGLE))
      pangoweight = PANGO_WEIGHT_BOLD;
    GValue gvalue = G_VALUE_INIT;
    g_value_init(&gvalue, PANGO_TYPE_WEIGHT);
    g_value_set_enum(&gvalue, pangoweight);
    g_object_set_property(G_OBJECT(tag), "weight", &gvalue);
    g_value_unset(&gvalue);
  }
  {
    PangoUnderline pangounderline = PANGO_UNDERLINE_NONE;
    if ((style->underline == ON) || (style->underline == TOGGLE))
      pangounderline = PANGO_UNDERLINE_SINGLE;
    GValue gvalue = G_VALUE_INIT;
    g_value_init(&gvalue, PANGO_TYPE_UNDERLINE);
    g_value_set_enum(&gvalue, pangounderline);
    g_object_set_property(G_OBJECT(tag), "underline", &gvalue);
    g_value_unset(&gvalue);
  }
  {
    /*
       The small caps variant has not yet been implemented in Pango.
       PangoVariant pangovariant = PANGO_VARIANT_NORMAL;
       if ((style->smallcaps == ON) || (style->smallcaps == TOGGLE))
       pangovariant = PANGO_VARIANT_NORMAL;
       GValue gvalue = {0,};
       g_value_init (&gvalue, PANGO_TYPE_VARIANT);
       g_value_set_enum (&gvalue, pangovariant);
       g_object_set_property (G_OBJECT (tag), "variant", &gvalue);
       g_value_unset (&gvalue);
     */
    if ((style->smallcaps == ON) || (style->smallcaps == TOGGLE)) {
      double percentage = (double)0.6 * font_multiplier;
      GValue gvalue = G_VALUE_INIT;
      g_value_init(&gvalue, G_TYPE_DOUBLE);
      g_value_set_double(&gvalue, percentage);
      g_object_set_property(G_OBJECT(tag), "scale", &gvalue);
      g_value_unset(&gvalue);
    }
  }

  /*
     Superscript.
     Make size of verse or indeed any superscript equal to around 70% of normal font.
     Top of verse number should be even with top of capital T.
   */
  if (style->superscript) {
    // Rise n pixels.
    {
      gint rise = 6 * PANGO_SCALE;
      GValue gvalue = G_VALUE_INIT;
      g_value_init(&gvalue, G_TYPE_INT);
      g_value_set_int(&gvalue, rise);
      g_object_set_property(G_OBJECT(tag), "rise", &gvalue);
      g_value_unset(&gvalue);
    }
    // Smaller size.
    {
      double percentage = 0.7;
      GValue gvalue = G_VALUE_INIT;
      g_value_init(&gvalue, G_TYPE_DOUBLE);
      g_value_set_double(&gvalue, percentage);
      g_object_set_property(G_OBJECT(tag), "scale", &gvalue);
      g_value_unset(&gvalue);
    }
  }
  // Styles that occur in paragraphs only, not in character styles.  
  if (paragraph) {

    GtkJustification gtkjustification;
    if (style->justification == CENTER) {
      gtkjustification = GTK_JUSTIFY_CENTER;
    } else if (style->justification == RIGHT) {
      gtkjustification = GTK_JUSTIFY_RIGHT;
    } else if (style->justification == JUSTIFIED) {
      // Gtk+ supports this from version 2.12.
      gtkjustification = GTK_JUSTIFY_LEFT;
      if (GTK_MAJOR_VERSION >= 2)
        if (GTK_MINOR_VERSION >= 12)
          gtkjustification = GTK_JUSTIFY_FILL;
    } else {                    // Default is LEFT.
      gtkjustification = GTK_JUSTIFY_LEFT;
    }
    {
      GValue gvalue = G_VALUE_INIT;
      g_value_init(&gvalue, GTK_TYPE_JUSTIFICATION);
      g_value_set_enum(&gvalue, gtkjustification);
      g_object_set_property(G_OBJECT(tag), "justification", &gvalue);
      g_value_unset(&gvalue);
    }

    // For property "pixels-above/below-...", only values >= 0 are valid.
    if (style->spacebefore > 0) {
      gint spacebefore = (gint) (4 * style->spacebefore);
      GValue gvalue = G_VALUE_INIT;
      g_value_init(&gvalue, G_TYPE_INT);
      g_value_set_int(&gvalue, spacebefore);
      g_object_set_property(G_OBJECT(tag), "pixels-above-lines", &gvalue);
      g_value_unset(&gvalue);
    }

    if (style->spaceafter > 0) {
      gint spaceafter = (gint) (4 * style->spaceafter);
      GValue gvalue = G_VALUE_INIT;
      g_value_init(&gvalue, G_TYPE_INT);
      g_value_set_int(&gvalue, spaceafter);
      g_object_set_property(G_OBJECT(tag), "pixels-below-lines", &gvalue);
      g_value_unset(&gvalue);
    }

    {
      gint leftmargin = (gint) (4 * style->leftmargin);
      // A little left margin is desired to make selecting words easier.
      leftmargin += 5;
      GValue gvalue = G_VALUE_INIT;
      g_value_init(&gvalue, G_TYPE_INT);
      g_value_set_int(&gvalue, leftmargin);
      g_object_set_property(G_OBJECT(tag), "left-margin", &gvalue);
      g_value_unset(&gvalue);
    }

    if (style->rightmargin > 0) {
      gint rightmargin = (gint) (4 * style->rightmargin);
      GValue gvalue = G_VALUE_INIT;
      g_value_init(&gvalue, G_TYPE_INT);
      g_value_set_int(&gvalue, rightmargin);
      g_object_set_property(G_OBJECT(tag), "right-margin", &gvalue);
      g_value_unset(&gvalue);
    }

    {
      gint firstlineindent = (gint) (4 * style->firstlineindent);
      GValue gvalue = G_VALUE_INIT;
      g_value_init(&gvalue, G_TYPE_INT);
      g_value_set_int(&gvalue, firstlineindent);
      g_object_set_property(G_OBJECT(tag), "indent", &gvalue);
      g_value_unset(&gvalue);
    }

  }

  {
    GdkColor color;
    color_decimal_to_gdk(style->color, &color);
    GValue gvalue = G_VALUE_INIT;
    g_value_init(&gvalue, GDK_TYPE_COLOR);
    g_value_set_boxed(&gvalue, &color);
    g_object_set_property(G_OBJECT(tag), "foreground-gdk", &gvalue);
    g_value_unset(&gvalue);
  }
}


void Editor3::on_buffer_insert_text_after(GtkTextBuffer * textbuffer, GtkTextIter * pos_iter, gchar * text, gint length, gpointer user_data)
{
  // The "length" parameter does not give the length as the number of characters 
  // but the byte length of the "text" parameter. It is not passed on.
  ((Editor3 *) user_data)->buffer_insert_text_after(textbuffer, pos_iter, text);
}


void Editor3::buffer_insert_text_after(GtkTextBuffer * textbuffer, GtkTextIter * pos_iter, gchar * text)
// This function is called after the default handler has inserted the text.
// At this stage "pos_iter" points to the end of the inserted text.
{
  // If text buffers signals are to be disregarded, bail out.
  if (disregard_text_buffer_signals) {
    return;
  }
  
  // Text to work with.
  ustring utext (text);
  
  // New lines in notes are not supported.
  if (focused_textview == GTK_TEXT_VIEW(notetextview)) {
    replace_text (utext, "\n", " ");
  }
  //DEBUG("utext="+utext)
  // Get offset of text insertion point.
  gint text_insertion_offset = gtk_text_iter_get_offset (pos_iter) - utext.length();

  //----------------------------------------------------------------------------------
  // 1. Figure out what character style (= USFM codes) should be applied to the new text
  //----------------------------------------------------------------------------------
  
  // Variable for the character style that the routines below indicate should be applied to the inserted text.
  ustring character_style_to_be_applied;

  //----------------------------------------------------------------------------------
  // STEP 1.1
  //----------------------------------------------------------------------------------
  // If text is inserted right BEFORE where a character style was in effect,
  // the GtkTextBuffer does not apply any style to that text.
  // The user expects that the character style that applies to the insertion point
  // l be applied to the new text as well. 
  // not extend a note style.
  if (character_style_to_be_applied.empty()) {
    ustring paragraph_style;
    GtkTextIter iter = *pos_iter;
    get_styles_at_iterator(iter, paragraph_style, character_style_to_be_applied);
	//DEBUG("paragraph_style="+paragraph_style+" and character_style_to_be_applied="+character_style_to_be_applied)
    if (character_style_to_be_applied.find (note_starting_style ()) == 0) {
      character_style_to_be_applied.clear();
    }
  }

  //----------------------------------------------------------------------------------
  // STEP 1.2
  //----------------------------------------------------------------------------------
  // If text is inserted right AFTER where a character style is in effect,
  // the user expects this character style to be used for the inserted text as well.
  // This does not happen automatically in the GtkTextBuffer.
  // The code below cares for it.
  // Do not extend a note style.
  if (character_style_to_be_applied.empty()) {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset (textbuffer, &iter, text_insertion_offset);
    if (gtk_text_iter_backward_char (&iter)) {
      ustring paragraph_style;
      get_styles_at_iterator(iter, paragraph_style, character_style_to_be_applied);
    }
    if (character_style_to_be_applied.find (note_starting_style ()) == 0) {
      character_style_to_be_applied.clear();
    }
  }

  //----------------------------------------------------------------------------------
  // STEP 1.3
  //----------------------------------------------------------------------------------
  // When a character style has been previously applied, and then the user starts typing,
  // he expects that this style is going to be applied to the text he types.
  // The code below cares for that.
  if (character_style_to_be_applied.empty()) {
    character_style_to_be_applied = character_style_on_start_typing;
    character_style_on_start_typing.clear();
  }

  //----------------------------------------------------------------------------------
  // STEP 1.4: Handle the end of a verse number
  //----------------------------------------------------------------------------------
  if (!character_style_to_be_applied.empty()) {
    ustring verse_style = Style::get_verse_marker(project);
    if (character_style_to_be_applied == verse_style) {
      gunichar character = g_utf8_get_char(text);
      // When the cursor is at a verse, and the user types a space,
      // he wishes to stop the verse and start normal text.
      if (g_unichar_isspace(character)) {
        character_style_to_be_applied.clear();
      }
      // When the cursor is after a verse, and the user types anything
      // but a numeral, a hyphen, or a comma, it means he wishes to stop the verse.
      if (!g_unichar_isdigit(character) && (character != '-') && (character != ',')) {
        character_style_to_be_applied.clear();
      }
    }
  }

#ifdef OLD_STUFF
  // Remove the text that was inserted into the textbuffer.
  // Then, it needs to be inserted through Editor Actions.
  // This is for the Undo and Redo system.
  disregard_text_buffer_signals++;
  GtkTextIter startiter = *pos_iter;
  gtk_text_iter_backward_chars (&startiter, utext.length());
  gtk_text_buffer_delete (textbuffer, &startiter, pos_iter);
  disregard_text_buffer_signals--;
#endif
  
  //----------------------------------------------------------------------------------
  // STEP 2: Apply the style
  //----------------------------------------------------------------------------------
  // Previously, the text was removed, then added again.
  // Instead, apply the style to the new text.
  // Then record what we have done for the undo system.
  GtkTextIter startiter = *pos_iter;
  gtk_text_iter_backward_chars (&startiter, utext.length());
  gtk_text_buffer_apply_tag_by_name(textbuffer, character_style_to_be_applied.c_str(), &startiter, pos_iter);

#ifdef TODO
  // If there are one or more backslashes in the text, then USFM code is being entered.
  // Else treat it as if the user is typing text.
  if (utext.find ("\\") != string::npos) {
    // Load USFM code.
//DEBUG("about to text_load, text="+ustring(text))
//DEBUG("about to text_load, utext="+utext)
//DEBUG("about to text_load, character_style_to_be_applied="+character_style_to_be_applied)
      // TODO: below is an error...can't use text_load, because that does the whole chapter!!!
    text_load (text, character_style_to_be_applied, false);
  } else {
    // Load plain text. We used to "handle new lines as well" but what was done was to split into multiple textviews.
    //size_t newlineposition = utext.find("\n");

      if (!utext.empty()) {
        text_load (utext, character_style_to_be_applied, false);
        character_style_to_be_applied.clear();
      }
#endif

#ifdef OLD_STUFF
      // Get markup after insertion point. New paragraph.
      ustring paragraph_style = unknown_style ();
      vector <ustring> text;
      vector <ustring> styles;        
      if (focused_paragraph) {
        paragraph_style = focused_paragraph->style;
        EditorActionDeleteText * delete_action = paragraph_get_text_and_styles_after_insertion_point(focused_paragraph, text, styles);
        if (delete_action) {
          apply_editor_action (delete_action);
        }
      }      
      editor_start_new_standard_paragraph (paragraph_style);
      // Transfer anything from the previous paragraph to the new one.
      gint initial_offset = editor_paragraph_insertion_point_get_offset (focused_paragraph);
      gint accumulated_offset = initial_offset;
      for (unsigned int i = 0; i < text.size(); i++) {
        EditorActionInsertText * insert_action = new EditorActionInsertText (this, focused_paragraph, accumulated_offset, text[i]);
        apply_editor_action (insert_action);
        if (!styles[i].empty()) {
          EditorActionChangeCharacterStyle * style_action = new EditorActionChangeCharacterStyle(this, focused_paragraph, styles[i], accumulated_offset, text[i].length());
          apply_editor_action (style_action);
        }
        accumulated_offset += text[i].length();
      }
      // Move insertion points to the proper position.
      editor_paragraph_insertion_point_set_offset (focused_paragraph, initial_offset);
      // Remove the part of the input text that has been handled.
#endif

  //}
  
  // Insert the One Action boundary.
  apply_editor_action (new EditorAction (this, eatOneActionBoundary));

  // The pos_iter variable that was passed to this function was invalidated because text was removed and added.
  // Here it is validated again. This prevents critical errors within GTK.
  gtk_text_buffer_get_iter_at_offset (textbuffer, pos_iter, text_insertion_offset);
}


void Editor3::on_buffer_delete_range_before(GtkTextBuffer * textbuffer, GtkTextIter * start, GtkTextIter * end, gpointer user_data)
{
  ((Editor3 *) user_data)->buffer_delete_range_before(textbuffer, start, end);
}


void Editor3::buffer_delete_range_before(GtkTextBuffer * textbuffer, GtkTextIter * start, GtkTextIter * end)
{
  // Bail out if we don't care about textbuffer signals.
  if (disregard_text_buffer_signals) {
    return;
  }
  disregard_text_buffer_signals++;
  //DEBUG("Delete range before - setting textbuffer_delete_range_was_fired")
  textbuffer_delete_range_was_fired = true;

  // Record the content that is about to be deleted.
  get_text_and_styles_between_iterators(start, end, text_to_be_deleted, styles_to_be_deleted);

  // Make the end iterator the same as the start iterator, so that nothing gets deleted.
  // It will get deleted through EditorActions, so that Undo and Redo work.
  * end = * start;

  // Care about textbuffer signals again.
  disregard_text_buffer_signals--;
}


void Editor3::on_buffer_delete_range_after(GtkTextBuffer * textbuffer, GtkTextIter * start, GtkTextIter * end, gpointer user_data)
{
  ((Editor3 *) user_data)->buffer_delete_range_after(textbuffer, start, end);
}


void Editor3::buffer_delete_range_after(GtkTextBuffer * textbuffer, GtkTextIter * start, GtkTextIter * end)
{
  // Bail out if we don't care about textbuffer signals.
  if (disregard_text_buffer_signals) { return; }
  disregard_text_buffer_signals++;
  
  //DEBUG("Delete range after - about to delete text 'after'")
  // Delete the text.  
  
  ustring text;
  // TODO: Need documentation on where text_to_be_deleted comes from, who initializes it, etc.
  // Same with styles_to_be_deleted.
  for (unsigned int i = 0; i < text_to_be_deleted.size(); i++) {
      text.append (text_to_be_deleted[i]);
  }
  gint offset = gtk_text_iter_get_offset (start);
  //TODO: EditorActionDeleteText * delete_action = new EditorActionDeleteText(this, focused_paragraph, offset, text.length());
  // apply_editor_action (delete_action);
  
  // If there are any notes among the deleted text, delete these notes as well.
  for (unsigned int i = 0; i < styles_to_be_deleted.size(); i++) {
    if (styles_to_be_deleted[i].find (note_starting_style ()) == 0) {
      EditorActionCreateNoteParagraph * paragraph_action = note2paragraph_action (styles_to_be_deleted[i]);
      if (paragraph_action) {
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter (paragraph_action->textbuffer, &iter);
        gint length = gtk_text_iter_get_offset (&iter);
        apply_editor_action (new EditorActionDeleteText (this, paragraph_action, 0, length));
        apply_editor_action (new EditorActionDeleteParagraph(this, paragraph_action));
      }
    }
  }
  
  // Clear data that was used to find out what to delete.
  text_to_be_deleted.clear();
  styles_to_be_deleted.clear();

  // Insert the One Action boundary.
  apply_editor_action (new EditorAction (this, eatOneActionBoundary));

  // Care about textbuffer signals again.
  disregard_text_buffer_signals--;

  // Turn off this mode
  textbuffer_delete_range_was_fired = false;
}


void Editor3::signal_if_styles_changed() // Todo
{
  set < ustring > styles = get_styles_at_cursor();
  if (styles != styles_at_cursor) {
    styles_at_cursor = styles;
    if (new_styles_signal) {
      gtk_button_clicked(GTK_BUTTON(new_styles_signal));
    }
  }
}


set < ustring > Editor3::get_styles_at_cursor()
// Gets all the styles that apply to the cursor, or to the selection.
{
  // The styles.
  set <ustring> styles;
  GtkTextBuffer *focused_textbuffer = gtk_text_view_get_buffer (focused_textview);
  // Carry on if there's a focused textbuffer.
  if (focused_textbuffer) {
    // Get the iterators at the selection bounds of the focused textview.
    GtkTextIter startiter, enditer;
    gtk_text_buffer_get_selection_bounds(focused_textbuffer, &startiter, &enditer);

    // Get the applicable styles.
    // This is done by getting the names of the styles between these iterators.
    GtkTextIter iter = startiter;
    do {
      ustring paragraphstyle, characterstyle;
      get_styles_at_iterator(iter, paragraphstyle, characterstyle);
      if (!paragraphstyle.empty()) {
        styles.insert(paragraphstyle);
      }
      if (!characterstyle.empty()) {
        styles.insert(characterstyle);
      }
      gtk_text_iter_forward_char(&iter);
    } while (gtk_text_iter_in_range(&iter, &startiter, &enditer));

  }

  // Return the list.
  return styles;
}

// TODO: I think this is unused here an in editor.cpp
set < ustring > Editor3::styles_at_iterator(GtkTextIter iter)
// Get all the styles that apply at the iterator.
{
  set < ustring > styles;
  ustring paragraph_style, character_style;
  get_styles_at_iterator(iter, paragraph_style, character_style);
  if (!paragraph_style.empty()) {
    styles.insert(paragraph_style);
  }
  if (!character_style.empty()) {
    styles.insert(character_style);
  }
  return styles;
}


GtkTextBuffer *Editor3::last_focused_textbuffer()
// Returns the focused textbuffer, or NULL if none.
{
  if (focused_textview) {
    GtkTextBuffer *focused_textbuffer = gtk_text_view_get_buffer (focused_textview);
    return focused_textbuffer;
  }
  return NULL;
}

// TODO: Don't know what to do here
EditorTextViewType Editor3::last_focused_type()
// Returns the type of the textview that was focused most recently.
// This could be the main body of text, or a note, (or a table, but we don't support tables right now in Editor3).
{ 
    if (focused_textview == GTK_TEXT_VIEW(textview)) { return etvtBody; }
    else if (focused_textview == GTK_TEXT_VIEW(notetextview)) { return etvtNote; }
}


void Editor3::apply_style(const ustring & marker)
/*
 It applies the style of "marker" to the text.
 If it is a paragraph style, it applies the style throughout the paragraph.
 If it is a character style, it only applies it on the selected text,
 or on the cursor position, or on the word that the cursor is in.
 Applying character styles works as a toggle: if the style is not there,
 it will be put there. If it was there, it will be removed.
 */
{
  // Get the type and subtype of the marker.
  StyleType type;
  int subtype;
  Style::marker_get_type_and_subtype(project, marker, type, subtype);

  /*
  Get the type of textview that was focused last, 
  and find out whether the style that is now going to be inserted
  is applicable in this particular textview.
  For example to a table only the relevant table styles can be applied. 
  Give a message if there is a mismatch.
   */
  EditorTextViewType textviewtype = last_focused_type();
  bool style_application_ok = true;
  ustring style_application_fault_reason;
  switch (textviewtype) {
    case etvtBody:
    {
      if ((type == stFootEndNote) || (type == stCrossreference) || (type == stTableElement)) {
        style_application_ok = false;
      }
      style_application_fault_reason = _("Trying to apply a ");
      if (type == stFootEndNote)
        style_application_fault_reason.append(_("foot- or endnote"));
      if (type == stCrossreference)
        style_application_fault_reason.append(_("crossreference"));
      if (type == stTableElement)
        style_application_fault_reason.append(_("table"));
      style_application_fault_reason.append(_(" style"));
      break;
    }
    case etvtNote:
    {
      if ((type != stFootEndNote) && (type != stCrossreference)) {
        style_application_ok = false;
      }
      style_application_fault_reason = _("Only note related styles apply");
      break;
    }
    case etvtTable:
    {
      gw_warning(_("USFM tables are not supported in the experimental editor (Editor3)."));
      // Check that only a table element is going to be inserted.
      if (type != stTableElement) {
        style_application_ok = false;
        style_application_fault_reason = _("Only table styles apply");
        break;
      }
      // Check that only a style with the right column number is going to be applied.
      ustring stylesheet = stylesheet_get_actual();
      extern Styles *styles;
      Usfm *usfm = styles->usfm(stylesheet);
      for (unsigned int i = 0; i < usfm->styles.size(); i++) {
        if (marker == usfm->styles[i].marker) {
          /*
          unsigned int column = usfm->styles[i].userint1;
          if (column > 5) {
            style_application_ok = false;
            style_application_fault_reason = _("Table column number mismatch");
            break;
          }
          */
        }
      }
      break;
    }
  }
  // Whether there's a paragraph to apply the style to.
#ifdef OLD_STUFF
  if (!focused_paragraph) {
    style_application_ok = false;
    style_application_fault_reason = _("Cannot find paragraph");
  }
#endif
  // If necessary give error message.
  if (!style_application_ok) {
    ustring message(_("This style cannot be applied here."));
    message.append("\n\n");
    message.append(style_application_fault_reason);
    message.append(".");
    gtkw_dialog_error(NULL, message.c_str());
    return;
  }

  // Get the active textbuffer and textview.
    GtkTextBuffer *focused_textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(focused_textview));

#ifdef OLD_STUFF
  if (Style::get_starts_new_line_in_editor(type, subtype)) {
    // Handle a paragraph style.
    apply_editor_action (new EditorActionChangeParagraphStyle (this, marker, focused_paragraph));
  } else {
#endif
    // Handle a character style.
    // Find the iterator at the start and at the end of the text to be put in this style.
    GtkTextIter iter, startiter, enditer;
    // If some text has been selected, take that.
    if (gtk_text_buffer_get_selection_bounds(focused_textbuffer, &startiter, &enditer)) {
    } else {
      // No selection:
      // If the insertion point is inside a word, take that word.  
      // Else just take the insertion point.
      gtk_text_buffer_get_iter_at_mark(focused_textbuffer, &iter, gtk_text_buffer_get_insert(focused_textbuffer));
      startiter = iter;
      enditer = iter;
      if (gtk_text_iter_inside_word(&iter) && !gtk_text_iter_starts_word(&iter)) {
        gtk_text_iter_backward_word_start(&startiter);
        gtk_text_iter_forward_word_end(&enditer);
      }
    }
    // Check whether the character style that we are going to 
    // apply has been applied already throughout the range.
    iter = startiter;
    bool character_style_already_applied = true;
    do {
      ustring paragraph_style, character_style;
      get_styles_at_iterator(iter, paragraph_style, character_style);
      if (character_style != marker)
        character_style_already_applied = false;
      GtkTextIter iter2 = iter;
      gtk_text_iter_forward_char(&iter2);
      gtk_text_iter_forward_char(&iter);
    } while (gtk_text_iter_in_range(&iter, &startiter, &enditer));
    // If the character style was applied already, toggle it.
    ustring style (marker);
    if (character_style_already_applied) {
      style.clear();
    }
    // Apply the new character style
    gint startoffset = gtk_text_iter_get_offset (&startiter);
    gint endoffset = gtk_text_iter_get_offset (&enditer);
    //TODO: EditorActionChangeCharacterStyle * style_action = new EditorActionChangeCharacterStyle(this, focused_paragraph, style, startoffset, endoffset - startoffset);
    //apply_editor_action (style_action);
    // Store this character style so it can be re-used when the user starts typing text.
    character_style_on_start_typing = style;
#ifdef OLD_STUFF
  }
#endif
  
  // One Action boundary.
  apply_editor_action (new EditorAction (this, eatOneActionBoundary));

  // Update gui.
  signal_if_styles_changed();

  // Focus editor.
  give_focus(textview);
}


void Editor3::insert_note(const ustring & marker, const ustring & rawtext)
/*
 Inserts a note in the editor.
 marker:    The marker that starts the note, e.g. "fe" for an endnote.
 rawtext:   The raw text of the note, e.g. "+ Mat 1.1.". Note that this excludes
 the note opener and note closer. It has only the caller and the
 USFM code of the note body.
 */
{
  ustring usfmcode;
  usfmcode.append (usfm_get_full_opening_marker (marker));
  usfmcode.append (rawtext);
  usfmcode.append (usfm_get_full_closing_marker (marker));
  //DEBUG("usfmcode="+usfmcode)
  //DEBUG("Inserting note at cursor of focused paragraph")
  gtk_text_buffer_insert_at_cursor (gtk_text_view_get_buffer(focused_textview), usfmcode.c_str(), -1);
}


void Editor3::insert_table(const ustring & rawtext)
// Inserts a table in the editor.
// rawtext: The raw text of the table, e.g. "\tr \tc1 \tc2 \tc3 \tc4 ".
{
  gw_warning(_("USFM tables are not supported in the experimental editor (Editor3)."));
  gtk_text_buffer_insert_at_cursor (gtk_text_view_get_buffer(focused_textview), rawtext.c_str(), -1);
}


void Editor3::on_textbuffer_changed(GtkTextBuffer * textbuffer, gpointer user_data)
{
  ((Editor3 *) user_data)->textbuffer_changed(textbuffer);
}


void Editor3::textbuffer_changed(GtkTextBuffer * textbuffer)
{
  spelling_trigger();
}


void Editor3::highlight_searchwords()
// Highlights all the search words.
{
  // Destroy optional previous object.
  if (highlight) { 
	delete highlight;
	highlight = NULL;
  }

#ifdef OLD_STUFF
  // Bail out if there's no focused paragraph.
  if (!focused_paragraph) {
    return;
  }
#endif
  // This code is why the highlighting is slow. It does all the
  // operations essentially backward, waiting on a new thread to get
  // started and to its work before doing anything else. Thread
  // creation is a fairly very high overhead operation. As a result,
  // this method is guaranteed to introduce more delay in the process
  // of figuring out and marking the highlighted text than it would to
  // just do it. See below.
/*
  // Highlighting timeout.
  if (highlight_timeout_event_id) {
    gw_destroy_source (highlight_timeout_event_id);
  }
  highlight_timeout_event_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 500, GSourceFunc(on_highlight_timeout), gpointer(this), NULL);

  // Create a new highlighting object.
  highlight = new Highlight(focused_paragraph->textbuffer, focused_paragraph->textview, project, reference_tag, current_verse_number);
  // New g_thread_new ("highlight", GThreadFunc (highlight_thread_start), gpointer(this));
  g_thread_create(GThreadFunc(highlight_thread_start), gpointer(this), false, NULL);
*/
  DEBUG("W9.1 about to highlight search words");  
  // MAP: Here's how I think it should be done, more synchronously instead of threaded.
  highlight = new Highlight(this, gtk_text_view_get_buffer(focused_textview), GTK_WIDGET(focused_textview), project, reference_tag, current_verse_number);
  // The time-consuming part of highlighting is to determine what bits
  // of text to highlight. Because it takes time, and the program
  // should continue to respond, it is [was] done in a thread. MAP:
  // But there is a problem when you type text, delete, then start
  // typing again. The threaded-ness of this means that sometimes,
  // bibledit starts overwriting text from the beginning of the verse
  // instead of typing in the location where the cursor is. TODO: I am
  // planning eventually to FIX the TIME CONSUMING PART so that the
  // whole thing will be more efficient.
  DEBUG("W9.2 about to determine locations");  
  highlight->determine_locations();
  DEBUG("W9.3 about to assert");  
  assert(highlight->locations_ready);
  DEBUG("W9.4 about to highlight->highlight");  
  highlight->highlight();
  DEBUG("W9.5 about delete highlight");
  // Delete and NULL the object making it ready for next use.
  delete highlight;
  highlight = NULL;
}


bool Editor3::on_highlight_timeout(gpointer data)
{
  return ((Editor3 *) data)->highlight_timeout();
}


bool Editor3::highlight_timeout()
{
  // If the highlighting object is not there, destroy timer and bail out.
  if (!highlight) {
    return false;
  }
  // Proceed if the locations for highlighting are ready.
  if (highlight->locations_ready) {
    highlight->highlight();
    // Delete and NULL the object making it ready for next use.
    delete highlight;
    highlight = NULL;
  }
  // Timer keeps going.
  return true;
}


void Editor3::highlight_thread_start(gpointer data)
{
  ((Editor3 *) data)->highlight_thread_main();
}


void Editor3::highlight_thread_main()
{
  // The time-consuming part of highlighting is to determine what bits of text
  // to highlight. Because it takes time, and the program should continue
  // to respond, it is done in a thread. MAP 2015: But there is a problem when you type
  // text, delete, then start typing again, the threaded-ness of this means that
  // sometimes, be starts overwriting text from the beginning of the verse instead
  // of typing in the location where the cursor is. MAP 4/7/2016: This routine may not
  // actually be the problem. Keystroke press and release (like on Backspace) is not handled
  // properly.
  if (highlight) {
    highlight->determine_locations();
  }
}

void Editor3::spelling_trigger()
{
  if (project.empty())
    return;
  extern Settings *settings;
  ProjectConfiguration *projectconfig = settings->projectconfig(project);
  if (!projectconfig)
    return;
  if (!projectconfig->spelling_check_get())
    return;
  gw_destroy_source(spelling_timeout_event_id);
  spelling_timeout_event_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 1000, GSourceFunc(on_spelling_timeout), gpointer(this), NULL);
}


bool Editor3::on_spelling_timeout(gpointer data)
{
  ((Editor3 *) data)->spelling_timeout();
  return false;
}


void Editor3::spelling_timeout()
{
  // Clear event id.
  spelling_timeout_event_id = 0;

  // Check spelling of all active textviews.
  vector <GtkWidget *>
  textviews = editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW);
  for (unsigned int i = 0; i < textviews.size(); i++) {
    GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textviews[i]));
    spellingchecker->check(textbuffer);
  }

  // Signal spelling checked.
  gtk_button_clicked (GTK_BUTTON (spelling_checked_signal));
}


void Editor3::on_button_spelling_recheck_clicked(GtkButton * button, gpointer user_data)
{
  ((Editor3 *) user_data)->spelling_timeout();
}


void Editor3::load_dictionaries()
{
  extern Settings *settings;
  ProjectConfiguration *projectconfig = settings->projectconfig(project);
  if (projectconfig->spelling_check_get()) {
    spellingchecker->set_dictionaries(projectconfig->spelling_dictionaries_get());
  }
}


vector <ustring> Editor3::spelling_get_misspelled ()
{
  // Collect the misspelled words.
  vector <ustring> words;
  vector <GtkWidget *>
  textviews = editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW);
  for (unsigned int i = 0; i < textviews.size(); i++) {
    GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textviews[i]));
    vector <ustring> words2 = spellingchecker->get_misspellings(textbuffer);
    for (unsigned int i2 = 0; i2 < words2.size(); i2++) {
      words.push_back (words2[i2]);
    }
  }
  // Remove double ones.
  set <ustring> wordset (words.begin (), words.end());
  words.clear();
  words.assign (wordset.begin (), wordset.end());
  // Give result.
  return words;  
}


void Editor3::spelling_approve (const vector <ustring>& words)
{
  // Approve all the words in the list.
  // Since this may take time, a windows will show the progress.
  ProgressWindow progresswindow (_("Adding words to dictionary"), false);
  progresswindow.set_iterate (0, 1, words.size());
  for (unsigned int i = 0; i < words.size(); i++)  {
    progresswindow.iterate ();
    spellingchecker->add_to_dictionary (words[i].c_str());
  }
  // Trigger a new spelling check.
  spelling_trigger();
}


bool Editor3::move_cursor_to_spelling_error (bool next, bool extremity)
// Move the cursor to the next (or previous) spelling error.
// Returns true if it was found, else false.
{
  bool moved = false;
  moved = spellingchecker->move_cursor_to_spelling_error (gtk_text_view_get_buffer(focused_textview), next, extremity);
#ifdef OLDSTUFF
  if (focused_paragraph) {
    //GtkTextBuffer * textbuffer = focused_paragraph->textbuffer;
    //vector <GtkWidget *> textviews = editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW);
    do {
      moved = spellingchecker->move_cursor_to_spelling_error (gtk_text_view_get_buffer(focused_textview), next, extremity);

      if (!moved) {
        GtkWidget * textview = focused_paragraph->textview;
        textbuffer = NULL;
        if (next) {
          textview = editor_get_next_textview (textviews, textview);
        } else {
          textview = editor_get_previous_textview (textviews, textview);
        }
        if (textview) {
          give_focus (textview);
          textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
          GtkTextIter iter;
          if (next) 
            gtk_text_buffer_get_start_iter (textbuffer, &iter);
          else
            gtk_text_buffer_get_end_iter (textbuffer, &iter);
          gtk_text_buffer_place_cursor (textbuffer, &iter);
        }
      }
    } while (!moved && textbuffer);
#endif

//  }
    if (moved) {
        scroll_to_insertion_point_on_screen(textview);
        highlightCurrVerse(textview);
    }
    
  return moved;
}

// Maybe textview could be the textview or the notetextview
void Editor3::scroll_to_insertion_point_on_screen(GtkWidget * textview)
{
    //DEBUG("doVerseHighlighting="+std::to_string(int(doVerseHighlighting)))
	//ustring debug_verse_number = verse_number_get();
	//DEBUG("debug_verse_number "+debug_verse_number)
//	if (!focused_paragraph) { return; }
//	if (!focused_paragraph->textbuffer) { return; }

	// Ensure that the screen has been fully displayed.
	//while (gtk_events_pending()) { gtk_main_iteration(); }
	// MAP 8/19/2016 commented out above line: I suspect that
	// sometimes we are here processing an event, but if we wait
	// until there are no events, we will be stuck in an infinite
	// loop. This change did not solve the Warao Psalms lockup
	// when changing from USFM view back to formatted view.

	// Adjustment.
	//GtkAdjustment * adjustment = gtk_viewport_get_vadjustment (GTK_VIEWPORT (viewport));
	GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(scrolledwindow));

	// Visible window height.
	gdouble visible_window_height = gtk_adjustment_get_page_size (adjustment);

	// Total window height.
	gdouble total_window_height = gtk_adjustment_get_upper (adjustment);

	// Formerly, we would get all the textviews.
    // Now, we take that as an incoming argument so the vector doesn't have to be
    // rebuilt scrom scratch as frequently.

    // TESTING: Does this help with scrolling to the right position, and not 
    // jumping to "verse 0" at some random times? The theory is that gtk_widget_get_allocation
    // is not returning the right sizes because the widgets have not been fully drawn yet.
    // Somehow, a timeout handler handled this in a previous iteration.
    while (gtk_events_pending()) { gtk_main_iteration(); }
    
    // Offset of insertion point starting from top.
    gint insertion_point_offset = 0;
    {
        GtkTextMark * gtktextmark = gtk_text_buffer_get_insert (textbuffer);
        //GtkTextMark * gtktextmark = gtk_text_buffer_get_mark (textbuffer, "insert");
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_mark (textbuffer, &iter, gtktextmark);
        GdkRectangle rectangle;
        gtk_text_view_get_iter_location (GTK_TEXT_VIEW (textview), &iter, &rectangle);
        gint window_x, window_y;
        gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (textview), GTK_TEXT_WINDOW_WIDGET, rectangle.x, rectangle.y, &window_x, &window_y);
        insertion_point_offset += rectangle.y;
    }

	DEBUG("Insertion point offset is " + std::to_string(int(insertion_point_offset)))
	DEBUG("Visible window height is "  + std::to_string(double(visible_window_height)))
	DEBUG("Total window height is "    + std::to_string(double(total_window_height)))
	
	// Set the adjustment to move the insertion point into 1/3th of
	// the visible part of the window. TODO: This should be an option
	// that the user can set. Sometimes it is distracting to have the
	// text move automatically. In Emacs, for instance, the user can
	// hit Ctrl-L to do that manually. We could have a preference that
	// says "auto-scroll text window to center 1/3 of window" or
	// something like that. This code slows the perceived user
	// experience because they have to reorient their eyes to where
	// the text moves to. Certainly when the cursor moves out of the
	// window then we need to auto-scroll by some amount, but it is
	// debatable whether we should auto-center the text or just move
	// the window by a line or two. I've attempted to do the latter, but
	// it is not perfect at the moment, so there is still some more TODO.

	/*
	If the insertion point is at 800, and the height of the visible window is 500,
	and the total window height is 1000, then the calculation of the offset is as follows:
	Half the height of the visible window is 250.
	Insertion point is at 800, so the start of the visible window should be at 800 - 250 = 550.
	Therefore the adjustment should move to 550.
	The adjustment value should stay within its limits. If it exceeds these, the viewport draws double lines.
	*/
	
	//gdouble adjustment_value = insertion_point_offset - (visible_window_height * 0.33);
	// While working within a viewport, we will not scroll
	gdouble adjustment_value = gtk_adjustment_get_value(adjustment);
	DEBUG("adjustment_value " + std::to_string(double(adjustment_value)))
	if (insertion_point_offset < adjustment_value+20) {
		adjustment_value = insertion_point_offset - 60;
	}
	else if (insertion_point_offset > adjustment_value + visible_window_height - 20) {
		adjustment_value = insertion_point_offset - visible_window_height + 60;
	}
	//DEBUG("adjustment_value " + std::to_string(double(adjustment_value)))
	if (adjustment_value < 0) {
		adjustment_value = 0;
	}
	else if (adjustment_value > (total_window_height - visible_window_height)) {
		adjustment_value = total_window_height - visible_window_height;
	}
	DEBUG("gtk_adjustment_set_value called with " + std::to_string(double(adjustment_value)))
	//DEBUG("adjustment->lower = " + std::to_string(double(adjustment->lower)))
	//DEBUG("adjustment->upper = " + std::to_string(double(adjustment->upper)))
	gtk_adjustment_set_value (adjustment, adjustment_value);
}

void Editor3::highlightCurrVerse(GtkWidget *textview)
{
	// Don't do unnecessary work: if verse is already highlighted, bail out
	if (current_verse_number == currHighlightedVerse) { return; }

	//DEBUG("Verse highlight was on "+currHighlightedVerse+" now to be "+current_verse_number)
	// Highlighting is not necessary if you just pressed a letter key within the same verse you were in. Arrow keys, yes.

	// Remove any previous verse number highlight.
	GtkTextIter startiter, enditer;
    GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
    gtk_text_buffer_get_start_iter (textbuffer, &startiter);
    gtk_text_buffer_get_end_iter (textbuffer, &enditer);
    gtk_text_buffer_remove_tag (textbuffer, verse_highlight_tag, &startiter, &enditer);
    
    // Highlight the verse if it is non-zero.
    if (current_verse_number != "0") {
        //GtkWidget * textview;
        GtkTextIter startiter, enditer;
        if (get_iterator_at_verse_number (current_verse_number, Style::get_verse_marker(project), startiter, textview)) {
            //GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
            enditer = startiter;
            gtk_text_iter_forward_chars (&enditer, current_verse_number.length());
            gtk_text_buffer_apply_tag (textbuffer, verse_highlight_tag, &startiter, &enditer);
        }
        currHighlightedVerse = current_verse_number;
    }
}

void Editor3::apply_editor_action (EditorAction * action, EditorActionApplication application)
{
  // An editor action is being applied.
  disregard_text_buffer_signals++;
  
  // Pointer to any widget that should grab focus.
  GtkWidget * widget_that_should_grab_focus = NULL;
  
  // Whether contents was been changed.
  bool contents_was_changed = false;
  
  // Deal with the action depending on its type.
  switch (action->type) {

    case eatCreateParagraph:
    {
        gw_warning("Create Paragraph in body?");
#ifdef OLD_STUFF
      EditorActionCreateParagraph * paragraph_action = static_cast <EditorActionCreateParagraph *> (action);
      switch (application) {
        case eaaInitial:
        {
          paragraph_action->apply(texttagtable, editable, focused_paragraph, widget_that_should_grab_focus);
          paragraph_create_actions (paragraph_action);
          break;
        }
        case eaaUndo: paragraph_action->undo (vbox_parking_lot, widget_that_should_grab_focus); break;
        case eaaRedo: paragraph_action->redo (widget_that_should_grab_focus); break;
      }
#endif
      break;
    }

    case eatChangeParagraphStyle:
    {
      EditorActionChangeParagraphStyle * style_action = static_cast <EditorActionChangeParagraphStyle *> (action);
      switch (application) {
        case eaaInitial: style_action->apply(widget_that_should_grab_focus); break;
        case eaaUndo:    style_action->undo (widget_that_should_grab_focus); break;
        case eaaRedo:    style_action->redo (widget_that_should_grab_focus); break;
      }
      break;
    }

    case eatInsertText:
    {
      EditorActionInsertText * insert_action = static_cast <EditorActionInsertText *> (action);
      switch (application) {
        case eaaInitial: insert_action->apply(widget_that_should_grab_focus); break;
        case eaaUndo:    insert_action->undo (widget_that_should_grab_focus); break;
        case eaaRedo:    insert_action->redo (widget_that_should_grab_focus); break;
      }
      contents_was_changed = true;
	  //DEBUG("Just did insert_action->apply")
      break;
    }

    case eatDeleteText:
    {
	  DEBUG("action DeleteText")
      EditorActionDeleteText * delete_action = static_cast <EditorActionDeleteText *> (action);
      switch (application) {
        case eaaInitial: delete_action->apply(widget_that_should_grab_focus); break;
        case eaaUndo:    delete_action->undo (widget_that_should_grab_focus); break;
        case eaaRedo:    delete_action->redo (widget_that_should_grab_focus); break;
      }
      contents_was_changed = true;
      break;
    }

    case eatChangeCharacterStyle:
    {
      EditorActionChangeCharacterStyle * style_action = static_cast <EditorActionChangeCharacterStyle *> (action);
      switch (application) {
        case eaaInitial: style_action->apply(widget_that_should_grab_focus); break;
        case eaaUndo:    style_action->undo (widget_that_should_grab_focus); break;
        case eaaRedo:    style_action->redo (widget_that_should_grab_focus); break;
      }
      break;
    }
    
    case eatLoadChapterBoundary:
    case eatOneActionBoundary:
    {
      break;
    }

    case eatDeleteParagraph:
    {
	  DEBUG("action DeleteParagraph")
      EditorActionDeleteParagraph * delete_action = static_cast <EditorActionDeleteParagraph *> (action);
      switch (application) {
        case eaaInitial: delete_action->apply(vbox_parking_lot, widget_that_should_grab_focus); break;
        case eaaUndo:    delete_action->undo (widget_that_should_grab_focus); break;
        case eaaRedo:    delete_action->redo (vbox_parking_lot, widget_that_should_grab_focus); break;
      }
      break;
    }

    case eatCreateNoteParagraph:
    {
        gw_warning("Create Paragraph in body?");
#ifdef OLD_STUFF
      EditorActionCreateNoteParagraph * paragraph_action = static_cast <EditorActionCreateNoteParagraph *> (action);
      switch (application) {
        case eaaInitial:
        {
          paragraph_action->apply(texttagtable, editable, focused_paragraph, widget_that_should_grab_focus);
          paragraph_create_actions (paragraph_action);
          break;
        }
        case eaaUndo: paragraph_action->undo (vbox_parking_lot, widget_that_should_grab_focus); break;
        case eaaRedo: paragraph_action->redo (widget_that_should_grab_focus); break;
      }
#endif
      break;
    }

  }

  // Put this action in the right list.
  switch (application) {
    case eaaInitial: action->apply(actions_done                ); break;
    case eaaUndo:    action->undo (actions_done, actions_undone); break;
    case eaaRedo:    action->redo (actions_done, actions_undone); break;
  }

  // If there's any widget that was earmarked to be focused, grab its focus.
  // This can only be done at the end when the whole object has been set up,
  // because the callback for grabbing the focus uses this object.
  if (widget_that_should_grab_focus) {
    give_focus (widget_that_should_grab_focus);
  }

  // Handle situation where the contents of the editor has been changed.
  if (contents_was_changed) {
    show_quick_references ();
    gtk_button_clicked(GTK_BUTTON(changed_signal));
  }
  
  // Applying the editor action is over.
  disregard_text_buffer_signals--;
}

void Editor3::textviewbuffer_create_actions (GtkTextBuffer *textbuffer, GtkWidget *textview)
{
  // Connect text buffer signals.
  g_signal_connect_after(G_OBJECT(textbuffer), "insert-text",  G_CALLBACK(on_buffer_insert_text_after),   gpointer(this));
  g_signal_connect      (G_OBJECT(textbuffer), "delete-range", G_CALLBACK(on_buffer_delete_range_before), gpointer(this));
  g_signal_connect_after(G_OBJECT(textbuffer), "delete-range", G_CALLBACK(on_buffer_delete_range_after),  gpointer(this));
  g_signal_connect      (G_OBJECT(textbuffer), "changed",      G_CALLBACK(on_textbuffer_changed),         gpointer(this));
  // Connect spelling checker.
  spellingchecker->attach(textview);
  // Connect text view signals.
  g_signal_connect_after((gpointer) textview, "move_cursor",         G_CALLBACK(on_textview_move_cursor),     gpointer(this));
  g_signal_connect      ((gpointer) textview, "motion-notify-event", G_CALLBACK(on_motion_notify_event),      gpointer(this));
  g_signal_connect_after((gpointer) textview, "grab-focus",          G_CALLBACK(on_textview_grab_focus),      gpointer(this));
  g_signal_connect      ((gpointer) textview, "key-press-event",     G_CALLBACK(on_textview_key_press_event), gpointer(this));
  //EXPERIMENTALg_signal_connect((gpointer) paragraph_action->textview, "key-release-event", G_CALLBACK(on_textview_key_release_event), gpointer(this));
  g_signal_connect      ((gpointer) textview, "button_press_event",  G_CALLBACK(on_textview_button_press_event), gpointer(this));

  // Signal the parent window to connect to the signals of the text view.
  new_widget_pointer = textview;
  gtk_button_clicked (GTK_BUTTON (new_widget_signal));
  // Extra bits to be done for a note.
#if 0
  if (type == eatCreateNoteParagraph) {
    // Cast the object to the right type.
    EditorActionCreateNoteParagraph * note_action = static_cast <EditorActionCreateNoteParagraph *> (paragraph_action);
    // Connect signal for note caller in note.
    g_signal_connect ((gpointer) note_action->eventbox, "button_press_event", G_CALLBACK (on_caller_button_press_event), gpointer (this));
}  
#endif
}
#ifdef OLD_STUFF
// About to be obsolete
void Editor3::paragraph_create_actions (EditorActionCreateParagraph * paragraph_action)
{
  // Connect text buffer signals.
  g_signal_connect_after(G_OBJECT(paragraph_action->textbuffer), "insert-text",  G_CALLBACK(on_buffer_insert_text_after),   gpointer(this));
  g_signal_connect      (G_OBJECT(paragraph_action->textbuffer), "delete-range", G_CALLBACK(on_buffer_delete_range_before), gpointer(this));
  g_signal_connect_after(G_OBJECT(paragraph_action->textbuffer), "delete-range", G_CALLBACK(on_buffer_delete_range_after),  gpointer(this));
  g_signal_connect      (G_OBJECT(paragraph_action->textbuffer), "changed",      G_CALLBACK(on_textbuffer_changed),         gpointer(this));
  // Connect spelling checker.
  spellingchecker->attach(paragraph_action->textview);
  // Connect text view signals.
  g_signal_connect_after((gpointer) paragraph_action->textview, "move_cursor",         G_CALLBACK(on_textview_move_cursor),     gpointer(this));
  g_signal_connect      ((gpointer) paragraph_action->textview, "motion-notify-event", G_CALLBACK(on_motion_notify_event),      gpointer(this));
  g_signal_connect_after((gpointer) paragraph_action->textview, "grab-focus",          G_CALLBACK(on_textview_grab_focus),      gpointer(this));
  g_signal_connect      ((gpointer) paragraph_action->textview, "key-press-event",     G_CALLBACK(on_textview_key_press_event), gpointer(this));
  //EXPERIMENTALg_signal_connect((gpointer) paragraph_action->textview, "key-release-event", G_CALLBACK(on_textview_key_release_event), gpointer(this));
  g_signal_connect      ((gpointer) paragraph_action->textview, "button_press_event",  G_CALLBACK(on_textview_button_press_event), gpointer(this));
  // Set font
  set_font_textview (paragraph_action->textview);
  // Signal the parent window to connect to the signals of the text view.
  new_widget_pointer = paragraph_action->textview;
  gtk_button_clicked (GTK_BUTTON (new_widget_signal));
  // Extra bits to be done for a note.
  if (paragraph_action->type == eatCreateNoteParagraph) {
    // Cast the object to the right type.
    EditorActionCreateNoteParagraph * note_action = static_cast <EditorActionCreateNoteParagraph *> (paragraph_action);
    // Connect signal for note caller in note.
    g_signal_connect ((gpointer) note_action->eventbox, "button_press_event", G_CALLBACK (on_caller_button_press_event), gpointer (this));
  }
}
#endif

void Editor3::editor_start_new_standard_paragraph (const ustring& marker_text)
// This function deals with a marker that starts a standard paragraph.
{
  // Create a new paragraph.
  EditorActionCreateParagraph * paragraph = new EditorActionCreateParagraph (this, vbox_paragraphs);
  apply_editor_action (paragraph); 

  // The new paragraph markup.
  EditorActionChangeParagraphStyle * style_action = new EditorActionChangeParagraphStyle (this, marker_text, paragraph);
  apply_editor_action (style_action);

  // Some styles insert their marker: Do that here if appropriate.
  StyleType type;
  int subtype;
  Style::marker_get_type_and_subtype(project, marker_text, type, subtype);
  if (Style::get_displays_marker(type, subtype)) {
    gint insertion_offset = editor_paragraph_insertion_point_get_offset (paragraph);
    EditorActionInsertText * insert_action = new EditorActionInsertText (this, paragraph, insertion_offset, usfm_get_full_opening_marker (marker_text));
    apply_editor_action (insert_action);
  }
}


Editor3::EditorActionCreateParagraph * Editor3::widget2paragraph_action (GtkWidget * widget)
// Given a pointer to a GtkTextView or HBox, it returns its (note) paragraph create action.
{
  for (unsigned int i = 0; i < actions_done.size(); i++) {
    EditorAction * action = actions_done[i];
    if ((action->type == eatCreateParagraph) || (action->type == eatCreateNoteParagraph)) {
      EditorActionCreateParagraph * paragraph_action = static_cast <EditorActionCreateParagraph *> (action);
      if (paragraph_action->textview == widget) {
        return paragraph_action;
      }
      if (action->type == eatCreateNoteParagraph) {
        EditorActionCreateNoteParagraph * note_paragraph_action = static_cast <EditorActionCreateNoteParagraph *> (action);
        if (note_paragraph_action->hbox == widget) {
          return paragraph_action;
        }
        if (note_paragraph_action->eventbox == widget) {
          return paragraph_action;
        }
      }
    }
  }
  return NULL;
}


Editor3::EditorActionCreateNoteParagraph * Editor3::note2paragraph_action (const ustring& note)
// Given a note identifier, it returns its note paragraph create action.
{
  for (unsigned int i = 0; i < actions_done.size(); i++) {
    EditorAction * action = actions_done[i];
    if (action->type == eatCreateNoteParagraph) {
      EditorActionCreateNoteParagraph * note_paragraph_action = static_cast <EditorActionCreateNoteParagraph *> (action);
      if (note_paragraph_action->identifier == note) {
        return note_paragraph_action;
      }
    }
  }
  return NULL;
}


ustring Editor3::usfm_get_text(GtkTextBuffer * textbuffer, GtkTextIter startiter, GtkTextIter enditer)
{
  // To hold the text it is going to retrieve.
  ustring text;
  
  // Initialize the iterator.
  GtkTextIter iter = startiter;

  // Paragraph and character styles.
  ustring previous_paragraph_style;
  ustring previous_character_style;

  // Iterate through the text.
  unsigned int iterations = 0;
  while (gtk_text_iter_compare(&iter, &enditer) < 0) {

    // Get the new paragraph and character style.
    // This is done by getting the names of the styles at this iterator.
    // With the way the styles are applied currently, the first 
    // style is a paragraph style, and the second style is optional 
    // and would be a character style.
    ustring new_paragraph_style;
    ustring new_character_style;
    get_styles_at_iterator(iter, new_paragraph_style, new_character_style);

    // Omit the starting paragraph marker except when at the start of a line.
    if (iterations == 0) {
      if (!gtk_text_iter_starts_line(&iter)) {
        previous_paragraph_style = new_paragraph_style;
      }
    }

    // Is it a note caller or normal text?
    if (new_character_style.find (note_starting_style ()) == 0) {
      
      // Note caller found. Retrieve its text.
      EditorActionCreateNoteParagraph * note_paragraph = note2paragraph_action (new_character_style);
      if (note_paragraph) {
        ustring note_text;
        // Add the note opener.
        note_text.append (usfm_get_full_opening_marker(note_paragraph->opening_closing_marker));
        // Add the usfm caller.
        note_text.append (note_paragraph->caller_usfm);
        note_text.append (" ");
        // Get the main note body.
        GtkTextBuffer * textbuffer = note_paragraph->textbuffer;
        GtkTextIter startiter, enditer;
        gtk_text_buffer_get_start_iter(textbuffer, &startiter);
        gtk_text_buffer_get_end_iter(textbuffer, &enditer);
        note_text.append (usfm_get_note_text(startiter, enditer, project));
        // Add the note closer.
        note_text.append (usfm_get_full_closing_marker(note_paragraph->opening_closing_marker));
        // Add the note to the main text.
        text.append (note_text);
		//DEBUG("Appended note_text="+note_text)
      }
      
    } else {
      
      // Normal text found.

      // Get the text at the iterator, and whether this is a linebreak.
      ustring new_character;
      bool line_break;
      {
        gunichar unichar = gtk_text_iter_get_char(&iter);
        gchar buf[7];
        gint length = g_unichar_to_utf8(unichar, (gchar *) & buf);
        buf[length] = '\0';
        new_character = buf;
        line_break = (new_character.find_first_of("\n\r") == 0);
        if (line_break) {
          new_character.clear();
		}
      }
  
      // Flags for whether styles are opening or closing.
      bool character_style_closing = false;
      bool paragraph_style_closing = false;
      bool paragraph_style_opening = false;
      bool character_style_opening = false;
  
      // Paragraph style closing.
      if (new_paragraph_style != previous_paragraph_style) {
        if (!previous_paragraph_style.empty()) {
          paragraph_style_closing = true;
        }
      }
      // If a new line is encountered, then the paragraph closes.
      if (line_break)
        paragraph_style_closing = true;
      // If the paragraph closes, then the character style, if open, should close too.
      if (paragraph_style_closing) {
        new_character_style.clear();
      }
      // Character style closing. 
      if (new_character_style != previous_character_style)
        if (!previous_character_style.empty())
          character_style_closing = true;
      // Paragraph style opening.
      if (new_paragraph_style != previous_paragraph_style)
        if (!new_paragraph_style.empty())
          paragraph_style_opening = true;
      // Character style opening.
      if (new_character_style != previous_character_style)
        if (!new_character_style.empty())
          character_style_opening = true;
  
      // Handle possible character style closing.
      if (character_style_closing) {
        usfm_internal_get_text_close_character_style(text, project, previous_character_style);
      }
      // USFM doesn't need anything if a paragraph style is closing.
      if (paragraph_style_closing) {
      }
      // Handle possible paragraph style opening.
      if (paragraph_style_opening) {
        usfm_internal_add_text(text, "\n");
        // We would need to add the USFM code to the text.
        // But in some cases the code is already in the text,
        // e.g. in the case of "\id JHN".
        // In such cases the code is fine already, so it does not need to be added anymore.
        // Accomodate cases such as \toc
        // These don't have the full marker as "\toc ", but only without the last space.
        ustring usfm_code = usfm_get_full_opening_marker(new_paragraph_style);
        GtkTextIter iter2 = iter;
        gtk_text_iter_forward_chars(&iter2, usfm_code.length());
        ustring usfm_code_in_text = gtk_text_iter_get_slice(&iter, &iter2);
        replace_text(usfm_code_in_text, "\n", " ");
        if (usfm_code_in_text.length() < usfm_code.length())
          usfm_code_in_text.append(" ");
        if (usfm_code != usfm_code_in_text) {
          // A space after an opening marker gets erased in USFM: move it forward.
          if (new_character == " ") {
            usfm_internal_add_text(text, new_character);
            new_character.clear();
          }
          // Don't insert the unknown style
          if (new_paragraph_style != unknown_style())
            usfm_internal_add_text(text, usfm_code);
        }
      }
      // Handle possible character style opening.
      if (character_style_opening) {
        // Get the type and the subtype.
        StyleType type;
        int subtype;
        Style::marker_get_type_and_subtype(project, new_character_style, type, subtype);
        // Normally a character style does not start a new line, but a verse (\v) does.
        if (Style::get_starts_new_line_in_usfm(type, subtype)) {
          usfm_internal_add_text(text, "\n");
        }
        // A space after an opening marker gets erased in USFM: move it forward.
        if (new_character == " ") {
          usfm_internal_add_text(text, new_character);
          new_character.clear();
        }
        usfm_internal_add_text(text, usfm_get_full_opening_marker(new_character_style));
      }
      // Store all styles for next iteration.
      previous_paragraph_style = new_paragraph_style;
      previous_character_style = new_character_style;
      if (paragraph_style_closing)
        previous_paragraph_style.clear();
      if (character_style_closing)
        previous_character_style.clear();
  
      // Store this character.
      usfm_internal_add_text(text, new_character);
      
    }

    // Next iteration.
    gtk_text_iter_forward_char(&iter);
    iterations++;
  }

  // If a character style has been applied to the last character or word 
  // in the buffer, the above code would not add the closing marker.
  // Thus we may be found to have text like \p New paragraph with \add italics
  // The \add* marker is missing. This violates the USFM standard.
  // The code below fixes that.
  if (!previous_character_style.empty()) {
    usfm_internal_get_text_close_character_style(text, project, previous_character_style);
  }
  
  // Return the text it got.
  return text;
}

void Editor3::get_next_note_caller_and_style (EditorNoteType type, ustring& caller, ustring& style, bool restart)
// Gets the next note caller style.
// Since note callers have sequential numbers, it needs another one for each note.
{
  switch (type) {
    case entFootnote:       caller = "f"; break;
    case entEndnote:        caller = "e"; break;
    case entCrossreference: caller = "x"; break;
  }
  note_style_num++;
  style = note_starting_style ();
  ustring notenum = convert_to_string (note_style_num);
  caller.append(notenum);
  style.append (notenum);
}

void Editor3::copy_clipboard_intelligently ()
// Copies the plain text to the clipboard, and copies both plain and usfm text to internal storage.
{
  GtkTextIter startiter, enditer;
  if (gtk_text_buffer_get_selection_bounds (gtk_text_view_get_buffer(focused_textview), &startiter, &enditer)) {
    clipboard_text_plain.clear();
    vector <ustring> text;
    vector <ustring> styles;
    get_text_and_styles_between_iterators(&startiter, &enditer, text, styles);
    for (unsigned int i = 0; i < text.size(); i++) {
      if (styles[i].find (note_starting_style()) == string::npos) {
        clipboard_text_plain.append (text[i]);
      }
    }
    clipboard_text_usfm = text_get_selection ();
    // If no plain text is put on the clipboard, but usfm text, then put something on the clipboard.
    // This facilitates copying of notes.
    if (clipboard_text_plain.empty()) {
      if (!clipboard_text_usfm.empty()) {
        clipboard_text_plain = usfm_clipboard_code ();
      }
    }
    // Put the plain text on the clipboard.
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text (clipboard, clipboard_text_plain.c_str(), -1);
  }
}


void Editor3::cut ()
// Cut to clipboard.
{
  if (editable) {
      // Copy the text to the clipboard in an intelligent manner.
      copy_clipboard_intelligently ();
      // Remove the text from the text buffer.
      gtk_text_buffer_delete_selection (gtk_text_view_get_buffer(focused_textview), true, editable);
  }
}


void Editor3::copy ()
// Copy to clipboard.
{
    // Copy the text to the clipboard in an intelligent manner.
    copy_clipboard_intelligently ();
}


void Editor3::paste ()
// Paste from clipboard.
{
  // Proceed if the Editor is editable and there's a focused paragraph where to put the text into.
  if (editable) {
      // Get the text that would be pasted.
      GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
      gchar * text = gtk_clipboard_wait_for_text (clipboard);
      if (text) {
        ustring utext (text);
        
        if ((utext == clipboard_text_plain) || (utext == usfm_clipboard_code ())) {
          // Since the text that would be pasted is the same as the plain text 
          // that results from the previous copy or cut operation, 
          // it inserts the equivalent usfm text instead.
          // Or if USFM code only was copied, nothing else, then take that too.
          gtk_text_buffer_delete_selection (gtk_text_view_get_buffer(focused_textview), true, editable);
          gtk_text_buffer_insert_at_cursor (gtk_text_view_get_buffer(focused_textview), clipboard_text_usfm.c_str(), -1);
        } else {
          // The text that would be pasted differs from the plain text
          // that resulted from the previous copy or cut operation,
          // so insert the text that would be pasted as it is.
          gtk_text_buffer_paste_clipboard(gtk_text_view_get_buffer(focused_textview), clipboard, NULL, true);
        }
        // Free memory.
        g_free (text);
      }
  }
}


gboolean Editor3::on_textview_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  return ((Editor3 *) user_data)->textview_key_press_event(widget, event);
}


gboolean Editor3::textview_key_press_event(GtkWidget *widget, GdkEventKey *event)
{
  // Fix for backspace may also have to do with if (disregard_text_buffer_signals) {
  // Clear flag for monitoring deletions from textbuffers.
  // MAP: WHY IS THIS HERE? messes up backspc and delete if random character is thrown in between press and release of backspace
  // because it causes the program to forget it is in the midst of a backspacing delete. This flag should only be reset
  // (set to false) when the delete range action is done. A random key press doesn't indicate that. This is probably here
  // for some other reason. Not sure why 4/7/2016.
  // textbuffer_delete_range_was_fired = false;
  keystrokeNum++;
  //DEBUG(ustring(gdk_keyval_name (event->keyval))+" keystroke="+std::to_string(unsigned(keystrokeNum)))

  // Store data for paragraph crossing control.
  paragraph_crossing_textview_at_key_press = widget;
  //GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_get_iter_at_mark(textbuffer, &paragraph_crossing_insertion_point_iterator_at_key_press, gtk_text_buffer_get_insert(textbuffer));
  //DEBUG("paragraph_crossing_insertion...="+std::to_string(int(gtk_text_iter_get_line_index(&paragraph_crossing_insertion_point_iterator_at_key_press))))

  if (!keyboard_any_cursor_move(event)) {
	// This handles the case where the insertion point is off the screen
	// and the user begins typing, as in after he slides the scroll bar so his work moves out of the viewport.
    //vector <GtkWidget *> textviews = editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW);
	scroll_to_insertion_point_on_screen(textview);
    // Usually after we call above, we highlight verses. This time, don't highlight verses. 
    // Subsequent cursor-move signal will do that if there was a cursor move and not a regular key press.
  }
  // Else, in the case of a cursor movement key, scroll_to_insertion will be called by the move-cursor handler.

  // We are monkeying with the move-cursor signal and are not supposed to be (see elsewhere in this file)
  //screen_scroll_to_iterator(GTK_TEXT_VIEW(widget), &paragraph_crossing_insertion_point_iterator_at_key_press);
  //textview_scroll_to_mark(GTK_TEXT_VIEW(widget), gtk_text_buffer_get_insert(textbuffer), false);
  
  // A GtkTextView has standard keybindings for clipboard operations.
  // It has been found that if the user presses, e.g. Ctrl-c, that
  // text is copied to the clipboard twice, or e.g. Ctrl-v, that it is
  // pasted twice. This is probably a bug in Gtk2. MAP 4/7/2016: I believe it 
  // probably is that the key press and key release are triggering an action in our code.
  // OR, that the textview has smarts in it to do these things so we don't have to,
  // but we thought we had to do so ourselves.
  // The relevant key bindings for clipboard operations are blocked here.
  // The default bindings for copying to clipboard are Ctrl-c and Ctrl-Insert.
  // The default bindings for cutting to clipboard are Ctrl-x and Shift-Delete.
  // The default bindings for pasting from clipboard are Ctrl-v and Shift-Insert.
  // TODO: Set up a switch statement to handle all these cases.
  if (keyboard_control_state(event)) {
    if (event->keyval == GDK_KEY_c) return true;
    if (event->keyval == GDK_KEY_x) return true;
    if (event->keyval == GDK_KEY_v) return true;
    if (keyboard_insert_pressed(event)) return true;
  }
  if (keyboard_shift_state(event)) {
    if (keyboard_delete_pressed(event)) return true;
    if (keyboard_insert_pressed(event)) return true;
  }

	// Pressing Page Up while the cursor is in the note brings the user
	// to the note caller in the text.
	if (keyboard_page_up_pressed(event)) {
		if (focused_textview == GTK_TEXT_VIEW(notetextview)) {
				on_caller_button_press (notetextview);
		}
	}

	// Handle pressing the Backspace key.
	if (keyboard_backspace_pressed (event) && editable && !textbuffer_delete_range_was_fired) {
		//DEBUG("Backspace pressed")
		// Determine if we are at the beginning of a paragraph because without this code,
		// backspace will get "stuck" and not be able to go back to the prior paragraph.
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_offset(textbuffer, &iter, 0);
		// Is above equivalent to a call to gtk_text_buffer_get_start_iter  ???
		  //DEBUG("paragraph_crossing_insertion...="+std::to_string(int(gtk_text_iter_get_line_index(&paragraph_crossing_insertion_point_iterator_at_key_press))))
		if (gtk_text_iter_equal(&iter, &paragraph_crossing_insertion_point_iterator_at_key_press)) {
			//DEBUG("It looks like backspace pressed at beginning of paragraph")
			//DEBUG("Combining paragraphs after backspace")
			EditorActionCreateParagraph * current_paragraph = widget2paragraph_action (widget);
			EditorActionCreateParagraph * preceding_paragraph = widget2paragraph_action (
					editor_get_previous_textview (
							editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW),
							widget));
			combine_paragraphs(preceding_paragraph, current_paragraph);
			return TRUE; // processing is finished
		}
	}
  
	// Handle pressing the Delete key.
	else if (keyboard_delete_pressed (event) && editable && !textbuffer_delete_range_was_fired) {
		//DEBUG("Delete pressed")
		// Determine if we are at the end of a paragraph because without this code,
		// delete will get "stuck" and not be able to join text from the next paragraph.
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_offset(textbuffer, &iter, -1);
		// I think above is equivalent to a call to gtk_text_buffer_get_end_iter  ???
		  //DEBUG("paragraph_crossing_insertion...="+std::to_string(int(gtk_text_iter_get_line_index(&paragraph_crossing_insertion_point_iterator_at_key_press))))
		if (gtk_text_iter_equal(&iter, &paragraph_crossing_insertion_point_iterator_at_key_press)) {
			//DEBUG("It looks like delete pressed at end of paragraph")
			if (gtk_text_buffer_get_selection_bounds(textbuffer, NULL, NULL)) {
				//DEBUG("Looks like there was a selection, however, so don't do anything...")
			}
			else {
				//DEBUG("Combining paragraphs after delete")
				EditorActionCreateParagraph * current_paragraph = widget2paragraph_action (widget);
				EditorActionCreateParagraph * following_paragraph = widget2paragraph_action (
						editor_get_next_textview (
								editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW),
								widget));
				// Next line makes sure cursor insertion point is set to beginning of second paragraph; else 
				// some or all of the paragraph will be deleted instead of copied to the first one (up to insertion point)
				editor_paragraph_insertion_point_set_offset (following_paragraph, 0);
				combine_paragraphs(current_paragraph, following_paragraph);
				return TRUE; // processing is finished...otherwise delete_range can be called and delete the verse number!
			}
		}
	}

  // Propagate event further.
  return FALSE;
}

// Helper function to implement backspace and delete at paragraph boundaries.
void Editor3::combine_paragraphs(EditorActionCreateParagraph * first_paragraph, EditorActionCreateParagraph * second_paragraph)
{
	//DEBUG("textbuffer_delete_range_was_fired=" + std::to_string(int(textbuffer_delete_range_was_fired)))
	
	if (first_paragraph && second_paragraph) {
		// Get the text and styles of the second paragraph.
		vector <ustring> text;
		vector <ustring> styles;
		// Note the text and styles are grabbed AFTER the insertion point. Thus we have to set the insertion point
		// to the start of the second paragraph to capture everything we need. The caller is responsible to do this.
		EditorActionDeleteText * delete_action = paragraph_get_text_and_styles_after_insertion_point(second_paragraph, text, styles);
		// Delete the text from the second paragraph.
		if (delete_action) {
			apply_editor_action (delete_action);
		}
		// Insert the second paragraph text at the end of the first paragraph.
		GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (first_paragraph->textview));
		GtkTextIter enditer;
		gtk_text_buffer_get_end_iter (textbuffer, &enditer);
		gint initial_offset = gtk_text_iter_get_offset (&enditer);
		for (unsigned int i = 0; i < text.size(); i++) {
			gtk_text_buffer_get_end_iter (textbuffer, &enditer);
			gint offset = gtk_text_iter_get_offset (&enditer);
			EditorActionInsertText * insert_action = new EditorActionInsertText (this, first_paragraph, offset, text[i]);
			apply_editor_action (insert_action);
			if (!styles[i].empty()) {
				EditorActionChangeCharacterStyle * style_action = new EditorActionChangeCharacterStyle(this, first_paragraph, styles[i], offset, text[i].length());
				apply_editor_action (style_action);
			}
		}
		// Move the insertion point to the position just before the joined text.
		editor_paragraph_insertion_point_set_offset (first_paragraph, initial_offset);
		// Focus the first paragraph. This must be done  before deleting the 
		// current_paragraph (which has focus), otherwise the blinking cursor is lost.
		give_focus (first_paragraph->textview);
		// Remove the second paragraph.
		apply_editor_action (new EditorActionDeleteParagraph(this, second_paragraph));
		// WAS HERE BUT LOST cursor: give_focus (first_paragraph->textview);
		// Insert the One Action boundary.
		apply_editor_action (new EditorAction (this, eatOneActionBoundary));
	}
}

bool Editor3::on_textview_button_press_delayed (gpointer user_data)
{
  ((Editor3 *) user_data)->textview_button_press_delayed();
  return false;
}


void Editor3::textview_button_press_delayed ()
{
  textview_button_press_event_id = 0;
  signal_if_styles_changed();
  DEBUG("4 Calling signal_if_verse_changed");
  signal_if_verse_changed(); // rarely will cause verse to change
}


void Editor3::switch_verse_tracking_off ()
{
  verse_tracking_on = false;
}


void Editor3::switch_verse_tracking_on ()
{
  verse_tracking_on = true;
}


void Editor3::go_to_verse(const ustring& number, bool focus)
// Moves the insertion point of the editor to the verse number.
{
  DEBUG("go_to_verse "+number+" current_verse_number was "+current_verse_number)
  // Ensure verse tracking is on.
  switch_verse_tracking_on ();
  DEBUG("W7 switched verse tracking on");
  // Save the go-to verse number as the new current one. This prevents a race condition.
  current_verse_number = number;
  current_reference.verse_set(number);

  // Only move the insertion point if it goes to another verse.
  ustring priorVerse = verse_number_get();
  DEBUG("priorVerse="+priorVerse+" and go_to_verse number="+number)
  if (number != priorVerse) {
    //DEBUG("go_to_verse number="+number+" but verse_number_get=" + computedVerse)
    // Get the iterator and textview that contain the verse number.
    GtkTextIter iter;
    DEBUG("Searching for iterator at verse "+number);
    if (get_iterator_at_verse_number (number, Style::get_verse_marker(project), iter, textview)) {
      DEBUG("Found iterator at verse "+number)
      give_focus (textview);
      gtk_text_buffer_place_cursor(textbuffer, &iter);
    }
    else {
      DEBUG("Did not find iterator at verse "+number)   
    }
  
  }
  DEBUG("W8 about to scroll to insertion point");
  scroll_to_insertion_point_on_screen(textview);
  DEBUG("W9 scrolled to insertion point");
  highlightCurrVerse(textview);
  highlight_searchwords();
  DEBUG("W10 highlighted current verse and search words");
}

// The idea of this routine is that if a bunch of such signals are emitted close in time,
// only the last one will actually "fire" and do something. This will erase the earlier 
// timed signal and set a new one.
void Editor3::signal_if_verse_changed ()
{
  DEBUG("Set up verse_changed_timeout signal")
  gw_destroy_source(signal_if_verse_changed_event_id);
  signal_if_verse_changed_event_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, GSourceFunc(on_signal_if_verse_changed_timeout), gpointer(this), NULL);
}

bool Editor3::on_signal_if_verse_changed_timeout(gpointer data)
{
  ((Editor3 *) data)->signal_if_verse_changed_timeout();
  return false;
}

// Postiff: I believe this signals the verse navigator pull down menus 
// to change the verse number they are showing.
void Editor3::signal_if_verse_changed_timeout()
{
    DEBUG("Called signal_if_verse_changed_timeout")
    // Proceed if verse tracking is on.
    if (verse_tracking_on && focused_textview) {
            // Proceed if there's no selection.
            if (!gtk_text_buffer_get_has_selection (gtk_text_view_get_buffer (focused_textview))) {
                // Emit a signal if the verse number at the insertion point changed.
                ustring verse_number = verse_number_get();
                if (verse_number != current_verse_number) {
                    DEBUG("Changing from v"+current_verse_number+" to v"+verse_number+" as new current_verse_number")
                    current_verse_number = verse_number;
                    current_reference.verse_set(verse_number);
//                    vector <GtkWidget *> textviews = editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW);
                    scroll_to_insertion_point_on_screen(textview);
                    highlightCurrVerse(textview);
                    // Need I highligh search words now?
                    if (new_verse_signal) {
                        gtk_button_clicked(GTK_BUTTON(new_verse_signal));
                    }
                }
            }
        
    }
}

gboolean Editor3::on_caller_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  return ((Editor3 *) user_data)->on_caller_button_press(widget);
}


gboolean Editor3::on_caller_button_press (GtkWidget *widget)
// Called when the user clicks on a note at the bottom of the screen.
// It will focus the note caller in the text.
{
  // Look for the note paragraph.
  EditorActionCreateParagraph * paragraph = widget2paragraph_action (widget);
  if (paragraph) {
    EditorActionCreateNoteParagraph * note_paragraph = static_cast <EditorActionCreateNoteParagraph *> (paragraph);
    // Get the note style.
    ustring note_style = note_paragraph->identifier;
    // Get the iterator and the textview of the note caller in the text.
    vector <GtkWidget *>
    textviews = editor_get_widgets (vbox_paragraphs, GTK_TYPE_TEXT_VIEW);
    for (unsigned int i = 0; i < textviews.size(); i++) {
      GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textviews[i]));
      GtkTextIter iter;
      gtk_text_buffer_get_start_iter (textbuffer, &iter);
      do {
        ustring paragraph_style, character_style, verse_at_iter;
        get_styles_at_iterator(iter, paragraph_style, character_style);
        if (character_style == note_style) {
          gtk_text_buffer_place_cursor (textbuffer, &iter);
          give_focus (textviews[i]);
        }
      } while (gtk_text_iter_forward_char(&iter));
    }
  }
  // Propagate the button press event.
  return false;
}


bool Editor3::has_focus ()
// Returns whether the editor has focus.
// Recursively defined in the sense that it has focues
// if any of its children widgets (paragraphs or notes)
// have focus.
{
#if 0
  vector <GtkWidget *> widgets = editor_get_widgets (vbox_paragraphs);
  for (unsigned int i = 0; i < widgets.size(); i++) {
    if (gtk_widget_has_focus (widgets[i]))
      return true;
  }
  widgets = editor_get_widgets (vbox_notes);
  for (unsigned int i = 0; i < widgets.size(); i++) {
    if (gtk_widget_has_focus (widgets[i]))
      return true;
  }
#endif
  if (gtk_widget_has_focus (textview) || gtk_widget_has_focus (notetextview)) {
   DEBUG("One of the textviews has focus")
   return true;
  }
  DEBUG("Neither of the textviews has focus")
  return false;
}


void Editor3::give_focus (GtkWidget * widget)
// Gives focus to a widget.
{
    DEBUG("Called ")
  if (has_focus ()) {
    // If the editor has focus, then the widget is actually given focus.
    gtk_widget_grab_focus (widget);
	//DEBUG("Called give_focus and gtk_widget_grab_focus")
  } else {
    // If the editor does not have focus, only the internal focus system is called, without actually having the widget grab focus
      // as if the user clicked on .
    textview_grab_focus(widget);
	//DEBUG("Called give_focus and textview_grab_focus")
  }
}

// From old editor_aids.cpp

gint Editor3::table_get_n_rows(GtkTable * table)
{
  gint n_rows = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(table), "n_rows"));
  return n_rows;
}


gint Editor3::table_get_n_columns(GtkTable * table)
{
  gint n_columns = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(table), "n_columns"));
  return n_columns;
}


void Editor3::usfm_internal_add_text(ustring & text, const ustring & addition)
// This is an internal function that adds an addition to already existing
// USFM text.
{
  // Special handling for adding an end-of-line.
  if (addition == "\n") {
    // No end-of-line is to be added at the beginning of the text.
    if (text.empty()) { return; }
    // Ensure that no white-space exists before the end-of-line.
    text = trim(text);
  }
  // Add text.
  text.append(addition);
}


void Editor3::usfm_internal_get_text_close_character_style(ustring & text, const ustring & project, const ustring & style)
// Adds the USFM code for a character style that closes.
{
  // Get the type and the subtype.
  StyleType type;
  int subtype;
  Style::marker_get_type_and_subtype(project, style, type, subtype);
  // A verse number, normally the \v, does not have a closing marker.
  if (!Style::get_starts_verse_number(type, subtype)) {
    usfm_internal_add_text(text, usfm_get_full_closing_marker(style));
  }
}


ustring Editor3::usfm_get_note_text(GtkTextIter startiter, GtkTextIter enditer, const ustring & project)
{
  // Variable to hold the note text.
  ustring notetext;

  // Initialize the iterator.
  GtkTextIter iter = startiter;

  // Paragraph and character styles.
  ustring previous_paragraph_style;
  ustring previous_character_style;
  bool paragraph_initialized = false;

  // Iterate through the text.  
  while (gtk_text_iter_compare(&iter, &enditer) < 0) {

    // Get the new paragraph and character style.
    // This is done by getting the names of the styles at this iterator.
    // With the way the styles are applied currently, the first 
    // style is a paragraph style, and the second style is optional 
    // and would be a character style.
    ustring new_paragraph_style;
    ustring new_character_style;
    get_styles_at_iterator(iter, new_paragraph_style, new_character_style);

    // Get the text at the iterator, and whether this is a linebreak.
    ustring new_character;
    bool line_break;
    {
      gunichar unichar = gtk_text_iter_get_char(&iter);
      gchar buf[7];
      gint length = g_unichar_to_utf8(unichar, (gchar *) & buf);
      buf[length] = '\0';
      new_character = buf;
      line_break = (new_character.find_first_of("\n\r") == 0);
      if (line_break) {
        new_character.clear();
        previous_paragraph_style.clear();
      }
    }

    /*
       How to get the usfm code of a note.
       Generally speaking, this is the way to do it:
       If a character marker of the type "note content" appears or changes,
       then that is the code to be inserted as an opening marker.
       At the moment that such a character marker disappears,
       then insert the then prevailing paragraph marker.
       If a character marker of type "note content with endmarker" appears,
       then insert the opening code of it, 
       and if such a marker disappears, insert the closing code.
     */

    bool note_content_opening = false;
    bool note_content_closing = false;
    bool note_content_with_endmarker_opening = false;
    bool note_content_with_endmarker_closing = false;
    if (new_character_style != previous_character_style) {
      StyleType type;
      int subtype;
      if (new_character_style.empty()) {
        Style::marker_get_type_and_subtype(project, previous_character_style, type, subtype);
        if (Style::get_starts_note_content(type, subtype)) {
          note_content_closing = true;
        }
        if (Style::get_starts_character_style(type, subtype)) {
          note_content_with_endmarker_closing = true;
        }
      } else {
        Style::marker_get_type_and_subtype(project, new_character_style, type, subtype);
        if (Style::get_starts_note_content(type, subtype)) {
          note_content_opening = true;
        }
        if (Style::get_starts_character_style(type, subtype)) {
          note_content_with_endmarker_opening = true;
        }
      }
    }

    if (new_paragraph_style != previous_paragraph_style) {
      if (paragraph_initialized) {
        if (!new_paragraph_style.empty()) {
          usfm_internal_add_text(notetext, usfm_get_full_opening_marker(new_paragraph_style));
        }
      }
    }
    // Determine the usfm code to be inserted.
    ustring usfm_code;
    if (note_content_opening || note_content_with_endmarker_opening) {
      usfm_code = usfm_get_full_opening_marker(new_character_style);
    } else if (note_content_closing) {
      usfm_code = usfm_get_full_opening_marker(new_paragraph_style);
    } else if (note_content_with_endmarker_closing) {
      usfm_code = usfm_get_full_closing_marker(previous_character_style);
    }
    // Store all styles and flags for next iteration.
    previous_paragraph_style = new_paragraph_style;
    previous_character_style = new_character_style;
    paragraph_initialized = true;

    /*
       Store the possible code and the character.
       We have had cases that such code was produced:
       \f + \fr v1:\ft  one \fdc two\fdc* three.\f*
       This was coming from v1: one two three.
       But the fr style was only applied to "v1:", thus giving a space after the \ft marker.
       The "\ft " marker itself already has a space. 
       We then find that case that one space follows the other.
       Two consecusitive spaces in USFM count as only one space, hence this space
       was removed. And that again results in this faulty text:
       "v1:one two three".
       A special routine solves that, putting the space before the "\ft " marker.
     */
    bool swap_code_and_text = false;
    if (paragraph_initialized) {
      if (new_character == " ") {
        if (trim(usfm_code) != usfm_code) {
          swap_code_and_text = true;
        }
      }
    }
    if (swap_code_and_text) {
      usfm_internal_add_text(notetext, new_character);
      usfm_internal_add_text(notetext, usfm_code);
    } else {
      usfm_internal_add_text(notetext, usfm_code);
      usfm_internal_add_text(notetext, new_character);
    }

    // Next iteration.
    gtk_text_iter_forward_char(&iter);
  }

  // Return what it got.
  return notetext;
}

vector <ustring> Editor3::get_character_styles_between_iterators (GtkTextIter startiter, GtkTextIter enditer)
// Gets the character styles between two iterators given.
// To do this properly, it is assumed that the first style encountered will always be the paragraph style,
// and the second the character style.
{
  vector <ustring> styles;
  if (!gtk_text_iter_equal (&startiter, &enditer)) {
    GtkTextIter iter = startiter;
    do {
      ustring paragraphstyle, characterstyle;
      get_styles_at_iterator(iter, paragraphstyle, characterstyle);
      styles.push_back (characterstyle);
      gtk_text_iter_forward_char(&iter);
    } while (gtk_text_iter_in_range(&iter, &startiter, &enditer));
  }
  return styles;
}

void Editor3::textbuffer_apply_named_tag(GtkTextBuffer * buffer, const ustring & name, const GtkTextIter * start, const GtkTextIter * end)
// Applies the tag on the textbuffer, if the tag exists.
// Else applies the "unknown" style.
{
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
  GtkTextTag *tag = gtk_text_tag_table_lookup(table, name.c_str());
  if (tag) {
    gtk_text_buffer_apply_tag_by_name(buffer, name.c_str(), start, end);
  }
  else {
    gtk_text_buffer_apply_tag_by_name(buffer, unknown_style(), start, end);
  }
}

void Editor3::textbuffer_insert_with_named_tags(GtkTextBuffer * buffer, GtkTextIter * iter, const ustring & text, ustring first_tag_name, ustring second_tag_name)
// Inserts text into the buffer applying one or two named tags at the same time.
// If a tag does not exist, it applies the "unknown" style instead.
{
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
  GtkTextTag *tag = gtk_text_tag_table_lookup(table, first_tag_name.c_str());
  if (!tag)
    first_tag_name = unknown_style();
  if (!second_tag_name.empty()) {
    tag = gtk_text_tag_table_lookup(table, second_tag_name.c_str());
    if (!tag)
      second_tag_name = unknown_style();
  }
  if (second_tag_name.empty()) {
    gtk_text_buffer_insert_with_tags_by_name(buffer, iter, text.c_str(), -1, first_tag_name.c_str(), NULL);
  } else {
    gtk_text_buffer_insert_with_tags_by_name(buffer, iter, text.c_str(), -1, first_tag_name.c_str(), second_tag_name.c_str(), NULL);
  }
}


void Editor3::clear_and_destroy_editor_actions (deque <EditorAction *>& actions)
{
  for (unsigned int i = 0; i < actions.size(); i++) {
    EditorAction * action = actions[i];
    delete action;
  }
  actions.clear();
}


void Editor3::on_container_tree_callback_destroy (GtkWidget *widget, gpointer user_data)
{
  gtk_widget_destroy (widget);
}


void Editor3::editor_text_append(GtkTextBuffer * textbuffer, const ustring & text, const ustring & paragraph_style, const ustring & character_style)
// This function appends text to the textbuffer.
// It inserts the text at the cursor.
{
  // Get the iterator at the text insertion point.
  GtkTextIter insertiter;
  gtk_text_buffer_get_iter_at_mark(textbuffer, &insertiter, gtk_text_buffer_get_insert(textbuffer));
  // Insert text together with the style(s).
  textbuffer_insert_with_named_tags(textbuffer, &insertiter, text, paragraph_style, character_style);
}


gint Editor3::editor_paragraph_insertion_point_get_offset (EditorActionCreateParagraph * paragraph_action)
{
  gint offset = 0;
  if (paragraph_action) {
    GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (paragraph_action->textview));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(textbuffer, &iter, gtk_text_buffer_get_insert(textbuffer));
    offset = gtk_text_iter_get_offset (&iter);
  }
  return offset;
}


void Editor3::editor_paragraph_insertion_point_set_offset (EditorActionCreateParagraph * paragraph_action, gint offset)
{
  if (paragraph_action) {
    GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (paragraph_action->textview));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset (textbuffer, &iter, offset);
    gtk_text_buffer_place_cursor (textbuffer, &iter);
  }
}


Editor3::EditorActionDeleteText * Editor3::paragraph_delete_last_character_if_space(EditorActionCreateParagraph * paragraph_action)
// Creates an action for deleting text for the last character in the text buffer if it is a space.
{
  if (paragraph_action) {
    GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (paragraph_action->textview));
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter (textbuffer, &iter);
    bool text_available = gtk_text_iter_backward_char(&iter);
    if (text_available) {
      gunichar last_character = gtk_text_iter_get_char(&iter);
      if (g_unichar_isspace(last_character)) {
        gint offset = gtk_text_iter_get_offset (&iter);
        return new EditorActionDeleteText (this, paragraph_action, offset, 1);
      }
    }
  }
  return NULL;
}

Editor3::EditorActionDeleteText * Editor3::paragraph_get_text_and_styles_after_insertion_point(EditorActionCreateParagraph * paragraph, vector <ustring>& text, vector <ustring>& styles)
// This function accepts a paragraph, and gives a list of text and their associated character styles
// from the insertion point to the end of the buffer.
// It returns the EditorAction that would be required to erase that text from the paragraph.
// Note that this EditorAction needs to be applied for the effect to be obtained.
// If it is not applied, it should then be destroyed.
{
  GtkTextBuffer * textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (paragraph->textview));
  gint start_offset = editor_paragraph_insertion_point_get_offset (paragraph);
  GtkTextIter startiter, enditer;
  gtk_text_buffer_get_iter_at_offset (textbuffer, &startiter, start_offset);
  gtk_text_buffer_get_end_iter (textbuffer, &enditer);
  gint end_offset = gtk_text_iter_get_offset (&enditer);
  get_text_and_styles_between_iterators(&startiter, &enditer, text, styles);
  EditorActionDeleteText * delete_action = NULL;
  if (end_offset > start_offset) {
    delete_action = new EditorActionDeleteText(this, paragraph, start_offset, end_offset - start_offset);
  }
  return delete_action;
}


void Editor3::get_text_and_styles_between_iterators(GtkTextIter * startiter, GtkTextIter * enditer, vector <ustring>& text, vector <ustring>& styles)
// This function gives a list of the text and character styles between two iterators.
// The "text" variable contains a chunks of text with the same style.
{
  text.clear();
  styles.clear();
  ustring previous_style;
  ustring accumulated_text;
  GtkTextIter iter = *startiter;
  do {
    ustring paragraph_style, character_style;
    get_styles_at_iterator(iter, paragraph_style, character_style);
    GtkTextIter iter2 = iter;
    gtk_text_iter_forward_char (&iter2);
    ustring character = gtk_text_iter_get_text(&iter, &iter2);
    if (character_style != previous_style) {
      if (!accumulated_text.empty()) {
        text.push_back (accumulated_text);
        styles.push_back (previous_style);
        accumulated_text.clear();
      }
    }
    previous_style = character_style;    
    accumulated_text.append (character);
    gtk_text_iter_forward_char(&iter);
  } while (gtk_text_iter_in_range(&iter, startiter, enditer));
  if (!accumulated_text.empty()) {
    text.push_back (accumulated_text);
    styles.push_back (previous_style);
  }
}


void Editor3::editor_park_widget (GtkWidget * vbox, GtkWidget * widget, gint& offset, GtkWidget * parking)
// Do the administration of parking a widget.
{
  // Look for the widget's offset within its parent.
  vector <GtkWidget *> widgets = editor_get_widgets (vbox);
  for (unsigned int i = 0; i < widgets.size(); i++) {
    if (widget == widgets[i]) {
      offset = i;
    }
  }
  // Transfer the widget to the parking lot. It is kept alive.
  gtk_widget_reparent (widget, parking);
}

EditorNoteType Editor3::note_type_get(const ustring & project, const ustring & marker)
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

NoteNumberingType Editor3::note_numbering_type_get(const ustring & project, const ustring & marker)
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


ustring Editor3::note_numbering_user_sequence_get(const ustring & project, const ustring & marker)
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

Editor3::EditorAction::EditorAction(Editor3 *_parent_editor, EditorActionType type_in)
{
  parent_editor = _parent_editor;
  // The type of this EditorAction.
  type = type_in;
}


Editor3::EditorAction::~EditorAction ()
{
  parent_editor = NULL;    
}


void Editor3::EditorAction::apply (deque <Editor3::EditorAction *>& done)
{
  // Store the EditorAction on the stack of actions done.
  done.push_back (this);
}


void Editor3::EditorAction::undo (deque <Editor3::EditorAction *>& done, deque <Editor3::EditorAction *>& undone)
{
  // Move the EditorAction from the stack of actions done to the one of actions undone.
  done.pop_back();
  undone.push_back (this);
}


void Editor3::EditorAction::redo (deque <Editor3::EditorAction *>& done, deque <Editor3::EditorAction *>& undone)
{
  // Move the EditorAction from the stack of actions undone to the one of actions done.
  undone.pop_back();
  done.push_back (this);
}


Editor3::EditorActionCreateParagraph::EditorActionCreateParagraph(Editor3 *_parent_editor, GtkWidget * vbox) :
EditorAction (_parent_editor, eatCreateParagraph)
{
  // Pointer to the GtkTextView is created on apply.
  textview = NULL;
  // Pointer to the GtkTextBuffer is created on apply.
  textbuffer = NULL;
  // The default style of the paragraph will be "unknown".
  style = unknown_style();
  // Offset of this widget at time of deletion.
  offset_at_delete = -1;
  // Pointer to the parent vertical box.
  parent_vbox = vbox;
}


Editor3::EditorActionCreateParagraph::~EditorActionCreateParagraph ()
{
  if (textview) {
    gtk_widget_destroy (textview);
    textview = NULL;
  }
  parent_vbox = NULL;
}


void Editor3::EditorActionCreateParagraph::apply (GtkTextTagTable * texttagtable, bool editable, Editor3::EditorActionCreateParagraph * focused_paragraph, GtkWidget *& to_focus)
{
  // The textbuffer uses the text tag table.
  textbuffer = gtk_text_buffer_new(texttagtable);

  // New text view to view the text buffer.
  textview = gtk_text_view_new_with_buffer(textbuffer);
  gtk_widget_show(textview);

  // Add text view to the GUI.
  gtk_box_pack_start(GTK_BOX(parent_vbox), textview, false, false, 0);

  // Set some parameters of the view.
  gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(textview), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), editable);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 5);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), 5);
  
  // Move the widget to the right position, 
  // which is next to the currently focused paragraph.
  // This move is important since a new paragraph can be created anywhere among the current ones.
  vector <GtkWidget *> widgets = parent_editor->editor_get_widgets (parent_vbox);
  gint new_paragraph_offset = 0;
  if (focused_paragraph) {
    for (unsigned int i = 0; i < widgets.size(); i++) {
      if (focused_paragraph->textview == widgets[i]) {
        new_paragraph_offset = i + 1;
        break;
      }
    }
  }
  gtk_box_reorder_child (GTK_BOX(parent_vbox), textview, new_paragraph_offset);
  
  // Let the newly created textview be earmarked to grab focus
  // so that the user can type in it,
  // and the internal Editor logic knows about it.
  to_focus = textview;
}


void Editor3::EditorActionCreateParagraph::undo (GtkWidget * parking_vbox, GtkWidget *& to_focus)
{
  // Remove the widget by parking it in an invisible location. It is kept alive.
  parent_editor->editor_park_widget (parent_vbox, textview, offset_at_delete, parking_vbox);
  // Focus textview.
  to_focus = textview;
}


void Editor3::EditorActionCreateParagraph::redo (GtkWidget *& to_focus)
{
  // Restore the live widget to the editor.
  gtk_widget_reparent (textview, parent_vbox);
  gtk_box_reorder_child (GTK_BOX(parent_vbox), textview, offset_at_delete);
  // Let the restored textview be earmarked to grab focus.
  to_focus = textview;
}


Editor3::EditorActionChangeParagraphStyle::EditorActionChangeParagraphStyle(Editor3 *_parent_editor, const ustring& style, Editor3::EditorActionCreateParagraph * parent_action) :
Editor3::EditorAction (_parent_editor, eatChangeParagraphStyle)
{
  // The EditorAction object that created the paragraph whose style it going to be set.
  paragraph = parent_action;
  // The style of the paragraph before the new style was applied.
  previous_style = parent_action->style;
  // The new style for the paragraph.
  current_style = style;
}


Editor3::EditorActionChangeParagraphStyle::~EditorActionChangeParagraphStyle ()
{
}


void Editor3::EditorActionChangeParagraphStyle::apply (GtkWidget *& to_focus)
{
  paragraph->style = current_style;
  set_style (paragraph->style);
  to_focus = paragraph->textview;
}


void Editor3::EditorActionChangeParagraphStyle::undo (GtkWidget *& to_focus)
{
  paragraph->style = previous_style;
  set_style (paragraph->style);
  to_focus = paragraph->textview;
}


void Editor3::EditorActionChangeParagraphStyle::redo (GtkWidget *& to_focus)
{
  apply (to_focus);
}


void Editor3::EditorActionChangeParagraphStyle::set_style (const ustring& style)
{
  // Define the work area.
  GtkTextIter startiter;
  gtk_text_buffer_get_start_iter (paragraph->textbuffer, &startiter);
  GtkTextIter enditer;
  gtk_text_buffer_get_end_iter (paragraph->textbuffer, &enditer);
  // Apply the style in such a way that the paragraph style is always applied first, 
  // then after that the character styles.
  vector <ustring> current_character_styles = parent_editor->get_character_styles_between_iterators (startiter, enditer);
  gtk_text_buffer_remove_all_tags (paragraph->textbuffer, &startiter, &enditer);
  gtk_text_buffer_apply_tag_by_name (paragraph->textbuffer, style.c_str(), &startiter, &enditer);
  for (unsigned int i = 0; i < current_character_styles.size(); i++) {
    if (!current_character_styles[i].empty()) {
      gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, i);
      enditer = startiter;
      gtk_text_iter_forward_char (&enditer);
      gtk_text_buffer_apply_tag_by_name (paragraph->textbuffer, current_character_styles[i].c_str(), &startiter, &enditer);
    }
  }
}


Editor3::EditorActionInsertText::EditorActionInsertText(Editor3 *_parent_editor, Editor3::EditorActionCreateParagraph * parent_action, gint offset_in, const ustring& text_in) :
Editor3::EditorAction (_parent_editor, eatInsertText)
{
  // The paragraph to operate on.
  paragraph = parent_action;
  // Where to insert the text, that is, at which offset within the GtkTextBuffer.
  offset = offset_in;
  // The text to insert.
  text = text_in;
}


Editor3::EditorActionInsertText::~EditorActionInsertText ()
{
}


void Editor3::EditorActionInsertText::apply (GtkWidget *& to_focus)
{
  GtkTextIter iter;
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &iter, offset);
  gtk_text_buffer_insert (paragraph->textbuffer, &iter, text.c_str(), -1);
  // Apply the paragraph style to the new inserted text.
  // It is important that paragraph styles are applied first, and character styles last.
  // Since this is new inserted text, there's no character style yet, 
  // so the paragraph style can be applied normally.
  GtkTextIter startiter;
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, offset);
  GtkTextIter enditer;
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &enditer, offset + text.length());
  gtk_text_buffer_apply_tag_by_name (paragraph->textbuffer, paragraph->style.c_str(), &startiter, &enditer);
  // Focus widget.
  to_focus = paragraph->textview;
}


void Editor3::EditorActionInsertText::undo (GtkWidget *& to_focus)
{
  // Undo the insertion of text, that is, remove it again.
  GtkTextIter startiter, enditer;
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, offset);
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &enditer, offset + text.length());
  gtk_text_buffer_delete (paragraph->textbuffer, &startiter, &enditer);
  // Focus widget.
  to_focus = paragraph->textview;
}


void Editor3::EditorActionInsertText::redo (GtkWidget *& to_focus)
{
  apply (to_focus);
}


Editor3::EditorActionDeleteText::EditorActionDeleteText(Editor3 *_parent_editor, Editor3::EditorActionCreateParagraph * parent_action, gint offset_in, gint length_in) :
Editor3::EditorAction (_parent_editor, eatDeleteText)
{
  // The paragraph to operate on.
  paragraph = parent_action;
  // Where to start deleting the text, that is, at which offset within the GtkTextBuffer.
  offset = offset_in;
  // The length of the text to be deleted.
  length = length_in;
  // The text which was deleted will be set when this action is executed.
}


Editor3::EditorActionDeleteText::~EditorActionDeleteText ()
{
}


void Editor3::EditorActionDeleteText::apply (GtkWidget *& to_focus)
{
  // Limit the area.
  GtkTextIter startiter, enditer;
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, offset);
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &enditer, offset + length);
  // Save existing content.
  parent_editor->get_text_and_styles_between_iterators(&startiter, &enditer, deleted_text, deleted_styles);
  // Delete text.
  gtk_text_buffer_delete (paragraph->textbuffer, &startiter, &enditer);
  // Focus widget.
  to_focus = paragraph->textview;
}


void Editor3::EditorActionDeleteText::undo (GtkWidget *& to_focus)
{
  // Undo the text deletion action, that means, re-insert the text.
  // Get initial insert position.
  gint accumulated_offset = offset;
  // Go through the text to re-insert.
  for (unsigned int i = 0; i < deleted_text.size(); i++) {
    // Get the position where to insert.
    GtkTextIter startiter;
    gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, accumulated_offset);
    // Re-insert the text.
    gtk_text_buffer_insert (paragraph->textbuffer, &startiter, deleted_text[i].c_str(), -1);
    // Apply the paragraph style to the new inserted text.
    // It is important that paragraph styles are applied first, and character styles last.
    // Since this is new inserted text, there's no character style yet, 
    // so the paragraph style can be applied normally.
    gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, accumulated_offset);
    GtkTextIter enditer;
    gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &enditer, accumulated_offset + deleted_text[i].length());
    gtk_text_buffer_apply_tag_by_name (paragraph->textbuffer, paragraph->style.c_str(), &startiter, &enditer);
    // Apply the character style.
    if (!deleted_styles[i].empty()) {
      gtk_text_buffer_apply_tag_by_name (paragraph->textbuffer, deleted_styles[i].c_str(), &startiter, &enditer);
    }
    // Modify the accumulated offset for the next iteration.
    accumulated_offset += deleted_text[i].length();
  }
  // Focus widget.
  to_focus = paragraph->textview;
}


void Editor3::EditorActionDeleteText::redo (GtkWidget *& to_focus)
{
  // Limit the area.
  GtkTextIter startiter, enditer;
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, offset);
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &enditer, offset + length);
  // Delete text.
  gtk_text_buffer_delete (paragraph->textbuffer, &startiter, &enditer);
  // Focus widget.
  to_focus = paragraph->textview;
}


Editor3::EditorActionChangeCharacterStyle::EditorActionChangeCharacterStyle(Editor3 *_parent_editor, Editor3::EditorActionCreateParagraph * parent_action, const ustring& style_in, gint offset_in, gint length_in) :
Editor3::EditorAction (_parent_editor, eatChangeCharacterStyle)
{
  // The identifier of the paragraph to operate on.
  paragraph = parent_action;
  // The name of the style.
  style = style_in;
  // Where to start applying the style, that is, at which offset within the GtkTextBuffer.
  offset = offset_in;
  // The length of the text where the style is to be applied.
  length = length_in;
  // The previous styles are stored per character when this action is executed.
}


Editor3::EditorActionChangeCharacterStyle::~EditorActionChangeCharacterStyle ()
{
}


void Editor3::EditorActionChangeCharacterStyle::apply (GtkWidget *& to_focus)
{
  // Mark off the affected area.
  GtkTextIter startiter;
  GtkTextIter enditer;
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, offset);
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &enditer, offset + length);
  // Get the styles applied now, and store these so as to track the state of this bit of text.
  previous_styles = parent_editor->get_character_styles_between_iterators (startiter, enditer);
  // The new styles to apply.
  vector <ustring> new_styles;
  for (gint i = 0; i < length; i++) {
    new_styles.push_back (style);
  }
  // Change the styles.
  change_styles (previous_styles, new_styles);
  // Focus widget.
  to_focus = paragraph->textview;
}


void Editor3::EditorActionChangeCharacterStyle::undo (GtkWidget *& to_focus)
{
  // The styles to remove.
  vector <ustring> styles_to_remove;
  for (gint i = 0; i < length; i++) {
    styles_to_remove.push_back (style);
  }
  // Change the styles, putting back the original ones.
  change_styles (styles_to_remove, previous_styles);
  // Focus widget.
  to_focus = paragraph->textview;
}


void Editor3::EditorActionChangeCharacterStyle::redo (GtkWidget *& to_focus)
{
  // Mark off the affected area.
  GtkTextIter startiter;
  GtkTextIter enditer;
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, offset);
  gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &enditer, offset + length);
  // Get the styles applied now, and store these so as to track the state of this bit of text.
  vector <ustring> styles_to_delete = parent_editor->get_character_styles_between_iterators (startiter, enditer);
  // The new styles to apply.
  vector <ustring> new_styles;
  for (gint i = 0; i < length; i++) {
    new_styles.push_back (style);
  }
  // Change the styles.
  change_styles (styles_to_delete, new_styles);
  // Focus widget.
  to_focus = paragraph->textview;
}


void Editor3::EditorActionChangeCharacterStyle::change_styles (const vector <ustring>& old_ones, const vector <ustring>& new_ones)
{
  // Remove old styles and apply new ones.
  for (gint i = 0; i < length; i++) {
    GtkTextIter startiter, enditer;
    gtk_text_buffer_get_iter_at_offset (paragraph->textbuffer, &startiter, offset + i);
    enditer = startiter;
    gtk_text_iter_forward_char (&enditer);
    if (!old_ones[i].empty()) {
      gtk_text_buffer_remove_tag_by_name (paragraph->textbuffer, old_ones[i].c_str(), &startiter, &enditer);
    }
    if (!new_ones[i].empty()) {
      gtk_text_buffer_apply_tag_by_name (paragraph->textbuffer, new_ones[i].c_str(), &startiter, &enditer);
    }
  }
}


Editor3::EditorActionDeleteParagraph::EditorActionDeleteParagraph(Editor3 *_parent_editor, Editor3::EditorActionCreateParagraph * paragraph_in) :
Editor3::EditorAction (_parent_editor, eatDeleteParagraph)
{
  // The identifier of the paragraph to operate on.
  paragraph = paragraph_in;
  // Initialize the offset within the parent GtkBox.
  offset = -1;
}


Editor3::EditorActionDeleteParagraph::~EditorActionDeleteParagraph ()
{
}


void Editor3::EditorActionDeleteParagraph::apply (GtkWidget * parking_vbox, GtkWidget *& to_focus)
{
  // Park this widget, keeping it alive.
  GtkWidget * widget_to_park = paragraph->textview;
  if (paragraph->type == eatCreateNoteParagraph) {
    EditorActionCreateNoteParagraph * note_paragraph = static_cast <EditorActionCreateNoteParagraph *> (paragraph);
    widget_to_park = note_paragraph->hbox;
  }
  parent_editor->editor_park_widget (paragraph->parent_vbox, widget_to_park, offset, parking_vbox);
}


void Editor3::EditorActionDeleteParagraph::undo (GtkWidget *& to_focus)
{
  // Restore the live widget to the editor.
  GtkWidget * widget_to_restore = paragraph->textview;
  if (paragraph->type == eatCreateNoteParagraph) {
    EditorActionCreateNoteParagraph * note_paragraph = static_cast <EditorActionCreateNoteParagraph *> (paragraph);
    widget_to_restore = note_paragraph->hbox;
  }
  gtk_widget_reparent (widget_to_restore, paragraph->parent_vbox);
  gtk_box_reorder_child (GTK_BOX(paragraph->parent_vbox), widget_to_restore, offset);
  // Let the restored textview be earmarked to grab focus.
  to_focus = paragraph->textview;
}


void Editor3::EditorActionDeleteParagraph::redo (GtkWidget * parking_vbox, GtkWidget *& to_focus)
{
  // Park this widget, keeping it alive.
  // Don't store the offset, since we already have that value.
  GtkWidget * widget_to_park = paragraph->textview;
  if (paragraph->type == eatCreateNoteParagraph) {
    EditorActionCreateNoteParagraph * note_paragraph = static_cast <EditorActionCreateNoteParagraph *> (paragraph);
    widget_to_park = note_paragraph->hbox;
  }
  gint dummy;
  parent_editor->editor_park_widget (paragraph->parent_vbox, widget_to_park, dummy, parking_vbox);
}


Editor3::EditorActionCreateNoteParagraph::EditorActionCreateNoteParagraph(Editor3 *_parent_editor, GtkWidget * vbox, const ustring& marker_in, const ustring& caller_usfm_in, const ustring& caller_text_in, const ustring& identifier_in) :
Editor3::EditorActionCreateParagraph (_parent_editor, vbox)
{
  // Change the type to a note paragraph.
  type = eatCreateNoteParagraph;
  // Store opening and closing marker (e.g. "f" for a footnote).
  opening_closing_marker = marker_in;
  // Store USFM caller (e.g. "+" for automatic numbering).
  caller_usfm = caller_usfm_in;
  // Store caller in text (e.g. "f" for a footnote).
  caller_text = caller_text_in;
  // Store identifier. Is used as the style in the main text body.
  identifier = identifier_in;
  // Widgets will be set on initial application.
  hbox = NULL;
  eventbox = NULL;
  label = NULL;
}


Editor3::EditorActionCreateNoteParagraph::~EditorActionCreateNoteParagraph ()
{
  if (hbox) {
    gtk_widget_destroy (hbox);
    hbox = NULL;
    eventbox = NULL;
    label = NULL;
    textview = NULL;
  }
}


void Editor3::EditorActionCreateNoteParagraph::apply (GtkTextTagTable * texttagtable, bool editable, Editor3::EditorActionCreateParagraph * focused_paragraph, GtkWidget *& to_focus)
{
  // Horizontal box to store the note.
  hbox = gtk_hbox_new (false, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start(GTK_BOX(parent_vbox), hbox, false, false, 0);

  // Eventbox to catch a few events on the caller of the note.
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_box_pack_start(GTK_BOX(hbox), eventbox, false, false, 0);
  g_signal_connect ((gpointer) eventbox, "enter_notify_event", G_CALLBACK (on_caller_enter_notify_event), gpointer (this));
  g_signal_connect ((gpointer) eventbox, "leave_notify_event", G_CALLBACK (on_caller_leave_notify_event), gpointer (this));

  // The background of the caller is going to be grey.
  // Courier font is chosen to make the spacing of the callers equal so they line up nicely.
  label = gtk_label_new ("");
  gtk_widget_show (label);
  char *markup = g_markup_printf_escaped("<span background=\"grey\" size=\"x-small\"> </span><span background=\"grey\" face=\"Courier\">%s</span><span background=\"grey\" size=\"x-small\"> </span>", caller_text.c_str());
  gtk_label_set_markup(GTK_LABEL(label), markup);
  g_free(markup);
  gtk_container_add (GTK_CONTAINER (eventbox), label);

  // The textbuffer uses the text tag table.
  textbuffer = gtk_text_buffer_new(texttagtable);

  // Text view to view the text buffer.
  textview = gtk_text_view_new_with_buffer(textbuffer);
  gtk_widget_show(textview);
  gtk_box_pack_start(GTK_BOX(hbox), textview, true, true, 0);

  // Set some parameters of the view.
  gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(textview), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), editable);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 5);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), 5);

  // Move the widget to the right position. To be calculated.
  /*
    vector <GtkWidget *> widgets = parent_editor->editor_get_widgets (parent_vbox);
    gint new_paragraph_offset = 0;
    if (focused_paragraph) {
      for (unsigned int i = 0; i < widgets.size(); i++) {
        if (focused_paragraph->textview == widgets[i]) {
          new_paragraph_offset = i + 1;
          break;
        }
      }
    }
    gtk_box_reorder_child (GTK_BOX(parent_vbox), textview, new_paragraph_offset);
  */
  
  // Let the newly created textview be earmarked to grab focus
  // so that the user can type in it,
  // and the internal Editor logic knows about it.
  to_focus = textview;
}


void Editor3::EditorActionCreateNoteParagraph::undo (GtkWidget * parking_vbox, GtkWidget *& to_focus)
{
  // Remove the widget by parking it in an invisible location. It is kept alive.
  parent_editor->editor_park_widget (parent_vbox, textview, offset_at_delete, parking_vbox);
  // Focus textview.
  to_focus = textview;
}


void Editor3::EditorActionCreateNoteParagraph::redo (GtkWidget *& to_focus)
{
  // Restore the live widget to the editor.
  gtk_widget_reparent (textview, parent_vbox);
  gtk_box_reorder_child (GTK_BOX(parent_vbox), textview, offset_at_delete);
  // Let the restored textview be earmarked to grab focus.
  to_focus = textview;
}


gboolean Editor3::EditorActionCreateNoteParagraph::on_caller_enter_notify_event (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
  return ((EditorActionCreateNoteParagraph *) user_data)->on_caller_enter_notify(event);
}


gboolean Editor3::EditorActionCreateNoteParagraph::on_caller_enter_notify (GdkEventCrossing *event)
{
  // Set the cursor to a shape that shows that the caller can be clicked.
  GtkWidget *toplevel_widget = gtk_widget_get_toplevel(label);
  GdkWindow *gdk_window = gtk_widget_get_window (toplevel_widget);
  GdkCursor *cursor = gdk_cursor_new(GDK_HAND2);
  gdk_window_set_cursor(gdk_window, cursor);
  gdk_cursor_unref (cursor);
  return false;
}


gboolean Editor3::EditorActionCreateNoteParagraph::on_caller_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
  return ((EditorActionCreateNoteParagraph *) user_data)->on_caller_leave_notify(event);
}


gboolean Editor3::EditorActionCreateNoteParagraph::on_caller_leave_notify (GdkEventCrossing *event)
{
  // Restore the original cursor.
  GtkWidget * toplevel_widget = gtk_widget_get_toplevel(label);
  GdkWindow *gdk_window = gtk_widget_get_window (toplevel_widget);
  gdk_window_set_cursor(gdk_window, NULL);
  return false;
}
