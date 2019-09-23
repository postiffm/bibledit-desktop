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
#include "directories.h"
#include "gwrappers.h"
#include "settings.h"
#include "localizedbooks.h"
#include "versifications.h"
#include "mappings.h"
#include "styles.h"
#include "urltransport.h"
#include "vcs.h"
#include "readwrite.h"
#include "books.h" // TEMP - MAP
#include <glib/gi18n.h>
#include "concordance.h"

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


int main(int argc, char *argv[])
{
  // g_type_init has been deprecated since version 2.36.
  // The type system is now initialised automatically.
  // g_type_init();

  // Settings object. 
  Settings mysettings(true);
  settings = &mysettings;

  // Create a new directories 'factory' and initialize it with argv[0]
  Directories = new directories(argv[0]);
  books_init(); // TEMP - MAP
  Directories->init(); // important step

  // Bibledit can read from or write to Bible data.
  // Syntax: bibledit-rdwrt -r|-w ...
  bool readdata = false;
  bool writedata = false;
  if (argc > 2) {
    readdata = (strcmp(argv[1], "-r") == 0);
    writedata = (strcmp(argv[1], "-w") == 0);
  }
  if (readdata ^ writedata) {
    // Do the reading or the writing.
    read_write_data (argc, argv, readdata, writedata);
  } else {
    // Nothing to do.
    gw_message (_("Nothing was done"));
  }

  // Quit.
  return 0;
}


