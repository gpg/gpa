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
#include "gtktools.h"
#include "expirydlg.h"

typedef struct {
  
  /* The toplevel window of the dialog */
  GtkWidget * window;

  /* The radio buttons */
  GtkWidget * radio_never;
  GtkWidget * radio_date;

  /* The calendar field */
  GtkWidget * calendar;
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
      guint day, month, year;
      gtk_calendar_get_date (GTK_CALENDAR (dialog->calendar),
                             &year, &month, &day);

      *new_date = g_date_new_dmy (day, month+1, year);

      if (!g_date_valid (*new_date))
	{
	  /* Can't happen */
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

static void
expire_date_toggled_cb (GtkToggleButton *togglebutton, gpointer user_data)
{
  GtkWidget *calendar = user_data;
  
  gtk_widget_set_sensitive (calendar,
                            gtk_toggle_button_get_active (togglebutton));
}

/* Run the expiry date dialog as a modal dialog and return TRUE and set
 * (*new_date) to the (newly allocated) new expiry date if the user
 * clicked OK, otherwise return FALSE and don't modify *new_date.
 * *new_date == NULL means never expire
 */
gboolean
gpa_expiry_dialog_run (GtkWidget * parent, gpgme_key_t key, GDate ** new_date)
{
  GtkWidget * window;
  GtkWidget * vbox;
  GtkWidget * radio;
  GtkWidget * calendar;
  time_t expiry_date;

  GPAExpiryDialog dialog;

  window = gtk_dialog_new_with_buttons (_("Change expiry date"),
                                        GTK_WINDOW (parent),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        _("_Cancel"),
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  dialog.window = window;

  vbox = GTK_DIALOG (window)->vbox;
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  radio = gtk_radio_button_new_with_mnemonic (NULL, _("_never expire"));
  dialog.radio_never = radio;
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);

  radio = gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (radio), _("_expire on:"));
  dialog.radio_date = radio;
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);

  calendar = gtk_calendar_new ();
  dialog.calendar = calendar;
  /* Disable the calendar by default */
  gtk_widget_set_sensitive (calendar, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), calendar, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (dialog.radio_date), "toggled",
                    G_CALLBACK (expire_date_toggled_cb), calendar);

  expiry_date = (time_t) key->subkeys->expires;
  
  if (expiry_date > 0)
    {
      GDate tmp;
      g_date_set_time_t (&tmp, expiry_date);
      gtk_calendar_select_month (GTK_CALENDAR (calendar),
                                 g_date_get_month (&tmp)-1,
                                 g_date_get_year (&tmp));
      gtk_calendar_select_day (GTK_CALENDAR (calendar),
                               g_date_get_day (&tmp));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog.radio_date),
				    TRUE);
    }
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
