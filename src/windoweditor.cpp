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
#include <glib.h>
#include "windoweditor.h"
#include "help.h"
#include "floatingwindow.h"
#include "keyterms.h"
#include "tiny_utilities.h"
#include "settings.h"
#include <glib/gi18n.h>
#include "debug.h"

WindowEditor::WindowEditor(const ustring& project_name, const ustring &window_title, 
                           GtkWidget * parent_layout, GtkAccelGroup *accelerator_group, bool startup, viewType vt):
FloatingWindow(parent_layout, widEditor, window_title, startup)
// Text editor.
{
  // Initialize variables.
  projectname = project_name;
  currvt = vt;
  init();
  switch_view();
}

void WindowEditor::init(void)
{
  currView = NULL;
  editor2 = NULL;
  usfmview = NULL;
  editor3 = NULL;

  // Signalling buttons.
  new_verse_signal = gtk_button_new();
  new_styles_signal = gtk_button_new();
  quick_references_button = gtk_button_new();
  word_double_clicked_signal = gtk_button_new();
  reload_signal = gtk_button_new();
  changed_signal = gtk_button_new();
  spelling_checked_signal = gtk_button_new ();

  hID1 = 0;  hID2 = 0;  hID3 = 0;  hID4 = 0;
  hID5 = 0;  hID6 = 0;  hID7 = 0;  hID8 = 0;

  // Gui.
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(vbox_client), vbox);
}

WindowEditor::~WindowEditor()
{
  cleanup();
}

void WindowEditor::cleanup(void)
{
  // Disconnect signals. Next two if stmts attempt to fix Warao Psalms bug on switch to USFM view
  // Did not work. Apparently our "cleanup" of the WindowEditor object was not good enough to
  // clear out the source of the bug. See MainWindow::on_view_chapteras for explanation of
  // another approach that I ended up taking.
  if (editor2) {
    g_signal_handler_disconnect ((gpointer) editor2->new_verse_signal, hID1);
    g_signal_handler_disconnect ((gpointer) editor2->new_styles_signal, hID2);
    g_signal_handler_disconnect ((gpointer) editor2->quick_references_button, hID3);
    g_signal_handler_disconnect ((gpointer) editor2->word_double_clicked_signal, hID4);
    g_signal_handler_disconnect ((gpointer) editor2->reload_signal, hID5);
    g_signal_handler_disconnect ((gpointer) editor2->changed_signal, hID6);
    g_signal_handler_disconnect ((gpointer) editor2->spelling_checked_signal, hID7);
    g_signal_handler_disconnect ((gpointer) editor2->new_widget_signal, hID8);
  }
  else if (usfmview) {
    g_signal_handler_disconnect ((gpointer) usfmview->new_verse_signal, hID1);
    g_signal_handler_disconnect ((gpointer) usfmview->word_double_clicked_signal, hID4);
    g_signal_handler_disconnect ((gpointer) usfmview->reload_signal, hID5);
    g_signal_handler_disconnect ((gpointer) usfmview->changed_signal, hID6);
  }
  else if (editor3) {
    g_signal_handler_disconnect ((gpointer) editor3->new_verse_signal, hID1);
    g_signal_handler_disconnect ((gpointer) editor3->new_styles_signal, hID2);
    g_signal_handler_disconnect ((gpointer) editor3->quick_references_button, hID3);
    g_signal_handler_disconnect ((gpointer) editor3->word_double_clicked_signal, hID4);
    g_signal_handler_disconnect ((gpointer) editor3->reload_signal, hID5);
    g_signal_handler_disconnect ((gpointer) editor3->changed_signal, hID6);
    g_signal_handler_disconnect ((gpointer) editor3->spelling_checked_signal, hID7);
    g_signal_handler_disconnect ((gpointer) editor3->new_widget_signal, hID8);
  }

  hID1 = 0;  hID2 = 0;  hID3 = 0;  hID4 = 0;
  hID5 = 0;  hID6 = 0;  hID7 = 0;  hID8 = 0;

  gtk_widget_destroy (new_verse_signal);
  gtk_widget_destroy (new_styles_signal);
  gtk_widget_destroy (quick_references_button);
  gtk_widget_destroy (word_double_clicked_signal);
  gtk_widget_destroy (reload_signal);
  gtk_widget_destroy (changed_signal);
  gtk_widget_destroy (spelling_checked_signal);

  if (editor2)  { delete editor2;  editor2 = NULL; }
  if (usfmview) { delete usfmview; usfmview = NULL; }
  if (editor3)  { delete editor3;  editor3 = NULL; }
  // Parent class FloatingWindow destroys vbox_client, which should also destroy vbox,
  // but if we are in the middle of switching views, I want to clean up everything and
  // start over as much as possible.
  gtk_widget_destroy (vbox); vbox = NULL;
  currView = NULL;
}

