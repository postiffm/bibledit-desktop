/*
 ** Copyright (©) 2018 Rafał Lużyński.
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


#ifndef INCLUDED_COMBO_WITH_ARROWS_H
#define INCLUDED_COMBO_WITH_ARROWS_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BIBLEDIT_TYPE_COMBO_WITH_ARROWS (bibledit_combo_with_arrows_get_type())

G_DECLARE_DERIVABLE_TYPE (BibleditComboWithArrows, bibledit_combo_with_arrows,
		BIBLEDIT, COMBO_WITH_ARROWS, GtkFrame)

struct _BibleditComboWithArrowsClass {
	GtkFrameClass parent_class;

	void (*changed) (BibleditComboWithArrows *combo);
};

GtkWidget *bibledit_combo_with_arrows_new();

GtkWidget *
bibledit_combo_with_arrows_get_gtk_combo (BibleditComboWithArrows *combo);

int bibledit_combo_with_arrows_get_active (BibleditComboWithArrows *combo);
void bibledit_combo_with_arrows_set_active (BibleditComboWithArrows *combo,
                                            int index);

int bibledit_combo_with_arrows_get_wrap_width (BibleditComboWithArrows *combo);
void
bibledit_combo_with_arrows_set_wrap_width (BibleditComboWithArrows *combo,
                                           int width);

void
bibledit_combo_with_arrows_append_text (BibleditComboWithArrows *combo,
                                        const gchar *text);
void
bibledit_combo_with_arrows_remove_all  (BibleditComboWithArrows *combo);

G_END_DECLS

#endif /* INCLUDED_COMBO_WITH_ARROWS_H */
