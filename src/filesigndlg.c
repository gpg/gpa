/* filesigndlg.c  -  The GNU Privacy Assistant
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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>
#include <errno.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"

struct _GPAFileSignDialog {
  GtkWidget *window;
  GtkWidget *radio_comp;
  GtkWidget *radio_sign;
  GtkWidget *radio_sep;
  GtkWidget *check_armor;
  GtkCList *clist_who;
  GList *files;
  GList *signed_files;
};
typedef struct _GPAFileSignDialog GPAFileSignDialog;

static FILE *
open_destination_file (const gchar *filename, GpgmeSigMode sign_type,
		       gboolean armor, gchar **target_filename,
		       GtkWidget *parent)
{
  const gchar *extension;
  FILE *target;
  
  if (!armor)
    {
      if (sign_type == GPGME_SIG_MODE_DETACH)
	extension = ".sig";
      else
	extension = ".gpg";
    }
  else
    extension = ".asc";
  *target_filename = g_strconcat (filename, extension, NULL);
  target = gpa_fopen (*target_filename, parent);
  if (!target)
    {
      g_free (*target_filename);
    }
  return target;
}

static void
sign_files (GPAFileSignDialog *dialog, gchar *fpr, GpgmeSigMode sign_type, 
	    gboolean armor)
{
  GList *cur;
  GpgmeError err;
  GpgmeKey signer;

  dialog->signed_files = NULL;

  /* Get ready to sign: common for all signed files */
  gpgme_signers_clear (ctx);
  signer = gpa_keytable_secret_lookup (keytable, fpr);
  if (!signer)
    {
      /* Can't happen */
      gpa_window_error (_("The key you selected is not available for "
			  "signing"), dialog->window);
      return;
    }
  err = gpgme_signers_add (ctx, signer);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  gpgme_set_armor (ctx, armor);
  
  /* Sign each file */
  for (cur = dialog->files; cur; cur = g_list_next (cur))
    {
      FILE *target;
      gchar *filename, *target_filename;
      GpgmeData input, output;

      /* Find out the name of the signed file */
      filename = cur->data;
      target = open_destination_file (filename, sign_type, armor, 
				      &target_filename, dialog->window);
      if (!target)
	break;
      /* Create the appropiate GpgmeData's*/
      err = gpgme_data_new_from_file (&input, filename, 1);
      if (err == GPGME_File_Error)
	{
	  gchar *message;
	  message = g_strdup_printf ("%s: %s", filename, strerror(errno));
	  gpa_window_error (message, dialog->window);
	  g_free (message);
	  g_free (target_filename);
	  fclose (target);
	  break;
	}
      else if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
      err = gpgme_data_new (&output);
      if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
      /* Sign */
      err = gpgme_op_sign (ctx, input, output, sign_type);
      if (err == GPGME_No_Passphrase)
	{
	  gpa_window_error (_("Wrong passphrase!"), dialog->window);
	  break;
	}
      else if (err == GPGME_Canceled)
	{
	  break;
	}
      else if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
      dialog->signed_files = g_list_append (dialog->signed_files,
					    target_filename);
      /* Write the output */
      dump_data_to_file (output, target);
      fclose (target);
      gpgme_data_release (input);
      gpgme_data_release (output);
    }
}

static void
file_sign_ok (GPAFileSignDialog *dialog)
{
  GpgmeSigMode sign_type;
  gboolean armor;
  gchar *fpr;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_comp)))
    sign_type = GPGME_SIG_MODE_NORMAL;
  else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->radio_sign)))
    sign_type = GPGME_SIG_MODE_CLEAR;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(dialog->radio_sep)))
    sign_type = GPGME_SIG_MODE_DETACH;
  else
    {
      gpa_window_error (_("Internal error:\nInvalid sign mode"), dialog->window);
      return;
    } /* else */

  if (gpa_simplified_ui ())
    {
      if (sign_type == GPGME_SIG_MODE_DETACH)
        armor = FALSE;
      else
	armor = TRUE;
    }
  else
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->check_armor)))
        armor = TRUE;
      else
        armor = FALSE;
    }

  if (dialog->clist_who->selection)
    {
      gint row = GPOINTER_TO_INT (dialog->clist_who->selection->data);
      fpr = gtk_clist_get_row_data (dialog->clist_who, row);
    }
  else
    {
      gpa_window_error (_("No key selected!"), dialog->window);
      return;
    }

  sign_files (dialog, fpr, sign_type, armor);
} /* file_sign_ok */


