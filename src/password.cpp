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
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 *USA.
 **
 */

#include "password.h"
#include "dialogentry.h"
#include "gtkwrappers.h"
#include "settings.h"
#include <glib/gi18n.h>

void password_edit(GtkWidget *window)
// The routine to edit the administrator's password.
{
  // Get the current administrator's password.
  extern Settings *settings;
  ustring currentpassword = settings->genconfig.administration_password_get();

  // If the password is set, first ask for the current password before
  // proceeding.
  if (!currentpassword.empty()) {
    EntryDialog dialog(
        _("Password"),
        _("Please provide the current password before proceeding"), "");
    dialog.text_invisible();
    if (dialog.run() != GTK_RESPONSE_OK)
      return;
    if (dialog.entered_value != currentpassword) {
      gtkw_dialog_warning(window, _("The password provided is incorrect"));
      return;
    }
  }
  // Ask whether the user wishes to set a(nother) password.
  ustring message;
  if (currentpassword.empty()) {
    message.append(_("The current password is unset"));
  } else {
    message.append(_("The current password is set"));
  }
  message.append("\n\n");
  if (currentpassword.empty()) {
    message.append("Would you like to set an administrator's password?");
  } else {
    message.append("Would you like to set another administrator's password?");
  }

  if (gtkw_dialog_question(window, message, GTK_RESPONSE_YES) !=
      GTK_RESPONSE_YES)
    return;

  // Set another password.
  ustring password1;
  {
    EntryDialog dialog1(_("Password"), _("Provide a new password"), "");
    dialog1.always_ok();
    dialog1.text_invisible();
    if (dialog1.run() != GTK_RESPONSE_OK)
      return;
    password1 = dialog1.entered_value;
  }
  ustring password2;
  {
    EntryDialog dialog2(_("Password"), _("Repeat this password"), "");
    dialog2.always_ok();
    dialog2.text_invisible();
    if (dialog2.run() != GTK_RESPONSE_OK)
      return;
    password2 = dialog2.entered_value;
  }
  if (password1 != password2) {
    gtkw_dialog_warning(window, _("The two passwords differ"));
    return;
  }
  // Store the new password.
  settings->genconfig.administration_password_set(password1);

  // Give feedback.
  if (password1.empty()) {
    message.append("The password has been erased");
  } else {
    message.append("The password has been set");
  }
  gtkw_dialog_info(window, message);
}

bool password_pass(GtkWidget *window)
// Ask the user for a password.
// Returns true if the user provides a correct password.
{
  // Retrieve the password.
  extern Settings *settings;
  ustring currentpassword = settings->genconfig.administration_password_get();

  // Return true if the password is unset.
  if (currentpassword.empty())
    return true;

  // Request the user to provide the password.
  EntryDialog dialog(_("Password"), _("Provide the administrator's password"),
                     "");
  dialog.text_invisible();
  if (dialog.run() != GTK_RESPONSE_OK)
    return false;

  // Check the password.
  if (dialog.entered_value == currentpassword)
    return true;

  // Give feedback if there is an error.
  gtkw_dialog_warning(window, _("The password provided is incorrect"));
  return false;
}
