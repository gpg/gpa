/* filemenu.c  -  The GNU Privacy Assistant
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gpapa.h>
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gpafilesel.h"
#include "gtktools.h"
#include "filemenu.h"
#include "keysmenu.h"

static GtkWidget *fileOpenSelect;

GList *filesOpened = NULL;
GList *filesSelected = NULL;

gchar *writtenFileStatus[] = {
  N_("unknown"),
  N_("clear"),
  N_("encrypted"),
  N_("protected"),
  N_("signed"),
  N_("clearsigned"),
  N_("detach-signed")
};


gchar *
getStringForFileStatus (GpapaFileStatus fileStatus)
{
  return (writtenFileStatus[fileStatus]);
}				/* getStringForFileStatus */

gchar *
getTargetFileID (GpapaFile * file, GpapaArmor armor, GtkWidget * window)
{
/* var */
  gchar appendix[5];
  gchar *targetFileID;
/* commands */
  switch (armor)
    {
    case GPAPA_ARMOR:
      strcpy (appendix, ".asc");
      break;
    case GPAPA_NO_ARMOR:
      strcpy (appendix, ".gpg");
      break;
    }				/* switch */
  targetFileID =
    xstrcat2 (gpapa_file_get_identifier (file, gpa_callback, window),
	      appendix);
  return (targetFileID);
}				/* getTargetFileID */

void
file_open_countSigs (gpointer data, gpointer userData)
{
/* var */
  GpapaSignature *signature;
  gint *countSigs;
/* commands */
  signature = (GpapaSignature *) data;
  countSigs = (gint *) userData;
  switch (gpapa_signature_get_validity
	  (signature, gpa_callback, global_windowMain))
    {
    case GPAPA_SIG_VALID:
      countSigs[0]++;
      break;
    case GPAPA_SIG_INVALID:
      countSigs[1]++;
      break;
    default:
      break;
    }				/* switch */
}				/* file_open_countSigs */

void
file_add (gchar * anIdentifier)
{
/* var */
  gchar *fileEntry[5];
  GList *indexFile;
  GList *signatures;
  gint countSigs[2];
  gchar buffer[256];
  GpapaFile *aFile;
  GtkWidget *fileList;
/* commands */
  aFile = gpapa_file_new (anIdentifier, gpa_callback, global_windowMain);
  signatures =
    gpapa_file_get_signatures (aFile, gpa_callback, global_windowMain);
  fileEntry[0] = gpapa_file_get_name (aFile, gpa_callback, global_windowMain);
  indexFile = g_list_first (filesOpened);
  while (indexFile)
    {
      if (!strcmp (fileEntry[0],
		   gpapa_file_get_identifier (
					      (GpapaFile *) indexFile->data,
					      gpa_callback,
					      global_windowMain)))
	{
	  gtk_widget_hide (fileOpenSelect);
	  gpa_window_error (_("The file is already open."),
			    global_windowMain);
	  return;
	}			/* if */
      indexFile = g_list_next (indexFile);
    }				/* while */
  filesOpened = g_list_append (filesOpened, aFile);
  fileEntry[1] =
    getStringForFileStatus (gpapa_file_get_status
			    (aFile, gpa_callback, global_windowMain));
  sprintf (buffer, "%d", g_list_length (signatures));
  fileEntry[2] = xstrdup (buffer);
  countSigs[0] = 0;
  countSigs[1] = 0;
  g_list_foreach (signatures, file_open_countSigs, countSigs);
  sprintf (buffer, "%d", countSigs[0]);
  fileEntry[3] = xstrdup (buffer);
  if (countSigs[1])
    {
      sprintf (buffer, "%d", countSigs[1]);
      fileEntry[4] = xstrdup (buffer);
    }				/* if */
  else
    fileEntry[4] = "";
  fileList = gpa_get_global_clist_file ();
  gtk_clist_append (GTK_CLIST (fileList), fileEntry);
  gtk_widget_grab_focus (fileList);
  gtk_widget_hide (fileOpenSelect);
}				/* file_add */


void
file_sign_sign_exec (gpointer param)
{
/* var */
  gpointer *localParam;
  gpointer *data;
  GpapaSignType *signType;
  GpapaArmor *armor;
  const gchar *keyID;
  GpaWindowKeeper *keeperSign;
  GtkWidget *entryPasswd;
  GpaWindowKeeper *keeperPassphrase;
  gint afterLastFile, countFiles, i;
  GList *indexFile;
  GpapaFile *file;
  gchar *identifierNew;
  gchar appendixNew[5];
  GtkWidget *clistFiles;
  gpointer paramDone[2];
/* commands */
  localParam = (gpointer *) param;
  data = (gpointer *) localParam[0];
  entryPasswd = localParam[1];
  keeperPassphrase = localParam[2];
  signType = (GpapaSignType *) data[0];
  armor = (GpapaArmor *) data[1];
  keyID = data[2];
  keeperSign = (GpaWindowKeeper *) data[3];
  if (!filesSelected)
    {
      gpa_window_error (_("No files selected for signing."),
			keeperSign->window);
      return;
    }				/* if */
  afterLastFile = g_list_length (filesOpened);
  indexFile = g_list_first (filesSelected);
  countFiles = g_list_length (filesSelected);
  i = 0;
  while (indexFile != NULL)
    {
      global_lastCallbackResult = GPAPA_ACTION_NONE;
      file = (GpapaFile *) indexFile->data;
      gpapa_file_sign (file, NULL, keyID,
		       gtk_entry_get_text (GTK_ENTRY (entryPasswd)),
		       *signType, *armor, gpa_callback, keeperSign->window);
      switch (*armor)
	{
	case GPAPA_NO_ARMOR:
	  if (*signType == GPAPA_SIGN_DETACH)
	    strcpy (appendixNew, ".sig");
	  else
	    strcpy (appendixNew, ".gpg");
	  break;
	case GPAPA_ARMOR:
	  strcpy (appendixNew, ".asc");
	  break;
	}			/* switch */
      if (global_lastCallbackResult == GPAPA_ACTION_ERROR)
	break;
      identifierNew =
	xstrcat2 (gpapa_file_get_identifier
		  (file, gpa_callback, keeperPassphrase->window),
		  appendixNew);
      file_add (identifierNew);
      free (identifierNew);
      indexFile = g_list_next (indexFile);
      i++;
    }				/* while */
  clistFiles = gpa_get_global_clist_file ();
  gtk_clist_unselect_all (GTK_CLIST (clistFiles));
  countFiles = g_list_length (filesOpened);
  while (afterLastFile < countFiles)
    {
      gtk_clist_select_row (GTK_CLIST (clistFiles), afterLastFile, 0);
      afterLastFile++;
    }				/* while */
  free (signType);
  free (armor);
  paramDone[0] = keeperPassphrase;
  paramDone[1] = NULL;
  gpa_window_destroy (paramDone);
  paramDone[0] = keeperSign;
  gpa_window_destroy (paramDone);
}				/* file_sign_sign_exec */

void
file_sign_sign (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *radioSignComp;
  GtkWidget *radioSign;
  GtkWidget *radioSignSep;
  GtkWidget *checkerArmor;
  GtkWidget *clistWho;
  int *as;
  GpaWindowKeeper *keeperSign;
  GtkSignalFunc funcSign;
  gpointer userData;
  GpapaSignType *signType;
  GpapaArmor *armor;
  gint errorCode;
  gchar *keyID;
  gpointer *newParam;
/* commands */
  localParam = (gpointer *) param;
  radioSignComp = localParam[0];
  radioSign = localParam[1];
  radioSignSep = localParam[2];
  checkerArmor = localParam[3];
  clistWho = localParam[4];
  as = (gint *) localParam[5];
  keeperSign = localParam[6];
  funcSign = localParam[7];
  userData = localParam[8];
  signType = (GpapaSignType *) xmalloc (sizeof (GpapaSignType));
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radioSignComp)))
    *signType = GPAPA_SIGN_NORMAL;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radioSign)))
    *signType = GPAPA_SIGN_CLEAR;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radioSignSep)))
    *signType = GPAPA_SIGN_DETACH;
  else
    {
      gpa_window_error (_("Internal error:\nInvalid sign mode"),
			keeperSign->window);
      return;
    }
  armor = (GpapaArmor *) xmalloc (sizeof (GpapaArmor));
  if (checkerArmor) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerArmor)))
      *armor = GPAPA_ARMOR;
    else
      *armor = GPAPA_NO_ARMOR;
  }
  errorCode = gtk_clist_get_text (GTK_CLIST (clistWho), *as, 1, &keyID);
  free (as);
  newParam = (gpointer *) xmalloc (5 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeperSign, newParam);
  newParam[0] = signType;
  newParam[1] = armor;
  newParam[2] = keyID;
  newParam[3] = keeperSign;
  newParam[4] = userData;
  gpa_window_passphrase (keeperSign->window, funcSign, "file_sign.tip",
			 newParam);
}

