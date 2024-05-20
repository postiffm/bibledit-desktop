/*
 ** Copyright (©) 2018-2024 Matt Postiff.
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

#ifndef INCLUDED_WEBVIEW_SIMPLE_H
#define INCLUDED_WEBVIEW_SIMPLE_H

#include <webkit2/webkit2.h>
#include "ustring.h"

// This class abstracts out some code that is common to several windows that
// show simple content in a webview.
class webview_simple {
 public:
  void decide_policy_cb (WebKitWebView           *web_view,
			 WebKitPolicyDecision    *decision,
			 WebKitPolicyDecisionType decision_type);

  // Ensure that this class cannot be instantiated, using this
  // pure virtual method. The derived class MUST implement
  // this method for things to work.
  virtual void webview_process_navigation (const ustring &url /*gchar * url*/) = 0;
};

#endif
