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
#include "windowcheckkeyterms.h"
#include "help.h"
#include "floatingwindow.h"
#include "keyterms.h"
#include "tiny_utilities.h"
#include "utilities.h"
#include <gdk/gdkkeysyms.h>
#include "combobox.h"
#include "settings.h"
#include "projectutils.h"
#include "categorize.h"
#include "mapping.h"
#include "bible.h"
#include "books.h"
#include "xmlutils.h"
#include "gwrappers.h"
#include <glib/gi18n.h>
#include "debug.h"

WindowCheckKeyterms::WindowCheckKeyterms(GtkWidget * parent_layout, GtkAccelGroup *accelerator_group, bool startup):
  FloatingWindow(parent_layout, widCheckKeyterms, _("Check keyterms"), startup)
// Window for checking keyterms.
{
  // Save / initialize variables.
  bool newCollection = false;
  keyword_id = 0;
  text_changed_event_id = 0;
  my_editor = NULL;
  
  // Build gui.
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(vbox_client), vbox);

  // Produce the signal to be given on a new reference.
  signal = gtk_button_new();
  gtk_box_pack_start(GTK_BOX(vbox), signal, FALSE, FALSE, 0);

  hbox_collection = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox_collection);
  gtk_box_pack_start (GTK_BOX (vbox), hbox_collection, FALSE, FALSE, 0);

  label_collection = gtk_label_new_with_mnemonic (_("_Collection"));
  gtk_widget_show (label_collection);
  gtk_box_pack_start (GTK_BOX (hbox_collection), label_collection, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label_collection), 0, 0.5);

  combobox_collection = gtk_combo_box_text_new ();
  gtk_widget_show (combobox_collection);
  gtk_box_pack_start (GTK_BOX (hbox_collection), combobox_collection, TRUE, TRUE, 0);

  connect_focus_signals (combobox_collection);
  
  label_list = gtk_label_new_with_mnemonic (_("_List"));
  gtk_widget_show (label_list);
  gtk_box_pack_start (GTK_BOX (vbox), label_list, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label_list), 0, 0.5);

  scrolledwindow_terms = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow_terms);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow_terms, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_terms), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_terms), GTK_SHADOW_IN);

  webview_terms = webkit_web_view_new();
  gtk_widget_show (webview_terms);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_terms), webview_terms);

  connect_focus_signals (webview_terms);
  
  // Store for the renderings.
  treestore_renderings = gtk_tree_store_new(4, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_BOOLEAN);

  treeview_renderings = gtk_tree_view_new_with_model(GTK_TREE_MODEL(treestore_renderings));
  gtk_widget_show(treeview_renderings);
  gtk_box_pack_start(GTK_BOX(vbox), treeview_renderings, false, false, 0);

  connect_focus_signals (treeview_renderings);

  // Renderer, column and selection.
  GtkCellRenderer *renderer_renderings = gtk_cell_renderer_toggle_new();
  g_signal_connect(renderer_renderings, "toggled", G_CALLBACK(keyterm_whole_word_toggled), gpointer(this));
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Whole\nword", renderer_renderings, "active", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_renderings), column);
  renderer_renderings = gtk_cell_renderer_toggle_new();
  g_signal_connect(renderer_renderings, "toggled", G_CALLBACK(keyterm_case_sensitive_toggled), gpointer(this));
  column = gtk_tree_view_column_new_with_attributes(_("Case\nsensitive"), renderer_renderings, "active", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_renderings), column);
  renderer_renderings = gtk_cell_renderer_text_new();
  g_signal_connect(renderer_renderings, "edited", G_CALLBACK(cell_edited), gpointer(this));
  column = gtk_tree_view_column_new_with_attributes(_("Rendering"), renderer_renderings, "text", 2, "editable", 3, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_renderings), column);
  treeselect_renderings = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_renderings));
  gtk_tree_selection_set_mode(treeselect_renderings, GTK_SELECTION_SINGLE);

  g_signal_connect((gpointer) webview_terms, "decide-policy", G_CALLBACK(on_decide_policy_cb), gpointer(this));
  g_signal_connect((gpointer) combobox_collection, "changed", G_CALLBACK(on_combobox_keyterm_collection_changed), gpointer(this));

  gtk_label_set_mnemonic_widget(GTK_LABEL(label_collection), combobox_collection);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label_list), webview_terms);

  // Load the categories.
  reload_collections ();

  // Load the keyterms.
  on_combobox_keyterm_collection ();
  
  // Main focused widget.
  last_focused_widget = combobox_collection;
  gtk_widget_grab_focus (last_focused_widget);
  
  set_fonts ();
}


