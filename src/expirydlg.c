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
#include <gpgme.h>
#include <gtk/gtk.h>
#include "gpa.h"
#include "gpapastrings.h"
#include "gtktools.h"
#include "expirydlg.h"

typedef struct {
  
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
static gboolean
expiry_ok (GPAExpiryDialog * dialog, GDate **new_date)
{
  gboolean result;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_date)))
    {
      *new_date = g_date_new ();
      g_date_set_parse (*new_date,
			gtk_entry_get_text (GTK_ENTRY (dialog->entry_date)));

      if (!g_date_valid (*new_date))
	{
	  /* FIXME: This error message should be more informative */
	  gpa_window_error (_("Please provide a correct date."),
                            dialog->window);
	  result = FALSE;
	  g_date_free (*new_date);
	  *new_date = NULL;
	}
      else
	{
	  result = TRUE;
	}
    }
  else
    {
      *new_date = NULL;
      result = TRUE;
    }
  return result;
} /* expiry_ok */


/* Run the expiry date dialog as a modal dialog and return TRUE and set
 * (*new_date) to the (newly allocated) new expiry date if the user
 * clicked OK, otherwise return FALSE and don't modify *new_date.
 * *new_date == NULL means never expire
 */
gboolean
gpa_expiry_dialog_run (GtkWidget * parent, GpgmeKey key, GDate ** new_date)
{
  GtkWidget * window;
  GtkWidget * vbox;
  GtkWidget * radio;
  GtkWidget * hbox;
  GtkWidget * entry;
  GtkAccelGroup * accel_group;
  unsigned long expiry_date;

  GPAExpiryDialog dialog;

  accel_group = gtk_accel_group_new ();

  window = gtk_dialog_new_with_buttons (_("Change expiry data"),
                                        GTK_WINDOW (parent),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  dialog.window = window;
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  vbox = GTK_DIALOG (window)->vbox;
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  radio = gpa_radio_button_new (accel_group, _("_never expire"));
  dialog.radio_never = radio;
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);


  radio = gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio),
					    accel_group, _("_expire on"));
  dialog.radio_date = radio;
  gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);

  entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  dialog.entry_date = entry;
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);

  expiry_date = gpgme_key_get_ulong_attr (key, GPGME_ATTR_EXPIRE, NULL, 0);
  
  if (expiry_date > 0)
    {
      gchar *buffer;
      buffer = gpa_expiry_date_string (expiry_date);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      g_free (buffer);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog.radio_date),
				    TRUE);
    } /* if */
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog.radio_never),
				    TRUE);
    }

  gtk_widget_show_all (window);
  if (gtk_dialog_run (GTK_DIALOG (window)) == GTK_RESPONSE_OK)
    {
      gboolean result;
      result =  expiry_ok (&dialog, new_date);
      gtk_widget_destroy (window);
      return result;
    }
  else
    {
      gtk_widget_destroy (window);
      return FALSE;
    }
}
