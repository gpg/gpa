/* gtktools.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "icons.h"


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
}

void
gpa_window_error (gchar * message, GtkWidget * messenger)
{
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  GtkWidget *windowError;
  GtkWidget *vboxError;
  GtkWidget *labelMessage;
  GtkWidget *buttonClose;

  keeper = gpa_windowKeeper_new ();
  windowError = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowError);
  gtk_window_set_title (GTK_WINDOW (windowError), _("GPA Error"));
  gtk_window_set_modal (GTK_WINDOW (windowError), TRUE);
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowError), accelGroup);
  vboxError = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxError), 10);
  labelMessage = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (vboxError), labelMessage, TRUE, FALSE, 10);
  buttonClose = gpa_button_new (accelGroup, _("_OK"));
  gtk_signal_connect_object (GTK_OBJECT (buttonClose), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) windowError);
  gtk_widget_add_accelerator (buttonClose, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  gtk_box_pack_start (GTK_BOX (vboxError), buttonClose, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowError), vboxError);

  if (messenger)
    gpa_window_show_centered (windowError, messenger);
  else
    gtk_widget_show_all (windowError);
  gtk_widget_grab_focus (buttonClose);
} /* gpa_window_error */

void
gpa_window_message (gchar * message, GtkWidget * messenger)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowMessage;
  GtkWidget *vboxMessage;
  GtkWidget *labelMessage;
  GtkWidget *buttonClose;
/* commands */
  keeper = gpa_windowKeeper_new ();
  windowMessage = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowMessage);
  gtk_window_set_title (GTK_WINDOW (windowMessage), _("GPA Message"));
  gtk_window_set_modal (GTK_WINDOW (windowMessage), TRUE);
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowMessage), accelGroup);
  vboxMessage = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxMessage), 10);
  labelMessage = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (vboxMessage), labelMessage, TRUE, FALSE, 10);
  buttonClose = gpa_button_new (accelGroup, _("_OK"));
  gtk_signal_connect_object (GTK_OBJECT (buttonClose), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) windowMessage);
  gtk_widget_add_accelerator (buttonClose, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  gtk_box_pack_start (GTK_BOX (vboxMessage), buttonClose, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowMessage), vboxMessage);
  gpa_window_show_centered (windowMessage, messenger);
  gtk_widget_grab_focus (buttonClose);
}				/* gpa_window_message */


/* a simple but flexible message box */


typedef struct {
  gchar * result;
  GtkWidget * window;
} GPAMessageBox;

static void
message_box_destroy (GtkWidget *widget, gpointer param)
{
  gtk_main_quit ();
}

static void
message_box_clicked (GtkWidget * button, gpointer param)
{
  GPAMessageBox * dialog = param;

  dialog->result = gtk_object_get_data (GTK_OBJECT (button), "user_data");
  gtk_widget_destroy (dialog->window);
}

gchar *
gpa_message_box_run (GtkWidget * parent, const gchar * title,
		     const gchar * message,
		     const gchar ** buttons)
{
  GtkAccelGroup *accel_group;

  GtkWidget * window;
  GtkWidget * vbox;
  GtkWidget * label;
  GtkWidget * bbox;
  GtkWidget * button;
  int i;
  GPAMessageBox dialog;
  

  dialog.result = NULL;

  window = gtk_window_new (GTK_WINDOW_DIALOG);
  dialog.window = window;
  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (message_box_destroy),
		      NULL);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  bbox = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), bbox, TRUE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 10);

  for (i = 0; buttons[i]; i++)
    {
      button = gpa_button_new (accel_group, (gchar*)(buttons[i]));
      gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
      gtk_object_set_data (GTK_OBJECT (button), "user_data",
			   (gpointer)buttons[i]);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (message_box_clicked),
			  (gpointer)&dialog);
    }

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gpa_window_show_centered (window, parent);

  gtk_main ();

  return dialog.result;
}
  

void
gpa_window_passphrase (GtkWidget * messenger, GtkSignalFunc func, gchar * tip,
		       gpointer data)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gpointer *param;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowPassphrase;
  GtkWidget *vboxPassphrase;
  GtkWidget *hboxPasswd;
  GtkWidget *labelPasswd;
  GtkWidget *entryPasswd;
  GtkWidget *hButtonBoxPassphrase;
  GtkWidget *buttonCancel;
  GtkWidget *buttonOK;
/* commands */
  keeper = gpa_windowKeeper_new ();
  windowPassphrase = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowPassphrase);
  gpa_windowKeeper_add_param (keeper, data);
  gtk_window_set_title (GTK_WINDOW (windowPassphrase), _("Insert password"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowPassphrase), accelGroup);
  vboxPassphrase = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxPassphrase), 5);
  hboxPasswd = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxPasswd), 5);
  labelPasswd = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hboxPasswd), labelPasswd, FALSE, FALSE, 0);
  entryPasswd = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entryPasswd), FALSE);
  param = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, param);
  param[0] = data;
  param[1] = entryPasswd;
  param[2] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (entryPasswd), "activate",
			     GTK_SIGNAL_FUNC (func), (gpointer) param);
  gtk_box_pack_start (GTK_BOX (hboxPasswd), entryPasswd, TRUE, TRUE, 0);
  gpa_connect_by_accelerator (GTK_LABEL (labelPasswd), entryPasswd,
			      accelGroup, _("_Password: "));
  gtk_box_pack_start (GTK_BOX (vboxPassphrase), hboxPasswd, TRUE, TRUE, 0);
  hButtonBoxPassphrase = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxPassphrase),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxPassphrase), 10);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = tip;
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxPassphrase), 5);
  gtk_container_add (GTK_CONTAINER (hButtonBoxPassphrase), buttonCancel);
  buttonOK = gpa_button_new (accelGroup, _("_OK"));
  gtk_signal_connect_object (GTK_OBJECT (buttonOK), "clicked",
			     GTK_SIGNAL_FUNC (func), (gpointer) param);
  gtk_container_add (GTK_CONTAINER (hButtonBoxPassphrase), buttonOK);
  gtk_box_pack_start (GTK_BOX (vboxPassphrase), hButtonBoxPassphrase, FALSE,
		      FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowPassphrase), vboxPassphrase);
  gpa_window_show_centered (windowPassphrase, messenger);
  gtk_widget_grab_focus (entryPasswd);
}				/* gpa_window_passphrase */

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
  GPASaveFileNameDialog * dialog = param;

  dialog->filename \
      = gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog->window));
  dialog->filename = xstrdup (dialog->filename);

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
