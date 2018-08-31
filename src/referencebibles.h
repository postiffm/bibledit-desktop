/*
 ** Copyright (©) 2018- Matt Postiff.
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

#ifndef INCLUDED_REFBIBLES_H
#define INCLUDED_REFBIBLES_H

#include "ustring.h"
#include <glibmm/iochannel.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include "directories.h"
#include "settings.h"
#include "books.h"
#include "localizedbooks.h"
#include "versifications.h"
#include "versification.h"
#include "mappings.h"
#include "styles.h"
#include "urltransport.h"
#include "vcs.h"
#include "projectutils.h"
#include "usfmtools.h"
#include "usfm-inline-markers.h"
#include "bookdata.h"
#include "options.h"
#include "htmlwriter2.h"
#include "utilities.h"
#include "biblebookchapterverse.h"

class ReferenceBibles {
public:
  ReferenceBibles();
  ~ReferenceBibles();
  void write(const Reference &ref, HtmlWriter2 &htmlwriter);
private:
  vector<bible *> bibles;
};

#endif
