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

#ifndef INCLUDED_DIALOG_REFERENCE_SETTINGS
#define INCLUDED_DIALOG_REFERENCE_SETTINGS

#include "libraries.h"
#include "notes_utils.h"
#include <gtk/gtk.h>

class ReferenceSettingsDialog {
public:
  ReferenceSettingsDialog(int dummy);
  ~ReferenceSettingsDialog();
  int run();

protected:
  GtkBuilder *gtkbuilder;
  GtkWidget *dialog;
  GtkWidget *checkbutton_verse_text;
  GtkWidget *checkbutton_relevant_bits;
  GtkWidget *cancelbutton;
  GtkWidget *okbutton;

private:
  static void on_checkbutton_verse_text_toggled(GtkToggleButton *togglebutton,
                                                gpointer user_data);
  void set_gui();
  static void on_okbutton_clicked(GtkButton *button, gpointer user_data);
  void on_ok();
};

#endif
