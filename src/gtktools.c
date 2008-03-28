/* gtktools.c  -  The GNU Privacy Assistant
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* NOTE: Here are a lot of old GTK+ functions and wrappers.  They
   should be replaced by modern GTK+ code and some of the wrappers are
   not needed anymore. */

#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gpa.h"
#include "gtktools.h"
#include "gpawindowkeeper.h"
#include "icons.h"


/* BEGIN of old unchecked code (wk 2008-03-07) */


/* Show all children of the GtkWindow widget, realize it, set its
   position so that it's in the center of the parent and show the
   window itself.  */
void
gpa_window_show_centered (GtkWidget * widget, GtkWidget * parent)
{
  int parent_x, parent_y, parent_width, parent_height;
  int center_x, center_y;
  int width, height;
  GtkWidget * child = GTK_BIN (widget)->child;

  gtk_widget_show_all (child);
  gtk_widget_realize (child);

  gdk_window_get_size (widget->window, &width, &height);
  gdk_window_get_origin (parent->window, &parent_x, &parent_y);
  gdk_window_get_size (parent->window, &parent_width, &parent_height);

  center_x = parent_x + (parent_width - width) / 2;
  center_y = parent_y + (parent_height - height) / 2;

  gtk_window_set_position (GTK_WINDOW(widget), GTK_WIN_POS_NONE);
  gtk_widget_set_uposition (widget, center_x, center_y);
  gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(parent));
  gtk_widget_show_all (widget);
}


void
gpa_connect_by_accelerator (GtkLabel * label, GtkWidget * widget,
			    GtkAccelGroup * accelGroup, gchar * labelText)
{
  guint accelKey;

  accelKey = gtk_label_parse_uline (label, labelText);
  gtk_widget_add_accelerator (widget, "grab_focus", accelGroup, accelKey,
			      GDK_MOD1_MASK, 0);
}


void
gpa_window_error (const gchar *message, GtkWidget *messenger)
{
  GtkWidget *windowError;
  GtkWidget *hboxError;
  GtkWidget *labelMessage;
  GtkWidget *pixmap;

  windowError = gtk_dialog_new_with_buttons (_("GPA Error"),
                                             (messenger ? 
                                             GTK_WINDOW(messenger) : NULL),
                                             GTK_DIALOG_MODAL,
                                             _("_Close"),
                                             GTK_RESPONSE_CLOSE,
                                             NULL);
  gtk_container_set_border_width (GTK_CONTAINER (windowError), 5);
  gtk_dialog_set_default_response (GTK_DIALOG (windowError),
                                   GTK_RESPONSE_CLOSE);
  hboxError = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxError), 5);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (windowError)->vbox),
                               hboxError);
  pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR,
                                     GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hboxError), pixmap, TRUE, FALSE, 10);
  labelMessage = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (hboxError), labelMessage, TRUE, FALSE, 10);

  gtk_widget_show_all (windowError);
  gtk_dialog_run (GTK_DIALOG (windowError));
  gtk_widget_destroy (windowError);
}


void
gpa_window_message (gchar * message, GtkWidget * messenger)
{
  GtkWidget *window;
  GtkWidget *hbox;
  GtkWidget *labelMessage;
  GtkWidget *pixmap;

  window = gtk_dialog_new_with_buttons (_("GPA Message"),
					(messenger ? 
					 GTK_WINDOW(messenger) : NULL),
					GTK_DIALOG_MODAL,
					_("_Close"),
					GTK_RESPONSE_CLOSE,
					NULL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  gtk_dialog_set_default_response (GTK_DIALOG (window),
                                   GTK_RESPONSE_CLOSE);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (window)->vbox),
                               hbox);
  pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
                                     GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), pixmap, TRUE, FALSE, 10);
  labelMessage = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (hbox), labelMessage, TRUE, FALSE, 10);

  gtk_widget_show_all (window);
  gtk_dialog_run (GTK_DIALOG (window));
  gtk_widget_destroy (window);
}


/* END of old unchecked code (wk 2008-03-07) */



/* Set a tooltip TEXT to WIDGET.  TEXT and WIDGET may both be NULL.
   This function is useful so that GPA can be build with older GTK+
   versions.  */ 
void
gpa_add_tooltip (GtkWidget *widget, const char *text)
{
#if GTK_CHECK_VERSION (2, 12, 0)
  if (widget && text && *text)
    gtk_widget_set_tooltip_text (widget, text);
#endif
}

/* Set the title of COLUMN to TITLE and also set TOOLTIP. */
void
gpa_set_column_title (GtkTreeViewColumn *column,
                      const char *title, const char *tooltip)
{
  GtkWidget *label;

  label = gtk_label_new (title);
  /* We need to show the label before setting the widget.  */
  gtk_widget_show (label);
  gtk_tree_view_column_set_widget (column, label);
  if (tooltip)
    gpa_add_tooltip (gtk_tree_view_column_get_widget (column), tooltip);
}


static void
set_homogeneous (GtkWidget *widget, gpointer data)
{
  gboolean *is_hom_p = data;

  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), *is_hom_p);
}


/* Set the homogeneous property for all children of TOOLBAR to IS_HOM.  */
void
gpa_toolbar_set_homogeneous (GtkToolbar *toolbar, gboolean is_hom)
{
  gtk_container_foreach (GTK_CONTAINER (toolbar),
			 (GtkCallback) set_homogeneous, &is_hom);
}

