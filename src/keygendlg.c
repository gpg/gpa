/* keygendlg.c - The GNU Privacy Assistant
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2008 g10 Code GmbH

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>

#include "gpa.h"
#include "gpawidgets.h"
#include "gtktools.h"
#include "keygendlg.h"
#include "qdchkpwd.h"

#define	XSTRDUP_OR_NULL(s)	((s != NULL) ? g_strdup (s) : NULL)

struct _GPAKeyGenDialog
{
  gboolean forcard;		/* Specifies if this is a dialog for
				   on-card key generation or not. */
  GtkWidget *window;
  GtkWidget *entryUserID;
  GtkWidget *entryPasswd;
  GtkWidget *entryRepeat;
  GtkWidget *frameExpire;
  GtkWidget *comboKeysize;
};
typedef struct _GPAKeyGenDialog GPAKeyGenDialog;


/* This callback gets called each time the user clicks on the [OK] or
   [Cancel] buttons.  If the button was [OK], it verifies that the
   input makes sense.  */
static void
response_cb (GtkDialog *dlg, gint response, gpointer param)
{
  GPAKeyGenDialog *dialog = param;
  gchar *expiry_error;
  const gchar *userid;
  const gchar *passwd;
  const gchar *repeat;
  const gchar *keysize;

  if (response != GTK_RESPONSE_OK)
    return;

  userid = gtk_entry_get_text (GTK_ENTRY (dialog->entryUserID));
  passwd = (dialog->entryPasswd
            ? gtk_entry_get_text (GTK_ENTRY (dialog->entryPasswd)) 
            : NULL);
  repeat = (dialog->entryRepeat
            ? gtk_entry_get_text (GTK_ENTRY (dialog->entryRepeat))
            : NULL);
  keysize = gtk_combo_box_get_active_text (GTK_COMBO_BOX 
                                           (dialog->comboKeysize));

  if (keysize == NULL || *keysize == '\0')
    {
      /* FIXME: We should check it is a valid number.  */
      gpa_window_error (_("You must enter a key size."), dialog->window);
      g_signal_stop_emission_by_name (dlg, "response");
    }      
  else if (! *userid)
    {
      gpa_window_error (_("You must enter a User ID."), dialog->window);
      g_signal_stop_emission_by_name (dlg, "response");
    }
  else if ((!dialog->forcard) && (!g_str_equal (passwd, repeat)))
    {
      gpa_window_error (_("In \"Passphrase\" and \"Repeat passphrase\",\n"
			  "you must enter the same passphrase."),
			dialog->window);
      g_signal_stop_emission_by_name (dlg, "response");
    }
  else if ((!dialog->forcard) && strlen (passwd) == 0)
    {
      gpa_window_error (_("You did not enter a passphrase.\n"
			  "It is needed to protect your private key."),
			dialog->window);
      g_signal_stop_emission_by_name (dlg, "response");
    }
  else if ((!dialog->forcard) && (strlen (passwd) < 10 || qdchkpwd (passwd) < 0.6))
    {
      GtkWidget *msgbox;
      
      msgbox = gtk_message_dialog_new (GTK_WINDOW (dialog->window),
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_WARNING,
				       GTK_BUTTONS_NONE,
				       _("Warning: You have entered a "
					 "passphrase\n"
					 "that is obviously not secure.\n\n"
					 "Please enter a new passphrase."));
      gtk_dialog_add_buttons (GTK_DIALOG (msgbox), 
			      _("_Enter new passphrase"),
			      GTK_RESPONSE_CANCEL,
			      _("Take this one _anyway"), GTK_RESPONSE_OK,
			      NULL);
      if (gtk_dialog_run (GTK_DIALOG (msgbox)) == GTK_RESPONSE_CANCEL)
	g_signal_stop_emission_by_name (dlg, "response");
      gtk_widget_destroy (msgbox);
    }
  else if ((expiry_error = gpa_expiry_frame_validate (dialog->frameExpire)))
    {
      g_signal_stop_emission_by_name (dlg, "response");
      gpa_window_error (expiry_error, dialog->window);
    }
}


/* Run the "Generate Key" dialog and if the user presses OK, return
   the values from the dialog in a newly allocated GPAKeyGenParameters
   struct.  If FORCARD is true, display the dialog suitable for
   generation keys on the OpenPGP smartcard. If the user pressed
   "Cancel", return NULL.  The returned struct has to be deleted with
   gpa_key_gen_free_parameters.  */