WindowCheckKeyterms::~WindowCheckKeyterms()
{
  my_editor = NULL;
  gw_destroy_source (text_changed_event_id);
}


void WindowCheckKeyterms::go_to_term(unsigned int id)
{
  ustring url = _("keyterm ") + convert_to_string (id);
  webview_process_navigation (url);
}


void WindowCheckKeyterms::copy_clipboard()
{
  if (gtk_widget_has_focus (webview_terms)) {
    // Copy text to the clipboard.
    // I know, I should  use webkit_web_view_can_execute_editing_command() to check whether it's possible to execute the command.
    webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW (webview_terms),
					  WEBKIT_EDITING_COMMAND_COPY);

    // Add the selected text to the renderings.
    GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    char * text = gtk_clipboard_wait_for_text (clipboard);
    if (text) {
      add_to_renderings (text, false);
      free (text); 
    }
  }
}


void WindowCheckKeyterms::on_combobox_keyterm_collection_changed(GtkComboBox * combobox, gpointer user_data)
{
  ((WindowCheckKeyterms *) user_data)->on_combobox_keyterm_collection();
}


void WindowCheckKeyterms::keyterm_whole_word_toggled(GtkCellRendererToggle * cell, gchar * path_str, gpointer data)
{
  ((WindowCheckKeyterms *) data)->on_rendering_toggle(cell, path_str, true);
}


void WindowCheckKeyterms::keyterm_case_sensitive_toggled(GtkCellRendererToggle * cell, gchar * path_str, gpointer data)
{
  ((WindowCheckKeyterms *) data)->on_rendering_toggle(cell, path_str, false);
}


void WindowCheckKeyterms::cell_edited(GtkCellRendererText * cell, const gchar * path_string, const gchar * new_text, gpointer data)
{
  ((WindowCheckKeyterms *) data)->on_cell_edited(cell, path_string, new_text);
}


void WindowCheckKeyterms::on_combobox_keyterm_collection()
{
  newCollection = true;
  webview_process_navigation(collection());
  newCollection = false;
}


void WindowCheckKeyterms::load_renderings()
{
  extern Settings *settings;
  ustring project = settings->genconfig.project_get();
  ProjectConfiguration *projectconfig = settings->projectconfig(project);
  ustring versification = projectconfig->versification_get();
  ustring keyterm;
  keyterms_get_term(keyword_id, keyterm);
  vector <ustring> renderings;
  vector <bool> wholewords;
  vector <bool> casesensitives;
  ustring category;
  {
    ustring dummy1;
    vector < Reference > dummy2;
    keyterms_get_data(keyword_id, category, dummy1, dummy2);
  }
  keyterms_retrieve_renderings(project, keyterm, category, renderings, wholewords, casesensitives);
  clear_renderings();
  GtkTreeIter iter;
  for (unsigned int i = 0; i < renderings.size(); i++) {
    gtk_tree_store_append(treestore_renderings, &iter, NULL);
    bool wholeword = wholewords[i];
    bool casesensitive = casesensitives[i];
    gtk_tree_store_set(treestore_renderings, &iter, 0, wholeword, 1, casesensitive, 2, renderings[i].c_str(), 3, 1, -1);
  }
  gtk_tree_store_append(treestore_renderings, &iter, NULL);
  gtk_tree_store_set(treestore_renderings, &iter, 0, false, 1, true, 2, enter_new_rendering_here().c_str(), 3, 1, -1);
}


void WindowCheckKeyterms::save_renderings()
{
  vector <ustring> renderings;
  vector <bool> wholewords;
  vector <bool> casesensitives;
  get_renderings(renderings, wholewords, casesensitives);
  ustring keyterm;
  keyterms_get_term(keyword_id, keyterm);
  ustring category;
  {
    ustring dummy1;
    vector < Reference > dummy2;
    keyterms_get_data(keyword_id, category, dummy1, dummy2);
  }
  extern Settings *settings;
  ustring project = settings->genconfig.project_get();
  keyterms_store_renderings(project, keyterm, category, renderings, wholewords, casesensitives);
  load_renderings();
  webview_process_navigation (last_keyword_url);
}