void
file_sign_as (GtkCList * clist, gint row, gint column, GdkEventButton * event,
	      gpointer as)
{
  *(gint *) as = row;
}

void
file_sign_dialog (GtkSignalFunc funcSign, GtkWidget * parent, gchar * tip,
		  gboolean withRadio, gboolean withCheckerArmor,
		  gpointer userData)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gchar *titlesWho[2] = { "User identity / role", "Key ID" };
  gint i;
  gint contentsCountWho;
  GpapaSecretKey *key;
  gchar *contentsWho[2];
  gint *as;
  gint rows;
  gchar *keyID;
  gpointer *paramSign;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowSign;
  GtkWidget *vboxSign;
  GtkWidget *frameMode;
  GtkWidget *vboxMode;
  GtkWidget *radioSignComp;
  GtkWidget *radioSign;
  GtkWidget *radioSignSep;
  GtkWidget *checkerArmor;
  GtkWidget *vboxWho;
  GtkWidget *labelJfdWho;
  GtkWidget *labelWho;
  GtkWidget *scrollerWho;
  GtkWidget *clistWho;
  GtkWidget *hButtonBoxSign;
  GtkWidget *buttonCancel;
  GtkWidget *buttonSign;
/* commands */
  contentsCountWho = gpapa_get_secret_key_count (gpa_callback, parent);
  if (contentsCountWho == 0)
    {
      gpa_window_error (_("No secret keys available for signing.\nPlease generate or import a secret key first."),
                        parent);
      return;
    }
  keeper = gpa_windowKeeper_new ();
  windowSign = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gpa_windowKeeper_set_window (keeper, windowSign);
  gtk_window_set_title (GTK_WINDOW (windowSign), _("Sign files"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowSign), accelGroup);
  vboxSign = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxSign), 5);
  frameMode = gtk_frame_new (_("Signing mode"));
  gtk_container_set_border_width (GTK_CONTAINER (frameMode), 5);
  vboxMode = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxMode), 5);
  radioSignComp = gpa_radio_button_new (accelGroup, _("si_gn and compress"));
  gtk_box_pack_start (GTK_BOX (vboxMode), radioSignComp, FALSE, FALSE, 0);
  radioSign =
    gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radioSignComp),
				      accelGroup, _("sign, do_n't compress"));
  gtk_box_pack_start (GTK_BOX (vboxMode), radioSign, FALSE, FALSE, 0);
  radioSignSep =
    gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radioSignComp),
				      accelGroup,
				      _("sign in separate _file"));
  gtk_box_pack_start (GTK_BOX (vboxMode), radioSignSep, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frameMode), vboxMode);
  if (withRadio)
    gtk_box_pack_start (GTK_BOX (vboxSign), frameMode, FALSE, FALSE, 0);
  if (withCheckerArmor)
    {
      checkerArmor = gpa_check_button_new (accelGroup, _("a_rmor"));
      gtk_container_set_border_width (GTK_CONTAINER (checkerArmor), 5);
      gtk_box_pack_start (GTK_BOX (vboxSign), checkerArmor, FALSE, FALSE, 0);
    }				/* if */
  else
    checkerArmor = NULL;
  vboxWho = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxWho), 5);
  labelWho = gtk_label_new ("");
  labelJfdWho = gpa_widget_hjustified_new (labelWho, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxWho), labelJfdWho, FALSE, FALSE, 0);
  scrollerWho = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerWho, 260, 75);
  clistWho = gtk_clist_new_with_titles (2, titlesWho);
  gtk_clist_set_column_width (GTK_CLIST (clistWho), 0, 185);
  gtk_clist_set_column_width (GTK_CLIST (clistWho), 1, 120);
  while (contentsCountWho)
    {
      contentsCountWho--;
      key =
	gpapa_get_secret_key_by_index (contentsCountWho, gpa_callback,
				       windowSign);
      contentsWho[0] =
	gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, windowSign);
      contentsWho[1] =
	gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback, windowSign);
      gtk_clist_prepend (GTK_CLIST (clistWho), contentsWho);
    }				/* while */
  gtk_clist_set_selection_mode (GTK_CLIST (clistWho), GTK_SELECTION_SINGLE);
  for (i = 0; i < 2; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistWho), i);
  as = (gint *) xmalloc (sizeof (gint));
  gpa_windowKeeper_add_param (keeper, as);
  *as = 0;
  rows = GTK_CLIST (clistWho)->rows;
  if (gpa_default_key ())
    {
      gtk_clist_get_text (GTK_CLIST (clistWho), *as, 1, &keyID);
      while (*as < rows && strcmp (gpa_default_key (), keyID) != 0)
	{
	  (*as)++;
	  if (*as < rows)
	    gtk_clist_get_text (GTK_CLIST (clistWho), *as, 1, &keyID);
	}			/* while */
      if (*as >= rows)
	*as = 0;
    }				/* if */
  gtk_signal_connect (GTK_OBJECT (clistWho), "select-row",
		      GTK_SIGNAL_FUNC (file_sign_as), (gpointer) as);
  gtk_clist_select_row (GTK_CLIST (clistWho), *as, 0);
  gpa_connect_by_accelerator (GTK_LABEL (labelWho), clistWho, accelGroup,
			      _("Sign _as "));
  gtk_container_add (GTK_CONTAINER (scrollerWho), clistWho);
  gtk_box_pack_start (GTK_BOX (vboxWho), scrollerWho, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxSign), vboxWho, TRUE, FALSE, 0);
  hButtonBoxSign = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxSign),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxSign), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxSign), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = tip;
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSign), buttonCancel);
  buttonSign = gpa_button_new (accelGroup, "_Sign");
  paramSign = (gpointer *) xmalloc (9 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSign);
  paramSign[0] = radioSignComp;
  paramSign[1] = radioSign;
  paramSign[2] = radioSignSep;
  paramSign[3] = checkerArmor;
  paramSign[4] = clistWho;
  paramSign[5] = as;
  paramSign[6] = keeper;
  paramSign[7] = funcSign;
  paramSign[8] = userData;
  gtk_signal_connect_object (GTK_OBJECT (buttonSign), "clicked",
			     GTK_SIGNAL_FUNC (file_sign_sign),
			     (gpointer) paramSign);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSign), buttonSign);
  gtk_box_pack_start (GTK_BOX (vboxSign), hButtonBoxSign, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowSign), vboxSign);
  gpa_window_show_centered (windowSign, parent);
}


void
file_browse_ok (GtkWidget * param[])
{
  gtk_entry_set_text (GTK_ENTRY (param[0]),
		      gpa_file_selection_get_filename (GPA_FILE_SELECTION
						       (param[1])));
  gtk_widget_destroy (param[1]);
}

