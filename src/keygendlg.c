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

struct _GPAKeyGenDialog {
  GtkWidget * window;
  GtkWidget * entryPasswd;
  GtkWidget * entryRepeat;
  GtkWidget * frameExpire;
  gboolean result;
};
typedef struct _GPAKeyGenDialog GPAKeyGenDialog;


static GPAKeyGenParameters *
key_gen_params_new(void)
{
  GPAKeyGenParameters * params = xmalloc (sizeof (*params));
  params->userID = NULL;
  params->email = NULL;
  params->comment = NULL;
  params->expiryDate = NULL;
  params->interval = 0;
  params->generate_revocation = FALSE;
  params->send_to_server = FALSE;
  return params;
}

void
gpa_key_gen_free_parameters(GPAKeyGenParameters * params)
{
  free (params->userID);
  free (params->email);
  free (params->comment);
  if (params->expiryDate)
    g_date_free (params->expiryDate);
  free (params);
}



static void
key_gen_cancel (gpointer param)
{
  GPAKeyGenDialog * dialog = param;

  dialog->result = FALSE;
  gtk_main_quit ();
}

static void
key_gen_ok (gpointer param)
{
  GPAKeyGenDialog * dialog = param;
  gchar * expiry_error;

  if (strcmp (gtk_entry_get_text (GTK_ENTRY (dialog->entryPasswd)),
	      gtk_entry_get_text (GTK_ENTRY (dialog->entryRepeat))) != 0)
    {
      gpa_window_error (_("In \"Password\" and \"Repeat Password\""
			  "\nyou must enter the same password."),
			dialog->window);
    }
  else if ((expiry_error = gpa_expiry_frame_validate (dialog->frameExpire)))
    {
      gpa_window_error (expiry_error, dialog->window);
    }
  else
    {
      dialog->result = TRUE;
      gtk_main_quit ();
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
  GtkWidget *tableTop;
  GtkWidget *labelAlgorithm;
  GtkWidget *comboAlgorithm;
  GtkWidget *labelKeysize;
  GtkWidget *comboKeysize;
  GtkWidget *frameExpire;
  GtkWidget *tableMisc;
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
  GtkWidget *vboxMisc;
  GtkWidget *checkerRevoc;
  GtkWidget *checkerSend;
  GtkWidget *hButtonBoxGenerate;
  GtkWidget *buttonCancel;
  GtkWidget *buttonGenerate;

  GPAKeyGenDialog dialog;
  GPAKeyGenParameters * params = NULL;

  GList *contentsAlgorithm = NULL;
  GpapaAlgo algo;
  GList *contentsKeysize = NULL;

  dialog.result = FALSE;

  accelGroup = gtk_accel_group_new ();

  windowGenerate = gtk_window_new (GTK_WINDOW_DIALOG);
  dialog.window = windowGenerate;
  gtk_window_set_title (GTK_WINDOW (windowGenerate), _("Generate key"));
  gtk_window_add_accel_group (GTK_WINDOW (windowGenerate), accelGroup);

  vboxGenerate = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowGenerate), vboxGenerate);
  gtk_container_set_border_width (GTK_CONTAINER (vboxGenerate), 5);

  tableTop = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (tableTop), 5);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), tableTop, FALSE, FALSE, 0);

  labelAlgorithm = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelAlgorithm), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (tableTop), labelAlgorithm, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  comboAlgorithm = gtk_combo_new ();
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (comboAlgorithm)->entry),
			     FALSE);
  for (algo = GPAPA_ALGO_FIRST; algo <= GPAPA_ALGO_LAST; algo++)
    contentsAlgorithm = g_list_append (contentsAlgorithm,
				       gpa_algorithm_string(algo));
  gtk_combo_set_popdown_strings (GTK_COMBO (comboAlgorithm),
				 contentsAlgorithm);
  gpa_connect_by_accelerator (GTK_LABEL (labelAlgorithm),
			      GTK_COMBO (comboAlgorithm)->entry, accelGroup,
			      _("_Encryption algorithm: "));
  gtk_table_attach (GTK_TABLE (tableTop), comboAlgorithm, 1, 2, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);

  labelKeysize = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelKeysize), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (tableTop), labelKeysize, 0, 1, 1, 2,
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
  gtk_table_attach (GTK_TABLE (tableTop), comboKeysize, 1, 2, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);

  frameExpire = gpa_expiry_frame_new (accelGroup, NULL);
  dialog.frameExpire = frameExpire;
  gtk_container_set_border_width (GTK_CONTAINER (frameExpire), 5);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), frameExpire, FALSE, FALSE, 0);

  tableMisc = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), tableMisc, TRUE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (tableMisc), 5);

  labelUserID = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelUserID), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (tableMisc), labelUserID, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryUserID = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelUserID), entryUserID,
			      accelGroup, _("_User ID: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryUserID, 1, 2, 0, 1, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelEmail = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelEmail), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (tableMisc), labelEmail, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryEmail = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelEmail), entryEmail, accelGroup,
			      _("E-_Mail: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryEmail, 1, 2, 1, 2, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelComment = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelComment), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (tableMisc), labelComment, 0, 1, 2, 3,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryComment = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelComment), entryComment,
			      accelGroup, _("C_omment: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryComment, 1, 2, 2, 3, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelPasswd = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelPasswd), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (tableMisc), labelPasswd, 0, 1, 3, 4,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryPasswd = gtk_entry_new ();
  dialog.entryPasswd = entryPasswd;
  gtk_entry_set_visibility (GTK_ENTRY (entryPasswd), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelPasswd), entryPasswd,
			      accelGroup, _("_Password: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryPasswd, 1, 2, 3, 4, GTK_FILL,
		    GTK_SHRINK, 0, 0);

  labelRepeat = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelRepeat), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (tableMisc), labelRepeat, 0, 1, 4, 5,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryRepeat = gtk_entry_new ();
  dialog.entryRepeat = entryRepeat;
  gtk_entry_set_visibility (GTK_ENTRY (entryRepeat), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelRepeat), entryRepeat,
			      accelGroup, _("_Repeat password: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryRepeat, 1, 2, 4, 5,
		    GTK_FILL, GTK_SHRINK, 0, 0);

  vboxMisc = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), vboxMisc, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxMisc), 5);

  checkerRevoc = gpa_check_button_new (accelGroup,
				       _("generate re_vocation certificate"));
  gtk_box_pack_start (GTK_BOX (vboxMisc), checkerRevoc, FALSE, FALSE, 0);

  checkerSend = gpa_check_button_new (accelGroup, _("_send to key server"));
  gtk_box_pack_start (GTK_BOX (vboxMisc), checkerSend, FALSE, FALSE, 0);

  hButtonBoxGenerate = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vboxGenerate), hButtonBoxGenerate, FALSE,
		      FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxGenerate),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxGenerate), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxGenerate), 5);

  buttonCancel = gpa_button_cancel_new(accelGroup, _("_Cancel"),
				       GTK_SIGNAL_FUNC (key_gen_cancel),
				       (gpointer)&dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxGenerate), buttonCancel);

  buttonGenerate = gpa_button_new (accelGroup, _("_Generate key"));

  gtk_signal_connect_object (GTK_OBJECT (buttonGenerate), "clicked",
			     GTK_SIGNAL_FUNC (key_gen_ok),
			     (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxGenerate), buttonGenerate);

  gpa_widget_show (windowGenerate, parent, _("keys_generateKey.tip"));

  gtk_grab_add (windowGenerate);
  gtk_main ();
  gtk_grab_remove (windowGenerate);

  if (dialog.result)
  {
      /* the user pressed OK, so create a GPAKeyGenParameters struct and
       * fill it with the values from the dialog
       */
      gchar * temp;

      params = key_gen_params_new ();
      params->userID = xstrdup (gtk_entry_get_text (GTK_ENTRY (entryUserID)));
      params->email = xstrdup(gtk_entry_get_text (GTK_ENTRY (entryEmail)));
      params->comment = xstrdup (gtk_entry_get_text (GTK_ENTRY(entryComment)));

      params->password = xstrdup (gtk_entry_get_text (GTK_ENTRY(entryPasswd)));
	  
      temp = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO(comboAlgorithm)->entry));
      params->algo = gpa_algorithm_from_string (temp);
      temp = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (comboKeysize)->entry));
      params->keysize = atoi (temp);

      params->generate_revocation \
	  = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerRevoc)));
      params->send_to_server \
	  = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerSend)));

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
  else /* if !dialog.result */
  {
      params = NULL;
  }

  gtk_widget_destroy (windowGenerate);

  return params;
} /* gpa_key_gen_run_dialog */
