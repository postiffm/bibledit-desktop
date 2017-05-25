/*
 ** Copyright (©) 2015-2016 Matt Postiff.
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


#include "directories.h"
#include <stdlib.h>
#include <glib.h>
#include <config.h>
#include "constants.h"
#include "utilities.h"
#include "gwrappers.h"
#include "shell.h"
#include "unixwrappers.h"
#include "tiny_utilities.h"
#include "restore.h"
#include <glib/gi18n.h>
#include "windowsoutpost.h"

// Change forward slash to backslash on WIN32, else there are major
// problems with system calls to the file system (unix_rmdir, for instance).
ustring directories::fix_slashes(const ustring &tofix)
{
  ustring retval;
#ifdef WIN32
  ustring::const_iterator it;
  for (it = tofix.begin(); it < tofix.end(); it++) {
      if (*it == '/') { retval.push_back('\\'); }
      else { retval.push_back(*it); }
  }
#else
  retval = tofix;
#endif
  return retval;
}

ustring directories::backslashes_to_forwardslashes(const ustring &tofix)
{
  ustring retval;
  ustring::const_iterator it;
  for (it = tofix.begin(); it < tofix.end(); it++) {
      if (*it == '\\') { retval.push_back('/'); }
      else { retval.push_back(*it); }
  }
  return retval;
}

// Constructor
directories::directories(char *argv0)
{
  char *dirname = (char *)g_path_get_dirname (argv0);
  rundir = dirname;
  free(dirname);
  char *basename = (char *)g_path_get_basename (argv0);
  exename = basename;
  free(basename);

  // Instead of re-computing the directory every time
  // get_package_data() is called, we compute it once and store it,
  // making repeated calls more efficient by simply returning the
  // answer we have pre-computed.
#ifdef WIN32
  // A clever way to take a path like C:\Program
  // Files\Bibledit-Desktop\editor\bin\ and chop the
  // last component to get back to <something>\Bibledit-Desktop\editor...
  package_data = gw_path_get_dirname(rundir);
  // ... Then add two more dirs back on, resulting in
  // <something>Bibledit-Desktop\editor\share\bibledit
  package_data = gw_build_filename(package_data, "share", "bibledit");
#else
  // For Linux, this is hard-coded to match the variable set in config.h
  package_data = PACKAGE_DATA_DIR;
#endif
  package_data = fix_slashes(package_data);

  // User home directory
  home = g_get_home_dir();
  home = fix_slashes(home);
  
  // The root directory of all data.
  root = tiny_directories_get_root();
  root = fix_slashes(root);

  // Directory containing all the projects
  projects = tiny_directories_get_projects();
  projects = fix_slashes(projects);

  // Directory with the notes
  notes = gw_build_filename(root, "notes");
  notes = fix_slashes(notes);

  // Directory with the stylesheets
  stylesheets = gw_build_filename(root, "stylesheets");
  stylesheets = fix_slashes(stylesheets);

  // Directory with the configuration
  configuration = gw_build_filename(root, "configuration");
  configuration = fix_slashes(configuration);

  // Directory with the pictures
  pictures = gw_build_filename(root, "pictures");
  pictures = fix_slashes(pictures);

  // Directory with the resources.
  resources = gw_build_filename(root, "resources");
  resources = fix_slashes(resources);

  // Directory with the scripts.
  scripts = gw_build_filename(root, "scripts");
  scripts = fix_slashes(scripts);

  // Temporary directory bibledit uses.
  temp = gw_build_filename(g_get_tmp_dir(), "bibledit");
  temp = fix_slashes(temp);

  // Directory with the templates
  templates = gw_build_filename(get_temp(), "templates");
  templates = fix_slashes(templates);

  // Directory with the User's custom raw templates
  templates_user = gw_build_filename(root, "templates");
  templates_user = fix_slashes(templates_user);

  // Directory, if exists, to restore from
  restore = root + ".restored";
  restore = fix_slashes(restore);
}

void directories::init(void)
{
  // Have to have the temp directory to do almost anything...and may need to create it
  // so we need mkdir, at least a temporary version of it.
#ifdef WIN32
    // mkdir (md) does not play nice. If you run it with /?, it returns code 1, not 0. So
	// I rely on the #ifdef to just "know" that we have compiled on Windows and can assume
	// that mkdir is available.
    mkdir = "mkdir"; // no mkdir_args
#else
	// Linux
	mkdir = "mkdir";
	mkdir_args = "-p";
#endif
  gw_mkdir_with_parents(get_temp());
  gw_mkdir_with_parents(get_templates());

  //find_mkdir();
  find_rmdir();
  check_structure(); // Check on required directory structure.
  find_utilities();  // Find rest of core utilities like copy, rm, etc.

  path = ustring(getenv("PATH"));
#ifdef WIN32
  // Set path properly for Windows. We need this because tar -czf, for instance, needs
  // to know how to find gzip in the path. If no gzip in path, then tar
  // does not work; backups are not created; and end-users not happy!
  path = rundir + ";" + path;
  //setenv("PATH", path.c_str(), TRUE/*overwrite*/);
  ustring pathspec = "PATH="+path;
  putenv(pathspec.c_str());
  // This could affect how utilities are found, but we do that ABOVE.
