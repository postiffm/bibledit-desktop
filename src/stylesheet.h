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


#ifndef INCLUDED_STYLESHEET_H
#define INCLUDED_STYLESHEET_H


#include "libraries.h"
#include "style.h"


class Stylesheet
{
public:
  Stylesheet (const ustring& name_in);
  ~Stylesheet ();
  void save ();
  ustring name;
  Style * style (const ustring& marker);
  vector <Style *> styles;
  void erase (const ustring& marker);
  void insert (Style * style);
private:
  map <ustring, Style *> styles_map;
};


#endif