void
file_browse (gpointer param)
{
/* var */
  gpointer *localParam;
  gchar *title;
  GtkWidget *entryFilename;
  GpaWindowKeeper *keeper;
  GtkWidget **paramOK;
  gpointer *paramClose;
/* objects */
  GtkWidget *browseSelect;
/* commands */
  localParam = (gpointer *) param;
  title = (gchar *) localParam[0];
  entryFilename = (GtkWidget *) localParam[1];
  keeper = gpa_windowKeeper_new ();
  browseSelect = gpa_file_selection_new (title);
  gpa_windowKeeper_set_window (keeper, browseSelect);
  paramOK = (GtkWidget **) xmalloc (2 * sizeof (GtkWidget *));
  gpa_windowKeeper_add_param (keeper, paramOK);
  paramOK[0] = entryFilename;
  paramOK[1] = browseSelect;
  gtk_signal_connect_object (GTK_OBJECT
			     (GPA_FILE_SELECTION (browseSelect)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (file_browse_ok),
			     (gpointer) paramOK);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  paramClose[0] = keeper;
  paramClose[1] = "";
  gtk_signal_connect_object (GTK_OBJECT
			     (GPA_FILE_SELECTION (browseSelect)->
			      cancel_button), "clicked",
			     GTK_SIGNAL_FUNC (gpa_window_destroy),
			     (gpointer) paramClose);
  gtk_widget_show (browseSelect);
}				/* file_browse */

void
file_encrypt_selectEncryptAs (GtkCList * clist, gint row, gint column,
			      GdkEventButton * event, gpointer userData)
{
/* var */
  gpointer *localParam;
  GpapaSecretKey **key;
  GtkWidget *windowEncrypt;
  gchar *keyID;
/* commands */
  localParam = (gpointer *) userData;
  key = (GpapaSecretKey **) localParam[0];
  windowEncrypt = (GtkWidget *) localParam[1];
  gtk_clist_get_text (clist, row, 1, &keyID);
  *key = gpapa_get_secret_key_by_ID (keyID, gpa_callback, windowEncrypt);
}				/* file_encrypt_selectEncryptAs */

void
file_encrypt_detail (gpointer param)
{
/* var */
  GpaWindowKeeper *keeper;
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *windowEncrypt;
  gchar *tip;
  GpapaPublicKey *key;
  GList *signatures;
  GtkAccelGroup *accelGroup;
  gchar *titlesSignatures[] = {
    N_("Signature"), N_("Validity"), N_("Key ID")
  };
  gint i;
  gpointer *paramSigs;
  gchar *contentsFingerprint;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowSigs;
  GtkWidget *vboxSigs;
  GtkWidget *tableKey;
  GtkWidget *vboxSignatures;
  GtkWidget *labelJfdSignatures;
  GtkWidget *labelSignatures;
  GtkWidget *scrollerSignatures;
  GtkWidget *clistSignatures;
  GtkWidget *vboxFingerprint;
  GtkWidget *labelJfdFingerprint;
  GtkWidget *labelFingerprint;
  GtkWidget *entryFingerprint;
  GtkWidget *hButtonBoxSigs;
  GtkWidget *buttonClose;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  windowEncrypt = (GtkWidget *) localParam[1];
  tip = (gchar *) localParam[2];
  if (!*keysSelected)
    {
      gpa_window_error (_("No key selected for detail view."), windowEncrypt);
      return;
    }				/* if */
  key = gpapa_get_public_key_by_ID (
				    (gchar *) g_list_last (*keysSelected)->
				    data, gpa_callback, windowEncrypt);
  keeper = gpa_windowKeeper_new ();
  windowSigs = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gpa_windowKeeper_set_window (keeper, windowSigs);
  gtk_window_set_title (GTK_WINDOW (windowSigs), _("Show public key detail"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowSigs), accelGroup);
  vboxSigs = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxSigs), 5);
  tableKey = gpa_tableKey_new (GPAPA_KEY (key), windowEncrypt);
  gtk_container_set_border_width (GTK_CONTAINER (tableKey), 5);
  gtk_box_pack_start (GTK_BOX (vboxSigs), tableKey, FALSE, FALSE, 0);
  vboxSignatures = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxSignatures), 5);
  labelSignatures = gtk_label_new ("");
  labelJfdSignatures =
    gpa_widget_hjustified_new (labelSignatures, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxSignatures), labelJfdSignatures, FALSE,
		      FALSE, 0);
  scrollerSignatures = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerSignatures, 350, 200);
  clistSignatures = gtk_clist_new_with_titles (3, titlesSignatures);
  gtk_clist_set_column_width (GTK_CLIST (clistSignatures), 0, 180);
  gtk_clist_set_column_justification (GTK_CLIST (clistSignatures), 0,
				      GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_width (GTK_CLIST (clistSignatures), 1, 80);
  gtk_clist_set_column_justification (GTK_CLIST (clistSignatures), 1,
				      GTK_JUSTIFY_CENTER);
  gtk_clist_set_column_width (GTK_CLIST (clistSignatures), 2, 120);
  gtk_clist_set_column_justification (GTK_CLIST (clistSignatures), 2,
				      GTK_JUSTIFY_LEFT);
  for (i = 0; i < 3; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistSignatures), i);
  gpa_connect_by_accelerator (GTK_LABEL (labelSignatures), clistSignatures,
			      accelGroup, _("_Signatures"));
  signatures =
    gpapa_public_key_get_signatures (key, gpa_callback, windowEncrypt);
  paramSigs = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSigs);
  paramSigs[0] = clistSignatures;
  paramSigs[1] = windowEncrypt;
  g_list_foreach (signatures, sigs_append, (gpointer) paramSigs);
  gtk_container_add (GTK_CONTAINER (scrollerSignatures), clistSignatures);
  gtk_box_pack_start (GTK_BOX (vboxSignatures), scrollerSignatures, TRUE,
		      TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxSigs), vboxSignatures, TRUE, TRUE, 0);
  vboxFingerprint = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxFingerprint), 5);
  labelFingerprint = gtk_label_new (_("Fingerprint: "));
  labelJfdFingerprint =
    gpa_widget_hjustified_new (labelFingerprint, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxFingerprint), labelJfdFingerprint, FALSE,
		      FALSE, 0);
  entryFingerprint = gtk_entry_new ();
  contentsFingerprint =
    gpapa_public_key_get_fingerprint (key, gpa_callback, windowEncrypt);
  gtk_entry_set_text (GTK_ENTRY (entryFingerprint), contentsFingerprint);
  gtk_editable_set_editable (GTK_EDITABLE (entryFingerprint), FALSE);
  gtk_widget_ensure_style (entryFingerprint);
  gtk_widget_set_usize (entryFingerprint,
    gdk_string_width (gtk_style_get_font (entryFingerprint->style),
                      contentsFingerprint)
    + gdk_string_width (gtk_style_get_font (entryFingerprint->style), "  ")
    + entryFingerprint->style->xthickness, 0);
  gtk_box_pack_start (GTK_BOX (vboxFingerprint), entryFingerprint, TRUE, TRUE,
		      0);
  gtk_box_pack_start (GTK_BOX (vboxSigs), vboxFingerprint, FALSE, FALSE, 0);
  hButtonBoxSigs = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxSigs),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxSigs), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxSigs), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = tip;
  buttonClose =
    gpa_buttonCancel_new (accelGroup, _("_Close"), (gpointer) paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSigs), buttonClose);
  gtk_box_pack_start (GTK_BOX (vboxSigs), hButtonBoxSigs, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowSigs), vboxSigs);
  gpa_window_show_centered (windowSigs, windowEncrypt);
}				/* file_encrypt_detail */

void
file_encrypt_encrypt_and_sign_exec (gpointer data, gpointer userData)
{
/* var */
  GpapaFile *file;
  gpointer *localParam;
  GList **recipients;
  gchar *keyID;
  GtkWidget *entryPasswd;
  GpapaSignType *signType;
  GpapaArmor *armor;
  GtkWidget *windowPassphrase;
  gchar *targetFileID;
/* commands */
  file = (GpapaFile *) data;
  localParam = (gpointer *) userData;
  recipients = (GList **) localParam[0];
  keyID = (gchar *) localParam[1];
  entryPasswd = (GtkWidget *) localParam[2];
  signType = (GpapaSignType *) localParam[3];
  armor = (GpapaArmor *) localParam[4];
  windowPassphrase = (GtkWidget *) localParam[5];
  targetFileID = getTargetFileID (file, *armor, windowPassphrase);
  global_lastCallbackResult = GPAPA_ACTION_NONE;
  gpapa_file_encrypt_and_sign (file, targetFileID, *recipients, keyID,
			       gtk_entry_get_text (GTK_ENTRY (entryPasswd)),
			       *signType, *armor, gpa_callback,
			       windowPassphrase);
  if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
    file_add (targetFileID);
  free (targetFileID);
}				/* file_encrypt_encrypt_and_sign_exec */

