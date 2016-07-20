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

#ifndef INCLUDED_CHECK_VALIDATE_REFERENCES_H
#define INCLUDED_CHECK_VALIDATE_REFERENCES_H

#include "libraries.h"
#include "progresswindow.h"

class CheckValidateReferences {
public:
  CheckValidateReferences(const ustring &project,
                          const vector<unsigned int> &books, bool gui);
  ~CheckValidateReferences();
  vector<ustring> references;
  vector<ustring> comments;
  bool cancelled;

private:
  ustring myproject;
  ustring versification;
  ustring language;
  unsigned int book;
  unsigned int chapter;
  ustring verse;
  void check(const ustring &text);
  void message(const ustring &message);
  ProgressWindow *progresswindow;
};

#endif
