/* gtktools.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000 Free Software Foundation, Inc.
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

void
gpa_widget_set_centered (GtkWidget * widget, GtkWidget * parent)
{
/* var */
  int x0, y0, w0, h0, x, y, w, h;
/* commands */
  gdk_window_get_size (widget->window, &w, &h);
  gdk_window_get_origin (parent->window, &x0, &y0);
  gdk_window_get_size (parent->window, &w0, &h0);
  x = x0 + (w0 - w) / 2;
  y = y0 + (h0 - h) / 2;
  gdk_window_move_resize (widget->window, x, y, w, h);
}				/* gpa_widget_set_centered */

void
gpa_widget_show (GtkWidget * widget, GtkWidget * parent, gchar * tip)
{
    if ( opt.tooltip_kludge ) {
	/* Some windowmangers minimizes the new windows, so we
	 * use this workaround to show them */
	gpa_widget_set_centered (widget, parent);
	gpa_windowTip_show (tip);
	gtk_widget_show_all (widget);
    }
    else {
	gtk_widget_show_all (widget);
	gpa_widget_set_centered (widget, parent);
	gpa_windowTip_show (tip);
    }
}				/* gpa_widget_show */

void
gpa_window_destroy (gpointer param)
{
/* var */
  gpointer *localParam;
  GpaWindowKeeper *keeper;
  gchar *tip;
/* commands */
  localParam = (gpointer *) param;
  keeper = (GpaWindowKeeper *) localParam[0];
  tip = (gchar *) localParam[1];
  gpa_windowKeeper_release (keeper);
  if (tip)
    {
      if (strcmp (tip, "") != 0)
	gpa_windowTip_show (tip);
    }				/* if */
  else
    gtk_widget_hide (global_windowTip);
}				/* gpa_window_destroy */

GtkWidget *
gpa_space_new (void)
{
  return gtk_label_new (_(""));
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
  button = gtk_button_new_with_label (_(""));
  accelKey =
    gtk_label_parse_uline (GTK_LABEL (GTK_BIN (button)->child), labelText);
  gtk_widget_add_accelerator (button, "clicked", accelGroup, accelKey,
			      GDK_MOD1_MASK, 0);
  return (button);
}				/* gpa_button_new */

/* Create a new hbox with an image and a label packed into it
 * and return the box. Then treat the accelerators. */
GtkWidget *gpa_xpm_label_box( GtkWidget *parent,
						 gchar     **xpm,
						 gchar     *label_text,
						 GtkWidget * button,
						 GtkAccelGroup * accelGroup)
{
   guint accelKey;
   GtkWidget *box1;
   GtkWidget *label;
   GtkWidget *pixmapwid;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GtkStyle *style;

   /* Create box for xpm and label */
   box1 = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (box1), 2);

   /* Get the style of the button to get the
	* background color. */
   style = gtk_widget_get_style(parent);

   /* Now on to the xpm stuff */
   pixmap = gdk_pixmap_create_from_xpm_d (parent->window, &mask,
						&style->bg[GTK_STATE_NORMAL],
						(gchar **)xpm);
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

GtkWidget *
gpa_check_button_new (GtkAccelGroup * accelGroup, gchar * labelText)
{
/* var */
  guint accelKey;
/* objects */
  GtkWidget *checker;
/* commands */
  checker = gtk_check_button_new_with_label (_(""));
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
  radio = gtk_radio_button_new_with_label (NULL, _(""));
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
  radio = gtk_radio_button_new_with_label_from_widget (widget, _(""));
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
  toggle = gtk_toggle_button_new_with_label (_(""));
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

void
gpa_window_error (gchar * message, GtkWidget * messenger)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowError;
  GtkWidget *vboxError;
  GtkWidget *labelMessage;
  GtkWidget *buttonClose;
/* commands */
  keeper = gpa_windowKeeper_new ();
  windowError = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowError);
  gtk_window_set_title (GTK_WINDOW (windowError), "GPA Error");
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
  gtk_widget_show_all (windowError);
  gpa_widget_set_centered (windowError, messenger);
  gtk_widget_grab_focus (buttonClose);
}				/* gpa_window_error */

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
  gtk_window_set_title (GTK_WINDOW (windowMessage), "GPA Message");
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
  gtk_widget_show_all (windowMessage);
  gpa_widget_set_centered (windowMessage, messenger);
  gtk_widget_grab_focus (buttonClose);
}				/* gpa_window_message */

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
  labelPasswd = gtk_label_new (_(""));
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
  gtk_widget_show_all (windowPassphrase);
  gpa_widget_set_centered (windowPassphrase, messenger);
  gtk_widget_grab_focus (entryPasswd);
}				/* gpa_window_passphrase */
