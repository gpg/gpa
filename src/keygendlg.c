/* keygendlg.c  -  The GNU Privacy Assistant
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

#include <gtk/gtk.h>
#include "gpa.h"
#include "gpapastrings.h"
#include "gpawidgets.h"
#include "gtktools.h"
#include "keygendlg.h"
#include "qdchkpwd.h"

#define	XSTRDUP_OR_NULL(s)	((s != NULL) ? g_strdup(s): NULL)

struct _GPAKeyGenDialog {
  GtkWidget * window;
  GtkWidget * entryPasswd;
  GtkWidget * entryRepeat;
  GtkWidget * frameExpire;
};
typedef struct _GPAKeyGenDialog GPAKeyGenDialog;

/* This callback gets called each time the user clicks on the [OK] or [Cancel]
 * buttons. If the button was [OK], it verifies that the input makes sense.
 */
static void
response_cb(GtkDialog *dlg, gint response, gpointer param)
{
  GPAKeyGenDialog * dialog = param;
  gchar * expiry_error;
  const gchar *passwd = gtk_entry_get_text (GTK_ENTRY (dialog->entryPasswd));
  const gchar *repeat = gtk_entry_get_text (GTK_ENTRY (dialog->entryRepeat));

  if (response == GTK_RESPONSE_OK)
    {
      if (!g_str_equal (passwd, repeat))
        {
          gpa_window_error (_("In \"Passphrase\" and \"Repeat passphrase\",\n"
                              "you must enter the same passphrase."),
                            dialog->window);
          g_signal_stop_emission_by_name (dlg, "response");
        }
      else if (strlen (passwd) == 0)
        {
          gpa_window_error (_("You did not enter a passphrase.\n"
                              "It is needed to protect your private key."),
                            dialog->window);
          g_signal_stop_emission_by_name (dlg, "response");
        }
      else if (strlen (passwd) < 10 || qdchkpwd (passwd) < 0.6)
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
                                  _("_Enter new passphrase"), GTK_RESPONSE_CANCEL,
                                  _("Take this one _anyway"), GTK_RESPONSE_OK,
                                  NULL);
          if (gtk_dialog_run (GTK_DIALOG (msgbox)) == GTK_RESPONSE_CANCEL)
            {
              g_signal_stop_emission_by_name (dlg, "response");
            }
          gtk_widget_destroy (msgbox);
        }
      else if ((expiry_error = gpa_expiry_frame_validate (dialog->frameExpire)))
        {
          g_signal_stop_emission_by_name (dlg, "response");
          gpa_window_error (expiry_error, dialog->window);
        }
    }
}


/* Run the "Generate Key" dialog and if the user presses OK, return the
 * values from the dialog in a newly allocated GPAKeyGenParameters struct.
 * If the user pressed "Cancel", return NULL
 *
 * The returned struct has to be deleted with gpa_key_gen_free_parameters.
 */
