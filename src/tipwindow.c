/* tipwindow.c  - Help facility
 * Copyright (C) 2000 OpenIT GmbH
 * Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
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

static GtkWidget *global_windowTip = NULL;

void
gpa_window_tip_init (void)
{
  GtkAccelGroup *accelGroup;
  GtkWidget *vboxTip;
  GtkWidget *vboxContents;
  GtkWidget *labelJfdContents;
  GtkWidget *labelContents;
  GtkWidget *hboxContents;
  GtkWidget *textContents;
  GtkWidget *vscrollbarContents;
  GtkWidget *hboxTip;
  GtkWidget *checkerNomore;
  GtkWidget *buttonClose;

  global_windowTip = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (global_windowTip), _("GPA Tip"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (global_windowTip), accelGroup);
  vboxTip = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxTip), 5);
  vboxContents = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxContents), 5);
  labelContents = gtk_label_new ("");
  labelJfdContents =
    gpa_widget_hjustified_new (labelContents, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxContents), labelJfdContents, FALSE, FALSE,
		      0);
  hboxContents = gtk_hbox_new (FALSE, 0);
  textContents = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (textContents), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelContents), textContents,
			      accelGroup, _("_Tip:"));
  global_textTip = textContents;
  gtk_box_pack_start (GTK_BOX (hboxContents), textContents, TRUE, TRUE, 0);
  vscrollbarContents = gtk_vscrollbar_new (GTK_TEXT (textContents)->vadj);
  gtk_box_pack_start (GTK_BOX (hboxContents), vscrollbarContents, FALSE,
		      FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxContents), hboxContents, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxTip), vboxContents, TRUE, TRUE, 0);
  hboxTip = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxTip), 5);
  buttonClose = gpa_button_new (accelGroup, _("   _Close   "));
  gtk_signal_connect_object (GTK_OBJECT (buttonClose), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_hide),
			     (gpointer) global_windowTip);
  gtk_widget_add_accelerator (buttonClose, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  gtk_box_pack_end (GTK_BOX (hboxTip), buttonClose, FALSE, FALSE, 0);
  checkerNomore =
    gpa_check_button_new (accelGroup, _("_No more tips, please"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkerNomore),
				global_noTips);
  gtk_signal_connect (GTK_OBJECT (checkerNomore), "clicked",
		      GTK_SIGNAL_FUNC (gpa_switch_tips), NULL);
  gtk_box_pack_end (GTK_BOX (hboxTip), checkerNomore, FALSE, FALSE, 10);
  gtk_box_pack_start (GTK_BOX (vboxTip), hboxTip, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (global_windowTip), vboxTip);
}				/* gpa_window_tip_init */

void
gpa_windowTip_show (const char *key)
{
  char line[512];
  FILE *fp;
  int empty_lines = 0;

  if (global_noTips)
    return;

  gtk_editable_delete_text (GTK_EDITABLE (global_textTip), 0, -1);
  {
    char *fname;
    const char *datadir = GPA_DATADIR;
    const char *lang = get_language ();

    fname = xmalloc (20 + strlen(datadir) + strlen(lang));
    strcpy (stpcpy (stpcpy (fname, datadir), "/gpa_tips."), lang);
    fp = fopen (fname, "r");
    free (fname);
  }
  if (!fp)
    {
      gtk_text_insert (GTK_TEXT (global_textTip), NULL,
		       &global_textTip->style->black, NULL,
		       _("Installation problem: tip file not found"), -1);
      goto leave;
    }

  /* Read until we find the key */
  while (fgets (line, DIM(line), fp))
    {
      trim_spaces (line);
      if (*line == '\\' && line[1] == ':')
	{
	  trim_spaces (line+2);
	  if (!strcmp (line+2, key))
	    break;
	}
    }
  if (ferror (fp) || feof (fp))
    {
      gtk_text_insert (GTK_TEXT (global_textTip), NULL,
		       &global_textTip->style->black, NULL,
		       key, -1);
      goto leave;
    }
  /* Continue with read, do the formatting and stop at the next key */
  while (fgets (line, DIM(line), fp))
    {
      trim_spaces (line);
      if (*line == '#')
	continue;
      if (*line == '\\' && line[1] == ':')
	break; /* next key - ready */
      if (!*line)
	{
	  empty_lines++;
	  continue;
	}
      if (empty_lines)
	{
	  /* make a paragraph */
	  gtk_text_insert (GTK_TEXT (global_textTip), NULL,
			   &global_textTip->style->black, NULL, "\n\n", -1);
	  empty_lines = 0;
	}
      /* fixme: handle control sequences */
      gtk_text_insert (GTK_TEXT (global_textTip), NULL,
		       &global_textTip->style->black, NULL, line, -1);
    }
 leave:
  if (fp)
    fclose (fp);
  gtk_widget_show_all (global_windowTip);
}



