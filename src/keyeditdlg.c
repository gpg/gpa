/* keyeditdlg.c  -	 The GNU Privacy Assistant
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

#include "gpa.h"
#include <time.h>
#include <config.h>
#include <gpapa.h>
#include <gtk/gtk.h>
#include "gpapastrings.h"
#include "gtktools.h"
#include "gpgmeedit.h"
#include "siglist.h"
#include "ownertrustdlg.h"
#include "expirydlg.h"
#include "keyeditdlg.h"

typedef struct
{
  GtkWidget *window;
  GtkWidget *expiry;
  GtkWidget *ownertrust;
  gchar *fpr;
  gboolean key_has_changed;
}
GPAKeyEditDialog;


/* internal API */
static GtkWidget *add_details_row (GtkWidget *table, gint row, gchar *label,
				   gchar *text, gboolean selectable);

static void key_edit_change_expiry (GtkWidget *widget, gpointer param);
static void key_edit_change_trust (GtkWidget *widget, gpointer param);


/* run the key edit dialog as a modal dialog */
gboolean
gpa_key_edit_dialog_run (GtkWidget * parent, gchar * fpr)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *table;
  GtkAccelGroup *accel_group;

  GpgmeKey key;
  GpgmeValidity trust;
  gchar *date_string;
  gchar *string;

  GPAKeyEditDialog dialog;

  dialog.fpr = fpr;
  dialog.key_has_changed = FALSE;

  key = gpa_keytable_lookup (keytable, fpr);

  accel_group = gtk_accel_group_new ();

  window = gtk_dialog_new_with_buttons (_("Edit Key"), GTK_WINDOW(parent),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_CLOSE,
                                        GTK_RESPONSE_CLOSE,
                                        NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_CLOSE);
  dialog.window = window;
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);

  vbox = GTK_DIALOG (window)->vbox;

  /* info about the key */
  table = gtk_table_new (2, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  
  string = gpa_gpgme_key_get_userid (key, 0);
  add_details_row (table, 0, _("User Name:"), string, FALSE);
  g_free (string); 
  add_details_row (table, 1, _("Key ID:"),
                   (gchar*) gpa_gpgme_key_get_short_keyid (key, 0), FALSE);


  /* change expiry date */
  frame = gtk_frame_new (_("Expiry Date"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 5);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_set_spacing (GTK_BOX (hbox), 10);

  date_string = gpa_expiry_date_string (gpgme_key_get_ulong_attr 
                                        (key, GPGME_ATTR_EXPIRE, NULL, 0));
  label = gtk_label_new (date_string);
  free (date_string);
  dialog.expiry = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  button = gpa_button_new (accel_group, _("Change _expiration"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, gpa_keytable_secret_lookup 
                            (keytable, fpr) != NULL);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc)key_edit_change_expiry, &dialog);


  /* Owner Trust */
  frame = gtk_frame_new (_("Owner Trust"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 5);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_set_spacing (GTK_BOX (hbox), 10);

  trust = gpgme_key_get_ulong_attr (key, GPGME_ATTR_OTRUST, NULL, 0);
  label = gtk_label_new (gpa_trust_string (trust));
  dialog.ownertrust = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  button = gpa_button_new (accel_group, _("Change _trust"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc)key_edit_change_trust, &dialog);


  gtk_widget_show_all (window);
  gtk_dialog_run (GTK_DIALOG (window));
  gtk_widget_destroy (window);
  return dialog.key_has_changed;
}

/* add a single row to the details table */
static GtkWidget *
add_details_row (GtkWidget * table, gint row, gchar *label_text,
		 gchar * text, gboolean selectable)
{
  GtkWidget * widget;

  widget = gtk_label_new (label_text);
  gtk_table_attach (GTK_TABLE (table), widget, 0, 1, row, row + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);

  if (!selectable)
    {
      widget = gtk_label_new (text);
      gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    }
  else
    {
      widget = gtk_entry_new ();
      gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);
      gtk_entry_set_text (GTK_ENTRY (widget), text);
    }
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, row, row + 1,
		    GTK_FILL | GTK_EXPAND, 0, 0, 0);

  return widget;
}


/* signal handler for the owner trust change button. */
static void
key_edit_change_trust(GtkWidget * widget, gpointer param)
{
  GPAKeyEditDialog * dialog = param;
  GpgmeKey key;
  GpgmeValidity ownertrust;
  GpgmeError err;
  gboolean result;

  key = gpa_keytable_lookup (keytable, dialog->fpr);
  result = gpa_ownertrust_run_dialog (key, dialog->window,
				      &ownertrust);

  if (result)
    {
      err = gpa_gpgme_edit_ownertrust (key, ownertrust);
      if (err == GPGME_No_Error)
        {
          gtk_label_set_text (GTK_LABEL (dialog->ownertrust),
                              gpa_trust_string (ownertrust));
          /* Reload all keys */
          gpa_keytable_reload (keytable);
          dialog->key_has_changed = TRUE;
        }
      else
        {
          gpa_gpgme_error (err);
        }
    }
}
  

/* signal handler for the expiry date change button. */
static void
key_edit_change_expiry(GtkWidget * widget, gpointer param)
{
  GPAKeyEditDialog * dialog = param;
  GpgmeKey key;
  GpgmeError err;
  GDate * new_date;
  struct tm tm;

  key = gpa_keytable_lookup (keytable, dialog->fpr);

  if (gpa_expiry_dialog_run (dialog->window, key, &new_date))
    {
      gchar * date_string;
      err = gpa_gpgme_edit_expiry (key, new_date);
      if (err == GPGME_No_Error)
        {
          if (new_date)
            {
              g_date_to_struct_tm (new_date, &tm);
              date_string = gpa_expiry_date_string (mktime(&tm));
            }
          else
            {
              date_string = gpa_expiry_date_string (0);
            }
          gtk_label_set_text (GTK_LABEL (dialog->expiry), date_string);
          g_free (date_string);
          if (new_date)
            g_date_free (new_date);
          /* Reload the key */
          gpa_keytable_load_key (keytable, dialog->fpr);
          dialog->key_has_changed = TRUE;
        }
      else if (err == GPGME_No_Passphrase)
        {
	  gpa_window_error (_("Wrong passphrase!"), dialog->window);
          if (new_date)
            g_date_free (new_date);
          dialog->key_has_changed = FALSE;
        }
      else if (err == GPGME_Canceled)
        {
          if (new_date)
            g_date_free (new_date);
          dialog->key_has_changed = FALSE;
        }
      else
        {
          gpa_gpgme_error (err);
        }
    }
}
