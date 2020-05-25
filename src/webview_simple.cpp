/*
 ** Copyright (©) 2018 Matt Postiff.
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

#include "webview_simple.h"

void webview_simple::decide_policy_cb(WebKitWebView *web_view,
									  WebKitPolicyDecision *decision,
									  WebKitPolicyDecisionType decision_type)
{
// Callback for clicking a link.
#if 0
  // I don't know how to do this with web_view...since it is not contained in a scrolledwindow anymore...
  // Store scrolling position for the now active url.
  GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow_terms));
  scrolling_position[active_url] = gtk_adjustment_get_value (adjustment);
  
  DEBUG("remember old scroll position="+std::to_string(scrolling_position[active_url])+" for old active_url="+active_url)
#endif
	switch (decision_type)
	{
	case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
	{
		WebKitNavigationPolicyDecision *navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
		WebKitNavigationAction *navigation_action =
			webkit_navigation_policy_decision_get_navigation_action(navigation_decision);
		WebKitNavigationType navigation_type =
			webkit_navigation_action_get_navigation_type(navigation_action);

		switch (navigation_type)
		{
		case WEBKIT_NAVIGATION_TYPE_LINK_CLICKED:
		{ // The navigation was triggered by clicking a link.
			// Don't follow pseudo-links clicked on this page.
			webkit_policy_decision_ignore(decision);

			WebKitURIRequest *request = webkit_navigation_action_get_request(navigation_action);
			const gchar *uri = webkit_uri_request_get_uri(request);

			// Load new page depending on the pseudo-link clicked.
			webview_process_navigation(uri);
		}
		break;
		case WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED:	  // The navigation was triggered by submitting a form.
													  // fall through
		case WEBKIT_NAVIGATION_TYPE_BACK_FORWARD:	  // The navigation was triggered by navigating forward or backward.
													  // fall through
		case WEBKIT_NAVIGATION_TYPE_RELOAD:			  // The navigation was triggered by reloading.
													  // fall through
		case WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED: // The navigation was triggered by resubmitting a form.
													  // fall through
		case WEBKIT_NAVIGATION_TYPE_OTHER:			  // The navigation was triggered by some other action.
			webkit_policy_decision_use(decision);
			break;
		}
	}
	break;

	case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
	{
		WebKitNavigationPolicyDecision *navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
		/* Make a policy decision here. */
		webkit_policy_decision_use(decision);
	}
	break;
	case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:
	{
		WebKitResponsePolicyDecision *response = WEBKIT_RESPONSE_POLICY_DECISION(decision);
		/* Make a policy decision here. */
		webkit_policy_decision_use(decision);
	}
	break;
	default:
		/* Making no decision results in webkit_policy_decision_use(). */
		return;
	}
	return;
	// return value handled by the above callback func; maybe shouldn't be.
}
