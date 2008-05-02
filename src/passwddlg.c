/* passwddlg.c - The GNU Privacy Assistant
 *      Copyright (C) 2002, Miguel Coca.
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

#include "gpa.h"
#include "passwddlg.h"
#include "gtktools.h"
#include "qdchkpwd.h"

#include <unistd.h>

static void
passwd_activated_cb (GtkWidget *entry, gpointer param)
{
  GtkWidget *repeat_entry = param;
  gtk_widget_grab_focus (repeat_entry);
}

static gboolean
is_passphrase_correct (GtkWidget *parent, const gchar *passwd, 
                       const gchar *repeat)
{
  gboolean result = TRUE;
  
  if (!g_str_equal (passwd, repeat))
    {
      gpa_window_error (_("In \"Passphrase\" and \"Repeat passphrase\",\n"
                          "you must enter the same passphrase."),
                       parent);
      result = FALSE;
    }
  else if (strlen (passwd) == 0)
    {
      gpa_window_error (_("You did not enter a passphrase.\n"
                          "It is needed to protect your private key."),
                        parent);
      result = FALSE;
    }
  else if (strlen (passwd) < 10 || qdchkpwd (passwd) < 0.6)
    {
      GtkWidget *dialog;
      
      dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_WARNING,
                                       GTK_BUTTONS_NONE,
                                       _("Warning: You have entered a "
                                         "passphrase\n"
                                         "that is obviously not secure.\n\n"
                                         "Please enter a new passphrase."));
      gtk_dialog_add_buttons (GTK_DIALOG (dialog), 
                              _("_Enter new passphrase"), GTK_RESPONSE_CANCEL,
                              _("Take this one _anyway"), GTK_RESPONSE_OK,
                              NULL);
      if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_CANCEL)
        {
          result = FALSE;
        }
      gtk_widget_destroy (dialog);
    }

  return result;
}

gpg_error_t gpa_change_passphrase_dialog_run (void *hook, 
					      const char *uid_hint,
					      const char *passphrase_info, 
					      int prev_was_bad, int fd)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label, *entry, *passwd_entry, *repeat_entry;
  GtkResponseType response;
  gchar *passwd = NULL;
  const gchar *repeat;

  dialog = gtk_dialog_new_with_buttons (_("Choose new passphrase"), NULL,
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  vbox = GTK_DIALOG (dialog)->vbox;
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 5);

  label = gtk_label_new (_("Passphrase: "));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  entry = passwd_entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);

  label = gtk_label_new (_("Repeat Passphrase: "));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  entry = repeat_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);

  /* Pressing enter on the passphrase entry switches the focus to the
   * "repeat" entry. */
  g_signal_connect (G_OBJECT (passwd_entry), "activate",
                    G_CALLBACK (passwd_activated_cb), repeat_entry);

  /* Run the dialog until the input is correct or the user cancels */
  do 
    {
      gtk_entry_set_text (GTK_ENTRY (passwd_entry), "");
      gtk_entry_set_text (GTK_ENTRY (repeat_entry), "");
      gtk_widget_grab_focus (passwd_entry);
      gtk_widget_show_all (dialog);
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      g_free (passwd);
      passwd = g_strdup (gtk_entry_get_text (GTK_ENTRY (passwd_entry)));
      repeat = gtk_entry_get_text (GTK_ENTRY (repeat_entry));
    } 
  while (response == GTK_RESPONSE_OK &&
         !is_passphrase_correct (dialog, passwd, repeat));  
  gtk_widget_destroy (dialog);

  if (response == GTK_RESPONSE_OK)
    {
      write (fd, passwd, strlen (passwd));
      g_free (passwd);
      /* GnuPG wants a newline here */
      write (fd, "\n", 1);
      return gpg_error (GPG_ERR_NO_ERROR);
    }
  else
    {
      return gpg_error (GPG_ERR_CANCELED);
    }
}
