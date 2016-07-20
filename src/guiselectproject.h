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

#ifndef INCLUDED_GUISELECTPROJECT_H
#define INCLUDED_GUISELECTPROJECT_H

#include "ustring.h"
#include <gtk/gtk.h>

class SelectProjectGui {
public:
  SelectProjectGui(int dummy);
  ~SelectProjectGui();
  void build(GtkWidget *box, const gchar *labeltext, const ustring &project_in);
  void focus();
  void set_label(const ustring &text);
  void set_sensitive(bool active);
  ustring project;

private:
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *hbox;
  static void on_button_clicked(GtkButton *button, gpointer user_data);
  void on_button();
};

#endif
