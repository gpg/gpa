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
#include <gpapa.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "passphrasedlg.h"
#include "filesigndlg.h"


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


static void
file_sign_do_sign (GPAFileSignDialog *dialog, gchar *key_id, gchar *passphrase,
		   GpapaSignType sign_type, GpapaArmor armor)
{
  GList *cur;

  dialog->signed_files = NULL;

  cur = dialog->files;
  while (cur)
    {
      GpapaFile *file;
      const gchar *extension;
      gchar *filename, *target_filename;

      global_lastCallbackResult = GPAPA_ACTION_NONE;

      file = cur->data;
      filename = gpapa_file_get_identifier (file, gpa_callback,
					     dialog->window);
      if (armor == GPAPA_NO_ARMOR)
        {
	  if (sign_type == GPAPA_SIGN_DETACH)
	    extension = ".sig";
	  else
	    extension = ".gpg";
        }
      else
        extension = ".asc";
      target_filename = xstrcat2 (filename, extension);

      gpapa_file_sign (file, target_filename, key_id, passphrase, sign_type, armor,
		       gpa_callback, dialog->window);

      if (global_lastCallbackResult == GPAPA_ACTION_ERROR)
	break;
      dialog->signed_files = g_list_append (dialog->signed_files, target_filename);
      cur = g_list_next (cur);
    }
}

static void
file_sign_ok (gpointer param)
{
  GPAFileSignDialog *dialog = param;
  GpapaSignType sign_type;
  GpapaArmor armor;
  gchar *key_id;
  GpapaSecretKey * key;
  gchar *passphrase;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_comp)))
    sign_type = GPAPA_SIGN_NORMAL;
  else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->radio_sign)))
    sign_type = GPAPA_SIGN_CLEAR;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(dialog->radio_sep)))
    sign_type = GPAPA_SIGN_DETACH;
  else
    {
      gpa_window_error (_("Internal error:\nInvalid sign mode"), dialog->window);
      return;
    } /* else */

  if (gpa_simplified_ui ())
    {
      if (sign_type == GPAPA_SIGN_DETACH)
        armor = GPAPA_NO_ARMOR;
      else
	armor = GPAPA_ARMOR;
    }
  else
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->check_armor)))
        armor = GPAPA_ARMOR;
      else
        armor = GPAPA_NO_ARMOR;
    }

  if (dialog->clist_who->selection)
    {
      gint row =  GPOINTER_TO_INT (dialog->clist_who->selection->data);
      key_id = gtk_clist_get_row_data (dialog->clist_who, row);
      key = gpapa_get_secret_key_by_ID (key_id, gpa_callback, dialog->window);
    }
  else
    {
      gpa_window_error (_("No key selected!"), dialog->window);
      return;
    }
  passphrase = gpa_passphrase_run_dialog (dialog->window, key);
  if (passphrase)
    {
      file_sign_do_sign (dialog, key_id, passphrase, sign_type, armor);
    }
  free (passphrase);
  gtk_widget_destroy (dialog->window);
} /* file_sign_ok */


static void
file_sign_cancel (gpointer param)
{
  GPAFileSignDialog *dialog = param;

  dialog->signed_files = NULL;
  gtk_widget_destroy (dialog->window);
}

static void
file_sign_destroy (GtkWidget * widget, gpointer param)
{
  gtk_main_quit ();
}

/* Run the file sign dialog and sign the files given by the arguments
 * files and num_files. Return a GList with the names of the newly
 * created files
 */
   
GList *
gpa_file_sign_dialog_run (GtkWidget * parent, GList *files)
{
  GPAFileSignDialog dialog;
  GtkAccelGroup *accelGroup;
  GtkWidget *windowSign;
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
  GtkWidget *hButtonBoxSign;
  GtkWidget *buttonCancel;
  GtkWidget *buttonSign;

  dialog.files = files;
  dialog.signed_files = NULL;

  if (gpapa_get_secret_key_count (gpa_callback, parent) == 0)
    {
      gpa_window_error (_("No secret keys available for signing.\n"
			  "Please generate or import a secret key first."),
			parent);
      return FALSE;
    } /* if */

  windowSign = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (windowSign), _("Sign Files"));
  dialog.window = windowSign;
  gtk_signal_connect (GTK_OBJECT (windowSign), "destroy",
		      GTK_SIGNAL_FUNC (file_sign_destroy), NULL);

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowSign), accelGroup);

  vboxSign = gtk_vbox_new (FALSE, 0);
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
      radio_sign =
	gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio_sign_comp),
					  accelGroup, _("_copy and add signature"));
      gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign, FALSE, FALSE, 0);
      dialog.radio_sign = radio_sign;
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
    }
  radio_sign_sep =
    gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio_sign_comp),
				      accelGroup,
				      _("sign in separate _file"));
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

  clistWho = gpa_secret_key_list_new (parent);
  dialog.clist_who = GTK_CLIST (clistWho);
  gtk_container_add (GTK_CONTAINER (scrollerWho), clistWho);
  gpa_connect_by_accelerator (GTK_LABEL (labelWho), clistWho, accelGroup,
			      _("Sign _as "));

  hButtonBoxSign = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vboxSign), hButtonBoxSign, FALSE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxSign),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxSign), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxSign), 5);
  buttonCancel = gpa_button_cancel_new (accelGroup, _("_Cancel"),
					(GtkSignalFunc) file_sign_cancel, (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSign), buttonCancel);

  buttonSign = gpa_button_new (accelGroup, "_OK");
  gtk_signal_connect_object (GTK_OBJECT (buttonSign), "clicked",
			     GTK_SIGNAL_FUNC (file_sign_ok),
			     (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSign), buttonSign);
  gtk_container_add (GTK_CONTAINER (windowSign), vboxSign);
  gpa_window_show_centered (windowSign, parent);
  gtk_window_set_modal (GTK_WINDOW (windowSign), TRUE);

  gtk_main ();

  return dialog.signed_files;
} /* gpa_file_sign_dialog_run */
