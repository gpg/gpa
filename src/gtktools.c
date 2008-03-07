/* gtktools.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
 *      Copyright (C) 2008 g10 Code GmbH.
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* NOTE: Here are a lot of old GTK+ functions and wrappers.  They
   should be replaced by modern GTK+ code and some of the wrappers are
   not needed anymore. */

#include <config.h>

#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gpa.h"
#include "gtktools.h"
#include "gpawindowkeeper.h"
#include "icons.h"


/* BEGIN of old unchecked code (wk 2008-03-07) */


/* Show all children of the GtkWindow widget, realize it, set its
 * position so that it's in the center of the parent and show the window
 * itself.
 */
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
} /* gpa_window_show_centered */


void
gpa_window_destroy (gpointer param)
{
/* var */
  gpointer *localParam;
  GpaWindowKeeper *keeper;

/* commands */
  localParam = (gpointer *) param;
  keeper = (GpaWindowKeeper *) localParam[0];
  gpa_windowKeeper_release (keeper);
}				/* gpa_window_destroy */

GtkWidget *
gpa_space_new (void)
{
  return gtk_label_new ("");
}				/* gpa_space_new */

GtkWidget *
gpa_widget_hjustified_new (GtkWidget * widget, GtkJustification jtype)
{
/* objects */
  GtkWidget *result;
  GtkWidget *space;
/* commands */
  switch (jtype)
    {
    case GTK_JUSTIFY_LEFT:
      result = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (result), widget, FALSE, FALSE, 0);
      space = gpa_space_new ();
      gtk_box_pack_start (GTK_BOX (result), space, TRUE, TRUE, 0);
      break;
    case GTK_JUSTIFY_RIGHT:
      result = gtk_hbox_new (FALSE, 0);
      space = gpa_space_new ();
      gtk_box_pack_start (GTK_BOX (result), space, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (result), widget, FALSE, FALSE, 0);
      break;
    default:
      result = widget;
    }				/* switch */
  return result;
}				/* gpa_widget_hjustified_new */

GtkWidget *
gpa_button_new (GtkAccelGroup * accelGroup, gchar * labelText)
{
/* var */
  guint accelKey;
/* objects */
  GtkWidget *button;
/* commands */
  button = gtk_button_new_with_label ("");
  accelKey =
    gtk_label_parse_uline (GTK_LABEL (GTK_BIN (button)->child), labelText);
  gtk_widget_add_accelerator (button, "clicked", accelGroup, accelKey,
			      GDK_MOD1_MASK, 0);
  return (button);
}				/* gpa_button_new */

/* Create a new hbox with an image and a label packed into it
 * and return the box. Then treat the accelerators. */
GtkWidget *
gpa_xpm_label_box( GtkWidget *parent,
                   const char *xpm_name,
                   gchar	   *label_text,
                   GtkWidget * button,
                   GtkAccelGroup * accelGroup)
{
   guint accelKey;
   GtkWidget *box1;
   GtkWidget *label;
   GtkWidget *pixmapwid;
   GdkPixmap *pixmap;
   GdkBitmap *mask;

   /* Create box for xpm and label */
   box1 = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (box1), 2);

   /* Now on to the xpm stuff */
   pixmap = gpa_create_icon_pixmap ( parent, xpm_name, &mask );
   pixmapwid = gtk_pixmap_new (pixmap, mask);

   /* Create a label for the button */
   label = gtk_label_new (label_text);

   /* accelerators for gpa */
  accelKey = gtk_label_parse_uline (GTK_LABEL(label), label_text);
  gtk_widget_add_accelerator (button, "clicked", accelGroup, accelKey,
			      GDK_MOD1_MASK, 0);

  /* Pack the pixmap and label into the box */
  gtk_box_pack_start (GTK_BOX (box1),
                      pixmapwid, FALSE, FALSE, 3);
  
  gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 3);
  
  gtk_widget_show(pixmapwid);
  gtk_widget_show(label);

  return(box1);
}