GPAKeyGenParameters *
gpa_key_gen_run_dialog (GtkWidget *parent, gboolean forcard)
{
  GtkWidget *windowGenerate;
  GtkWidget *vboxGenerate;
  GtkWidget *table;
  GtkWidget *labelAlgorithm;
  GtkWidget *comboAlgorithm;
  GtkWidget *labelKeysize;
  GtkWidget *comboKeysize;
  GtkWidget *frameExpire;
  GtkWidget *labelUserID;
  GtkWidget *entryUserID;
  GtkWidget *labelEmail;
  GtkWidget *entryEmail;
  GtkWidget *labelComment;
  GtkWidget *entryComment;
  GtkWidget *labelPasswd;
  GtkWidget *entryPasswd;
  GtkWidget *labelRepeat;
  GtkWidget *entryRepeat;

  GPAKeyGenDialog dialog;
  GPAKeyGenParameters * params = NULL;

  GPAKeyGenAlgo algo;

  dialog.forcard = forcard;

  windowGenerate = gtk_dialog_new_with_buttons
    (forcard ? _("Generate new keys on card") : _("Generate key"), GTK_WINDOW (parent),
     GTK_DIALOG_MODAL,
     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
     GTK_STOCK_OK, GTK_RESPONSE_OK,
     NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (windowGenerate),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (windowGenerate),
                                   GTK_RESPONSE_OK);
  dialog.window = windowGenerate;

  /* Use g_signal_connect_object here to make the dialog pointer the
     first parameter of the handler.  */
  //  g_signal_connect (G_OBJECT (windowGenerate), "delete-event",
  //  		    G_CALLBACK (gtk_widget_destroy), dialog.window);
  g_signal_connect (G_OBJECT (windowGenerate), "response",
                    G_CALLBACK (response_cb), &dialog);

  vboxGenerate = GTK_DIALOG(dialog.window)->vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vboxGenerate), 5);

  table = gtk_table_new (7, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), table, FALSE, FALSE, 0);

  labelAlgorithm = gtk_label_new_with_mnemonic (_("_Algorithm: "));
  gtk_misc_set_alignment (GTK_MISC (labelAlgorithm), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelAlgorithm, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  comboAlgorithm = gtk_combo_box_new_text ();

  if (forcard)
    /* The OpenPGP smartcard does only support RSA. */
    gtk_combo_box_append_text (GTK_COMBO_BOX (comboAlgorithm), "RSA");
  else
    {
      for (algo = GPA_KEYGEN_ALGO_FIRST; algo <= GPA_KEYGEN_ALGO_LAST; algo++)
	gtk_combo_box_append_text (GTK_COMBO_BOX (comboAlgorithm), 
				   gpa_algorithm_string (algo));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (comboAlgorithm), 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (labelAlgorithm), comboAlgorithm);
			      
  gtk_table_attach (GTK_TABLE (table), comboAlgorithm, 1, 2, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);

  labelKeysize = gtk_label_new_with_mnemonic (_("_Key size (bits): "));
  gtk_misc_set_alignment (GTK_MISC (labelKeysize), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelKeysize, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  comboKeysize = gtk_combo_box_new_text ();
  dialog.comboKeysize = comboKeysize;

  if (forcard)
    {
      /* The OpenPGP smartcard does only support 1024bit RSA
	 keys. FIXME: should we really hardcode this? -mo */
      gtk_combo_box_append_text (GTK_COMBO_BOX (comboKeysize), _("1024"));
      gtk_combo_box_set_active (GTK_COMBO_BOX (comboKeysize), 0);
    }
  else
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (comboKeysize), _("768"));
      gtk_combo_box_append_text (GTK_COMBO_BOX (comboKeysize), _("1024"));
      gtk_combo_box_append_text (GTK_COMBO_BOX (comboKeysize), _("2048"));
      gtk_combo_box_set_active (GTK_COMBO_BOX (comboKeysize), 1 /* 1024 */);
    }
  gtk_label_set_mnemonic_widget (GTK_LABEL (labelKeysize), comboKeysize);
  gtk_table_attach (GTK_TABLE (table), comboKeysize, 1, 2, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);

  labelUserID = gtk_label_new_with_mnemonic (_("_User ID: "));
  gtk_misc_set_alignment (GTK_MISC (labelUserID), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelUserID, 0, 1, 2, 3,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryUserID = gtk_entry_new ();
  dialog.entryUserID = entryUserID;
  gtk_label_set_mnemonic_widget (GTK_LABEL (labelUserID), entryUserID);

  gtk_table_attach (GTK_TABLE (table), entryUserID, 1, 2, 2, 3, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelEmail = gtk_label_new_with_mnemonic (_("_Email: "));
  gtk_misc_set_alignment (GTK_MISC (labelEmail), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelEmail, 0, 1, 3, 4,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryEmail = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (labelEmail), entryEmail);
  gtk_table_attach (GTK_TABLE (table), entryEmail, 1, 2, 3, 4, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelComment = gtk_label_new_with_mnemonic (_("_Comment: "));
  gtk_misc_set_alignment (GTK_MISC (labelComment), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelComment, 0, 1, 4, 5,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryComment = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (labelComment), entryComment);
  gtk_table_attach (GTK_TABLE (table), entryComment, 1, 2, 4, 5, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  entryPasswd = NULL;		/* Silence compiler warning. */

  if (forcard)
    {
      /* This doesn't make sense for the smartcard. */
      dialog.entryPasswd = NULL;
      dialog.entryRepeat = NULL;
    }
  else
    {
      labelPasswd = gtk_label_new_with_mnemonic (_("_Passphrase: "));
      gtk_misc_set_alignment (GTK_MISC (labelPasswd), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), labelPasswd, 0, 1, 5, 6,
			GTK_FILL, GTK_SHRINK, 0, 0);
      entryPasswd = gtk_entry_new ();
      dialog.entryPasswd = entryPasswd;
      gtk_entry_set_visibility (GTK_ENTRY (entryPasswd), FALSE);
      gtk_label_set_mnemonic_widget (GTK_LABEL (labelPasswd), entryPasswd);
      gtk_table_attach (GTK_TABLE (table), entryPasswd, 1, 2, 5, 6, GTK_FILL,
			GTK_SHRINK, 0, 0);

      labelRepeat = gtk_label_new_with_mnemonic (_("_Repeat passphrase: "));
      gtk_misc_set_alignment (GTK_MISC (labelRepeat), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), labelRepeat, 0, 1, 6, 7,
			GTK_FILL, GTK_SHRINK, 0, 0);
      entryRepeat = gtk_entry_new ();
      dialog.entryRepeat = entryRepeat;
      gtk_entry_set_visibility (GTK_ENTRY (entryRepeat), FALSE);
      gtk_label_set_mnemonic_widget (GTK_LABEL (labelRepeat), entryRepeat);

      gtk_table_attach (GTK_TABLE (table), entryRepeat, 1, 2, 6, 7,
			GTK_FILL, GTK_SHRINK, 0, 0);
    }

  frameExpire = gpa_expiry_frame_new (NULL);
  dialog.frameExpire = frameExpire;
  gtk_container_set_border_width (GTK_CONTAINER (frameExpire), 5);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), frameExpire, FALSE, FALSE, 0);

  gtk_widget_show_all (windowGenerate);
  if (gtk_dialog_run (GTK_DIALOG (windowGenerate)) == GTK_RESPONSE_OK)
    {
      /* The user pressed OK, so create a GPAKeyGenParameters struct
	 and fill it with the values from the dialog.  */
      gchar *temp;

      params = key_gen_params_new ();
      params->userID
	= XSTRDUP_OR_NULL (gtk_entry_get_text (GTK_ENTRY (entryUserID)));
      params->email
	= XSTRDUP_OR_NULL(gtk_entry_get_text (GTK_ENTRY (entryEmail)));
      params->comment
	= XSTRDUP_OR_NULL (gtk_entry_get_text (GTK_ENTRY(entryComment)));

      if (forcard)
	params->password = NULL;
      else
	params->password
	  = XSTRDUP_OR_NULL (gtk_entry_get_text (GTK_ENTRY(entryPasswd)));

      if (forcard)
	/* Although this values isn't used... */
	params->algo = GPA_KEYGEN_ALGO_RSA;
      else
	{
	  temp = gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboAlgorithm));
	  params->algo = gpa_algorithm_from_string (temp);
	}

      temp = gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboKeysize));
      params->keysize = atoi (temp);

      params->expiryDate = NULL;
      params->interval = 0;
      if (! gpa_expiry_frame_get_expiration (frameExpire, &(params->expiryDate),
					     &(params->interval),
					     &(params->unit)))
	{
	  gpa_window_error (_("!FATAL ERROR!\n"
			      "Invalid insert mode for expiry date."),
			    parent);
	  gpa_key_gen_free_parameters(params);
	  params = NULL;
	}
    }
  else
    params = NULL;

  gtk_widget_destroy (windowGenerate);

  return params;
}
