/*
** Copyright (©) 2016 Matt Postiff, postiffm@umich.edu
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

#ifndef INCLUDED_DEBUG_H
#define INCLUDED_DEBUG_H

#include "gwrappers.h"

// See startup.cpp for definition of these variables
extern int global_debug_level;
extern int debug_msg_no;

#define DEBUG_CODE_INCLUDED

#ifdef DEBUG_CODE_INCLUDED
#define DEBUG(MSG)                                                             \
  if (global_debug_level) {                                                    \
    gw_debug(debug_msg_no++, MSG, __FILE__, __LINE__, __func__);               \
  }
#else
#define DEBUG(MSG)
#endif

#endif