#endif
}

void directories::find_mkdir(void)
{
  //---------------------------------------------
  // mkdir
  //---------------------------------------------
  {
#ifdef WIN32
    // mkdir (md) does not play nice. If you run it with /?, it returns code 1, not 0. So
	// I rely on the #ifdef to just "know" that we have compiled on Windows and can assume
	// that mkdir is available.
    mkdir = "mkdir"; // no mkdir_args
#else
	// Check for mkdir (Unix)
	GwSpawn spawn("mkdir");
	spawn.arg("--help");  // TODO: Something is messed up here. It creates Bibledit-x.y.z\--help as a directory!!!
	spawn.run();
	if (spawn.exitstatus == 0) { 
		// We have a mkdir command. Use it.
		mkdir = "mkdir"; mkdir_args = "-p";
	}
	else {
		// Check for mkdir.exe in the rundir (Windows directly through msys2/mingw binary)
		GwSpawn spawn(rundir + "\\mkdir.exe");
		spawn.arg("--help");
		if (spawn.exitstatus == 0) { mkdir = rundir + "\\mkdir.exe"; mkdir_args = "-p"; }
		else { gw_message(_("Cannot find a suitable mkdir utility")); }
	}
#endif
}
}

void directories::find_rmdir(void)
{
  //---------------------------------------------
  // Rmdir
  //---------------------------------------------
  {
  // Check for rm (Unix)
  GwSpawn spawn("rm");
  spawn.arg("--version");
  spawn.run();
  if (spawn.exitstatus == 0) { 
	// We have a rm command. Use it.
	rmdir = "rm";
	rmdir_args = "-rf";
  }
  else {
	// Check for rmdir (Windows/DOS through cmd.exe)
	GwSpawn spawn("rmdir");
    spawn.arg("/?");
	if (spawn.exitstatus == 0) { rmdir = "rmdir"; rmdir_args = "/s/q"; }
	else {
		// Check for rm.exe in the rundir (Windows directly through msys2/mingw binary)
		GwSpawn spawn(rundir + "\\rm.exe");
		spawn.arg("--version");
		if (spawn.exitstatus == 0) { rmdir = rundir + "\\rm.exe"; rmdir_args = "-rf"; }
		// We have rmdir.exe, but it only works if the directories are empty
		else { gw_message(_("Cannot find a suitable rmdir utility")); }
	}
  }
  }	
}

