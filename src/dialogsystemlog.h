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


#ifndef INCLUDED_DIALOG_SYSTEMLOG
#define INCLUDED_DIALOG_SYSTEMLOG


#include <gtk/gtk.h>
#include "libraries.h"


enum LogFileType {lftMain, lftDbus, lftVCS, lftShutdown, lftSettings};


ustring log_file_name(LogFileType type, bool previous);
void move_log_file (LogFileType type);


class SystemlogDialog
{
public:
  SystemlogDialog(int dummy);
  ~SystemlogDialog();
  GtkWidget *getDialog() { return dialog; }
  void reset(void);
  void setup(void);
  void close(void);
  int run();
  static gboolean on_system_log_key_press_activate(GtkWidget *dlg, GdkEventKey *event, gpointer user_data);
  static gboolean on_system_log_delete_event_activate(GtkWidget *dlg, gpointer user_data);
  static void     on_system_log_destroy_activate(GtkWidget *dlg, gpointer user_data);
  void on_system_log_destroy(void);
  
protected:
  GtkBuilder *gtkbuilder;
  GtkWidget *dialog;
  GtkWidget *textview;
  GtkWidget *checkbutton_session;
  GtkWidget *radiobutton_main;
  GtkWidget *radiobutton_shutdown;
  //GtkWidget *button_diag;
  GtkWidget *radiobutton_settings; // replaces old "Diagnostics" button
private:
  guint event_source_id;
  static bool show_script_dialog_load(gpointer data);
  void load(bool force);
  ustring logfilename();
  LogFileType currentLogFileType();
  static void on_checkbutton1_toggled(GtkToggleButton *togglebutton, gpointer user_data);
  //static void on_button_diagnostics_clicked(GtkButton *button, gpointer user_data);
  //void on_button_diagnostics();
  void writeSettings();
  static void on_radiobutton_toggled (GtkToggleButton *togglebutton, gpointer user_data);
  void on_radiobutton (GtkToggleButton *togglebutton);
};


#endif