void WindowCheckKeyterms::clear_renderings()
{
  gtk_tree_store_clear(treestore_renderings);
}


void WindowCheckKeyterms::on_rendering_toggle(GtkCellRendererToggle * cell, gchar * path_str, bool first_toggle)
{
  unsigned int column = 1;
  if (first_toggle)
    column = 0;
  GtkTreeModel *model = (GtkTreeModel *) treestore_renderings;
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
  gboolean setting;
  gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_model_get(model, &iter, column, &setting, -1);
  setting = !setting;
  gtk_tree_store_set(treestore_renderings, &iter, column, setting, -1);
  gtk_tree_path_free(path);
  save_renderings();
}


void WindowCheckKeyterms::on_cell_edited(GtkCellRendererText * cell, const gchar * path_string, const gchar * new_text)
{
  GtkTreeModel *model = (GtkTreeModel *) treestore_renderings;
  GtkTreePath *path = gtk_tree_path_new_from_string(path_string);
  GtkTreeIter iter;
  gtk_tree_model_get_iter(model, &iter, path);
  gchar *old_text;
  gtk_tree_model_get(model, &iter, 2, &old_text, -1);
  g_free(old_text);
  gtk_tree_store_set(treestore_renderings, &iter, 2, g_strdup(new_text), -1);
  gtk_tree_path_free(path);
  save_renderings();
}


void WindowCheckKeyterms::add_to_renderings(const ustring & rendering, bool wholeword)
// Adds "rendering" to renderings. If it contains any capitals, the 
// casesensitive is set too.
{
  ustring keyterm;
  keyterms_get_term(keyword_id, keyterm);
  GtkTreeIter iter;
  gtk_tree_store_append(treestore_renderings, &iter, NULL);
  bool casesensitive = rendering != rendering.casefold();
  gtk_tree_store_set(treestore_renderings, &iter, 0, wholeword, 1, casesensitive, 2, rendering.c_str(), 3, 1, -1);
  save_renderings();
}


bool WindowCheckKeyterms::find_renderings (const ustring& text, const vector <ustring>& renderings, const vector <bool>& wholewords, const vector <bool>& casesensitives, vector <size_t> * startpositions, vector <size_t> * lengths)
// Finds renderings in the text.
// text: Text to be looked into.
// renderings: Renderings to look for.
// wholewords / casesensitives: Attributes of the renderings.
// startpositions: If non-NULL, will be filled with the positions that each rendering starts at.
// lengths: If non-NULL, will be filled with the lengths of the renderings found.
// Returns whether one or more renderings were found in the verse.
{
  if (startpositions)
    startpositions->clear();
  if (lengths)
    lengths->clear();

  GtkTextBuffer * textbuffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_set_text (textbuffer, text.c_str(), -1);
  GtkTextIter startiter;
  gtk_text_buffer_get_start_iter(textbuffer, &startiter);

  bool found = false;

  for (unsigned int i2 = 0; i2 < renderings.size(); i2++) {

    ustring rendering = renderings[i2];
    bool wholeword = wholewords[i2];
    bool casesensitive = casesensitives[i2];

    ustring mytext;
    ustring myrendering;
    if (casesensitive) {
      mytext = text;
      myrendering = rendering;
    } else {
      mytext = text.casefold();
      myrendering = rendering.casefold();
    }
    size_t position = mytext.find(myrendering);
    while (position != string::npos) {
      bool temporally_approved = true;
      GtkTextIter approvedstart = startiter;
      GtkTextIter approvedend;
      gtk_text_iter_forward_chars(&approvedstart, position);
      approvedend = approvedstart;
      gtk_text_iter_forward_chars(&approvedend, rendering.length());
      if (wholeword) {
        if (!gtk_text_iter_starts_word(&approvedstart))
          temporally_approved = false;
        if (!gtk_text_iter_ends_word(&approvedend))
          temporally_approved = false;
      }
      if (temporally_approved) {
        found = true;
        if (startpositions)
          startpositions->push_back (position);
        if (lengths)
          lengths->push_back (rendering.length());
      }
      position = mytext.find(myrendering, ++position);
    }
  }

  g_object_unref (textbuffer);  
  
  return found;
}


