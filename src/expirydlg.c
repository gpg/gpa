/* expirydlg.c - The GNU Privacy Assistant
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <gpapa.h>
#include <gtk/gtk.h>
#include "gpa.h"
#include "gpapastrings.h"
#include "gtktools.h"
#include "passphrasedlg.h"
#include "expirydlg.h"

typedef struct {
  
  /* True, if the user clicked OK, false otherwise */
  gboolean result;

  /* The new expiry date. Only valid if result is true. NULL means,
     never exipre */
  GDate * expiry_date;

  /* The password for the key. */
  gchar * password;

  /* The toplevel window of the dialog */
  GtkWidget * window;

  /* The radio buttons */
  GtkWidget * radio_never;
  GtkWidget * radio_date;

  /* The date entry field */
  GtkWidget * entry_date;
} GPAExpiryDialog;


/* Handler for the OK button. Read and validate the user's input and set
 * result and expiry_date in the dialog struct accordingly. If all is
 * OK, destroy the dialog.
 */
static void
expiry_ok (GtkWidget *widget, gpointer param)
{
  GPAExpiryDialog * dialog = param;
  gboolean result;
  GDate * date = NULL;
  gchar * password;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_date)))
    {
      date = g_date_new ();
      g_date_set_parse (date,
			gtk_entry_get_text (GTK_ENTRY (dialog->entry_date)));

      if (!g_date_valid (date))
	{
	  const gchar * buttons[] = {_("Ok"), NULL};
	  /* FIXME: This error message should be more informative */
	  gpa_message_box_run (dialog->window, _("Invalid Date"),
			       _("Please provide a correct date"), buttons);
	  result = FALSE;
	  g_date_free (date);
	  date = NULL;
	}
      else
	{
	  result = TRUE;
	}
    }
  else
    {
      date = NULL;
      result = TRUE;
    }

  if (result)
    {
      /* The data is OK now ask for the password */
      password = gpa_passphrase_run_dialog (dialog->window);
      if (!password)
	{
	  /* the user cancelled, so free the data that was allocated in
	   * this function */
	  if (date)
	    g_date_free (date);
	  result = FALSE;
	}
      else
	{
	  dialog->result = TRUE;
	  dialog->expiry_date = date;
	  dialog->password = password;
	  gtk_widget_destroy (dialog->window);
	}
    }
} /* expiry_ok */


/* Handler for the cancel button. Just destroy the window.
 * dialog->result is false by default */
static void
expiry_cancel (gpointer param)
{
  GPAExpiryDialog * dialog = param;

  gtk_widget_destroy (dialog->window);
}


/* Handler for the destroy signal of the dialog window. Just quit the
 * recursive mainloop */
static void
expiry_destroy (GtkWidget *widget, gpointer param)
{
  gtk_main_quit ();
}


/* Run the expiry date dialog as a modal dialog and return TRUE and set
 * ** (*new_date) to the (newly allocated) new expiry date if the user *
 * *clicked OK, otherwise return FALSE and don't modify *new_date.
 * *new_date == NULL means never expire
 */
gboolean
gpa_expiry_dialog_run (GtkWidget * parent, GDate * expiry_date,
		       GDate ** new_date, gchar ** password)
{
  GtkWidget * window;
  GtkWidget * vbox;
  GtkWidget * radio;
  GtkWidget * hbox;
  GtkWidget * entry;
  GtkWidget * bbox;
  GtkWidget * button;
  GtkAccelGroup * accel_group;

  GPAExpiryDialog dialog;
  dialog.result = FALSE;
  dialog.expiry_date = NULL;

  accel_group = gtk_accel_group_new ();

  window = gtk_window_new (GTK_WINDOW_DIALOG);
  dialog.window = window;
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_window_set_title (GTK_WINDOW (window), _("Change Expiry Date"));
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (expiry_destroy), &dialog);

  vbox = gtk_vbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  radio = gpa_radio_button_new (accel_group, _("_never expire"));
  dialog.radio_never = radio;
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);


  radio = gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio),
					    accel_group, _("expire _on"));
  dialog.radio_date = radio;
  gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);

  entry = gtk_entry_new ();
  dialog.entry_date = entry;
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);

  if (expiry_date)
    {
      gchar *buffer;
      buffer = gpa_expiry_date_string (expiry_date);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      free (buffer);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog.radio_date),
				    TRUE);
    } /* if */
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog.radio_never),
				  TRUE);

  /* buttons */
  bbox = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, TRUE, 5);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);

  button = gpa_button_new (accel_group, _("_Ok"));
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc)expiry_ok, &dialog);

  button = gpa_button_cancel_new (accel_group, _("_Cancel"),
				  (GtkSignalFunc)expiry_cancel, &dialog);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, TRUE, 0);

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gpa_window_show_centered (window, parent);

  gtk_main ();

  if (dialog.result)
    {
      *new_date = dialog.expiry_date;
      *password = dialog.password;
    }
  return dialog.result;
} /* gpa_expiry_dialog_run */
