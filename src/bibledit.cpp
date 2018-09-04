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
#include "mainwindow.h"
#include "directories.h"
#include "utilities.h"
#include "gwrappers.h"
#include "dialogsystemlog.h"
#include "shell.h"
#include "maintenance.h"
#include "gtkwrappers.h"
#include "upgrade.h"
#include "unixwrappers.h"
#include "settings.h"
#include "constants.h"
#include "localizedbooks.h"
#include "versifications.h"
#include "mappings.h"
#include "ot-quotations.h"
#include "styles.h"
#include <libxml/xmlreader.h>
#include "startup.h"
#include <libxml/nanohttp.h>
#include "urltransport.h"
#include "runtime.h"
#include "vcs.h"
#include "books.h"
#include <new>
#include <libintl.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <errno.h>
#include "options.h"
#include "concordance.h"
#include "referencebibles.h"
#include "crossrefs.h"
//#ifdef WIN32
//#include <Windows.h>
//#endif
//#include "debug.h"

Options *options;
directories *Directories;
Settings *settings;
BookLocalizations *booklocalizations;
Versifications *versifications;
Mappings *mappings;
Styles *styles;
GtkAccelGroup *accelerator_group;
URLTransport * urltransport;
VCS *vcs;
Concordance *concordance;
ReferenceBibles *refbibles;
CrossReferences *crossrefs;