ustring WindowCheckKeyterms::enter_new_rendering_here()
{
  return _("<Enter new rendering here>");
}


void WindowCheckKeyterms::get_renderings(vector <ustring> &renderings, vector <bool> &wholewords, vector <bool> &casesensitives)
{
  GtkTreeModel *model = (GtkTreeModel *) treestore_renderings;
  GtkTreeIter iter;
  gboolean valid;
  valid = gtk_tree_model_get_iter_first(model, &iter);
  while (valid) {
    int wholeword, casesensitive;
    gchar *str_data;
    gtk_tree_model_get(model, &iter, 0, &wholeword, 1, &casesensitive, 2, &str_data, -1);
    ustring rendering(str_data);
    if (!rendering.empty()) {
      if (rendering != enter_new_rendering_here()) {
        renderings.push_back(rendering);
        wholewords.push_back(wholeword);
        casesensitives.push_back(casesensitive);
      }
    }
    g_free(str_data);
    valid = gtk_tree_model_iter_next(model, &iter);
  }
}


ustring WindowCheckKeyterms::collection ()
{
  return combobox_get_active_string(combobox_collection);
}

gboolean
WindowCheckKeyterms::on_decide_policy_cb (WebKitWebView           *web_view,
					  WebKitPolicyDecision    *decision,
					  WebKitPolicyDecisionType decision_type,
					  gpointer                 user_data)
{
  ((WindowCheckKeyterms *) user_data)->decide_policy_cb (web_view, decision, decision_type);
  return true;
}

// Called by webview_simple::decide_policy_cb
// newCollection=true if we are loading the index of an entirely new set of keywords
void WindowCheckKeyterms::webview_process_navigation (const ustring& url)
{
  // Store scrolling position for the now active url.
  GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow_terms));
  scrolling_position[active_url] = gtk_adjustment_get_value (adjustment);

  //DEBUG("remember old scroll position="+std::to_string(scrolling_position[active_url])+" for old active_url="+active_url)
  //DEBUG("active_url="+active_url+" new url="+ustring(url))
  
  // New url.
  active_url = url;

  // Whether to show some widgets.
  bool show_collections = false;
  bool show_renderings = false;
  
  // Start writing a html page.
  HtmlWriter2 htmlwriter ("");
  bool display_another_page = false;

  if (!newCollection && (active_url.find (_("keyterm ")) == 0)) {
    // Store url of this keyterm.
    last_keyword_url = active_url;
    // Get the keyterm identifier.
    ustring url = active_url;
    url.erase (0, 8);
    keyword_id = convert_to_int (url);
    // Load the renderings. 
    // To be done before displaying the verses themselves since the latter depends on the former.
    load_renderings ();
    // Write extra bits.
    html_write_keyterms (htmlwriter, keyword_id);
    show_renderings = true;
    display_another_page = true;
  }

  else if (!newCollection && (active_url.find ("goto ") == 0)) {
    // Signal the editors to go to a reference.
    ustring url = active_url;
    url.erase (0, 5);
    myreference.assign (Reference(url));
    new_reference_showing = &myreference;
    gtk_button_clicked(GTK_BUTTON(signal));
  }
  
  else if (!newCollection && (active_url.find ("send") == 0)) {
    // Send the references to the references window.
    ustring url = active_url;
    new_reference_showing = NULL;
    gtk_button_clicked(GTK_BUTTON(signal));
  }
  
  else if (newCollection || (collection() == active_url)) {
    // Give the starting page with all keyterms of the active selection.
    // We are either coming here "fresh" (newCollection==true) or
    // coming back after looking at a particular keyterm list.
    show_collections = true;
    if (collection().find (_("Biblical")) != string::npos) {
      if (collection().find (_("Hebrew")) != string::npos) {
        htmlwriter.paragraph_open ();
        htmlwriter.text_add (_("Key Terms in Biblical Hebrew: The entries are an experimental sample set, not yet fully reviewed and approved. The KTBH team would welcome feed-back to christopher_samuel@sil.org."));
        htmlwriter.paragraph_close ();
      }
    }
    vector <ustring> terms;
    vector <unsigned int> ids;
    keyterms_get_terms("", collection(), terms, ids);
    for (unsigned int i = 0; i < terms.size(); i++) {
      htmlwriter.paragraph_open();
      htmlwriter.hyperlink_add ("keyterm " + convert_to_string (ids[i]), terms[i]);
      htmlwriter.paragraph_close();
    }
    display_another_page = true;
    // No renderings.
    clear_renderings ();
  }
  else { 
    gw_warning("Ran into a keyterm case I am not expecting...");
  }
  
  htmlwriter.finish();
  if (display_another_page) {
    // Load the page.
    webkit_web_view_load_html (WEBKIT_WEB_VIEW (webview_terms), htmlwriter.html.c_str(), NULL);
    // Scroll to the position that possibly was stored while this url was last active.
    // In order to do this, we need to let the web_view window fully load, and after it
    // is done, then we can instruct it to change the vertical adjustment. I found this
    // work-around on a couple of programming sites. MAP 8/28/2017.
    while (gtk_events_pending()) { gtk_main_iteration(); }
    GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow_terms));
    gtk_adjustment_set_value (adjustment, scrolling_position[active_url]);
    gtk_adjustment_value_changed(adjustment);
    //DEBUG("set new scroll position="+std::to_string(scrolling_position[active_url])+" for new active_url="+active_url)
    // Whether to show collections.
    if (show_collections) { gtk_widget_show (hbox_collection); }
    else                  { gtk_widget_hide (hbox_collection); }
    if (show_renderings)  { gtk_widget_show (treeview_renderings); }
    else                  { gtk_widget_hide (treeview_renderings); }
  }
}


