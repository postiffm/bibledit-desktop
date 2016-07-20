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

#include "startup.h"
#include "debug.h"
#include "gtkwrappers.h"
#include "shell.h"
#include <config.h>
#include <glib/gi18n.h>

int global_debug_level;
int debug_msg_no;

bool check_bibledit_startup_okay(int argc, char *argv[])
// Returns true if it is okay to start bibledit.
{
// Do not run as root (does not apply to Windows).
#ifndef WIN32
  bool root = (getuid() == 0);
  if (root) {
    gtkw_dialog_error(NULL, _("Bibledit-Gtk has not been designed to run as "
                              "user root.\nPlease run it as a standard user."));
    return false;
  }
#endif

  // Check arguments whether to bypass the check on another instance of
  // bibledit.
  if (argc >= 2 && strcmp(argv[1], "--debug") == 0) {
    // This is primitive argument checking, but I'll live with it for now.
    global_debug_level = 1;
    debug_msg_no = 1;
    DEBUG("Debugging is turned on")
    return true;
  }

  // See whether Bibledit itself is running already.
  if (programs_running_count("bibledit-gtk") > 1) {
    gtkw_dialog_error(NULL, _("Bibledit-Gtk is already running."));
    return false;
  }

  // See whether Bibledit is shutting down.
  if (program_is_running("bibledit-shutdown")) {
    gtkw_dialog_error(NULL,
                      _("The previous instance of Bibledit-Gtk is still "
                        "optimizing its data while shutting down.\nPlease wait "
                        "till that has been done, and try again."));
    return false;
  }

  // It's ok to start.
  return true;
}