/* Run the file sign dialog and sign the files given by the arguments
 * files and num_files. Return a GList with the names of the newly
 * created files
 */
   
GList *
gpa_file_sign_dialog_run (GtkWidget * parent, GList *files)
{
  GPAFileSignDialog dialog;
  GtkAccelGroup *accelGroup;
  GtkWidget *window;
  GtkWidget *vboxSign;
  GtkWidget *frameMode;
  GtkWidget *vboxMode;
  GtkWidget *radio_sign_comp;
  GtkWidget *radio_sign;
  GtkWidget *radio_sign_sep;
  GtkWidget *checkerArmor;
  GtkWidget *vboxWho;
  GtkWidget *labelWho;
  GtkWidget *scrollerWho;
  GtkWidget *clistWho;
  GtkResponseType response;

  dialog.files = files;
  dialog.signed_files = NULL;

  if (gpa_keytable_secret_size (keytable) == 0)
    {
      gpa_window_error (_("No secret keys available for signing.\n"
			  "Please generate or import a secret key first."),
			parent);
      return FALSE;
    } /* if */

  window = gtk_dialog_new_with_buttons (_("Sign Files"), GTK_WINDOW(parent),
					GTK_DIALOG_MODAL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  dialog.window = window;

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accelGroup);

  vboxSign = GTK_DIALOG (window)->vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vboxSign), 5);

  frameMode = gtk_frame_new (_("Signing Mode"));
  gtk_container_set_border_width (GTK_CONTAINER (frameMode), 5);
  gtk_box_pack_start (GTK_BOX (vboxSign), frameMode, FALSE, FALSE, 0);

  vboxMode = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxMode), 5);
  gtk_container_add (GTK_CONTAINER (frameMode), vboxMode);

  if (gpa_simplified_ui ())
    {
      radio_sign_comp = NULL;
      dialog.radio_comp = radio_sign_comp;
      radio_sign = gpa_radio_button_new (accelGroup, _("_cleartext signature"));
      gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign, FALSE, FALSE, 0);
      dialog.radio_sign = radio_sign;
      radio_sign_sep =
        gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio_sign),
				          accelGroup,
				          _("sign in separate _file"));
    }
  else
    {
      radio_sign_comp = gpa_radio_button_new (accelGroup, _("si_gn and compress"));
      gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign_comp, FALSE, FALSE, 0);
      dialog.radio_comp = radio_sign_comp;
      radio_sign =
	gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio_sign_comp),
					  accelGroup, _("sign, do_n't compress"));
      gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign, FALSE, FALSE, 0);
      dialog.radio_sign = radio_sign;
      radio_sign_sep =
        gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio_sign_comp),
				          accelGroup,
				          _("sign in separate _file"));
    }
  gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign_sep, FALSE, FALSE, 0);
  dialog.radio_sep = radio_sign_sep;

  if (gpa_simplified_ui ())
    checkerArmor = NULL;
  else
    {
      checkerArmor = gpa_check_button_new (accelGroup, _("a_rmor"));
      gtk_container_set_border_width (GTK_CONTAINER (checkerArmor), 5);
      gtk_box_pack_start (GTK_BOX (vboxSign), checkerArmor, FALSE, FALSE, 0);
    }
  dialog.check_armor = checkerArmor;
    
  vboxWho = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxWho), 5);
  gtk_box_pack_start (GTK_BOX (vboxSign), vboxWho, TRUE, TRUE, 0);

  labelWho = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelWho), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vboxWho), labelWho, FALSE, TRUE, 0);

  scrollerWho = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerWho, 260, 75);
  gtk_box_pack_start (GTK_BOX (vboxWho), scrollerWho, TRUE, TRUE, 0);

  clistWho = gpa_secret_key_list_new ();
  dialog.clist_who = GTK_CLIST (clistWho);
  gtk_container_add (GTK_CONTAINER (scrollerWho), clistWho);
  gpa_connect_by_accelerator (GTK_LABEL (labelWho), clistWho, accelGroup,
			      _("Sign _as "));

  gtk_widget_show_all (window);
  response = gtk_dialog_run (GTK_DIALOG (window));
  if (response == GTK_RESPONSE_OK)
    {
      file_sign_ok (&dialog);
    }

  gtk_widget_destroy (window);
  return dialog.signed_files;
} /* gpa_file_sign_dialog_run */