// Every time one editor window has a verse change, this routine is called for
// all editor window objects. That's fine, but the problem is that for the one
// that was currently focused, it has already changed its current_reference
// so this function thinks there is no more work to do for it--including
// highlighting. Therefore in Editor2::signal_if_verse_changed_timeout()
// I have added a line to highlight the current verse. This solution
// seems to work but it is not as clean as I would like it. I could
// add some more logic here, which may be cleaner...
void WindowEditor::go_to(const Reference & reference)
// Let the editor go to a reference.
{
  DEBUG("1 ref="+reference.human_readable(""))
  if (editor2 || usfmview) { // we know currView is set in this case

    // Find out what needs to be changed: book, chapter and/or verse.
    bool new_book = false;
    bool new_chapter = false;
    bool new_verse = false;

    Reference currRef = currView->current_reference_get();
    new_book = (reference.book_get() != currRef.book_get());
    new_chapter = (reference.chapter_get() != currRef.chapter_get());
    new_verse = (reference.verse_get() != currRef.verse_get());
    if (new_book) { new_chapter = true; }
    if (new_chapter) { new_verse = true; }
    
    DEBUG("2 ref="+reference.human_readable(""))
    // Save the editor if need be.
    if (new_book || new_chapter) {
      currView->chapter_save();
    }

    currView->current_reference_set(reference);

    DEBUG("3 ref="+reference.human_readable(""))
    // Deal with a new chapter.
    if (new_chapter) {
      // Load chapter, if need be.
      currView->chapter_load(reference);
      // When loading a new chapter, there is also a new verse.
      new_verse = true;
    }
    DEBUG("4 ref="+reference.human_readable(""))
    // New reference handling.  
    if (new_book || new_chapter || new_verse) {
      currView->go_to_verse(reference.verse_get(), false);
    }
    DEBUG("5 ref="+reference.human_readable(""))
    // Highlighting of searchwords.
    if (editor2) {
      if (editor2->go_to_new_reference_highlight) {
        editor2->highlight_searchwords();
        editor2->go_to_new_reference_highlight = false;
      }
    }
  }
  DEBUG("6 ref="+reference.human_readable(""))
}

void WindowEditor::load_dictionaries()
{
  currView->load_dictionaries();
}

bool WindowEditor::move_cursor_to_spelling_error (bool next, bool extremity)
{
  if (currView) {
    return currView->move_cursor_to_spelling_error (next, extremity);
  }
  gw_warning("Returning true from WindowEditor::move_cursor_to_spelling_error");
  return true;
}


void WindowEditor::undo()
{
  currView->undo(); 
}

void WindowEditor::redo()
{
  currView->redo(); 
}

bool WindowEditor::can_undo()
{
  if (currView) {
    return currView->can_undo();
  }
  gw_warning("Returning false from WindowEditor::can_undo");
  return false;
}

bool WindowEditor::can_redo()
{
  if (currView) {
    return currView->can_redo();
  }
  gw_warning("Returning false from WindowEditor::can_redo");
  return false;
}

EditorTextViewType WindowEditor::last_focused_type()
{
  if (editor2) {
    return editor2->last_focused_type();
  }
  return etvtBody;
}


vector <Reference> WindowEditor::quick_references()
{
  if (editor2) {
    return editor2->quick_references_get();
  }
  gw_warning("Returning empty Reference vector from WindowEditor::quick_references");
  vector <Reference> dummy;
  return dummy;
}


Reference WindowEditor::current_reference()
{
  if (currView) { 
    return currView->current_reference_get();
  }
  gw_warning("Returning empty Reference from WindowEditor::current_reference");
  Reference reference;
  return reference;
}


