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


#ifndef INCLUDED_DIALOGNOTESTRANSFER_H
#define INCLUDED_DIALOGNOTESTRANSFER_H


#include <gtk/gtk.h>
#include "ustring.h"


class NotesTransferDialog
{
public:
  NotesTransferDialog (GtkWindow *transient_parent);
  ~NotesTransferDialog ();
  int run ();
protected:
  GtkWidget *notestransferdialog;
  GtkWidget *dialog_vbox1;
  GtkWidget *vbox1;
  GtkWidget *label1;
  GtkWidget *hbox1;
  GtkWidget *label2;
  GtkWidget *combobox1;
  GtkWidget *label3;
  GtkWidget *dialog_action_area1;
  GtkWidget *cancelbutton;
  GtkWidget *okbutton;
private:
  static void on_okbutton_clicked (GtkButton *button, gpointer user_data);
  void on_okbutton ();
  void clean_note (ustring& note);
  void transfer_note (const ustring& project, unsigned int book, unsigned int chapter, const ustring& verse, ustring& text, const ustring& category);
};


#endif