void
file_encrypt_encrypt_and_sign (gpointer param)
{
/* var */
  gpointer *localParam;
  gpointer *data;
  GtkWidget *entryPasswd;
  GtkWidget *windowPassphrase;
  GpapaSignType *signType;
  GpapaArmor *armor;
  gchar *keyID;
  GtkWidget *windowSign;
  gpointer *userData;
  GList **recipientsSelected;
  GList **keysSelected;
  GList **recipients;
  GtkWidget *windowEncrypt;
  GtkWidget *entrySaveAs;
  gint afterLastFile, countFiles;
  gchar *fileID;
  GtkWidget *clistFile;
  gpointer paramSign[6];
  gpointer paramClose[3];
/* commands */
  localParam = (gpointer *) param;
  data = (gpointer *) localParam[0];
  entryPasswd = localParam[1];
  windowPassphrase = localParam[2];
  signType = (GpapaSignType *) data[0];
  armor = (GpapaArmor *) data[1];
  keyID = (gchar *) data[2];
  windowSign = (GtkWidget *) data[3];
  userData = (gpointer *) data[4];
  recipientsSelected = (GList **) userData[0];
  keysSelected = (GList **) userData[1];
  recipients = (GList **) userData[2];
  armor = (GpapaArmor *) userData[3];
  windowEncrypt = (GtkWidget *) userData[4];
  entrySaveAs = (GtkWidget *) userData[5];
  afterLastFile = g_list_length (filesOpened);
  if (entrySaveAs)
    {
      fileID = (gchar *) gtk_entry_get_text (GTK_ENTRY (entrySaveAs));
      global_lastCallbackResult = GPAPA_ACTION_NONE;
      gpapa_file_encrypt_and_sign (
				   (GpapaFile *) g_list_last (filesSelected)->
				   data, fileID, *recipients, keyID,
				   gtk_entry_get_text (GTK_ENTRY
						       (entryPasswd)),
				   *signType, *armor, gpa_callback,
				   windowEncrypt);
      if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
	file_add (fileID);
    }
  else
    {
      paramSign[0] = recipients;
      paramSign[1] = keyID;
      paramSign[2] = entryPasswd;
      paramSign[3] = signType;
      paramSign[4] = armor;
      paramSign[5] = windowPassphrase;
      g_list_foreach (filesSelected, file_encrypt_encrypt_and_sign_exec,
		      paramSign);
    }
  clistFile = gpa_get_global_clist_file ();
  gtk_clist_unselect_all (GTK_CLIST (clistFile));
  countFiles = g_list_length (filesOpened);
  while (afterLastFile < countFiles)
    {
      gtk_clist_select_row (GTK_CLIST (clistFile), afterLastFile, 0);
      afterLastFile++;
    }				/* while */
  free (armor);
  free (signType);
  paramClose[0] = windowPassphrase;
  paramClose[1] = NULL;
  gpa_window_destroy (paramClose);
  paramClose[0] = windowSign;
  gpa_window_destroy (paramClose);
  paramClose[0] = recipientsSelected;
  paramClose[1] = keysSelected;
  paramClose[2] = windowEncrypt;
  gpa_recipientWindow_close (paramClose);
}				/* file_encrypt_encrypt_and_sign */

void
file_encrypt_encrypt_exec (gpointer data, gpointer userData)
{
/* var */
  GpapaFile *file;
  gpointer *localParam;
  GList **recipients;
  GpapaArmor *armor;
  GtkWidget *windowEncrypt;
  gchar *targetFileID;
/* commands */
  file = (GpapaFile *) data;
  localParam = (gpointer *) userData;
  recipients = (GList **) localParam[0];
  armor = (GpapaArmor *) localParam[1];
  windowEncrypt = (GtkWidget *) localParam[2];
  targetFileID = getTargetFileID (file, *armor, windowEncrypt);
  global_lastCallbackResult = GPAPA_ACTION_NONE;
  gpapa_file_encrypt (file, targetFileID, *recipients, *armor, gpa_callback,
		      windowEncrypt);
  if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
    file_add (targetFileID);
  free (targetFileID);
}				/* file_encrypt_encrypt_exec */

void
file_encrypt_encrypt (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **recipientsSelected;
  GList **keysSelected;
  GtkWidget *checkerSign;
  GtkWidget *clistRecipients;
  GtkWidget *checkerArmor;
  GpaWindowKeeper *keeperEncrypt;
  gchar *tip;
  GtkWidget *entrySaveAs;
  gint countRecipients, row;
  GList **recipients = NULL;
  gint recipientReceived;
  gchar *recipientID;
  GpapaArmor *armor;
  gint afterLastFile, countFiles;
  GtkWidget *clistFile;
  gchar *fileID;
  gpointer *paramSign;
  gpointer paramEncrypt[3];
  gpointer paramClose[3];
/* commands */
  localParam = (gpointer *) param;
  recipientsSelected = (GList **) localParam[0];
  keysSelected = (GList **) localParam[1];
  checkerSign = (GtkWidget *) localParam[2];
  clistRecipients = (GtkWidget *) localParam[3];
  checkerArmor = (GtkWidget *) localParam[4];
  keeperEncrypt = (GpaWindowKeeper *) localParam[5];
  tip = (gchar *) localParam[6];
  entrySaveAs = (GtkWidget *) localParam[7];
  countRecipients = GTK_CLIST (clistRecipients)->rows;
  if (!countRecipients)
    {
      gpa_window_error (_("No recipients chosen to encrypt for."),
			keeperEncrypt->window);
      return;
    }				/* if */
  recipients = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeperEncrypt, recipients);
  *recipients = NULL;
  for (row = 0; row < countRecipients; row++)
    {
      recipientReceived =
	gtk_clist_get_text (GTK_CLIST (clistRecipients), row, 1,
			    &recipientID);
      *recipients = g_list_append (*recipients, recipientID);
    }
  armor = (GpapaArmor *) xmalloc (sizeof (GpapaArmor));
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerArmor)))
    *armor = GPAPA_ARMOR;
  else
    *armor = GPAPA_NO_ARMOR;
  paramSign = (gpointer *) xmalloc (6 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeperEncrypt, paramSign);
  paramSign[0] = recipientsSelected;
  paramSign[1] = keysSelected;
  paramSign[2] = recipients;
  paramSign[3] = armor;
  paramSign[4] = keeperEncrypt;
  paramSign[5] = entrySaveAs;
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerSign)))
    file_sign_dialog ((GtkSignalFunc) file_encrypt_encrypt_and_sign,
                      keeperEncrypt->window,
		      tip, TRUE, FALSE, paramSign);
  else
    {
      clistFile = gpa_get_global_clist_file ();
      afterLastFile = g_list_length (filesOpened);
      if (entrySaveAs)
	{
	  fileID = (gchar *) gtk_entry_get_text (GTK_ENTRY (entrySaveAs));
	  global_lastCallbackResult = GPAPA_ACTION_NONE;
	  gpapa_file_encrypt (
			      (GpapaFile *) g_list_last (filesSelected)->data,
			      fileID, *recipients, *armor, gpa_callback,
			      keeperEncrypt->window);
	  if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
	    file_add (fileID);
	}
      else
	{
	  paramEncrypt[0] = recipients;
	  paramEncrypt[1] = armor;
	  paramEncrypt[2] = keeperEncrypt->window;
	  g_list_foreach (filesSelected, file_encrypt_encrypt_exec,
			  paramEncrypt);
	}
      gtk_clist_unselect_all (GTK_CLIST (clistFile));
      countFiles = g_list_length (filesOpened);
      while (afterLastFile < countFiles)
	{
	  gtk_clist_select_row (GTK_CLIST (clistFile), afterLastFile, 0);
	  afterLastFile++;
	}			/* while */
      paramClose[0] = recipientsSelected;
      paramClose[1] = keysSelected;
      paramClose[2] = keeperEncrypt;
      free (armor);
      gpa_recipientWindow_close (paramClose);
    }				/* else */
}				/* file_encrypt_encrypt */