ustring WindowEditor::current_verse_number()
{
  if (currView) {
    ustring verse = currView->current_reference_get().verse_get();
    DEBUG("Returning v="+verse) 
    return verse;
  }
  gw_warning("Returning blank from WindowEditor::current_verse_number");
  return "0";
}


ustring WindowEditor::project()
{
  if (currView) {
    return currView->project_get();
  }
  gw_warning("Returning blank from WindowEditor::project");
  return "";
}


ustring WindowEditor::text_get_selection()
{
  if (currView) {
    return currView->text_get_selection();
  }
  gw_warning("Returning blank from WindowEditor::text_get_selection");
  return "";
}


void WindowEditor::insert_text(const ustring &text)
{
  currView->insert_text(text);
}


void WindowEditor::go_to_new_reference_highlight_set() 
{
  if (usfmview) {
  }
  if (editor2) {
    editor2->go_to_new_reference_highlight = true;
  }
}


ustring WindowEditor::word_double_clicked_text()
{
  if (currView) {
    return currView->word_double_clicked_text_get();
  }
  gw_warning("Returning empty string from WindowEditor::word_double_clicked_text");
  return "";
}


bool WindowEditor::editable()
{
  if (currView) { 
    return currView->editable_get();
  }
  gw_warning("Returning false from WindowEditor::editable");
  return false;
}


void WindowEditor::insert_note(const ustring& marker, const ustring& rawtext)
{
  currView->insert_note (marker, rawtext);
}

ustring WindowEditor::get_chapter()
{
  if (currView) {
    return currView->chapter_get_ustring();
  }
  gw_warning("Returning empty string from WindowEditor::get_chapter");
  return "";
}


void WindowEditor::insert_table(const ustring& rawtext)
{
  currView->insert_table (rawtext);
}


void WindowEditor::chapter_load(const Reference &ref)
{
  currView->chapter_load (ref);
}


void WindowEditor::chapter_save()
{
  currView->chapter_save();
}


unsigned int WindowEditor::reload_chapter_number()
{
  if (currView) { 
    return currView->reload_chapter_num_get();
  }
  gw_warning("Returning 0 from WindowEditor::reload_chapter_number");
  return 0;
}


void WindowEditor::apply_style(const ustring& marker)
{
  currView->apply_style (marker);
}


set <ustring> WindowEditor::get_styles_at_cursor()
{
  if (editor2) {
    return editor2->get_styles_at_cursor();
  }
  gw_warning("Returning dummy ustring set from WindowEditor::get_styles_at_cursor");
  set <ustring> dummy;
  return dummy;
}


void WindowEditor::create_or_update_formatting_data()
{
  currView->create_or_update_formatting_data();
}


void WindowEditor::set_font()
{
  currView->font_set();
}


Editor2 * WindowEditor::editor_get()
{
  if (usfmview) {
    return NULL;
  }
  return editor2; // if editor2 is NULL, we want to return NULL anyway
}


unsigned int WindowEditor::book()
{
  if (currView) { return currView->current_reference_get().book_get(); }
  gw_warning("Returning 1 from WindowEditor::book");
  return 1;
}


unsigned int WindowEditor::chapter()
{
  if (currView) { return currView->current_reference_get().chapter_get(); } 
  gw_warning("Returning 1 from WindowEditor::chapter");
  return 1;
}


void WindowEditor::on_new_verse_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_new_verse();
}


void WindowEditor::on_new_verse()
{
  gtk_button_clicked (GTK_BUTTON (new_verse_signal));
}


void WindowEditor::on_new_styles_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_new_styles();
}


void WindowEditor::on_new_styles()
{
  // Set the styles in the status bar.
  set <ustring> styles = get_styles_at_cursor();
  vector <ustring> styles2 (styles.begin(), styles.end());
  ustring text = _("Style ");
  for (unsigned int i = 0; i < styles2.size(); i++) {
    if (i) { text.append(", "); }
    text.append(styles2[i]);
  }
  status1 (text);
  // Give a signal that there are new styles.
  gtk_button_clicked (GTK_BUTTON (new_styles_signal));
}