void WindowCheckKeyterms::html_write_keyterms (HtmlWriter2& htmlwriter, unsigned int keyword_id)
{
  // Get data about the project.
  extern Settings *settings;
  ustring project = settings->genconfig.project_get();
  ProjectConfiguration *projectconfig = settings->projectconfig(project);
  ustring versification = projectconfig->versification_get();

  // Add action links at the top of the page.
  htmlwriter.paragraph_open ();
  //htmlwriter.hyperlink_add ("index", _("[Index]"));
  htmlwriter.hyperlink_add (collection(), _("[Index]"));
  htmlwriter.text_add (" ");
  htmlwriter.hyperlink_add ("send", _("[Send to references window]"));
  htmlwriter.paragraph_close ();

  // Retrieve the renderings.
  vector <ustring> renderings;
  vector <bool> wholewords;
  vector <bool> casesensitives;
  get_renderings(renderings, wholewords, casesensitives);

  // Get the data for the keyword identifier.
  ustring dummy;
  ustring information;
  keyterms_get_data(keyword_id, dummy, information, references);

  // Divide the information into lines.
  ParseLine parseline (information);

  // Add the keyterm itself.
  ustring keyterm;
  keyterms_get_term(keyword_id, keyterm);
  htmlwriter.heading_open (3);
  htmlwriter.text_add (keyterm);
  htmlwriter.heading_close();
  htmlwriter.heading_open(3);
  htmlwriter.text_add (std::to_string(parseline.lines.size()) + " verses");
  htmlwriter.heading_close();
  
  // Write the information.
  for (unsigned int i = 0; i < parseline.lines.size(); i++) {
 
    information = parseline.lines[i];
    htmlwriter.paragraph_open ();
    size_t pos = information.find (keyterms_reference_start_markup ());
    while (pos != string::npos) {
      htmlwriter.text_add (information.substr (0, pos));
      information.erase (0, pos + keyterms_reference_start_markup ().length());
      pos = information.find (keyterms_reference_end_markup ());
      if (pos != string::npos) {
        // Extract the reference.
        htmlwriter.paragraph_close ();
        ustring original_reference_text = information.substr (0, pos);
        Reference reference = Reference(original_reference_text);
        // Remap the reference.
        {
          Mapping mapping(versification, reference.book_get());
          vector <int> chapters;
          vector <int> verses;
          mapping.original_to_me(reference.chapter_get(), reference.verse_get(), chapters, verses);
          if (!chapters.empty()) {
            reference.chapter_set(chapters[0]);
            reference.verse_set(convert_to_string (verses[0]));
          }
        }
        ustring remapped_reference_text = reference.human_readable ("");
        ustring displayed_reference_text (remapped_reference_text);
        if (remapped_reference_text != original_reference_text) {
          displayed_reference_text.append (" (");
          displayed_reference_text.append (original_reference_text);
          displayed_reference_text.append (")");
        }
        // Add the reference with hyperlink.
        htmlwriter.hyperlink_add ("goto " + remapped_reference_text, remapped_reference_text);
        information.erase (0, pos + keyterms_reference_end_markup ().length());
        // Add the reference's text.
        ustring verse = project_retrieve_verse(project, reference);
        if (verse.empty()) {
          verse.append(_("<empty>"));
        } else {
	  // Sometimes, a verse (like KJV Psalm 64:5) has \n in the middle of a footnote.
	  // CategorizeLine splits that into two pieces, thus splitting a footnote in half.
	  // This does not parse properly, and the parsing code gets into an infinite loop.
	  // Since this is a single verse, we know that we can eliminate any \n in it and
	  // it should be fine. MAP 4/3/2017
	  replace_text(verse, "\n", " ");
          CategorizeLine cl(verse);
          cl.remove_verse_number(reference.verse_get());
          verse = cl.verse;
        }
        htmlwriter.text_add (" ");

        // Add the verse plus markup for approved text.
        vector <size_t> startpositions;
        vector <size_t> lengths;
        size_t processposition = 0;
        if (find_renderings (verse, renderings, wholewords, casesensitives, &startpositions, &lengths)) {
          quick_sort (startpositions, lengths, 0, startpositions.size());
          // Overlapping items need to be combined to avoid crashes.
          xml_combine_overlaps (startpositions, lengths);
          for (unsigned int i = 0; i < startpositions.size(); i++) {
            htmlwriter.text_add (verse.substr (0, startpositions[i] - processposition));
            htmlwriter.bold_open();
            htmlwriter.text_add (verse.substr (startpositions[i] - processposition, lengths[i]));
            htmlwriter.bold_close();
            verse.erase (0, startpositions[i] - processposition + lengths[i]);
            processposition = startpositions[i] + lengths[i];
          }
          // Add whatever is left over of the verse. This could be the full verse in case it wasn't processed.
          htmlwriter.text_add (verse);
        }
        else
        {
        	htmlwriter.highlight_open();
        	htmlwriter.text_add (verse);
        	htmlwriter.highlight_close();
        }

        // Proceed to next.
        htmlwriter.paragraph_open ();
        pos = information.find (keyterms_reference_start_markup ());
      }
    }
    htmlwriter.text_add (information);
    htmlwriter.paragraph_close ();
  }
  
  // Add action links at the bottom of the page.
  htmlwriter.paragraph_open ();
  htmlwriter.hyperlink_add (collection(), _("[Index]"));
  htmlwriter.text_add (" ");
  htmlwriter.hyperlink_add ("send", _("[Send to references window]"));
  htmlwriter.paragraph_close ();
}