gboolean
file_encrypt_dialog_evalMouse (GtkWidget * clistKeys, GdkEventButton * event,
			       gpointer param)
{
  if (event->type == GDK_2BUTTON_PRESS)
    file_encrypt_detail (param);
  return (TRUE);
}				/* file_encrypt_dialog_evalMouse */

void
file_encrypt_dialog_fillDefault (gpointer data, gpointer userData)
{
/* var */
  gchar *keyID;
  GtkWidget *clistDefault;
  GpapaPublicKey *key;
  gchar *contentsDefault[2];
/* commands */
  keyID = (gchar *) data;
  clistDefault = (GtkWidget *) userData;
  key = gpapa_get_public_key_by_ID (keyID, gpa_callback, global_windowMain);
  contentsDefault[0] =
    gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, global_windowMain);
  contentsDefault[1] = keyID;
  gtk_clist_append (GTK_CLIST (clistDefault), contentsDefault);
}				/* file_encrypt_dialog_fillDefault */

void
file_encrypt_dialog (gboolean withSaveAs, gchar * tip)
{
/* var */
  GpaWindowKeeper *keeper;
  gint publicKeyCount;
  gint secretKeyCount;
  gchar *titlesAnyClist[2] = { N_("Key owner"), N_("Key ID") };
  gint i;
  GtkAccelGroup *accelGroup;
  GList **recipientsSelected = NULL;
  gpointer *paramRemove;
  GpapaPublicKey *publicKey;
  gchar *contentsKeys[2];
  GList **keysSelected = NULL;
  static gint columnKeyID = 1;
  gpointer *paramKeys;
  gpointer *paramAdd;
  gpointer *paramDetail;
  gpointer *paramBrowse;
  gpointer *paramClose;
  gpointer *paramEncrypt;
/* objects */
  GtkWidget *windowEncrypt;
  GtkWidget *vboxEncrypt;
  GtkWidget *vboxRecKeys;
  GtkWidget *tableRecKeys;
  GtkWidget *vboxDefault;
  GtkWidget *checkerJfdDefault;
  GtkWidget *checkerDefault;
  GtkWidget *scrollerDefault;
  GtkWidget *clistDefault;
  GtkWidget *vboxRecipients;
  GtkWidget *labelJfdRecipients;
  GtkWidget *labelRecipients;
  GtkWidget *scrollerRecipients;
  GtkWidget *clistRecipients;
  GtkWidget *vboxKeys;
  GtkWidget *labelJfdKeys;
  GtkWidget *labelKeys;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;
  GtkWidget *hButtonBoxRecKeys;
  GtkWidget *buttonRemove;
  GtkWidget *buttonAdd;
  GtkWidget *buttonDetail;
  GtkWidget *vboxMisc;
  GtkWidget *hboxSaveAs;
  GtkWidget *labelSaveAs;
  GtkWidget *entrySaveAs;
  GtkWidget *spaceSaveAs;
  GtkWidget *buttonBrowse;
  GtkWidget *checkerSign;
  GtkWidget *checkerArmor;
  GtkWidget *hButtonBoxEncrypt;
  GtkWidget *buttonCancel;
  GtkWidget *buttonEncrypt;
/* commands */
  publicKeyCount =
    gpapa_get_public_key_count (gpa_callback, global_windowMain);
  if (!publicKeyCount)
    {
      gpa_window_error (_
			("No public keys available.\nCurrently, there is nobody who could read a\nfile encrypted by you."),
global_windowMain);
      return;
    }				/* if */
  secretKeyCount =
    gpapa_get_secret_key_count (gpa_callback, global_windowMain);
  if (!secretKeyCount)
    {
      gpa_window_error (_("No secret keys available."),
			global_windowMain);
      return;
    }				/* if */
  if (!filesSelected)
    {
      gpa_window_error (_("No files selected for encryption."),
			global_windowMain);
      return;
    }				/* if */
  keeper = gpa_windowKeeper_new ();
  windowEncrypt = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gpa_windowKeeper_set_window (keeper, windowEncrypt);
  gtk_window_set_title (GTK_WINDOW (windowEncrypt), _("Encrypt files"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowEncrypt), accelGroup);
  vboxEncrypt = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxEncrypt), 5);
  vboxRecKeys = gtk_vbox_new (FALSE, 0);
  tableRecKeys = gtk_table_new (2, 2, FALSE);
  vboxDefault = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxDefault), 5);
  checkerDefault =
    gpa_check_button_new (accelGroup, _("Add _default recipients:"));
  checkerJfdDefault =
    gpa_widget_hjustified_new (checkerDefault, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxDefault), checkerJfdDefault, FALSE, FALSE,
		      0);
  scrollerDefault = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerDefault, 300, 90);
  clistDefault = gtk_clist_new_with_titles (2, titlesAnyClist);
  gtk_clist_set_column_width (GTK_CLIST (clistDefault), 0, 230);
  gtk_clist_set_column_width (GTK_CLIST (clistDefault), 1, 120);
  for (i = 0; i < 2; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistDefault), i);
  gtk_signal_connect_object (GTK_OBJECT (checkerDefault), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_grab_focus),
			     (gpointer) clistDefault);
  g_list_foreach (global_defaultRecipients, file_encrypt_dialog_fillDefault,
		  clistDefault);
  gtk_container_add (GTK_CONTAINER (scrollerDefault), clistDefault);
  gtk_box_pack_start (GTK_BOX (vboxDefault), scrollerDefault, TRUE, TRUE, 0);
  gtk_table_attach (GTK_TABLE (tableRecKeys), vboxDefault, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  vboxRecipients = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxRecipients), 5);
  labelRecipients = gtk_label_new ("");
  labelJfdRecipients =
    gpa_widget_hjustified_new (labelRecipients, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxRecipients), labelJfdRecipients, FALSE,
		      FALSE, 0);
  scrollerRecipients = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerRecipients, 300, 120);
  clistRecipients = gtk_clist_new_with_titles (2, titlesAnyClist);
  gtk_clist_set_column_width (GTK_CLIST (clistRecipients), 0, 230);
  gtk_clist_set_column_width (GTK_CLIST (clistRecipients), 1, 120);
  for (i = 0; i < 2; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistRecipients), i);
  gtk_clist_set_selection_mode (GTK_CLIST (clistRecipients),
				GTK_SELECTION_EXTENDED);
  recipientsSelected = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, recipientsSelected);
  *recipientsSelected = NULL;
  gtk_signal_connect (GTK_OBJECT (clistRecipients), "select-row",
		      GTK_SIGNAL_FUNC (gpa_selectRecipient),
		      (gpointer) recipientsSelected);
  gtk_signal_connect (GTK_OBJECT (clistRecipients), "unselect-row",
		      GTK_SIGNAL_FUNC (gpa_unselectRecipient),
		      (gpointer) recipientsSelected);
  gpa_connect_by_accelerator (GTK_LABEL (labelRecipients), clistRecipients,
			      accelGroup, _("Rec_ipients"));
  gtk_container_add (GTK_CONTAINER (scrollerRecipients), clistRecipients);
  gtk_box_pack_start (GTK_BOX (vboxRecipients), scrollerRecipients, TRUE,
		      TRUE, 0);
  gtk_table_attach (GTK_TABLE (tableRecKeys), vboxRecipients, 0, 1, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  vboxKeys = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxKeys), 5);
  labelKeys = gtk_label_new ("");
  labelJfdKeys = gpa_widget_hjustified_new (labelKeys, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxKeys), labelJfdKeys, FALSE, FALSE, 0);
  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerKeys, 300, 120);
  clistKeys = gtk_clist_new_with_titles (2, titlesAnyClist);
  gtk_clist_set_selection_mode (GTK_CLIST (clistKeys),
				GTK_SELECTION_EXTENDED);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 0, 230);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 1, 120);
  for (i = 0; i < 2; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistKeys), i);
  keysSelected = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, keysSelected);
  *keysSelected = NULL;
  paramKeys = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramKeys);
  paramKeys[0] = keysSelected;
  paramKeys[1] = &columnKeyID;
  paramKeys[2] = windowEncrypt;
  gtk_signal_connect (GTK_OBJECT (clistKeys), "select-row",
		      GTK_SIGNAL_FUNC (keys_selectKey), (gpointer) paramKeys);
  gtk_signal_connect (GTK_OBJECT (clistKeys), "unselect-row",
		      GTK_SIGNAL_FUNC (keys_unselectKey),
		      (gpointer) paramKeys);
  paramDetail = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramDetail);
  paramDetail[0] = keysSelected;
  paramDetail[1] = windowEncrypt;
  paramDetail[2] = tip;
  gtk_signal_connect (GTK_OBJECT (clistKeys), "button-press-event",
		      GTK_SIGNAL_FUNC (file_encrypt_dialog_evalMouse),
		      (gpointer) paramDetail);
  gpa_connect_by_accelerator (GTK_LABEL (labelKeys), clistKeys, accelGroup,
			      _("_Public keys"));
  while (publicKeyCount)
    {
      publicKeyCount--;
      publicKey =
	gpapa_get_public_key_by_index (publicKeyCount, gpa_callback,
				       global_windowMain);
      contentsKeys[0] =
	gpapa_key_get_name (GPAPA_KEY (publicKey), gpa_callback,
			    global_windowMain);
      contentsKeys[1] =
	gpapa_key_get_identifier (GPAPA_KEY (publicKey), gpa_callback,
				  global_windowMain);
      gtk_clist_prepend (GTK_CLIST (clistKeys), contentsKeys);
    }				/* while */
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gtk_box_pack_start (GTK_BOX (vboxKeys), scrollerKeys, TRUE, TRUE, 0);
  gtk_table_attach (GTK_TABLE (tableRecKeys), vboxKeys, 1, 2, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (vboxRecKeys), tableRecKeys, TRUE, TRUE, 0);
  hButtonBoxRecKeys = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxRecKeys),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxRecKeys), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxRecKeys), 5);
  buttonRemove =
    gpa_button_new (accelGroup, _("Remo_ve keys from recipients"));
  paramRemove = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramRemove);
  paramRemove[0] = recipientsSelected;
  paramRemove[1] = clistRecipients;
  paramRemove[2] = windowEncrypt;
  gtk_signal_connect_object (GTK_OBJECT (buttonRemove), "clicked",
			     (GtkSignalFunc) gpa_removeRecipients, (gpointer) paramRemove);
  gtk_container_add (GTK_CONTAINER (hButtonBoxRecKeys), buttonRemove);
  buttonAdd = gpa_button_new (accelGroup, _("Add _keys to recipients"));
  paramAdd = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramAdd);
  paramAdd[0] = keysSelected;
  paramAdd[1] = clistKeys;
  paramAdd[2] = clistRecipients;
  paramAdd[3] = windowEncrypt;
  gtk_signal_connect_object (GTK_OBJECT (buttonAdd), "clicked",
			     (GtkSignalFunc) gpa_addRecipients, (gpointer) paramAdd);
  gtk_container_add (GTK_CONTAINER (hButtonBoxRecKeys), buttonAdd);
  buttonDetail = gpa_button_new (accelGroup, _("S_how detail"));
  gtk_signal_connect_object (GTK_OBJECT (buttonDetail), "clicked",
			     GTK_SIGNAL_FUNC (file_encrypt_detail),
			     (gpointer) paramDetail);
  gtk_container_add (GTK_CONTAINER (hButtonBoxRecKeys), buttonDetail);
  gtk_box_pack_start (GTK_BOX (vboxRecKeys), hButtonBoxRecKeys, TRUE, TRUE,
		      0);
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), vboxRecKeys, TRUE, TRUE, 0);
  vboxMisc = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxMisc), 5);
  if (withSaveAs)
    {
      hboxSaveAs = gtk_hbox_new (FALSE, 0);
      labelSaveAs = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (hboxSaveAs), labelSaveAs, FALSE, FALSE, 0);
      entrySaveAs = gtk_entry_new ();
      gpa_connect_by_accelerator (GTK_LABEL (labelSaveAs), entrySaveAs,
				  accelGroup, _("Save encrypted file _as: "));
      gtk_box_pack_start (GTK_BOX (hboxSaveAs), entrySaveAs, TRUE, TRUE, 0);
      spaceSaveAs = gpa_space_new ();
      gtk_box_pack_start (GTK_BOX (hboxSaveAs), spaceSaveAs, FALSE, FALSE, 5);
      buttonBrowse = gpa_button_new (accelGroup, _("   _Browse   "));
      paramBrowse = (gpointer *) xmalloc (2 * sizeof (gpointer));
      gpa_windowKeeper_add_param (keeper, paramBrowse);
      paramBrowse[0] = _("Save encrypted file as");
      paramBrowse[1] = entrySaveAs;
      gtk_signal_connect_object (GTK_OBJECT (buttonBrowse), "clicked",
				 GTK_SIGNAL_FUNC (file_browse),
				 (gpointer) paramBrowse);
      gtk_box_pack_start (GTK_BOX (hboxSaveAs), buttonBrowse, FALSE, FALSE,
			  0);
      gtk_box_pack_start (GTK_BOX (vboxMisc), hboxSaveAs, FALSE, FALSE, 0);
    }				/* if */
  else
    entrySaveAs = NULL;
  checkerSign = gpa_check_button_new (accelGroup, _("_sign"));
  gtk_box_pack_start (GTK_BOX (vboxMisc), checkerSign, FALSE, FALSE, 0);
  checkerArmor = gpa_check_button_new (accelGroup, _("a_rmor"));
  gtk_box_pack_start (GTK_BOX (vboxMisc), checkerArmor, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), vboxMisc, FALSE, FALSE, 0);
  hButtonBoxEncrypt = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxEncrypt),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxEncrypt), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxEncrypt), 5);
  buttonCancel = gpa_button_new (accelGroup, _("_Cancel"));
  paramClose = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = recipientsSelected;
  paramClose[1] = keysSelected;
  paramClose[2] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (buttonCancel), "clicked",
			     GTK_SIGNAL_FUNC (gpa_recipientWindow_close),
			     (gpointer) paramClose);
  gtk_widget_add_accelerator (buttonCancel, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEncrypt), buttonCancel);
  buttonEncrypt = gpa_button_new (accelGroup, _("_Encrypt"));
  paramEncrypt = (gpointer *) xmalloc (8 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramEncrypt);
  paramEncrypt[0] = recipientsSelected;
  paramEncrypt[1] = keysSelected;
  paramEncrypt[2] = checkerSign;
  paramEncrypt[3] = clistRecipients;
  paramEncrypt[4] = checkerArmor;
  paramEncrypt[5] = keeper;
  paramEncrypt[6] = tip;
  paramEncrypt[7] = entrySaveAs;
  gtk_signal_connect_object (GTK_OBJECT (buttonEncrypt), "clicked",
			     GTK_SIGNAL_FUNC (file_encrypt_encrypt),
			     (gpointer) paramEncrypt);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEncrypt), buttonEncrypt);
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), hButtonBoxEncrypt, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowEncrypt), vboxEncrypt);
  gpa_window_show_centered (windowEncrypt, global_windowMain);
}				/* file_encrypt_dialog */