void WindowEditor::on_quick_references_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_quick_references();
}


void WindowEditor::on_quick_references()
{
  gtk_button_clicked (GTK_BUTTON (quick_references_button));
}


void WindowEditor::on_word_double_click_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_word_double_click();
}


void WindowEditor::on_word_double_click()
{
  gtk_button_clicked (GTK_BUTTON (word_double_clicked_signal));
}


void WindowEditor::on_reload_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_reload();
}


void WindowEditor::on_reload()
{
  gtk_button_clicked (GTK_BUTTON (reload_signal));
}


void WindowEditor::on_changed_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_changed();
}


void WindowEditor::on_changed()
{
  gtk_button_clicked (GTK_BUTTON (changed_signal));
}

void WindowEditor::vt_set (viewType newvt)
{
  // Bail out if the setting does not change.
  if (currvt == newvt) { return; }

  // Take action.
  currvt = newvt;
  switch_view ();
}

void WindowEditor::switch_view ()
// Switch to the currvt view type; vtFormatted is default. vtUSFM is
// the "reveal codes" view. Assumes currvt has been set by the
// caller. Assumes a change in view will happen (that we are not
// switching from one view to the same view).
{
  DEBUG("Called with currvt="+std::to_string(int(currvt))+" and saved projectname="+projectname)
  Reference reference;
  
  // Get state of and destroy any previous view, if there was one.
  if (currView) {
    //project = currView->project_get();
    reference = currView->current_reference_get();
    // If I comment out above, program doesn't show right verse, but it also doesn't crash in Warao!
    DEBUG("reference="+reference.human_readable(""))
    DEBUG("book="+std::to_string(reference.book_get())+" ch="+std::to_string(reference.chapter_get())+" v="+reference.verse_get())
    cleanup();  // something does not get cleaned up sufficiently to make this work...see MainWindow::on_view_chapteras for details
    init(); // trying to put *this object back into the same state it was at startup...since for Warao, the first editor window works properly
  }

  // Create new view.
  switch (currvt) {
  case vtNone: break;

  case vtFormatted:
    editor2 = new Editor2 (vbox, projectname);
    currView = editor2;
    connect_focus_signals (editor2->scrolledwindow);
    hID1 = g_signal_connect ((gpointer) editor2->new_verse_signal, "clicked", G_CALLBACK(on_new_verse_signalled), gpointer(this));
    hID2 = g_signal_connect ((gpointer) editor2->new_styles_signal, "clicked", G_CALLBACK(on_new_styles_signalled), gpointer(this));
    hID3 = g_signal_connect ((gpointer) editor2->quick_references_button, "clicked", G_CALLBACK(on_quick_references_signalled), gpointer(this));
    hID4 = g_signal_connect ((gpointer) editor2->word_double_clicked_signal, "clicked", G_CALLBACK(on_word_double_click_signalled), gpointer(this));
    hID5 = g_signal_connect ((gpointer) editor2->reload_signal, "clicked", G_CALLBACK(on_reload_signalled), gpointer(this));
    hID6 = g_signal_connect ((gpointer) editor2->changed_signal, "clicked", G_CALLBACK(on_changed_signalled), gpointer(this));
    hID7 = g_signal_connect ((gpointer) editor2->spelling_checked_signal, "clicked", G_CALLBACK(on_spelling_checked_signalled), gpointer(this));
    hID8 = g_signal_connect ((gpointer) editor2->new_widget_signal, "clicked",	 G_CALLBACK(on_new_widget_signal_clicked), gpointer(this));
    last_focused_widget = editor2->last_focused_widget;			      	
    break;

  case vtUSFM:
    usfmview = new USFMView (vbox, projectname);
    currView = usfmview;
    connect_focus_signals (usfmview->sourceview);
    hID1 = g_signal_connect ((gpointer) usfmview->new_verse_signal, "clicked", G_CALLBACK(on_new_verse_signalled), gpointer(this));
    hID4 = g_signal_connect ((gpointer) usfmview->word_double_clicked_signal, "clicked", G_CALLBACK(on_word_double_click_signalled), gpointer(this));
    hID5 = g_signal_connect ((gpointer) usfmview->reload_signal, "clicked", G_CALLBACK(on_reload_signalled), gpointer(this));
    hID6 = g_signal_connect ((gpointer) usfmview->changed_signal, "clicked", G_CALLBACK(on_changed_signalled), gpointer(this));
    // See usfmview.cpp for spelling_checked_signal as well...not sure why it is not connected here.
    hID2 = 0; hID3 = 0; hID7 = 0; hID8 = 0;
    last_focused_widget = usfmview->sourceview;
    break;

  case vtExperimental: // for now, just like vtFormatted, Editor2, but will be morphing
    editor3 = new Editor3 (vbox, projectname);
    currView = editor3;
    connect_focus_signals (editor3->scrolledwindow);
    hID1 = g_signal_connect ((gpointer) editor3->new_verse_signal, "clicked", G_CALLBACK(on_new_verse_signalled), gpointer(this));
    hID2 = g_signal_connect ((gpointer) editor3->new_styles_signal, "clicked", G_CALLBACK(on_new_styles_signalled), gpointer(this));
    hID3 = g_signal_connect ((gpointer) editor3->quick_references_button, "clicked", G_CALLBACK(on_quick_references_signalled), gpointer(this));
    hID4 = g_signal_connect ((gpointer) editor3->word_double_clicked_signal, "clicked", G_CALLBACK(on_word_double_click_signalled), gpointer(this));
    hID5 = g_signal_connect ((gpointer) editor3->reload_signal, "clicked", G_CALLBACK(on_reload_signalled), gpointer(this));
    hID6 = g_signal_connect ((gpointer) editor3->changed_signal, "clicked", G_CALLBACK(on_changed_signalled), gpointer(this));
    hID7 = g_signal_connect ((gpointer) editor3->spelling_checked_signal, "clicked", G_CALLBACK(on_spelling_checked_signalled), gpointer(this));
    hID8 = g_signal_connect ((gpointer) editor3->new_widget_signal, "clicked",	 G_CALLBACK(on_new_widget_signal_clicked), gpointer(this));
    last_focused_widget = editor3->last_focused_widget;		
    break;
  }
  // Main widget grabs focus.
  gtk_widget_grab_focus (last_focused_widget);

  ustring viewName = "un-initialized";
  switch (currvt) {
      case vtNone:         viewName = _("None"); break;
      case vtFormatted:    viewName = _("Formatted View"); break;
      case vtUSFM:         viewName = _("USFM View");      break;
      case vtExperimental: viewName = _("Experimental View"); break;
  }
  title_change(projectname + " - " + viewName);

  // This call causes a hang in Warao Ps 139, but I do not find a
  // particular call in this routine that messes up.  It is
  // something about just displaying the data that does it, as if
  // the problem is in another library. Far more likely, it is our
  // input to that library through some stale state. The reality is
  // that the chapter displays fine the first time, so it seems that
  // something between the first time and the second time (with USFM
  // displayed in between) has changed.
  go_to (reference);
}


