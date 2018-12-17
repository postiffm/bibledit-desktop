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

#ifndef INCLUDED_WINDOW_TABBED_H
#define INCLUDED_WINDOW_TABBED_H

#include <gtk/gtk.h>
#include "ustring.h"
#include "reference.h"
#include "floatingwindow.h"
#include "htmlwriter2.h"
#include "editor.h"
#include <webkit/webkit.h>

//----------------------------------------------------------------------------------
// The tabbed window is a GTK notebook that can display information about the text
// or various built-in Bible versions. The first tab I created for this type window 
// was the concordance function, specifically the word frequency list. Eventually
// I would like to move the references, related verses, keyterms, and perhaps
// others into this structure to consolidate the view of things. The ultimate intent
// is to have multiple tabbed windows open at once.
//----------------------------------------------------------------------------------

class WindowTabbed; // forward declaration

// A tabbed (notebook) window can contain any number of SingleTab's. Each "thing" 
// that is intended to go into a tab should inherit from this class. For instance,
// a concordance tab "is" a SingleTab.
class SingleTab
{
public:
    SingleTab(const ustring &_title, HtmlWriter2 &html, GtkWidget *notebook, WindowTabbed *_parent);
    ~SingleTab();
    void updateHtml(HtmlWriter2 &html);
    // I might not have to store any of these...
    GtkWidget *scrolledwindow; // owned by the notebook, I think
    GtkWidget *tab_label; // owned by the notebook, I think
	GtkWidget *webview; // owned by scrolled window
private:
    ustring title;
    WindowTabbed *parent;
    friend class WindowTabbed;
    
    // Callbacks. These routines are replicated several times throughout the code base. Any way to refactor so as to simplify?
    static gboolean on_navigation_policy_decision_requested (WebKitWebView *web_view, WebKitWebFrame *frame, WebKitNetworkRequest *request, WebKitWebNavigationAction *navigation_action, WebKitWebPolicyDecision *policy_decision, gpointer user_data);
    void navigation_policy_decision_requested (WebKitNetworkRequest *request, WebKitWebNavigationAction *navigation_action, WebKitWebPolicyDecision *policy_decision);
    void html_link_clicked (const gchar * url);
    Reference get_reference (const ustring& text);
};

class WindowTabbed : public FloatingWindow
{
    friend class SingleTab;
public:
    WindowTabbed(ustring _title, GtkWidget * parent_layout, GtkAccelGroup *accelerator_group, bool startup);
    virtual ~WindowTabbed();
    void Concordance(const ustring &projname);
    void newTab(const ustring &tabTitle, HtmlWriter2 &tabHtml);    // create a new tab, fill with given content
    void updateTab(const ustring &tabTitle, HtmlWriter2 &tabHtml); // update existing tab, all new content
    GtkWidget * signalVerseChange;
    Reference *newReference; // for when a click in this window wants to navigate the program to a new Bible verse
  protected:
    ustring active_url;  
    Reference myreference;
  private:
    ustring title;
    GtkWidget *vbox;
    GtkWidget *notebook;
    vector<SingleTab *> tabs;
    bool ready;
 public:
    void setReady(bool _ready) { ready = _ready; }
    bool getReady(void)        { return ready; }
    
#if 0
public:
  void go_to_term(unsigned int id);
  void copy_clipboard();
  Reference * new_reference_showing;
  GtkWidget *signal;
  ustring collection ();
  void reload_collections ();
  vector <Reference> references;
  void set_fonts ();
private:
  Reference myreference;
  
  // Widgets.
  GtkWidget *hbox_collection;
  GtkWidget *label_collection;
  GtkWidget *combobox_collection;
  GtkWidget *label_list;
  GtkWidget *scrolledwindow_terms;
  GtkWidget *webview_terms;
  GtkWidget *treeview_renderings;

  // Underlying constructions.
  GtkTreeStore *treestore_renderings;
  GtkTreeSelection *treeselect_renderings;

  // Callbacks.
  static gboolean on_navigation_policy_decision_requested (WebKitWebView *web_view, WebKitWebFrame *frame, WebKitNetworkRequest *request, WebKitWebNavigationAction *navigation_action, WebKitWebPolicyDecision *policy_decision, gpointer user_data);
  void navigation_policy_decision_requested (WebKitNetworkRequest *request, WebKitWebNavigationAction *navigation_action, WebKitWebPolicyDecision *policy_decision);
  void html_link_clicked (const gchar * url);
  static void on_combobox_keyterm_collection_changed(GtkComboBox *combobox, gpointer user_data);
  static void keyterm_whole_word_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data);
  static void keyterm_case_sensitive_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data);
  static void cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data);

  // Action routines.
  void on_combobox_keyterm_collection();
  void load_renderings();
  void save_renderings();
  void clear_renderings();
  void on_rendering_toggle(GtkCellRendererToggle *cell, gchar *path_str, bool first_toggle);
  void on_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text);
  void add_to_renderings(const ustring& rendering, bool wholeword);
  bool find_renderings (const ustring& text, const vector <ustring>& renderings, const vector <bool>& wholewords, const vector <bool>& casesensitives, vector <size_t> * startpositions, vector <size_t> * lengths);
  vector <ustring> keyterm_text_selection;

  // Data routines.
  ustring enter_new_rendering_here();
  void get_renderings(vector <ustring>& renderings, vector<bool>& wholewords, vector<bool>& casesensitives);
  Reference get_reference (const ustring& text);

  // Variables.
  unsigned int keyword_id;
  
  // Html work.
  ustring last_keyword_url;
  map <ustring, unsigned int> scrolling_position;
  void html_write_keyterms (HtmlWriter2& htmlwriter, unsigned int keyword_id);

  // Text changed signalling.
public:
  void text_changed (Editor2 * editor);
private:
  guint text_changed_event_id;
  Editor2 * my_editor;
  static gboolean on_text_changed_timeout (gpointer user_data);
  void on_text_changed ();
#endif

};


#endif
