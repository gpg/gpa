/* helpmenu.c  -  The GNU Privacy Assistant
 *      Copyright (C) 2000 Free Software Foundation, Inc.
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
help_version (void)
{
  g_print (_("Show Version Information\n"));	/*!!! */
}				/* help_version */

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
}				// help_license

void
help_warranty (void)
{
  g_print (_("Show Warranty Information\n"));	/*!!! */
}				/* help_warranty */

void
help_help (void)
{
  g_print (_("Show Help Text\n"));	/*!!! */
}				/* help_help */