void WindowCheckKeyterms::reload_collections ()
{
  vector <ustring> categories = keyterms_get_categories();
  combobox_set_strings(combobox_collection, categories);
  if (!categories.empty()) {
    combobox_set_index(combobox_collection, 0);
  }
}


void WindowCheckKeyterms::text_changed (Editor2 * editor)
// To be called when the text in any of the USFM editors changed.
{
  gw_destroy_source (text_changed_event_id);
  text_changed_event_id = g_timeout_add_full (G_PRIORITY_DEFAULT, 500, GSourceFunc (on_text_changed_timeout), gpointer (this), NULL);
  my_editor = editor;
}


gboolean WindowCheckKeyterms::on_text_changed_timeout (gpointer user_data)
{
  ((WindowCheckKeyterms *) user_data)->on_text_changed();
  return false;
}


void WindowCheckKeyterms::on_text_changed ()

{
  if (active_url == last_keyword_url) {
    if (my_editor) {
      my_editor->chapter_save ();
    }
    my_editor = NULL;
    webview_process_navigation (last_keyword_url);
  }
}


void WindowCheckKeyterms::set_fonts()
{
  extern Settings *settings;
  if (!settings->genconfig.text_editor_font_default_get()) {
    PangoFontDescription *desired_font_description = pango_font_description_from_string(settings->genconfig.text_editor_font_name_get().c_str());
    const char * desired_font_family = pango_font_description_get_family (desired_font_description);
    WebKitSettings * webkit_settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (webview_terms));
    // Not 100% sure on following...
    g_object_set (G_OBJECT (webkit_settings), "default-font-family", desired_font_family, NULL);
    pango_font_description_free (desired_font_description);
  }
}