void
file_encryptAs (void)
{
  file_encrypt_dialog (TRUE, "file_encryptAs.tip");
}				/* file_encryptAs */

void
file_protect_protect_exec (gpointer data, gpointer userData)
{
/* var */
  GpapaFile *file;
  gpointer *localParam;
  GtkWidget *entryPasswd;
  GpapaArmor armor;
  GtkWidget *windowProtect;
  gchar *targetFileID;
/* commands */
  file = (GpapaFile *) data;
  localParam = (gpointer *) userData;
  entryPasswd = (GtkWidget *) localParam[0];
  armor = *(GpapaArmor *) localParam[1];
  windowProtect = (GtkWidget *) localParam[2];
  targetFileID = getTargetFileID (file, armor, windowProtect);
  global_lastCallbackResult = GPAPA_ACTION_NONE;
  gpapa_file_protect (file, targetFileID,
		      gtk_entry_get_text (GTK_ENTRY (entryPasswd)), armor,
		      gpa_callback, windowProtect);
  if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
    file_add (targetFileID);
  free (targetFileID);
}				/* file_protect_protect_exec */

void
file_protect_protect (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *entrySaveAs;
  GtkWidget *entryPasswd;
  GtkWidget *entryRepeat;
  GtkWidget *checkerArmor;
  GpaWindowKeeper *keeperProtect;
  GpapaArmor *armor;
  gint afterLastFile, countFiles;
  GtkWidget *clistFile;
  gchar *fileID;
  gpointer paramProtect[3];
  gpointer paramDone[2];
/* commands */
  localParam = (gpointer *) param;
  entrySaveAs = (GtkWidget *) localParam[0];
  entryPasswd = (GtkWidget *) localParam[1];
  entryRepeat = (GtkWidget *) localParam[2];
  checkerArmor = (GtkWidget *) localParam[3];
  keeperProtect = (GpaWindowKeeper *) localParam[4];
  if (strcmp (gtk_entry_get_text (GTK_ENTRY (entryPasswd)),
	      gtk_entry_get_text (GTK_ENTRY (entryRepeat))) != 0)
    {
      gpa_window_error (_
			("In \"Passphrase\" and \"Repeat passphrase\",\nyou must enter the same passphrase."),
keeperProtect->window);
      return;
    }				/* if */
  armor = (GpapaArmor *) xmalloc (sizeof (GpapaArmor));
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerArmor)))
    *armor = GPAPA_ARMOR;
  else
    *armor = GPAPA_NO_ARMOR;
  afterLastFile = g_list_length (filesOpened);
  if (entrySaveAs)
    {
      fileID = (gchar *) gtk_entry_get_text (GTK_ENTRY (entrySaveAs));
      global_lastCallbackResult = GPAPA_ACTION_NONE;
      gpapa_file_protect (
			  (GpapaFile *) g_list_last (filesSelected)->data,
			  fileID,
			  gtk_entry_get_text (GTK_ENTRY (entryPasswd)),
			  *armor, gpa_callback, keeperProtect->window);
      if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
	file_add (fileID);
    }
  else
    {
      paramProtect[0] = entryPasswd;
      paramProtect[1] = armor;
      paramProtect[2] = keeperProtect->window;
      g_list_foreach (filesSelected, file_protect_protect_exec, paramProtect);
    }
  clistFile = gpa_get_global_clist_file ();
  gtk_clist_unselect_all (GTK_CLIST (clistFile));
  countFiles = g_list_length (filesOpened);
  while (afterLastFile < countFiles)
    {
      gtk_clist_select_row (GTK_CLIST (clistFile), afterLastFile, 0);
      afterLastFile++;
    }
  paramDone[0] = keeperProtect;
  paramDone[1] = NULL;
  gpa_window_destroy (paramDone);
}