int main(int argc, char *argv[])
{
  // Internationalization: initialize gettext
  setlocale (LC_ALL, ""); // per instructions at https://www.gnu.org/software/gettext/FAQ.html
  bindtextdomain(GETTEXT_PACKAGE, BIBLEDIT_LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  // Initialize g threads.
  // g_thread_init has been deprecated since version 2.32 and should not be used in newly-written code. 
  // This function is no longer necessary. 
  // The GLib threading system is automatically initialized at the start of your program.
  // Initialize g types.
  // g_type_init has been deprecated since version 2.36.
  // The type system is now initialised automatically.
  // g_type_init();
  // Initialize GTK
  gtk_init(&argc, &argv);

  options = new Options(argc, argv);

  // Create a new directories 'factory' and initialize it with argv[0]
  Directories = new directories(argv[0]);

  // Check whether it is fine to start the program.
  // If not, quit the program normally.
  if (!check_bibledit_startup_okay(argc, argv)) {
    return 0;
  }

#ifdef WIN32
  // Do this after bibledit_startup...sets debug level.
  // Try to open a Windows Console. Opening works.
  // Doesn't capture anything of value because
  // printf goes there, but write(1, ...) does not???
  //if (global_debug_level && AllocConsole()) {
	//freopen("CONOUT$", "wt", stdout);
	//freopen("CONOUT$", "wt", stderr);
    //SetConsoleTitle("Bibledit Debug Console");
	//printf("Successfully allocated console.\n");
  //}
#endif
  
  books_init(); // MAP - should do this a better way, but this works well for now

  Directories->init(); // important step

  // Move logfile for shutdown program.
  move_log_file (lftShutdown);
  move_log_file (lftSettings);

  // Redirect stdout and stderr to file.
  {
    move_log_file (lftMain);
    // When a file is opened it is always allocated the lowest available file 
    // descriptor. Therefore the following commands cause stdout to be 
    // redirected to the logfile.
	//int stdin_copy = dup(0);
	//if (global_debug_level) { printf("Redirecting stdout/stderr to logfile.\n"); }
	int stdout_copy = dup(1);
	int stderr_copy = dup(2);
    close(1);
    creat (log_file_name (lftMain, false).c_str(), 0666);
    // The dup() routine makes a duplicate file descriptor for an already opened 
    // file using the first available file descriptor. Therefore the following 
    // commands cause stderr to be redirected to the file stdout writes to.
    close(2);
    int new_stderr_fd = dup(1);
	if (new_stderr_fd == -1) {
		// Restore stdout and stderr to what they were originally
		close(1); // and close(2) is unnecessary since 2 did not get opened
		dup2(stdout_copy, 1);
		dup2(stderr_copy, 2);
		perror("bibledit.cpp:main:dup(1) call failed");
		return 1;
	}
  }
  
#ifdef WIN32
  gw_message("WIN32 WAS defined in this build");
#else
  gw_message("WIN32 was NOT defined in this build");
#endif

  // Call after the stdout/stderr redirects above
  options->print();
  Directories->print();
  // Print what we know about the language setup
  gw_message("BIBLEDIT_LOCALEDIR " + ustring(BIBLEDIT_LOCALEDIR));
  
  ustring language, lang;
  
  char *cplanguage = getenv("LANGUAGE");
  if (cplanguage == NULL) { language="LANGUAGE variable not specified"; }
  else { language = ustring(cplanguage); }
  gw_message("LANGUAGE      \t" + language);
  
  char *cplang = getenv("LANG");
  if (cplang == NULL) { lang="LANG variable not specified"; }
  else { lang = ustring(cplang); }      
  gw_message("LANG          \t" + lang);
  gw_message("LC_ALL        \t" + ustring(setlocale(LC_ALL, NULL)));
  gw_message("LC_CTYPE      \t" + ustring(setlocale(LC_CTYPE, NULL)));

  // Check on runtime requirements.
  runtime_initialize ();
  // Initialize the xml library.
  xmlInitParser();
  // Initialize the http libraries.
  xmlNanoHTTPInit();
  // Maintenance system.
  maintenance_initialize ();
  // Settings object. 
  settings = new Settings(true);
  // LocalizedBooks object.
  booklocalizations = new BookLocalizations(0);
  gw_message(_("Finished booklocalizations"));
  // Versifications object.
  versifications = new Versifications(0);
  // Verse mappings object.
  mappings = new Mappings(0);
  // Styles object.
  styles = new Styles(0);
  // Version control object.
  vcs = new VCS(0);
  gw_message(_("Finished versifications, mappings, styles, and VCS"));
  // URLTransport object.
  urltransport = new URLTransport(0);
  gw_message(_("Finished URLTransport"));
  concordance = NULL;
  refbibles = NULL;
  // Accelerators.
  accelerator_group = gtk_accel_group_new();
  // Upgrade data.
  upgrade();
  gw_message("Finished upgrade");
  // Window icon fallback.
  gtk_window_set_default_icon_from_file(gw_build_filename(Directories->get_package_data(), "bibledit-desktop.xpm").c_str(), NULL);
  gw_message("Set up window icon fallback");
  // Start the gui.
  MainWindow *mainwindow = new MainWindow(accelerator_group, settings, urltransport, vcs);
  gw_message("Finished initialization...running gtk_main");
  gtk_main();
  delete mainwindow;

  // Destroy the accelerator group.
  g_object_unref(G_OBJECT(accelerator_group));
  // Clean up XML library.
  xmlCleanupParser();
  // Clean up http libraries.
  xmlNanoHTTPCleanup();
  //-------------------------------------------------------
  // Destroy global objects. For now, this order is
  // important. It shouldn't be, but it is. MAP 1/9/2015.
  // TO DO: Have ProjectConfiguration and Settings write out
  // their status using a method we call, NOT relying on
  // a destructor to do the work of saving important state.
  //-------------------------------------------------------
  delete urltransport;
  delete vcs;
  delete styles;
  delete mappings;
  delete versifications;
  delete booklocalizations;
  delete settings;
  delete Directories; // must be last, because above rely on it
  if (concordance) { delete concordance; }
  if (refbibles) { delete refbibles; }

  // Quit.
  return 0;
}


/*

Gtk3
http://developer.gnome.org/gtk3/stable/gtk-migrating-2-to-3.html
http://developer.gnome.org/gtk/2.24/

Do not include individual headers: Done.
Do not use deprecated symbols: Done.
Use accessor functions instead of direct access: Done.
Replace GDK_<keyname> with GDK_KEY_<keyname>: Done.
Use GIO for launching applications: Done.
Use cairo for drawing: Done.

Gtk3 is available on Windows.

Switching to Gtk3 needs the webkitgtk for gtk3 also, but we are going to move toward webkit2gtk (MAP 2/27/2018)

pkg-config gtk+-3.0 --modversion

*/