void directories::find_utilities(void)
{
  // Find some utilities that we need to use (cp, rm, tar, zip, etc.)

  //---------------------------------------------
  // Copy
  //---------------------------------------------
  {
  // Check for cp (Unix)
  GwSpawn spawn("cp");
  spawn.arg("--version");
  spawn.run();
  if (spawn.exitstatus == 0) { 
	// We have a cp command. Use it.
	copy = "cp";
  }
  else {
	// Check for copy (Windows/DOS through cmd.exe)
	GwSpawn spawn("copy");
    spawn.arg("/?");
	if (spawn.exitstatus == 0) { copy = "copy"; }
	else {
		// Check for cp.exe in the rundir (Windows directly through msys2/mingw binary)
		GwSpawn spawn(rundir + "\\cp.exe");
		spawn.arg("--version");
		if (spawn.exitstatus == 0) { copy = rundir + "\\cp.exe"; }
		else { gw_message("Cannot find a suitable copy utility"); }
	}
  }
  }
  
  //---------------------------------------------
  // Copy recursive
  //---------------------------------------------
  {
  // Check for cp (Unix)
  GwSpawn spawn("cp");
  spawn.arg("--version");
  spawn.run();
  if (spawn.exitstatus == 0) { 
	// We have a cp command. Use it.
	copy_recursive = "cp";
	copy_recursive_args = "-r";
  }
  else {
	// Check for copy (Windows/DOS through cmd.exe)
	GwSpawn spawn("xcopy");
    spawn.arg("/?");
	if (spawn.exitstatus == 0) { copy_recursive = "xcopy"; copy_recursive_args = "/E/I/Y"; }
	else {
		// Check for cp.exe in the rundir (Windows directly through msys2/mingw binary)
		GwSpawn spawn(rundir + "\\cp.exe");
		spawn.arg("--version");
		if (spawn.exitstatus == 0) { copy_recursive = rundir + "\\cp.exe"; copy_recursive_args = "-r"; }
		else { gw_message("Cannot find a suitable recursive copy utility"); }
	}
  }
  }
  
  //---------------------------------------------
  // Move
  //---------------------------------------------
  {
  // Check for mv (Unix)
  GwSpawn spawn("mv");
  spawn.arg("--version");
  spawn.run();
  if (spawn.exitstatus == 0) { 
	// We have a mv command. Use it.
	move = "mv";
	move_args = "-f";
  }
  else {
	// Check for move (Windows/DOS through cmd.exe)
	GwSpawn spawn("move");
    spawn.arg("/?");
	if (spawn.exitstatus == 0) { move = "move"; move_args = "/Y"; }
	else {
		// Check for mv.exe in the rundir (Windows directly through msys2/mingw binary)
		GwSpawn spawn(rundir + "\\mv.exe");
		spawn.arg("--version");
		if (spawn.exitstatus == 0) { move = rundir + "\\mv.exe"; move_args = "-f"; }
		else { gw_message(_("Cannot find a suitable move utility")); }
	}
  }
  }
  
  //---------------------------------------------
  // Gzip. Had a strange error 2/17/2017 that we had not had before, on Windows. The command works in an msys2-64 shell
  // as long as you specify the path right (like /c/Users/postiffm/backupTest6.tar.gz) but it doesn't work in a cmd shell.
  // T1 Shell command: cd C:\Users\postiffm\.bibledit;  tar   --force-local   -czf   C:\Users\postiffm\backupTest5.tar.gz   . 
  // tar (child): gzip: Cannot exec: No such file or directory
  // Found that the problem was that msys2 now requires /usr/bin/sh.exe to be placed in Program Files\Bibledit-version\usr\bin\sh.exe
  //---------------------------------------------
  {
  // Check for gzip (Unix)
  GwSpawn spawn("gzip");
  spawn.arg("--version");
  spawn.run();
  if (spawn.exitstatus == 0) { 
	// We have a gzip command. Use it.
	zip = "gzip";
  }
  else {
	// Check for gzip.exe in the rundir (Windows directly through msys2/mingw binary)
	GwSpawn spawn(rundir + "\\gzip.exe");
	spawn.arg("--version");
	if (spawn.exitstatus == 0) { zip = rundir + "\\gzip.exe"; }
	else { gw_message(_("Cannot find a suitable zip utility")); }
  }
  }

  find_pdfviewer();
  
  //---------------------------------------------
  // Tar
  //---------------------------------------------
  {
  // Check for tar (Unix)
  GwSpawn spawn("tar");
  spawn.arg("--version");
  spawn.run();
  if (spawn.exitstatus == 0) { 
	// We have a tar command. Use it.
	tar = "tar";
  }
  else {
	// Check for tar.exe in the rundir (Windows directly through msys2/mingw binary)
	GwSpawn spawn(rundir + "\\tar.exe");
    spawn.arg("--version");
	if (spawn.exitstatus == 0) { tar = rundir + "\\tar.exe"; }
	else { gw_message(_("Cannot find a suitable tar utility")); }
  }
  }
  
  //---------------------------------------------
  // Rm
  //---------------------------------------------
  {
  // Check for rm (Unix)
  GwSpawn spawn("rm");
  spawn.arg("--version");
  spawn.run();
  if (spawn.exitstatus == 0) { 
	// We have a rm command. Use it.
	rm = "rm";
  }
  else {
	// Check for del (Windows/DOS through cmd.exe)
	GwSpawn spawn("del");
    spawn.arg("/?");
	if (spawn.exitstatus == 0) { rm = "del"; }
	else {
		// Check for rm.exe in the rundir (Windows directly through msys2/mingw binary)
		GwSpawn spawn(rundir + "\\rm.exe");
		spawn.arg("--version");
		if (spawn.exitstatus == 0) { rm = rundir + "\\rm.exe"; }
		else { gw_message(_("Cannot find a suitable rm/del utility")); }
	}
  }
  }
  
  //---------------------------------------------
  // Diff
  //---------------------------------------------
  {
  // Check for diff (Unix)
  GwSpawn spawn("diff");
  spawn.arg("--version");
  spawn.run();
  if (spawn.exitstatus == 0) { 
	// We have a rm command. Use it.
	diff = "diff";
  }
  else {
	// Check for diff.exe in the rundir (Windows directly through msys2/mingw binary)
	GwSpawn spawn(rundir + "\\diff.exe");
	spawn.arg("--version");
	if (spawn.exitstatus == 0) { diff = rundir + "\\diff.exe"; }
	else { gw_message(_("Cannot find a suitable diff utility")); }
  }
  }

  //---------------------------------------------
  // Zip
  //---------------------------------------------
  {
  // Check for zip (Unix)
  GwSpawn spawn("zip");
  spawn.arg("--version");
  spawn.run();
  if (spawn.exitstatus == 0) { 
	// We have a zip command. Use it.
	zip = "zip";
  }
  else {
	// Check for zip.exe (Windows/DOS through cmd.exe)
	GwSpawn spawn("zip.exe");
    spawn.arg("--version"); // This is not standard for Windows, but in zip 3.0 by Info-ZIP that's what works
	if (spawn.exitstatus == 0) { zip = "zip.exe"; }
	else {
		// Check for zip.exe in the rundir (Windows directly through msys2/mingw binary)
		GwSpawn spawn(rundir + "\\zip.exe");
		spawn.arg("--version");
		if (spawn.exitstatus == 0) { zip = rundir + "\\zip.exe"; }
		else { gw_message(_("Cannot find a suitable zip utility")); }
	}
  }
  }

  //---------------------------------------------
  // Unzip
  //---------------------------------------------
  {
  // Check for unzip (Unix)
  GwSpawn spawn("unzip");
  spawn.arg("--version");  // unzip.exe on Windows and Linux (on my systems at least) returns 10 even though it is present and runs --version because it doesn't know the --version flag
  spawn.run();
  if (spawn.exitstatus == 0) {
	// We have an unzip command. Use it.
	unzip = "unzip";
  }
  else {
	  	// Check for zip.exe (Windows/DOS through cmd.exe)
	GwSpawn spawn("unzip.exe");
    spawn.arg("--version"); // This is not standard for Windows, but in unzip 6.0 by Info-ZIP that's what works
	if (spawn.exitstatus == 0) { unzip = "unzip.exe"; }
	else {
		// Check for unzip.exe in the rundir (Windows directly through msys2/mingw binary)
		GwSpawn spawn(rundir + "\\unzip.exe");
		spawn.arg("--version");
		if (spawn.exitstatus == 0) { unzip = rundir + "\\unzip.exe"; }
		else { gw_message(_("Cannot find a suitable unzip utility")); }
	}
  }
  }

  //---------------------------------------------
  // gzip
  //---------------------------------------------
  {
  // Check for unzip (Unix)
  GwSpawn spawn("gzip");
  spawn.arg("--version");  // unzip.exe on Windows and Linux (on my systems at least) returns 10 even though it is present and runs --version because it doesn't know the --version flag
  spawn.run();
  if (spawn.exitstatus == 0) {
	// We have an unzip command. Use it.
	gzip = "gzip";
  }
  else {
	// Check for gzip.exe in the rundir (Windows directly through msys2/mingw binary)
	GwSpawn spawn(rundir + "\\gzip.exe");
	spawn.arg("--version");
	if (spawn.exitstatus == 0) { gzip = rundir + "\\gzip.exe"; }
	else { gw_message(_("Cannot find a suitable gzip utility")); }
  }
  }

  
  //---------------------------------------------
  // Bibledit Outpost for Windows...eventually to be deprecated
  //---------------------------------------------
  bwoutpost = gw_build_filename (rundir, BIBLEDIT_WINDOWS_OUTPOST_EXE);
  bwoutpost_exeonly = BIBLEDIT_WINDOWS_OUTPOST_EXE;
}

