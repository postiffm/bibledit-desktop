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

/**
 * This is a custom widget implementing a combo box with two buttons with
 * arrows: up and down. Inspired by the search boxes in applications like
 * gedit or Firefox. Here is its internal structure:
 *
 * frame
 * ╰── box
 *     ├── combo
 *     ├── button up
 *     ╰── button down
 *
 * For practical reasons this widget class is derived from GtkFrame. It would
 * be better to have a widget derived from GtkComboBoxText or at least from
 * GtkComboBox. Unfortunately, it is impossible to implement a custom widget
 * having the structure as above and exposing combo as its external interface
 * because every attempt to put the combo into another parent container would
 * remove it from its own box and we can’t override this mechanism. Also it is
 * impossible to override all methods of GtkComboBox and delegate them to the
 * internal combo. Therefore, this implementation is not compatible with
 * GtkComboBox class which means that an instance of this class cannot be used
 * as an argument of gtk_combo_box_... functions. Instead it implements its
 * own functions which have similar names and functionality to those of
 * GtkComboBox and GtkComboBoxText widget classes.
 */

#include "combo_with_arrows.h"

typedef struct {
	GtkWidget *combo;
	GtkWidget *button_up;
	GtkWidget *button_down;
} BibleditComboWithArrowsPrivate;

enum {
	CHANGED,
	LAST_SIGNAL
};

static guint bibledit_combo_with_arrows_signals[LAST_SIGNAL] = {0,};

G_DEFINE_TYPE_WITH_PRIVATE (BibleditComboWithArrows,
		bibledit_combo_with_arrows, GTK_TYPE_FRAME)

#define BIBLEDIT_COMBO_PRIVATE(combo) ((BibleditComboWithArrowsPrivate*) \
		bibledit_combo_with_arrows_get_instance_private (combo))


static void
bibledit_combo_with_arrows_class_init (BibleditComboWithArrowsClass *cls)
{
	GtkWidgetClass *widget_class = (GtkWidgetClass*) cls;

	bibledit_combo_with_arrows_signals[CHANGED] =
			g_signal_new ("changed",
					G_OBJECT_CLASS_TYPE (cls),
					G_SIGNAL_RUN_LAST,
					G_STRUCT_OFFSET (BibleditComboWithArrowsClass, changed),
					NULL, NULL,
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
}

static void
bibledit_combo_with_arrows_button_clicked (GtkWidget *button, gpointer data)
{
	BibleditComboWithArrowsPrivate *priv;
	GtkComboBox *real_combo;
	gint next_item;
	GtkTreeModel *model;
	BibleditComboWithArrows *combo = BIBLEDIT_COMBO_WITH_ARROWS (data);

	priv = BIBLEDIT_COMBO_PRIVATE (combo);
	real_combo = GTK_COMBO_BOX (priv->combo);
	next_item = gtk_combo_box_get_active (real_combo);
	if (button == priv->button_up) {	/* button up => previous item */
		--next_item;
		if (next_item < 0)
			return;
	} else if (button == priv->button_down) {	/* button down => next item */
		++next_item;
		model = gtk_combo_box_get_model (real_combo);
		if (next_item >= gtk_tree_model_iter_n_children (model, NULL))
			return;
	} else
		return;	/* unknown button */

	gtk_combo_box_set_active (real_combo, next_item);
}

static void
bibledit_combo_with_arrows_combo_changed (GtkComboBox *real_combo, gpointer data)
{
	BibleditComboWithArrowsPrivate *priv;
	gint current_item;
	GtkTreeModel *model;
	BibleditComboWithArrows *combo = BIBLEDIT_COMBO_WITH_ARROWS (data);

	priv = BIBLEDIT_COMBO_PRIVATE (combo);
	current_item = gtk_combo_box_get_active (real_combo);
	model = gtk_combo_box_get_model (real_combo);

	gtk_widget_set_sensitive (priv->button_up, current_item > 0);
	gtk_widget_set_sensitive(priv->button_down,
			current_item < gtk_tree_model_iter_n_children(model, NULL) - 1);

	g_signal_emit (combo, bibledit_combo_with_arrows_signals[CHANGED], 0);
}

static void
bibledit_combo_with_arrows_init (BibleditComboWithArrows *combo)
{
	BibleditComboWithArrowsPrivate *priv;
	GtkWidget *box;

	priv = BIBLEDIT_COMBO_PRIVATE (combo);
	gtk_widget_set_can_focus (GTK_WIDGET (combo), FALSE);
	gtk_frame_set_shadow_type (GTK_FRAME (combo), GTK_SHADOW_NONE);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (box);
	gtk_style_context_add_class (gtk_widget_get_style_context (box),
			GTK_STYLE_CLASS_LINKED);
	gtk_container_add (GTK_CONTAINER (combo), box);

	priv->combo = gtk_combo_box_text_new();
	gtk_widget_show (priv->combo);
	gtk_container_add (GTK_CONTAINER (box), priv->combo);

	priv->button_up = gtk_button_new ();
	gtk_widget_set_can_focus (priv->button_up, FALSE);
	gtk_widget_set_sensitive (priv->button_up, FALSE);
	gtk_widget_show (priv->button_up);
	gtk_button_set_image (GTK_BUTTON (priv->button_up),
			gtk_image_new_from_icon_name ("go-up-symbolic", GTK_ICON_SIZE_MENU));
	gtk_container_add (GTK_CONTAINER (box), priv->button_up);
	g_signal_connect (priv->button_up, "clicked",
			G_CALLBACK(bibledit_combo_with_arrows_button_clicked), combo);

	priv->button_down = gtk_button_new ();
	gtk_widget_set_can_focus (priv->button_down, FALSE);
	gtk_widget_set_sensitive (priv->button_down, FALSE);
	gtk_widget_show (priv->button_down);
	gtk_button_set_image (GTK_BUTTON (priv->button_down),
			gtk_image_new_from_icon_name ("go-down-symbolic", GTK_ICON_SIZE_MENU));
	gtk_container_add (GTK_CONTAINER (box), priv->button_down);
	g_signal_connect (priv->button_down, "clicked",
			G_CALLBACK(bibledit_combo_with_arrows_button_clicked), combo);

	g_signal_connect (priv->combo, "changed",
			G_CALLBACK (bibledit_combo_with_arrows_combo_changed), combo);
}

/**
 * Creates a new #BibleditComboWithArrows which looks like #GtkComboBoxText
 * with additional two buttons with up and down arrows.
 *
 * Returns: A new #BibleditComboWithArrows
 */

GtkWidget *bibledit_combo_with_arrows_new()
{
	return GTK_WIDGET (g_object_new (BIBLEDIT_TYPE_COMBO_WITH_ARROWS, NULL));
}

/**
 * Returns an internal #GtkComboBoxText widget. It is not a good idea
 * to allow the external callers to operate on the internal widget directly
 * so this function should be removed as soon as we invent a better way
 * to achieve the same effects.
 *
 * Returns: An internal instance of #GtkComboBoxText. This object must not
 * be freed and no destructive operations must be performed.
 */
GtkWidget *
bibledit_combo_with_arrows_get_gtk_combo (BibleditComboWithArrows *combo)
{
	g_return_val_if_fail (BIBLEDIT_IS_COMBO_WITH_ARROWS (combo), NULL);

	return BIBLEDIT_COMBO_PRIVATE (combo)->combo;
}

/**
 * Returns the index of the currently active item in the internal combo box,
 * or -1 if there’s no active item. Internally just calls
 * #gtk_combo_box_get_active()
 *
 * Returns: An integer which is the index of the currently active item,
 *     or -1 if there’s no active item.
 */

int bibledit_combo_with_arrows_get_active (BibleditComboWithArrows *combo)
{
	BibleditComboWithArrowsPrivate *priv;
	g_return_val_if_fail (BIBLEDIT_IS_COMBO_WITH_ARROWS (combo), -1);

	priv = BIBLEDIT_COMBO_PRIVATE (combo);
	return gtk_combo_box_get_active (GTK_COMBO_BOX (priv->combo));
}

/**
 * bibledit_combo_with_arrows_set_active:
 * @combo: A #BibleditComboWithArrows
 * @index_: An index in the model passed during construction, or -1 to have
 * no active item
 *
 * Sets the active item of the internal combo box to be the item at @index.
 * Internally just calls #gtk_combo_box_set_active().
 */
void
bibledit_combo_with_arrows_set_active (BibleditComboWithArrows *combo,
                                       int index)
{
	BibleditComboWithArrowsPrivate *priv;
	g_return_if_fail (BIBLEDIT_IS_COMBO_WITH_ARROWS (combo));

	priv = BIBLEDIT_COMBO_PRIVATE (combo);
	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo), index);
}