void
file_protect_dialog (gboolean withSaveAs, gchar * tip)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gpointer *paramBrowse;
  gpointer *paramProtect;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowProtect;
  GtkWidget *vboxProtect;
  GtkWidget *tablePasswd;
  GtkWidget *labelJfdPasswd;
  GtkWidget *labelPasswd;
  GtkWidget *entryPasswd;
  GtkWidget *labelJfdRepeat;
  GtkWidget *labelRepeat;
  GtkWidget *entryRepeat;
  GtkWidget *labelJfdSaveAs;
  GtkWidget *labelSaveAs;
  GtkWidget *entrySaveAs;
  GtkWidget *spaceBrowse;
  GtkWidget *buttonBrowse;
  GtkWidget *checkerArmor;
  GtkWidget *hButtonBoxProtect;
  GtkWidget *buttonCancel;
  GtkWidget *buttonProtect;
/* commands */
  if (!filesSelected)
    {
      gpa_window_error (_("No files selected to protect."),
			global_windowMain);
      return;
    }				/* if */
  keeper = gpa_windowKeeper_new ();
  windowProtect = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gpa_windowKeeper_set_window (keeper, windowProtect);
  gtk_window_set_title (GTK_WINDOW (windowProtect),
			_("Protect files by Password"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowProtect), accelGroup);
  vboxProtect = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxProtect), 5);
  if (withSaveAs)
    tablePasswd = gtk_table_new (2, 4, FALSE);
  else
    tablePasswd = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (tablePasswd), 5);
  if (withSaveAs)
    {
      labelSaveAs = gtk_label_new ("");
      labelJfdSaveAs =
	gpa_widget_hjustified_new (labelSaveAs, GTK_JUSTIFY_RIGHT);
      gtk_table_attach (GTK_TABLE (tablePasswd), labelJfdSaveAs, 0, 1, 0, 1,
			GTK_FILL, GTK_SHRINK, 0, 10);
      entrySaveAs = gtk_entry_new ();
      gpa_connect_by_accelerator (GTK_LABEL (labelSaveAs), entrySaveAs,
				  accelGroup, _("Save protected _file as: "));
      gtk_table_attach (GTK_TABLE (tablePasswd), entrySaveAs, 1, 2, 0, 1,
			GTK_FILL, GTK_SHRINK, 0, 0);
      spaceBrowse = gpa_space_new ();
      gtk_table_attach (GTK_TABLE (tablePasswd), spaceBrowse, 2, 3, 0, 1,
			GTK_FILL, GTK_SHRINK, 5, 0);
      buttonBrowse = gpa_button_new (accelGroup, _("   _Browse   "));
      paramBrowse = (gpointer *) xmalloc (2 * sizeof (gpointer));
      gpa_windowKeeper_add_param (keeper, paramBrowse);
      paramBrowse[0] = _("Save protected file as");
      paramBrowse[1] = entrySaveAs;
      gtk_signal_connect_object (GTK_OBJECT (buttonBrowse), "clicked",
				 GTK_SIGNAL_FUNC (file_browse),
				 (gpointer) paramBrowse);
      gtk_table_attach (GTK_TABLE (tablePasswd), buttonBrowse, 3, 4, 0, 1,
			GTK_FILL, GTK_SHRINK, 0, 0);
    }				/* if */
  else
    entrySaveAs = NULL;
  labelPasswd = gtk_label_new ("");
  labelJfdPasswd = gpa_widget_hjustified_new (labelPasswd, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tablePasswd), labelJfdPasswd, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryPasswd = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entryPasswd), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelPasswd), entryPasswd,
			      accelGroup, _("P_assword: "));
  gtk_table_attach (GTK_TABLE (tablePasswd), entryPasswd, 1, 2, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  labelRepeat = gtk_label_new ("");
  labelJfdRepeat = gpa_widget_hjustified_new (labelRepeat, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tablePasswd), labelJfdRepeat, 0, 1, 2, 3,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryRepeat = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelRepeat), entryRepeat,
			      accelGroup, _("Repeat Pa_ssword: "));
  gtk_entry_set_visibility (GTK_ENTRY (entryRepeat), FALSE);
  if (entrySaveAs)
    gtk_signal_connect_object (GTK_OBJECT (entrySaveAs), "activate",
			       GTK_SIGNAL_FUNC (gtk_widget_grab_focus),
			       (gpointer) entryPasswd);
  gtk_table_attach (GTK_TABLE (tablePasswd), entryRepeat, 1, 2, 2, 3,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_signal_connect_object (GTK_OBJECT (entryPasswd), "activate",
			     GTK_SIGNAL_FUNC (gtk_widget_grab_focus),
			     (gpointer) entryRepeat);
  gtk_box_pack_start (GTK_BOX (vboxProtect), tablePasswd, TRUE, TRUE, 0);
  checkerArmor = gpa_check_button_new (accelGroup, _("a_rmor"));
  gtk_container_set_border_width (GTK_CONTAINER (checkerArmor), 5);
  gtk_box_pack_start (GTK_BOX (vboxProtect), checkerArmor, FALSE, FALSE, 0);
  paramProtect = (gpointer *) xmalloc (5 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramProtect);
  paramProtect[0] = entrySaveAs;
  paramProtect[1] = entryPasswd;
  paramProtect[2] = entryRepeat;
  paramProtect[3] = checkerArmor;
  paramProtect[4] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (entryRepeat), "activate",
			     GTK_SIGNAL_FUNC (file_protect_protect),
			     (gpointer) paramProtect);
  hButtonBoxProtect = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxProtect),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxProtect), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxProtect), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = NULL;
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxProtect), buttonCancel);
  buttonProtect = gpa_button_new (accelGroup, _("_Protect"));
  gtk_signal_connect_object (GTK_OBJECT (buttonProtect), "clicked",
			     GTK_SIGNAL_FUNC (file_protect_protect),
			     (gpointer) paramProtect);
  gtk_container_add (GTK_CONTAINER (hButtonBoxProtect), buttonProtect);
  gtk_box_pack_start (GTK_BOX (vboxProtect), hButtonBoxProtect, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowProtect), vboxProtect);
  gpa_window_show_centered (windowProtect, global_windowMain);
}				/* file_protect_dialog */

