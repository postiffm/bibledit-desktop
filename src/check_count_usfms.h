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

#ifndef INCLUDED_CHECK_COUNT_USFMS_H
#define INCLUDED_CHECK_COUNT_USFMS_H

#include "libraries.h"
#include "progresswindow.h"
#include "types.h"

class CheckCountUsfms {
public:
  CheckCountUsfms(const ustring &project, const vector<unsigned int> &books,
                  CheckSortType sorttype, bool gui);
  ~CheckCountUsfms();
  vector<ustring> markers;
  vector<unsigned int> counts;
  bool cancelled;

private:
  ProgressWindow *progresswindow;
};

#endif
