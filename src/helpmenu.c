/* helpmenu.c  -  The GNU Privacy Assistant
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
#include <gtk/gtk.h>
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gtktools.h"

void
help_about (void)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *button;

  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("About GPA"));
  gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy),
		      GTK_OBJECT (dialog));

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 1);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 1);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  label = gtk_label_new ( "GNU Privacy Assistant v" VERSION );
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 2);

  label = gtk_label_new (_("See http://www.gnupg.org for news"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 2);

  bbox = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), bbox,
							  TRUE, TRUE, 5);
  gtk_button_box_set_child_size(GTK_BUTTON_BOX(bbox), 80, 0);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 10);

  button = gtk_button_new_with_label(_("OK"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(dialog));

  gtk_widget_show_all (dialog);
}



void
help_license (void)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowLicense;
  GtkWidget *vboxLicense;
  GtkWidget *vboxGPL;
  GtkWidget *labelJfdGPL;
  GtkWidget *labelGPL;
  GtkWidget *hboxGPL;
  GtkWidget *textGPL;
  GtkWidget *vscrollbarGPL;
  GtkWidget *hButtonBoxLicense;
  GtkWidget *buttonClose;
/* commands */
  keeper = gpa_windowKeeper_new ();
  windowLicense = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowLicense);
  gtk_window_set_title (GTK_WINDOW (windowLicense),
			_("GNU general public license"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowLicense), accelGroup);
  vboxLicense = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxLicense), 5);
  vboxGPL = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxGPL), 5);
  labelGPL = gtk_label_new (_(""));
  labelJfdGPL = gpa_widget_hjustified_new (labelGPL, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxGPL), labelJfdGPL, FALSE, FALSE, 0);
  hboxGPL = gtk_hbox_new (FALSE, 0);
  textGPL = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (textGPL), FALSE);
  gtk_text_insert (GTK_TEXT (textGPL), NULL, &textGPL->style->black, NULL,
		   "GNU General public license", -1);
  gtk_widget_set_usize (textGPL, 280, 400);
  gpa_connect_by_accelerator (GTK_LABEL (labelGPL), textGPL, accelGroup,
			      _("_GNU general public license"));
  gtk_box_pack_start (GTK_BOX (hboxGPL), textGPL, TRUE, TRUE, 0);
  vscrollbarGPL = gtk_vscrollbar_new (GTK_TEXT (textGPL)->vadj);
  gtk_box_pack_start (GTK_BOX (hboxGPL), vscrollbarGPL, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxGPL), hboxGPL, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxLicense), vboxGPL, TRUE, TRUE, 0);
  hButtonBoxLicense = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxLicense),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxLicense), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxLicense), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = NULL;
  buttonClose = gpa_buttonCancel_new (accelGroup, _("_Close"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxLicense), buttonClose);
  gtk_box_pack_start (GTK_BOX (vboxLicense), hButtonBoxLicense, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowLicense), vboxLicense);
  gtk_widget_show_all (windowLicense);
  gpa_widget_set_centered (windowLicense, global_windowMain);
}

void
help_warranty (void)
{
  g_print (_("Show Warranty Information\n"));   /*!!! */
}				/* help_warranty */

void
help_help (void)
{
  g_print (_("Show Help Text\n"));      /*!!! */
}				/* help_help */