void
file_protect (void)
{
  file_protect_dialog (FALSE, "file_protect.tip");
}				/* file_protect */

void
file_protectAs (void)
{
  file_protect_dialog (TRUE, "file_protectAs.tip");
}				/* file_protectAs */




void
file_decryptAs_decrypt_exec (gpointer param)
{
/* var */
  gpointer *localParam;
  gpointer *data;
  gchar **targetFileID;
  GpaWindowKeeper *keeperDecrypt;
  GtkWidget *entryPasswd;
  GtkWidget *windowPassphrase;
  GpapaFile *file;
  gint afterLastFile, countFiles;
  GtkWidget *clistFile;
  gpointer paramClose[3];
/* commands */
  localParam = (gpointer *) param;
  data = (gpointer *) localParam[0];
  entryPasswd = (GtkWidget *) localParam[1];
  windowPassphrase = (GtkWidget *) localParam[2];
  targetFileID = (gchar **) data[0];
  keeperDecrypt = (GpaWindowKeeper *) data[1];
  file = (GpapaFile *) g_list_last (filesSelected)->data;
  global_lastCallbackResult = GPAPA_ACTION_NONE;
  gpapa_file_decrypt (file, *targetFileID,
		      gtk_entry_get_text (GTK_ENTRY (entryPasswd)),
		      gpa_callback, windowPassphrase);
  afterLastFile = g_list_length (filesOpened);
  if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
    file_add (*targetFileID);
  countFiles = g_list_length (filesOpened);
  clistFile = gpa_get_global_clist_file ();
  gtk_clist_unselect_all (GTK_CLIST (clistFile));
  while (afterLastFile < countFiles)
    {
      gtk_clist_select_row (GTK_CLIST (clistFile), afterLastFile, 0);
      afterLastFile++;
    }				/* while */
  paramClose[0] = windowPassphrase;
  paramClose[1] = NULL;
  gpa_window_destroy (paramClose);
  paramClose[0] = keeperDecrypt;
  gpa_window_destroy (paramClose);
}				/* file_decryptAs_decrypt_exec */

void
file_decryptAs_decrypt (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *entrySaveAs;
  GpaWindowKeeper *keeperDecrypt;
  gchar **targetFileID;
  gpointer *paramDecrypt;
/* commands */
  localParam = (gpointer *) param;
  entrySaveAs = (GtkWidget *) localParam[0];
  keeperDecrypt = (GpaWindowKeeper *) localParam[1];
  targetFileID = (gchar **) xmalloc (sizeof (gchar *));
  *targetFileID = (gchar *) gtk_entry_get_text (GTK_ENTRY (entrySaveAs));
  gpa_windowKeeper_add_param (keeperDecrypt, targetFileID);
  paramDecrypt = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeperDecrypt, paramDecrypt);
  paramDecrypt[0] = targetFileID;
  paramDecrypt[1] = keeperDecrypt;
  gpa_window_passphrase (keeperDecrypt->window,
                         (GtkSignalFunc) file_decryptAs_decrypt_exec,
			 NULL, (gpointer) paramDecrypt);
}				/* file_decryptAs_decrypt */

void
file_decryptAs (void)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gpointer *paramDecrypt;
  gpointer *paramBrowse;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowDecrypt;
  GtkWidget *vboxDecrypt;
  GtkWidget *hboxTop;
  GtkWidget *labelFilename;
  GtkWidget *entryFilename;
  GtkWidget *spaceFilename;
  GtkWidget *buttonFilename;
  GtkWidget *hButtonBoxDecrypt;
  GtkWidget *buttonCancel;
  GtkWidget *buttonDecrypt;
/* commands */
  if (!filesSelected)
    {
      gpa_window_error (_("No files selected to decrypt."),
			global_windowMain);
      return;
    }				/* if */
  keeper = gpa_windowKeeper_new ();
  windowDecrypt = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gpa_windowKeeper_set_window (keeper, windowDecrypt);
  gtk_window_set_title (GTK_WINDOW (windowDecrypt), "Decrypt file");
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowDecrypt), accelGroup);
  vboxDecrypt = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxDecrypt), 5);
  hboxTop = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxTop), 5);
  labelFilename = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hboxTop), labelFilename, FALSE, FALSE, 0);
  entryFilename = gtk_entry_new ();
  paramDecrypt = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramDecrypt);
  paramDecrypt[0] = entryFilename;
  paramDecrypt[1] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (entryFilename), "activate",
			     GTK_SIGNAL_FUNC (file_decryptAs_decrypt),
			     (gpointer) paramDecrypt);
  gpa_connect_by_accelerator (GTK_LABEL (labelFilename), entryFilename,
			      accelGroup, _("Save file _as: "));
  gtk_box_pack_start (GTK_BOX (hboxTop), entryFilename, TRUE, TRUE, 0);
  spaceFilename = gpa_space_new ();
  gtk_box_pack_start (GTK_BOX (hboxTop), spaceFilename, FALSE, FALSE, 5);
  buttonFilename = gpa_button_new (accelGroup, _("   _Browse   "));
  paramBrowse = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramBrowse);
  paramBrowse[0] = _("Save decrypted file as");
  paramBrowse[1] = entryFilename;
  gtk_signal_connect_object (GTK_OBJECT (buttonFilename), "clicked",
			     GTK_SIGNAL_FUNC (file_browse),
			     (gpointer) paramBrowse);
  gtk_box_pack_start (GTK_BOX (hboxTop), buttonFilename, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxDecrypt), hboxTop, TRUE, FALSE, 0);
  hButtonBoxDecrypt = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxDecrypt),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxDecrypt), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxDecrypt), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = NULL;
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxDecrypt), buttonCancel);
  buttonDecrypt = gpa_button_new (accelGroup, _("_Decrypt"));
  gtk_signal_connect_object (GTK_OBJECT (buttonDecrypt), "clicked",
			     GTK_SIGNAL_FUNC (file_decryptAs_decrypt),
			     (gpointer) paramDecrypt);
  gtk_container_add (GTK_CONTAINER (hButtonBoxDecrypt), buttonDecrypt);
  gtk_box_pack_start (GTK_BOX (vboxDecrypt), hButtonBoxDecrypt, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowDecrypt), vboxDecrypt);
  gpa_window_show_centered (windowDecrypt, global_windowMain);
}				/* file_decryptAs */


void
file_quit (void)
{
  gtk_main_quit ();
}				/* file_quit */