directories::~directories()
{
}

void directories::find_pdfviewer(void)
{
  // Possible viewers on Linux and Macintosh.
  struct {
    const char *command;
    const char *argument;
  } pdf_viewers[] = {
    { "acroread", ""}, 
	{ "evince", ""},
	{ "xpdf", "-paper match"},
	{ "ghostview", ""},
	{ "gpdf", ""},
	{ "kpdf", ""},
	{ "kghostview", ""},
	{ "open", ""}
  };

  // This is designed to handle Windows (without bw outpost), as well as Unix, Macintosh
  for (unsigned int i = 0; i < (sizeof(pdf_viewers) / sizeof(*pdf_viewers)); i++) {
    if (gw_find_program_in_path(pdf_viewers[i].command)) {
	  pdfviewer = pdf_viewers[i].command;
	  pdfviewer_args = pdf_viewers[i].argument;
      return;
    }
  }
  
  // If we get here, it means we did not find a PDF reader. Oh, bother!
  // On Windows, this happens because there is no PDF reader in the PATH.
  // Windows associates file extensions with viewer programs so there is no need
  // to have them in the PATH. In that case, we need to use "ShellExecute"
  // to "run" the PDF file and the shell will figure out the file association.
  // That will have to be done wherever Directories->get_pdfviewer() is called,
  // in the case that Directories->get_pdfviewer == "".
}