GtkTextBuffer * WindowEditor::edit_usfm_textbuffer ()
{
  GtkTextBuffer * textbuffer = NULL;
  if (usfmview) {
    textbuffer = GTK_TEXT_BUFFER (usfmview->sourcebuffer);
  }
  return textbuffer;
}


void WindowEditor::on_spelling_checked_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_spelling_checked();
}


void WindowEditor::on_spelling_checked()
{
  gtk_button_clicked (GTK_BUTTON (spelling_checked_signal));
}


void WindowEditor::spelling_trigger ()
{
  currView->spelling_trigger ();
}


void WindowEditor::on_new_widget_signal_clicked(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_new_widget_signal();
}


void WindowEditor::on_new_widget_signal ()
{
  if (editor2) {
    connect_focus_signals (editor2->new_widget_pointer);
  }
}


void WindowEditor::cut ()
{
  currView->cut();
}


void WindowEditor::copy ()
{
  currView->copy ();
}


void WindowEditor::paste ()
{
  currView->paste();
}


vector <ustring> WindowEditor::spelling_get_misspelled ()
{
  vector <ustring> misspelled_words;
  currView->spelling_get_misspelled ();
  return misspelled_words;
}


void WindowEditor::spelling_approve (const vector <ustring>& words)
{
  currView->spelling_approve (words);
}

