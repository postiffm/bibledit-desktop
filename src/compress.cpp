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


#include "compress.h"
#include "utilities.h"
#include "directories.h"
#include "gwrappers.h"
#include <glib/gi18n.h>
#include "debug.h"

/*

 * Tar (.tar) archives, including those compressed with
   gzip (.tar.gz, .tgz), bzip (.tar.bz, .tbz), bzip2 (.tar.bz2, .tbz2),
   compress (.tar.Z, .taz), lzop (.tar.lzo, .tzo) and lzma (.tar.lzma)
 * Zip archives (.zip)
 * Jar archives (.jar, .ear, .war)
 * 7z archives (.7z)
 * iso9660 CD images (.iso)
 * Lha archives (.lzh)
 * Single files compressed with gzip (.gz), bzip (.bz), bzip2 (.bz2),
   compress (.Z), lzop (.lzo) and lzma (.lzma)

*/


typedef struct {
  const char *suffix;           // Suffix of the archive.
  unsigned int compress;        // Compression identifier.
  unsigned int uncompress;      // Uncompression identifier.
} compression_record_data;


unsigned int compression_record_count();
unsigned int uncompression_identifier_get(const ustring & filename);


compression_record_data compression_table[] =
{
  { ".tar.gz",   0, 2}, 
  { ".tgz",      0, 2}, 
  { ".tar.bz",   0, 0}, 
  { ".tbz",      0, 0}, 
  { ".tar.bz2",  0, 0}, 
  { ".tbz2",     0, 0}, 
  { ".zip",      0, 1},
};


unsigned int compression_record_count()
{
  unsigned int count = sizeof(compression_table) / sizeof(*compression_table);
  return count;
}


unsigned int uncompression_identifier_get(const ustring & filename)
// Gets the identifier for uncompressing "filename".
{
  for (unsigned int i = 0; i < compression_record_count(); i++) {
    if (g_str_has_suffix(filename.c_str(), compression_table[i].suffix)) {
      if (compression_table[i].uncompress) {
        return compression_table[i].uncompress;
      }
    }
  }
  return 0;
}


bool compressed_archive_recognized(const ustring & archive)
// Returns true if the file is recognized as a compressed archive.
{
  return uncompression_identifier_get(archive);
}


bool uncompress(const ustring & archive, const ustring & directory)
// Uncompresses "archive" into "directory".
// Returns whether this was successful.
{
  // Bail out if the archive was not recognized.
  if (!compressed_archive_recognized(archive)) {
    gw_critical(_("cannot uncompress unrecognized archive"));
    return false;
  }
  // Ensure that the output directory is there.
  gw_mkdir_with_parents (directory);

  // Get the uncompression identifier.
  int uncompression_identifier = uncompression_identifier_get (archive);

  // Do the uncompression.
  int result = -1;
  switch (uncompression_identifier) {
  case 0:
    {
		gw_message("I'm not yet smart enough to handle the " + archive + " file type");
		break;
    }
  case 1:
    { // If you have a zip utility installed in Windows, etc.
      GwSpawn spawn (Directories->get_unzip());
      spawn.arg ("-o"); // overwrite without prompting
      if (!directory.empty ()) {
        spawn.arg ("-d"); // extract files into exdir
        spawn.arg (directory);
      }
      spawn.arg (archive);
      spawn.progress (_("Unpacking"), false);
      spawn.run ();
      result = 0;
      break;
    }
  case 2:
    {
      GwSpawn spawn(Directories->get_tar());
	  spawn.arg ("--force-local"); // to permit : in filename (like C:\Users\...)
      spawn.arg("-xvzf"); // x=eXtract, z=gunZip, f=Filename to extract
      if (!directory.empty()) {
        spawn.workingdirectory(directory);
      }
      spawn.arg(archive);
      spawn.progress(_("Unpacking"), false);
      spawn.run();
      result = spawn.exitstatus;
	  DEBUG("tar return code="+std::to_string(result));
      break;
    }
  }

  // Return whether things were ok.  
  return (result == 0);
}


void compress_ensure_tar_gz_suffix (ustring& filename)
// Ensure that "filename" has the ".tar.gz" suffix.
{
  if (!g_str_has_suffix (filename.c_str(), ".tar.gz")) {
    filename.append (".tar.gz");
  }
}


void compress_ensure_zip_suffix (ustring& filename)
// Ensure that "filename" has the ".zip" suffix.
{
  if (!g_str_has_suffix (filename.c_str(), ".zip")) {
    filename.append (".zip");
  }
}