void directories::print()
{
  gw_message(_("List of directories and other paths we know about:"));
  gw_message("Run directory: \t" + rundir);
  gw_message("Executable name: \t" + exename);
  gw_message("Package data: \t" + package_data);
  gw_message("Home: \t" + home);
  gw_message("Root: \t" + root);
  gw_message("Projects: \t" + projects);
  gw_message("Notes: \t" + notes);
  gw_message("Stylesheets: \t" + stylesheets);
  gw_message("Configuration: \t" + configuration);
  gw_message("Pictures: \t" + pictures);
  gw_message("Resources: \t" + resources);
  gw_message("Scripts: \t" + scripts);
  gw_message("User templates: \t" + templates_user);
  gw_message("Temp: \t" + temp);
  gw_message("Templates: \t" + templates);
  gw_message("Restore: \t" + restore);
  gw_message("Path: \t" + path);
  gw_message("Copy util: \t" + copy);
  gw_message("Copy recursive: \t" + copy_recursive + " " + copy_recursive_args);
  gw_message("Move util:   \t" + move + " " + move_args);
  gw_message("Remove util: \t" + rm);
  gw_message("Rmdir util:  \t" + rmdir + " " + rmdir_args);
  gw_message("Mkdir:       \t" + mkdir + " " + mkdir_args);
  gw_message("Diff util:   \t" + diff);
  gw_message("Tar util:    \t" + tar);
  gw_message("Zip util:    \t" + zip);
  gw_message("Unzip util:  \t" + unzip);
  gw_message("gzip util:   \t" + gzip);
  gw_message("pdfviewer:   \t" + pdfviewer + " " + pdfviewer_args);
  gw_message("Any of these that are blank indicate that we have not worked on them yet!!!!!");
  gw_message("Git:         \t" + git);
  gw_message("bibledit_git \t" + bibledit_git);
  gw_message("Curl util:   \t" + curl);
  gw_message("gobiblecreator:\t" + gobiblecreator);
  gw_message("bibledit_shutdown:\t" + bibledit_shutdown);
  gw_message("php-cli:      \t" + php);
  gw_message("texlive-xetex:\t" + xetex); 
  gw_message("teckit_compile:\t" + teckit_compile);
  gw_message("head:         \t" + head);
  gw_message("touch:        \t" + touch);
  gw_message("osis2mod:     \t" + osis2mod);
  gw_message("cmdshell:     \t" + cmdshell);
  // what about helpcommand?
  gw_message("tasklist:     \t" + tasklist);
  gw_message("merge:        \t" + merge);
  gw_message("bwoutpost:    \t" + bwoutpost);
}

