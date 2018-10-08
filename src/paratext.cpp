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


#include "paratext.h"
#include "directories.h"
#include "utilities.h"
#include "gwrappers.h"
#include "styles.h"
#include <glib/gi18n.h>

ExportParatextStylesheet::ExportParatextStylesheet (int dummy)
{
  // Load the standard Paratext stylesheet.
  {
    ReadText rt (gw_build_filename (Directories->get_package_data(), "usfm.sty"), true, false);
    stylesheet_lines = rt.lines;
  }
  // Indicate it was exported from bibledit.
  if (!stylesheet_lines.empty()) {
    stylesheet_lines[0].append (_("# Exported from Bibledit-Desktop"));
  }
}


ExportParatextStylesheet::~ExportParatextStylesheet ()
{
}


void ExportParatextStylesheet::convert (const ustring& name)
{
  // Pointer to the styles object.
  extern Styles * styles;

  // The stylesheet to export.
  Stylesheet * sheet = styles->stylesheet (name);
  
  // Go through the markers.
  for (unsigned int i = 0; i < sheet->styles.size(); i++) {
    StyleV2 * style = sheet->styles[i];
    marker = style->marker;
    if (marker == "id") {
    } else if (marker == "ide") {
    } else if (marker == "h") {
    } else if (marker == "h1") {
    } else if (marker == "h2") {
    } else if (marker == "h3") {
    } else if (marker == "toc1") {
    } else if (marker == "toc2") {
    } else if (marker == "toc3") {
    } else if (marker == "rem") {
    } else if (marker == "sts") {
    } else if (marker == "restore") {
    } else if (marker == "imt") {
      //set_font_size (style->fontsize);
    } else if (marker == "ide") {
    } else if (marker == "ide") {
    } else if (marker == "ide") {
    } else if (marker == "ide") {
    } else if (marker == "ide") {
    } // Be aware that this has not yet been implemented. The export just produces the standard Paratext stylesheet.
  }
}

void ExportParatextStylesheet::save (ustring filename)
{
  // Write the stylesheet to file.
  if (!g_str_has_suffix (filename.c_str(), ".sty")) {
    filename.append (".sty");
  }
  write_lines (filename, stylesheet_lines);
}
