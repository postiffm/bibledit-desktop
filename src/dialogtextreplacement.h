/*
** Copyright (©) 2003 The Free Software Foundation.
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

#ifndef INCLUDED_DIALOG_TEXT_REPLACEMENT_H
#define INCLUDED_DIALOG_TEXT_REPLACEMENT_H

#include "ustring.h"
#include <gtk/gtk.h>

class TextReplacementDialog {
public:
  TextReplacementDialog(int dummy);
  ~TextReplacementDialog();
  int run();

protected:
  GtkWidget *textreplacementdialog;
  GtkWidget *dialog_vbox1;
  GtkWidget *vbox1;
  GtkWidget *checkbutton1;
  GtkWidget *scrolledwindow1;
  GtkWidget *treeview1;
  GtkWidget *dialog_action_area1;
  GtkWidget *cancelbutton1;
  GtkWidget *okbutton1;
  static void on_checkbutton1_toggled(GtkToggleButton *togglebutton,
                                      gpointer user_data);
  void on_checkbutton1();
  static void cell_text_edited(GtkCellRendererText *cell,
                               const gchar *path_string, const gchar *new_text,
                               gpointer data);
  static void cell_replacement_edited(GtkCellRendererText *cell,
                                      const gchar *path_string,
                                      const gchar *new_text, gpointer data);
  void on_cell_edited(GtkCellRendererText *cell, const gchar *path_string,
                      const gchar *new_text, int column);
  static void on_okbutton1_clicked(GtkButton *button, gpointer user_data);
  void on_okbutton();
  void gui();

private:
  GtkListStore *model;
  const char *enter_new_text_here();
  const char *enter_new_replacement_here();
  ustring original_get(GtkTreeModel *model, GtkTreeIter *iter);
  ustring replacement_get(GtkTreeModel *model, GtkTreeIter *iter);
};

#endif