GPAKeyGenParameters *
gpa_key_gen_run_dialog (GtkWidget * parent)
{
  GtkAccelGroup *accelGroup;
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

  GList *contentsAlgorithm = NULL;
  GPAKeyGenAlgo algo;
  GList *contentsKeysize = NULL;

  accelGroup = gtk_accel_group_new ();

  windowGenerate = gtk_dialog_new_with_buttons (_("Generate key"),
                                                GTK_WINDOW (parent),
                                                GTK_DIALOG_MODAL,
                                                GTK_STOCK_OK,
                                                GTK_RESPONSE_OK,
                                                GTK_STOCK_CANCEL,
                                                GTK_RESPONSE_CANCEL,
                                                NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (windowGenerate),
                                   GTK_RESPONSE_OK);
  dialog.window = windowGenerate;
  gtk_window_add_accel_group (GTK_WINDOW (windowGenerate), accelGroup);
  /* use gtk_signal_connect_object here to make the dialog pointer the
   * first parameter of the handler */
  g_signal_connect (G_OBJECT (windowGenerate), "delete-event",
                    G_CALLBACK (gtk_widget_destroy), &dialog);
  g_signal_connect (G_OBJECT (windowGenerate), "response",
                    G_CALLBACK(response_cb), &dialog);

  vboxGenerate = GTK_DIALOG(dialog.window)->vbox;
  gtk_container_add (GTK_CONTAINER (windowGenerate), vboxGenerate);
  gtk_container_set_border_width (GTK_CONTAINER (vboxGenerate), 5);

  table = gtk_table_new (7, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), table, FALSE, FALSE, 0);

  labelAlgorithm = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelAlgorithm), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelAlgorithm, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  comboAlgorithm = gtk_combo_new ();
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (comboAlgorithm)->entry),
			     FALSE);
  for (algo = GPA_KEYGEN_ALGO_FIRST; algo <= GPA_KEYGEN_ALGO_LAST; algo++)
    contentsAlgorithm = g_list_append (contentsAlgorithm,
				       (gchar*) gpa_algorithm_string(algo));
  gtk_combo_set_popdown_strings (GTK_COMBO (comboAlgorithm),
				 contentsAlgorithm);
  gpa_connect_by_accelerator (GTK_LABEL (labelAlgorithm),
			      GTK_COMBO (comboAlgorithm)->entry, accelGroup,
			      _("_Algorithm: "));
  gtk_table_attach (GTK_TABLE (table), comboAlgorithm, 1, 2, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);

  labelKeysize = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelKeysize), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelKeysize, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  comboKeysize = gtk_combo_new ();
  gtk_combo_set_value_in_list (GTK_COMBO (comboKeysize), FALSE, FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelKeysize),
			      GTK_COMBO (comboKeysize)->entry, accelGroup,
			      _("_Key size (bits): "));
  contentsKeysize = g_list_append (contentsKeysize, _("768"));
  contentsKeysize = g_list_append (contentsKeysize, _("1024"));
  contentsKeysize = g_list_append (contentsKeysize, _("2048"));
  gtk_combo_set_popdown_strings (GTK_COMBO (comboKeysize), contentsKeysize);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboKeysize)->entry), "1024");
  gtk_table_attach (GTK_TABLE (table), comboKeysize, 1, 2, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  /* FIXME: In the above we should either translate all key sizes
   * (including the one in the gtk_entry_set_text call) or generate all
   * strings via sprintf or a similar locale aware function */

  labelUserID = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelUserID), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelUserID, 0, 1, 2, 3,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryUserID = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelUserID), entryUserID,
			      accelGroup, _("_User ID: "));
  gtk_table_attach (GTK_TABLE (table), entryUserID, 1, 2, 2, 3, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelEmail = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelEmail), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelEmail, 0, 1, 3, 4,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryEmail = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelEmail), entryEmail, accelGroup,
			      _("_Email: "));
  gtk_table_attach (GTK_TABLE (table), entryEmail, 1, 2, 3, 4, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelComment = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelComment), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelComment, 0, 1, 4, 5,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryComment = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelComment), entryComment,
			      accelGroup, _("_Comment: "));
  gtk_table_attach (GTK_TABLE (table), entryComment, 1, 2, 4, 5, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelPasswd = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelPasswd), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelPasswd, 0, 1, 5, 6,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryPasswd = gtk_entry_new ();
  dialog.entryPasswd = entryPasswd;
  gtk_entry_set_visibility (GTK_ENTRY (entryPasswd), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelPasswd), entryPasswd,
			      accelGroup, _("_Passphrase: "));
  gtk_table_attach (GTK_TABLE (table), entryPasswd, 1, 2, 5, 6, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelRepeat = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelRepeat), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), labelRepeat, 0, 1, 6, 7,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryRepeat = gtk_entry_new ();
  dialog.entryRepeat = entryRepeat;
  gtk_entry_set_visibility (GTK_ENTRY (entryRepeat), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelRepeat), entryRepeat,
			      accelGroup, _("_Repeat passphrase: "));
  gtk_table_attach (GTK_TABLE (table), entryRepeat, 1, 2, 6, 7,
		    GTK_FILL, GTK_SHRINK, 0, 0);

  frameExpire = gpa_expiry_frame_new (accelGroup, NULL);
  dialog.frameExpire = frameExpire;
  gtk_container_set_border_width (GTK_CONTAINER (frameExpire), 5);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), frameExpire, FALSE, FALSE, 0);

  gtk_widget_show_all (windowGenerate);
  if (gtk_dialog_run (GTK_DIALOG (windowGenerate)) == GTK_RESPONSE_OK)
  {
      /* the user pressed OK, so create a GPAKeyGenParameters struct and
       * fill it with the values from the dialog
       */
      gchar * temp;

      params = key_gen_params_new ();
      params->userID = XSTRDUP_OR_NULL (gtk_entry_get_text (GTK_ENTRY (entryUserID)));
      params->email = XSTRDUP_OR_NULL(gtk_entry_get_text (GTK_ENTRY (entryEmail)));
      params->comment = XSTRDUP_OR_NULL (gtk_entry_get_text (GTK_ENTRY(entryComment)));

      params->password = XSTRDUP_OR_NULL (gtk_entry_get_text (GTK_ENTRY(entryPasswd)));
	  
      temp = (gchar *) gtk_entry_get_text (GTK_ENTRY (GTK_COMBO(comboAlgorithm)->entry));
      params->algo = gpa_algorithm_from_string (temp);
      temp = (gchar *) gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (comboKeysize)->entry));
      params->keysize = atoi (temp);

      params->expiryDate = NULL;
      params->interval = 0;
      if (!gpa_expiry_frame_get_expiration (frameExpire, &(params->expiryDate),
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
  {
      params = NULL;
  }

  gtk_widget_destroy (windowGenerate);

  return params;
} /* gpa_key_gen_run_dialog */