/**
 * Returns the wrap width which is used to determine the number of columns
 * for the popup menu. Internally just calls #gtk_combo_box_get_wrap_width().
 *
 * Returns: the wrap width.
 */
int bibledit_combo_with_arrows_get_wrap_width (BibleditComboWithArrows *combo)
{
	BibleditComboWithArrowsPrivate *priv;
	g_return_val_if_fail (BIBLEDIT_IS_COMBO_WITH_ARROWS (combo), 0);

	priv = BIBLEDIT_COMBO_PRIVATE (combo);
	return gtk_combo_box_get_wrap_width (GTK_COMBO_BOX (priv->combo));
}

/**
 * bibledit_combo_with_arrows_set_wrap_width:
 * @combo: A #BibleditComboWithArrows
 * @width: Preferred number of columns
 *
 * Sets the wrap width of the internal combo box to be @width. Internally just
 * calls #gtk_combo_box_set_wrap_width().
 */

void
bibledit_combo_with_arrows_set_wrap_width (BibleditComboWithArrows *combo,
                                           int width)
{
	BibleditComboWithArrowsPrivate *priv;
	g_return_if_fail (BIBLEDIT_IS_COMBO_WITH_ARROWS (combo));

	priv = BIBLEDIT_COMBO_PRIVATE (combo);
	gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (priv->combo), width);
}

/**
 * @combo: A #BibleditComboWithArrows
 * @text: A string
 *
 * Appends @text to the list of strings stored in the internal combo box.
 * Internally just calls #gtk_combo_box_text_append_text().
 */
void
bibledit_combo_with_arrows_append_text (BibleditComboWithArrows *combo,
                                        const gchar *text)
{
	BibleditComboWithArrowsPrivate *priv;
	g_return_if_fail (BIBLEDIT_IS_COMBO_WITH_ARROWS (combo));

	priv = BIBLEDIT_COMBO_PRIVATE (combo);
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (priv->combo), text);
}

/**
 * Removes all the text entries from the internal combo box.
 * Internally just calls #gtk_combo_box_text_remove_all().
 */
void
bibledit_combo_with_arrows_remove_all  (BibleditComboWithArrows *combo)
{
	BibleditComboWithArrowsPrivate *priv;
	g_return_if_fail (BIBLEDIT_IS_COMBO_WITH_ARROWS (combo));

	priv = BIBLEDIT_COMBO_PRIVATE (combo);
	gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (priv->combo));
}