GtkWidget *
gpa_buttonCancel_new (GtkAccelGroup * accelGroup, gchar * labelText,
		      gpointer * param)
{
/* objects */
  GtkWidget *buttonCancel;
/* commands */
  buttonCancel = gpa_button_new (accelGroup, labelText);
  gtk_signal_connect_object (GTK_OBJECT (buttonCancel), "clicked",
			     GTK_SIGNAL_FUNC (gpa_window_destroy),
			     (gpointer) param);
  gtk_widget_add_accelerator (buttonCancel, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  return (buttonCancel);
}				/* gpa_buttonCancel_new */

/* an extended version of gpa_buttonCancel_new */
GtkWidget *
gpa_button_cancel_new (GtkAccelGroup * accel, gchar * label,
		       GtkSignalFunc func, gpointer param)
{
  GtkWidget *button;

  button = gpa_button_new (accel, label);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked", func, param);
  gtk_widget_add_accelerator (button, "clicked", accel, GDK_Escape, 0, 0);
  return button;
}				/* gpa_button_cancel_new */


GtkWidget *
gpa_check_button_new (GtkAccelGroup * accelGroup, gchar * labelText)
{
/* var */
  guint accelKey;
/* objects */
  GtkWidget *checker;
/* commands */
  checker = gtk_check_button_new_with_label ("");
  accelKey =
    gtk_label_parse_uline (GTK_LABEL (GTK_BIN (checker)->child), labelText);
  gtk_widget_add_accelerator (checker, "clicked", accelGroup, accelKey,
			      GDK_MOD1_MASK, 0);
  return (checker);
}				/* gpa_check_button_new */

GtkWidget *
gpa_radio_button_new (GtkAccelGroup * accelGroup, gchar * labelText)
{
/* var */
  guint accelKey;
/* objects */
  GtkWidget *radio;
/* commands */
  radio = gtk_radio_button_new_with_label (NULL, "");
  accelKey =
    gtk_label_parse_uline (GTK_LABEL (GTK_BIN (radio)->child), labelText);
  gtk_widget_add_accelerator (radio, "clicked", accelGroup, accelKey,
			      GDK_MOD1_MASK, 0);
  return (radio);
}				/* gpa_radio_button_new */

GtkWidget *
gpa_radio_button_new_from_widget (GtkRadioButton * widget,
				  GtkAccelGroup * accelGroup,
				  gchar * labelText)
{
/* var */
  guint accelKey;
/* objects */
  GtkWidget *radio;
/* commands */
  radio = gtk_radio_button_new_with_label_from_widget (widget, "");
  accelKey =
    gtk_label_parse_uline (GTK_LABEL (GTK_BIN (radio)->child), labelText);
  gtk_widget_add_accelerator (radio, "clicked", accelGroup, accelKey,
			      GDK_MOD1_MASK, 0);
  return (radio);
}				/* gpa_radio_button_new_from_widget */

GtkWidget *
gpa_toggle_button_new (GtkAccelGroup * accelGroup, gchar * labelText)
{
/* var */
  guint accelKey;
/* objects */
  GtkWidget *toggle;
/* commands */
  toggle = gtk_toggle_button_new_with_label ("");
  accelKey =
    gtk_label_parse_uline (GTK_LABEL (GTK_BIN (toggle)->child), labelText);
  gtk_widget_add_accelerator (toggle, "clicked", accelGroup, accelKey,
			      GDK_MOD1_MASK, 0);
  return (toggle);
}				/* gpa_toggle_button_new */

void
gpa_connect_by_accelerator (GtkLabel * label, GtkWidget * widget,
			    GtkAccelGroup * accelGroup, gchar * labelText)
{
/* var */
  guint accelKey;
/* commands */
  accelKey = gtk_label_parse_uline (label, labelText);
  gtk_widget_add_accelerator (widget, "grab_focus", accelGroup, accelKey,
			      GDK_MOD1_MASK, 0);
}				/* gpa_connect_by_accelerator */


/* Set the text of the GtkButton button to text, remove existing
 * accelerators and and add new accelerators if accel_group is not NULL
 */
void
gpa_button_set_text (GtkWidget * button, gchar * text,
		     GtkAccelGroup * accel_group)
{
  GtkWidget * label = GTK_BIN (button)->child;
  guint accel;

  accel = gtk_label_parse_uline (GTK_LABEL (label), text);

#if 0  /* @@@@@@ */
  /* In GTK 1.2.8, the visible_only parameter of
   * gtk_widget_remove_accelerators is not even looked at, it always
   * behaves as if it were TRUE. Therefore make sure that
   * gtk_widget_add_accelerator is called with the GTK_ACCEL_VISIBLE
   * flag set to make remvoing the accels work.
   */
  gtk_widget_remove_accelerators (button, "clicked", TRUE);
  if (accel_group && accel != GDK_VoidSymbol)
    {
      gtk_widget_add_accelerator (button, "clicked", accel_group,
				  accel, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
    }
#endif
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
} /* gpa_window_error */

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
}				/* gpa_window_message */


/*
 * Modal file dialog
 */

struct _GPASaveFileNameDialog {
  GtkWidget * window;
  gchar * filename;
};
typedef struct _GPASaveFileNameDialog GPASaveFileNameDialog;

static void
file_dialog_ok (gpointer param)
{
  GPASaveFileNameDialog *dialog = param;

  dialog->filename 
      = (gchar *) gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog->window));
  if( dialog->filename != NULL )
	  dialog->filename = g_strdup (dialog->filename);

  gtk_widget_destroy (dialog->window);
}

static void
file_dialog_cancel (gpointer param)
{
  GPASaveFileNameDialog * dialog = param;
  dialog->filename = NULL;
  gtk_widget_destroy (dialog->window);
}

static void
file_dialog_destroy (GtkWidget * widget, gpointer param)
{
  gtk_main_quit ();
}

/* Run the modal file selection dialog and return a new copy of the
 * filename if the user pressed OK and NULL otherwise
 */
gchar *
gpa_get_save_file_name (GtkWidget * parent, const gchar * title,
			const gchar * directory)
{
  GPASaveFileNameDialog dialog;
  GtkWidget * window = gtk_file_selection_new (title);

  dialog.window = window;
  dialog.filename = NULL;

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (file_dialog_destroy), NULL);

  gtk_signal_connect_object (GTK_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (file_dialog_ok),
			     (gpointer) &dialog);
  gtk_signal_connect_object (
		      GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
		      "clicked", GTK_SIGNAL_FUNC (file_dialog_cancel),
		      (gpointer) &dialog);

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gpa_window_show_centered (window, parent);

  gtk_main ();

  return dialog.filename;
}

gchar *
gpa_get_load_file_name (GtkWidget * parent, const gchar * title,
			const gchar * directory)
{
  return gpa_get_save_file_name (parent, title, directory);
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