void directories::check_structure()
{
  restore_all_stage_two ();

  // All these are in the user's home directory (~/.bibledit/... or C:\users\username\.bibledit)
  gw_mkdir_with_parents(root);
  gw_mkdir_with_parents(get_projects());
  gw_mkdir_with_parents(get_notes());
  gw_mkdir_with_parents(get_stylesheets());
  gw_mkdir_with_parents(get_configuration());
  gw_mkdir_with_parents(get_pictures());
  gw_mkdir_with_parents(get_resources());
  gw_mkdir_with_parents(get_scripts());
  gw_mkdir_with_parents(get_templates_user());
}

// Important directories in the user's home/.bibledit or temp
ustring directories::get_home()               { return home; }
ustring directories::get_root()               { return root; }
ustring directories::get_projects()           { return projects; }
ustring directories::get_notes()              { return notes; }
ustring directories::get_stylesheets()        { return stylesheets; }
ustring directories::get_configuration()      { return configuration; }
ustring directories::get_pictures()           { return pictures; }
ustring directories::get_resources()          { return resources; }
ustring directories::get_scripts()            { return scripts; }
ustring directories::get_temp()               { return temp; }
ustring directories::get_templates()          { return templates; }
ustring directories::get_templates_user()     { return templates_user; }
ustring directories::get_package_data()       { return package_data; }
ustring directories::get_restore ()           { return restore; }
ustring directories::get_path ()              { return path; }

// File utility programs
ustring directories::get_copy ()              { return copy;     }
ustring directories::get_copy_recursive ()    { return copy_recursive; }
ustring directories::get_copy_recursive_args(){ return copy_recursive_args; }
ustring directories::get_move ()              { return move; }
ustring directories::get_move_args ()         { return move_args; }
ustring directories::get_rm ()                { return rm; }
ustring directories::get_rmdir ()             { return rmdir; }
ustring directories::get_rmdir_args ()        { return rmdir_args; }
ustring directories::get_diff ()              { return diff; }
ustring directories::get_mkdir ()             { return mkdir; }
ustring directories::get_mkdir_args ()        { return mkdir_args; }
ustring directories::get_tar ()               { return tar; }
ustring directories::get_zip ()               { return zip; }
ustring directories::get_unzip ()             { return unzip; }
ustring directories::get_gzip ()              { return gzip; }
ustring directories::get_pdfviewer ()         { return pdfviewer; }
ustring directories::get_pdfviewer_args ()    { return pdfviewer_args; }
ustring directories::get_git ()               { return git; }
ustring directories::get_bibledit_git ()      { return bibledit_git; }
ustring directories::get_curl ()              { return curl; }
ustring directories::get_gobiblecreator ()    { return gobiblecreator; }
ustring directories::get_bibledit_shutdown () { return bibledit_shutdown; }
ustring directories::get_php ()               { return php; }
ustring directories::get_xetex ()             { return xetex; }
ustring directories::get_teckit_compile ()    { return teckit_compile; }
ustring directories::get_head ()              { return head; }
ustring directories::get_touch ()             { return touch; }
ustring directories::get_osis2mod ()          { return osis2mod; }
ustring directories::get_cmdshell ()          { return cmdshell; }
// what about helpcommand?
ustring directories::get_tasklist ()          { return tasklist; }
ustring directories::get_merge ()             { return merge; }
ustring directories::get_bwoutpost ()         { return bwoutpost; }
ustring directories::get_bwoutpost_exeonly () { return bwoutpost_exeonly; }
