/* keysmenu.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000 Free Software Foundation, Inc.
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
#include <gpapa.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include "filemenu.h"
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gtktools.h"
#include "icons.xpm"

gchar *writtenKeytrust[4] = {
  N_("unknown"),
  N_("don't trust"),
  N_("trust marginally"),
  N_("trust fully")
};

gchar *writtenOwnertrust[4] = {
  N_("unknown"),
  N_("don't trust"),
  N_("trust marginally"),
  N_("trust fully")
};

gchar **iconOwnertrust[4] = {
  gpa_trust_unknown_xpm,
  gpa_dont_trust_xpm,
  gpa_trust_marginally_xpm,
  gpa_trust_fully_xpm,
};

gchar *unitExpiryTime[4] = {
  N_("days"),
  N_("weeks"),
  N_("months"),
  N_("years")
};

gchar unitTime[4] = { 'd', 'w', 'm', 'y' };

gchar *writtenAlgorithm[4] = {
  N_("DSA and ElGamal"),
  N_("DSA (sign only)"),
  N_("ElGamal (sign and encrypt"),
  N_("ElGamal (encrypt only)")
};

gchar *
getStringForKeytrust (GpapaKeytrust keytrust)
{
  return (writtenKeytrust[keytrust]);
}				/* getStringForKeytrust */

gchar *
getStringForOwnertrust (GpapaOwnertrust ownertrust)
{
  return (writtenOwnertrust[ownertrust]);
}				/* getStringForOwnertrust */

gchar **
getIconForOwnertrust (GpapaOwnertrust ownertrust)
{
  return iconOwnertrust[ownertrust];
}				/* getIconForOwnertrust */

GpapaOwnertrust
getOwnertrustForString (gchar * aString)
{
/* var */
  GpapaOwnertrust result;
/* commands */
  result = GPAPA_OWNERTRUST_FIRST;
  while (result <= GPAPA_OWNERTRUST_LAST &&
	 strcmp (aString, writtenOwnertrust[result]) != 0)
    result++;
  return (result);
}				/* getOwnertrustForString */

gchar
getTimeunitForString (gchar * aString)
{
/* var */
  gchar result = ' ';
  gint i;
/* commands */
  i = 0;
  while (i < 4 && strcmp (aString, unitExpiryTime[i]) != 0)
    i++;
  if (i < 4)
    result = unitTime[i];
  return (result);
}				/* getTimeunitForString */

GpapaAlgo
getAlgorithmForString (gchar * aString)
{
/* var */
  GpapaAlgo result;
/* commands */
  result = GPAPA_ALGO_FIRST;
  while (result <= GPAPA_ALGO_LAST &&
	 strcmp (aString, writtenAlgorithm[result]) != 0)
    result++;
  return (result);
}				/* getAlgorithmForString */

gchar *
getStringForExpiryDate (GDate * expiryDate)
{
/* var */
  gchar dateBuffer[256];
  gchar *result;
/* commands */
  if (expiryDate != NULL)
    {
      g_date_strftime (dateBuffer, 256, "%d.%m.%Y", expiryDate);
      result = xstrdup (dateBuffer);
    }				/* if */
  else
    result = xstrdup (_("never expires"));
  return (result);
}				/* getStringForExpiryDate */

void
keys_selectKey (GtkWidget * clistKeys, gint row, gint column,
		GdkEventButton * event, gpointer param)
{
/* var */
  gpointer *localParam;
  GList **keysSelected;
  gint columnKeyID;
  GtkWidget *windowPublic;
  gint errorCode;
  gchar *keyID;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  columnKeyID = *(gint *) localParam[1];
  windowPublic = (GtkWidget *) localParam[2];
  errorCode =
    gtk_clist_get_text (GTK_CLIST (clistKeys), row, columnKeyID, &keyID);
  if (!g_list_find (*keysSelected, keyID))
    *keysSelected = g_list_append (*keysSelected, keyID);
}				/* keys_selectKey */

void
keys_unselectKey (GtkWidget * clistKeys, gint row, gint column,
		  GdkEventButton * event, gpointer param)
{
/* var */
  gpointer *localParam;
  GList **keysSelected;
  gint columnKeyID;
  GtkWidget *windowPublic;
  gint errorCode;
  gchar *keyID;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  columnKeyID = *(gint *) localParam[1];
  windowPublic = (GtkWidget *) localParam[2];
  errorCode =
    gtk_clist_get_text (GTK_CLIST (clistKeys), row, columnKeyID, &keyID);
  if (g_list_find (*keysSelected, keyID))
    *keysSelected = g_list_remove (*keysSelected, keyID);
}				/* keys_unselectKey */

void
keys_ringEditor_close (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **keysSelected;
  GpaWindowKeeper *keeper;
  gpointer paramDone[2];
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  keeper = (GpaWindowKeeper *) localParam[1];
  g_list_free (*keysSelected);
  paramDone[0] = keeper;
  paramDone[1] = NULL;
  gpa_window_destroy (paramDone);
}				/* keys_ringEditor_close */

GtkWidget *
gpa_tableKey_new (GpapaKey * key, GtkWidget * window)
{
/* var */
  gchar *contentsKeyname;
/* objects */
  GtkWidget *tableKey;
  GtkWidget *labelJfdKeyID;
  GtkWidget *labelKeyID;
  GtkWidget *entryKeyID;
  GtkWidget *labelJfdKeyname;
  GtkWidget *labelKeyname;
  GtkWidget *entryKeyname;
/* commands */
  tableKey = gtk_table_new (2, 2, FALSE);
  labelKeyID = gtk_label_new (_("Key: "));
  labelJfdKeyID = gpa_widget_hjustified_new (labelKeyID, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableKey), labelJfdKeyID, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryKeyID = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entryKeyID),
		      gpapa_key_get_identifier (key, gpa_callback, window));
  gtk_editable_set_editable (GTK_EDITABLE (entryKeyID), FALSE);
  gtk_table_attach (GTK_TABLE (tableKey), entryKeyID, 1, 2, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  labelKeyname = gtk_label_new ("Key owner: ");
  labelJfdKeyname =
    gpa_widget_hjustified_new (labelKeyname, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableKey), labelJfdKeyname, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryKeyname = gtk_entry_new ();
  contentsKeyname = gpapa_key_get_name (key, gpa_callback, window);
  gtk_widget_set_usize (entryKeyname,
			gdk_string_width (entryKeyname->style->font,
					  contentsKeyname) +
			gdk_string_width (entryKeyname->style->font,
					  "  ") +
			entryKeyname->style->klass->xthickness, 0);
  gtk_entry_set_text (GTK_ENTRY (entryKeyname), contentsKeyname);
  gtk_editable_set_editable (GTK_EDITABLE (entryKeyname), FALSE);
  gtk_table_attach (GTK_TABLE (tableKey), entryKeyname, 1, 2, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  return (tableKey);
}				/* gpa_tableKey_new */

void
gpa_frameExpire_dont (GtkToggleButton * radioDont, gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *entryAt;
/* commands */
  if (!gtk_toggle_button_get_active (radioDont))
    return;
  localParam = (gpointer *) param;
  entryAfter = (GtkWidget *) localParam[1];
  comboAfter = (GtkWidget *) localParam[2];
  entryAt = (GtkWidget *) localParam[3];
  gtk_entry_set_text (GTK_ENTRY (entryAfter), _(""));
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboAfter)->entry),
		      unitExpiryTime[0]);
  gtk_entry_set_text (GTK_ENTRY (entryAt), _(""));
}				/* gpa_frameExpire_dont */

void
gpa_frameExpire_after (GtkToggleButton * radioAfter, gpointer param)
{
/* var */
  gpointer *localParam;
  GDate **expiryDate;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *entryAt;
  GtkWidget *window;
/* commands */
  if (!gtk_toggle_button_get_active (radioAfter))
    return;
  localParam = (gpointer *) param;
  expiryDate = (GDate **) localParam[0];
  entryAfter = (GtkWidget *) localParam[1];
  comboAfter = (GtkWidget *) localParam[2];
  entryAt = (GtkWidget *) localParam[3];
  window = (GtkWidget *) localParam[4];
  if (*expiryDate)
    {
      gtk_entry_set_text (GTK_ENTRY (entryAfter), _("1"));	/*!!! */
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboAfter)->entry),
			  unitExpiryTime[0]);
    }				/* if */
  else
    gtk_entry_set_text (GTK_ENTRY (entryAfter), _("1"));
  gtk_entry_set_text (GTK_ENTRY (entryAt), _(""));
  gtk_widget_grab_focus (entryAfter);
}				/* gpa_frameExpire_after */

void
gpa_frameExpire_at (GtkToggleButton * radioAt, gpointer param)
{
/* var */
  gpointer *localParam;
  GDate **expiryDate;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *entryAt;
  GtkWidget *window;
  gchar dateBuffer[256];
/* commands */
  if (!gtk_toggle_button_get_active (radioAt))
    return;
  localParam = (gpointer *) param;
  expiryDate = (GDate **) localParam[0];
  entryAfter = (GtkWidget *) localParam[1];
  comboAfter = (GtkWidget *) localParam[2];
  entryAt = (GtkWidget *) localParam[3];
  window = (GtkWidget *) localParam[4];
  gtk_entry_set_text (GTK_ENTRY (entryAfter), _(""));
  if (*expiryDate)
    {
      g_date_strftime (dateBuffer, 256, "%d.%m.%Y", *expiryDate);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboAfter)->entry),
			  unitExpiryTime[0]);
      gtk_entry_set_text (GTK_ENTRY (entryAt), dateBuffer);
    }				/* if */
  else
    gtk_entry_set_text (GTK_ENTRY (entryAt), _("01.01.2000"));	/*!!! */
  gtk_widget_grab_focus (entryAt);
}				/* gpa_frameExpire_at */

GtkWidget *
gpa_frameExpire_new (GtkAccelGroup * accelGroup, GDate ** expiryDate,
		     GpaWindowKeeper * keeper, gpointer * paramSave)
{
/* var */
  GList *contentsAfter = NULL;
  gpointer *param;
  gint i;
  gchar dateBuffer[256];
/* objects */
  GtkWidget *frameExpire;
  GtkWidget *vboxExpire;
  GtkWidget *radioDont;
  GtkWidget *hboxAfter;
  GtkWidget *radioAfter;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *hboxAt;
  GtkWidget *radioAt;
  GtkWidget *entryAt;
/* commands */
  frameExpire = gtk_frame_new (_("Expiration"));
  vboxExpire = gtk_vbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxExpire), 5);
  radioDont = gpa_radio_button_new (accelGroup, _("_indefinitely valid"));
  gtk_box_pack_start (GTK_BOX (vboxExpire), radioDont, FALSE, FALSE, 0);
  hboxAfter = gtk_hbox_new (FALSE, 0);
  radioAfter =
    gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radioDont),
				      accelGroup, _("expire _after"));
  gtk_box_pack_start (GTK_BOX (hboxAfter), radioAfter, FALSE, FALSE, 0);
  entryAfter = gtk_entry_new ();
  gtk_widget_set_usize (entryAfter,
			gdk_string_width (entryAfter->style->font, " 00000 "),
			0);
  gtk_box_pack_start (GTK_BOX (hboxAfter), entryAfter, FALSE, FALSE, 0);
  comboAfter = gtk_combo_new ();
  gtk_combo_set_value_in_list (GTK_COMBO (comboAfter), TRUE, FALSE);
  for (i = 0; i < 4; i++)
    contentsAfter = g_list_append (contentsAfter, unitExpiryTime[i]);
  gtk_combo_set_popdown_strings (GTK_COMBO (comboAfter), contentsAfter);
  gtk_box_pack_start (GTK_BOX (hboxAfter), comboAfter, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxExpire), hboxAfter, FALSE, FALSE, 0);
  hboxAt = gtk_hbox_new (FALSE, 0);
  radioAt =
    gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radioDont),
				      accelGroup, _("expire a_t"));
  gtk_box_pack_start (GTK_BOX (hboxAt), radioAt, FALSE, FALSE, 0);
  entryAt = gtk_entry_new ();
  if (*expiryDate)
    {
      g_date_strftime (dateBuffer, 256, "%d.%m.%Y", *expiryDate);
      gtk_entry_set_text (GTK_ENTRY (entryAt), dateBuffer);
    }				/* if */
  gtk_box_pack_start (GTK_BOX (hboxAt), entryAt, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxExpire), hboxAt, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frameExpire), vboxExpire);
  if (*expiryDate)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radioAt), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radioDont), TRUE);
  param = (gpointer *) xmalloc (5 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, param);
  param[0] = expiryDate;
  param[1] = entryAfter;
  param[2] = comboAfter;
  param[3] = entryAt;
  param[4] = keeper->window;
  gtk_signal_connect (GTK_OBJECT (radioDont), "toggled",
		      GTK_SIGNAL_FUNC (gpa_frameExpire_dont),
		      (gpointer) param);
  gtk_signal_connect (GTK_OBJECT (radioAfter), "toggled",
		      GTK_SIGNAL_FUNC (gpa_frameExpire_after),
		      (gpointer) param);
  gtk_signal_connect (GTK_OBJECT (radioAt), "toggled",
		      GTK_SIGNAL_FUNC (gpa_frameExpire_at), (gpointer) param);
  paramSave[0] = radioDont;
  paramSave[1] = radioAfter;
  paramSave[2] = radioAt;
  paramSave[3] = entryAfter;
  paramSave[4] = comboAfter;
  paramSave[5] = entryAt;
  return (frameExpire);
}				/* gpa_frameExpire_new */

void
keys_openPublic_export_export_exec (gpointer data, gpointer userData)
{
/* var */
  gchar *keyID;
  gpointer *localParam;
  gchar *fileID;
  GpapaArmor *armor;
  GtkWidget *windowExport;
  GpapaPublicKey *key;
/* commands */
  keyID = (gchar *) data;
  localParam = (gpointer *) userData;
  fileID = (gchar *) localParam[0];
  armor = (GpapaArmor *) localParam[1];
  windowExport = (GtkWidget *) localParam[2];
  key = gpapa_get_public_key_by_ID (keyID, gpa_callback, windowExport);
  gpapa_public_key_export (key, fileID, *armor, gpa_callback, windowExport);
}				/* keys_openPublic_export_export_exec */

void
keys_openPublic_export_export (gpointer param)
{
/* var */
  gpointer *localParam;
  GpaWindowKeeper *keeperExport;
  gchar *tip;
  GList **keysSelected;
  GtkWidget *entryFilename;
  GtkWidget *checkerArmor;
  gchar *fileID;
  GpapaArmor armor;
  gpointer *paramExec;
  gpointer paramDone[2];
/* commands */
  localParam = (gpointer *) param;
  keeperExport = (GpaWindowKeeper *) localParam[0];
  tip = (gchar *) localParam[1];
  keysSelected = (GList **) localParam[2];
  entryFilename = (GtkWidget *) localParam[3];
  checkerArmor = (GtkWidget *) localParam[4];
  fileID = gtk_entry_get_text (GTK_ENTRY (entryFilename));
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerArmor)))
    armor = GPAPA_ARMOR;
  else
    armor = GPAPA_NO_ARMOR;
  paramExec = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeperExport, paramExec);
  paramExec[0] = fileID;
  paramExec[1] = &armor;
  paramExec[2] = keeperExport->window;
  g_list_foreach (*keysSelected, keys_openPublic_export_export_exec,
		  (gpointer) paramExec);
  paramDone[0] = keeperExport;
  paramDone[1] = tip;
  gpa_window_message (_("Keys exported."), keeperExport->window);
  gpa_window_destroy (paramDone);
}				/* keys_openPublic_export_export */

void
keys_export_dialog (gpointer param)
{
/* var */
  GpaWindowKeeper *keeper;
  gpointer *localParam;
  GList **keysSelected;
  gchar *tip;
  GtkSignalFunc exportFunc;
  GtkWidget *parent;
  GtkAccelGroup *accelGroup;
  gpointer *paramBrowse;
  gpointer *paramClose;
  gpointer *paramExport;
/* objects */
  GtkWidget *windowExport;
  GtkWidget *vboxExport;
  GtkWidget *hboxFilename;
  GtkWidget *labelFilename;
  GtkWidget *entryFilename;
  GtkWidget *spaceBrowse;
  GtkWidget *buttonBrowse;
  GtkWidget *checkerArmor;
  GtkWidget *hButtonBoxExport;
  GtkWidget *buttonCancel;
  GtkWidget *buttonExport;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  tip = (gchar *) localParam[1];
  exportFunc = (GtkSignalFunc) localParam[2];
  parent = (GtkWidget *) localParam[3];
  if (!*keysSelected)
    {
      gpa_window_error (_("No keys selected to export."), parent);
      return;
    }				/* if */
  keeper = gpa_windowKeeper_new ();
  windowExport = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowExport);
  gtk_window_set_title (GTK_WINDOW (windowExport), _("Export keys"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowExport), accelGroup);
  vboxExport = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxExport), 5);
  hboxFilename = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxFilename), 5);
  labelFilename = gtk_label_new (_(""));
  gtk_box_pack_start (GTK_BOX (hboxFilename), labelFilename, FALSE, FALSE, 0);
  entryFilename = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hboxFilename), entryFilename, TRUE, TRUE, 0);
  gpa_connect_by_accelerator (GTK_LABEL (labelFilename), entryFilename,
			      accelGroup, _("Export to _file: "));
  spaceBrowse = gpa_space_new ();
  gtk_box_pack_start (GTK_BOX (hboxFilename), spaceBrowse, FALSE, FALSE, 5);
  buttonBrowse = gpa_button_new (accelGroup, _("   _Browse   "));
  paramBrowse = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramBrowse);
  paramBrowse[0] = _("Export public keys to file");
  paramBrowse[1] = entryFilename;
  gtk_signal_connect_object (GTK_OBJECT (buttonBrowse), "clicked",
			     GTK_SIGNAL_FUNC (file_browse),
			     (gpointer) paramBrowse);
  gtk_box_pack_start (GTK_BOX (hboxFilename), buttonBrowse, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxExport), hboxFilename, TRUE, TRUE, 0);
  checkerArmor = gpa_check_button_new (accelGroup, _("a_rmor"));
  gtk_container_set_border_width (GTK_CONTAINER (checkerArmor), 5);
  gtk_box_pack_start (GTK_BOX (vboxExport), checkerArmor, FALSE, FALSE, 0);
  hButtonBoxExport = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxExport),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxExport), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxExport), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = tip;
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxExport), buttonCancel);
  buttonExport = gpa_button_new (accelGroup, _("E_xport"));
  paramExport = (gpointer *) xmalloc (5 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramExport);
  paramExport[0] = keeper;
  paramExport[1] = tip;
  paramExport[2] = keysSelected;
  paramExport[3] = entryFilename;
  paramExport[4] = checkerArmor;
  gtk_signal_connect_object (GTK_OBJECT (buttonExport), "clicked",
			     GTK_SIGNAL_FUNC (exportFunc),
			     (gpointer) paramExport);
  gtk_signal_connect_object (GTK_OBJECT (entryFilename), "activate",
			     GTK_SIGNAL_FUNC (exportFunc),
			     (gpointer) paramExport);
  gtk_container_add (GTK_CONTAINER (hButtonBoxExport), buttonExport);
  gtk_box_pack_start (GTK_BOX (vboxExport), hButtonBoxExport, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowExport), vboxExport);
  gpa_widget_show (windowExport, parent, _("keys_openPublic_export.tip"));
  gtk_widget_grab_focus (entryFilename);
}				/* keys_export_dialog */

void
keys_openPublic_delete (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *clistKeys;
  gint *columnKeyID;
  GList **rowsSelected;
  GtkWidget *windowPublic;
  GList *indexRow, *previousRow;
  gint row;
  gint foundKeyID;
  gchar *keyID;
  GpapaPublicKey *key;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  clistKeys = (GtkWidget *) localParam[1];
  columnKeyID = (gint *) localParam[2];
  rowsSelected = (GList **) localParam[3];
  windowPublic = (GtkWidget *) localParam[4];
  if (!*keysSelected)
    {
      gpa_window_error (_("No keys selected to delete."), windowPublic);
      return;
    }				/* if */
  if (!*rowsSelected)
    {
      gpa_window_error (_("!FATAL ERROR: Invalid key selection info!\n"),
			windowPublic);
      return;
    }				/* if */
  indexRow = g_list_last (*rowsSelected);
  while (indexRow)
    {
      previousRow = g_list_previous (indexRow);
      row = *(gint *) indexRow->data;
      foundKeyID =
	gtk_clist_get_text (GTK_CLIST (clistKeys), row, *columnKeyID, &keyID);
      key = gpapa_get_public_key_by_ID (keyID, gpa_callback, windowPublic);
      gtk_clist_remove (GTK_CLIST (clistKeys), row);
      gpapa_public_key_delete (key, gpa_callback, windowPublic);
      indexRow = previousRow;
    }				/* while */
  *rowsSelected = NULL;
  g_list_free (*keysSelected);
  *keysSelected = NULL;
}				/* keys_openPublic_delete */

void
keys_openPublic_editTrust_accept (gpointer param)
{
/* var */
  gpointer *localParam;
  GpaWindowKeeper *keeperTrust;
  GpapaPublicKey *key;
  GtkWidget *comboLevel;
  GpapaOwnertrust trust;
/* commands */
  localParam = (gpointer *) param;
  keeperTrust = (GpaWindowKeeper *) localParam[0];
  key = (GpapaPublicKey *) localParam[2];
  comboLevel = (GtkWidget *) localParam[3];
  trust =
    getOwnertrustForString (gtk_entry_get_text
			    (GTK_ENTRY (GTK_COMBO (comboLevel)->entry)));
  if (trust > GPAPA_OWNERTRUST_LAST)
    {
      gpa_window_error (_("Invalid ownertrust level."), keeperTrust->window);
      return;
    }				/* if */
  gpapa_public_key_set_ownertrust (key, trust, gpa_callback,
				   keeperTrust->window);
  gpa_window_destroy (param);
}				/* keys_openPublic_editTrust_accept */

void
keys_openPublic_editTrust (gpointer param)
{
/* var */
  GpaWindowKeeper *keeper;
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *parent;
  gchar *tip;
  GtkAccelGroup *accelGroup;
  GList *valueLevel = NULL;
  GpapaPublicKey *key;
  GpapaOwnertrust ownertrust;
  gpointer *paramClose;
  gpointer *paramAccept;
/* objects */
  GtkWidget *windowTrust;
  GtkWidget *vboxTrust;
  GtkWidget *tableKey;
  GtkWidget *hboxLevel;
  GtkWidget *labelLevel;
  GtkWidget *comboLevel;
  GtkWidget *hButtonBoxTrust;
  GtkWidget *buttonCancel;
  GtkWidget *buttonAccept;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  parent = (GtkWidget *) localParam[1];
  tip = (gchar *) localParam[2];
  if (!*keysSelected)
    {
      gpa_window_error (_("No key selected for editing."), parent);
      return;
    }				/* if */
  key = gpapa_get_public_key_by_ID (
				    (gchar *) g_list_last (*keysSelected)->
				    data, gpa_callback, parent);
  keeper = gpa_windowKeeper_new ();
  windowTrust = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowTrust);
  gtk_window_set_title (GTK_WINDOW (windowTrust), _("Change key ownertrust"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowTrust), accelGroup);
  vboxTrust = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxTrust), 5);
  tableKey = gpa_tableKey_new (GPAPA_KEY (key), windowTrust);
  gtk_container_set_border_width (GTK_CONTAINER (tableKey), 5);
  gtk_box_pack_start (GTK_BOX (vboxTrust), tableKey, FALSE, FALSE, 0);
  hboxLevel = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxLevel), 5);
  labelLevel = gtk_label_new (_(""));
  gtk_box_pack_start (GTK_BOX (hboxLevel), labelLevel, FALSE, FALSE, 0);
  comboLevel = gtk_combo_new ();
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (comboLevel)->entry),
			     FALSE);
  for (ownertrust = GPAPA_OWNERTRUST_FIRST;
       ownertrust <= GPAPA_OWNERTRUST_LAST; ownertrust++)
    valueLevel =
      g_list_append (valueLevel, getStringForOwnertrust (ownertrust));
  gtk_combo_set_popdown_strings (GTK_COMBO (comboLevel), valueLevel);
  gpa_connect_by_accelerator (GTK_LABEL (labelLevel),
			      GTK_COMBO (comboLevel)->entry, accelGroup,
			      _("_Ownertrust level: "));
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboLevel)->entry),
		      getStringForOwnertrust (gpapa_public_key_get_ownertrust
					      (key, gpa_callback, parent)));
  gtk_box_pack_start (GTK_BOX (hboxLevel), comboLevel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxTrust), hboxLevel, TRUE, TRUE, 0);
  hButtonBoxTrust = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxTrust),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxTrust), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxTrust), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = tip;
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxTrust), buttonCancel);
  buttonAccept = gpa_button_new (accelGroup, _("_Accept"));
  paramAccept = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramAccept);
  paramAccept[0] = keeper;
  paramAccept[1] = tip;
  paramAccept[2] = key;
  paramAccept[3] = comboLevel;
  gtk_signal_connect_object (GTK_OBJECT (buttonAccept), "clicked",
			     GTK_SIGNAL_FUNC
			     (keys_openPublic_editTrust_accept),
			     (gpointer) paramAccept);
  gtk_container_add (GTK_CONTAINER (hButtonBoxTrust), buttonAccept);
  gtk_box_pack_start (GTK_BOX (vboxTrust), hButtonBoxTrust, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowTrust), vboxTrust);
  gpa_widget_show (windowTrust, parent, _("keys_openPublic_editTrust.tip"));
}				/* keys_openPublic_editTrust */

void
keys_openPublic_sign_exec (gpointer param)
{
/* var */
  gpointer *localParam;
  gpointer *data;
  GtkWidget *entryPasswd;
  GpaWindowKeeper *keeperPassphrase;
  GpapaSignType *signType;
  GpapaArmor *armor;
  gchar *keyID;
  GpaWindowKeeper *keeperSign;
  gpointer *userData;
  gchar *tip;
  GtkWidget *checkerLocally;
  GpapaKeySignType keySignType;
  GList **keysSelected;
  GList *indexKey;
  GpapaPublicKey *key;
  gpointer paramClose[2];
/* commands */
  localParam = (gpointer *) param;
  data = (gpointer *) localParam[0];
  entryPasswd = (GtkWidget *) localParam[1];
  keeperPassphrase = (GpaWindowKeeper *) localParam[2];
  signType = (GpapaSignType *) data[0];
  armor = (GpapaArmor *) data[1];
  keyID = (gchar *) data[2];
  keeperSign = (GpaWindowKeeper *) data[3];
  userData = (gpointer *) data[4];
  keysSelected = (GList **) userData[0];
  checkerLocally = (GtkWidget *) userData[1];
  tip = (gchar *) userData[2];
  if (!*keysSelected)
    {
      gpa_window_error (_("No keys selected for signing."),
			keeperSign->window);
      return;
    }				/* if */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerLocally)))
    keySignType = GPAPA_KEY_SIGN_LOCALLY;
  else
    keySignType = GPAPA_KEY_SIGN_NORMAL;
  indexKey = g_list_first (*keysSelected);
  global_lastCallbackResult = GPAPA_ACTION_NONE;
  while (indexKey)
    {
      key = gpapa_get_public_key_by_ID (
					(gchar *) indexKey->data,
					gpa_callback,
					keeperPassphrase->window);
      if (global_lastCallbackResult == GPAPA_ACTION_ERROR)
	{
	  gpa_window_error (_("An error occured while signing keys."),
			    keeperPassphrase->window);
	  return;
	}			/* if */
      gpapa_public_key_sign (key, keyID,
			     gtk_entry_get_text (GTK_ENTRY (entryPasswd)),
			     keySignType, gpa_callback,
			     keeperPassphrase->window);
      if (global_lastCallbackResult == GPAPA_ACTION_ERROR)
	{
	  gpa_window_error (_("An error occured while signing keys."),
			    keeperPassphrase->window);
	  return;
	}			/* if */
      indexKey = g_list_next (indexKey);
    }				/* while */
  paramClose[0] = keeperPassphrase;
  paramClose[1] = tip;
  gpa_window_destroy (paramClose);
  paramClose[0] = keeperSign;
  gpa_window_destroy (paramClose);
}				/* keys_openPublic_sign_exec */

void
keys_openPublic_sign (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *checkerLocally;
  gchar *tip;
  GtkWidget *windowPublic;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  checkerLocally = (GtkWidget *) localParam[1];
  tip = (gchar *) localParam[2];
  windowPublic = (GtkWidget *) localParam[3];
  if (!*keysSelected)
    {
      gpa_window_error (_("No keys selected for signing."), windowPublic);
      return;
    }				/* if */
  file_sign_dialog (keys_openPublic_sign_exec, windowPublic, tip, FALSE,
		    FALSE, param);
}				/* keys_openPublic_sign */

void
keys_openPublic_editKey (gpointer param)
{
/* var */
  GpaWindowKeeper *keeper;
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *windowPublic;
  gchar *keyID;
  GpapaPublicKey *key;
  GtkAccelGroup *accelGroup;
  gchar *titlesSignatures[] = {
    N_("Signature"), N_("Validity"), N_("Key ID")
  };
  gint i;
  GList *signatures = NULL;
  gpointer *paramAppend;
  gpointer *paramTrust;
  GList **keyEdited = NULL;
  gpointer *paramSign;
  gpointer *paramExport;
  gpointer *paramClose;
  gchar *contentsFingerprint;
/* objects */
  GtkWidget *windowKey;
  GtkWidget *vboxEdit;
  GtkWidget *tableKey;
  GtkWidget *vboxFingerprint;
  GtkWidget *labelJfdFingerprint;
  GtkWidget *labelFingerprint;
  GtkWidget *entryFingerprint;
  GtkWidget *vboxSignatures;
  GtkWidget *labelJfdSignatures;
  GtkWidget *labelSignatures;
  GtkWidget *scrollerSignatures;
  GtkWidget *clistSignatures;
  GtkWidget *hboxSignatures;
  GtkWidget *buttonSign;
  GtkWidget *checkerLocally;
  GtkWidget *tableMisc;
  GtkWidget *labelJfdTrust;
  GtkWidget *labelTrust;
  GtkWidget *entryTrust;
  GtkWidget *labelJfdDate;
  GtkWidget *labelDate;
  GtkWidget *entryDate;
  GtkWidget *hButtonBoxEdit;
  GtkWidget *buttonEditTrust;
  GtkWidget *buttonExportKey;
  GtkWidget *buttonClose;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  windowPublic = (GtkWidget *) localParam[1];
  if (!*keysSelected)
    {
      gpa_window_error (_("No key selected for editing."), windowPublic);
      return;
    }				/* if */
  keyID = (gchar *) g_list_last (*keysSelected)->data;
  key = gpapa_get_public_key_by_ID (keyID, gpa_callback, windowPublic);
  keeper = gpa_windowKeeper_new ();
  windowKey = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowKey);
  keyEdited = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, keyEdited);
  *keyEdited = NULL;
  *keyEdited = g_list_append (*keyEdited, keyID);
  gtk_window_set_title (GTK_WINDOW (windowKey), _("Public key editor"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowKey), accelGroup);
  vboxEdit = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxEdit), 5);
  tableKey = gpa_tableKey_new (GPAPA_KEY (key), windowKey);
  gtk_container_set_border_width (GTK_CONTAINER (tableKey), 5);
  gtk_box_pack_start (GTK_BOX (vboxEdit), tableKey, FALSE, FALSE, 0);
  vboxFingerprint = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxFingerprint), 5);
  labelFingerprint = gtk_label_new (_("Fingerprint:"));
  labelJfdFingerprint =
    gpa_widget_hjustified_new (labelFingerprint, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxFingerprint), labelJfdFingerprint, FALSE,
		      FALSE, 0);
  entryFingerprint = gtk_entry_new ();
  contentsFingerprint =
    gpapa_public_key_get_fingerprint (key, gpa_callback, windowKey);
  gtk_widget_set_usize (entryFingerprint,
			gdk_string_width (entryFingerprint->style->font,
					  contentsFingerprint) +
			gdk_string_width (entryFingerprint->style->font,
					  "  ") +
			entryFingerprint->style->klass->xthickness, 0);
  gtk_entry_set_text (GTK_ENTRY (entryFingerprint), contentsFingerprint);
  gtk_editable_set_editable (GTK_EDITABLE (entryFingerprint), FALSE);
  gtk_box_pack_start (GTK_BOX (vboxFingerprint), entryFingerprint, FALSE,
		      FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxEdit), vboxFingerprint, FALSE, FALSE, 0);
  vboxSignatures = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxSignatures), 5);
  labelSignatures = gtk_label_new (_(""));
  labelJfdSignatures =
    gpa_widget_hjustified_new (labelSignatures, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxSignatures), labelJfdSignatures, FALSE,
		      FALSE, 0);
  scrollerSignatures = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerSignatures, 350, 100);
  clistSignatures = gtk_clist_new_with_titles (3, titlesSignatures);
  gpa_connect_by_accelerator (GTK_LABEL (labelSignatures), clistSignatures,
			      accelGroup, _("S_ignatures"));
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
  signatures =
    gpapa_public_key_get_signatures (key, gpa_callback, windowPublic);
  paramAppend = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramAppend);
  paramAppend[0] = clistSignatures;
  paramAppend[1] = windowPublic;
  g_list_foreach (signatures, sigs_append, (gpointer) paramAppend);
  gtk_container_add (GTK_CONTAINER (scrollerSignatures), clistSignatures);
  gtk_box_pack_start (GTK_BOX (vboxSignatures), scrollerSignatures, TRUE,
		      TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxEdit), vboxSignatures, TRUE, TRUE, 0);
  hboxSignatures = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxSignatures), 5);
  buttonSign = gpa_button_new (accelGroup, _("  _Sign  "));
  checkerLocally = gpa_check_button_new (accelGroup, _("sign only _locally"));
  gtk_box_pack_end (GTK_BOX (hboxSignatures), checkerLocally, FALSE, FALSE,
		    5);
  gtk_box_pack_end (GTK_BOX (hboxSignatures), buttonSign, FALSE, FALSE, 0);
  paramSign = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSign);
  paramSign[0] = keyEdited;
  paramSign[1] = checkerLocally;
  paramSign[2] = _("keys_openPublic_editKey.tip");
  paramSign[3] = windowKey;
  gtk_signal_connect_object (GTK_OBJECT (buttonSign), "clicked",
			     GTK_SIGNAL_FUNC (keys_openPublic_sign),
			     (gpointer) paramSign);
  gtk_box_pack_start (GTK_BOX (vboxEdit), hboxSignatures, FALSE, FALSE, 0);
  tableMisc = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (tableMisc), 5);
  labelTrust = gtk_label_new (_("Key Trust: "));
  labelJfdTrust = gpa_widget_hjustified_new (labelTrust, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableMisc), labelJfdTrust, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryTrust = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entryTrust),
		      getStringForKeytrust (gpapa_public_key_get_keytrust
					    (key, gpa_callback, windowKey)));
  gtk_editable_set_editable (GTK_EDITABLE (entryTrust), FALSE);
  gtk_table_attach (GTK_TABLE (tableMisc), entryTrust, 1, 2, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  labelDate = gtk_label_new (_("Expiry date: "));
  labelJfdDate = gpa_widget_hjustified_new (labelDate, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableMisc), labelJfdDate, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryDate = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entryDate),
		      getStringForExpiryDate (gpapa_key_get_expiry_date
					      (GPAPA_KEY (key), gpa_callback,
					       windowKey)));
  gtk_editable_set_editable (GTK_EDITABLE (entryDate), FALSE);
  gtk_table_attach (GTK_TABLE (tableMisc), entryDate, 1, 2, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_box_pack_start (GTK_BOX (vboxEdit), tableMisc, FALSE, FALSE, 0);
  hButtonBoxEdit = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxEdit),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxEdit), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxEdit), 5);
  buttonEditTrust = gpa_button_new (accelGroup, _("Edit _Ownertrust"));
  paramTrust = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramTrust);
  paramTrust[0] = keysSelected;
  paramTrust[1] = windowKey;
  paramTrust[2] = _("keys_openPublic_editKey.tip");
  gtk_signal_connect_object (GTK_OBJECT (buttonEditTrust), "clicked",
			     GTK_SIGNAL_FUNC (keys_openPublic_editTrust),
			     (gpointer) paramTrust);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEdit), buttonEditTrust);
  buttonExportKey = gpa_button_new (accelGroup, _("E_xport key"));
  paramExport = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramExport);
  paramExport[0] = keyEdited;
  paramExport[1] = _("keys_openPublic_editKey.tip");
  paramExport[2] = keys_openPublic_export_export;
  paramExport[3] = windowKey;
  gtk_signal_connect_object (GTK_OBJECT (buttonExportKey), "clicked",
			     GTK_SIGNAL_FUNC (keys_export_dialog),
			     (gpointer) paramExport);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEdit), buttonExportKey);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = _("keys_openPublic.tip");
  buttonClose = gpa_buttonCancel_new (accelGroup, _("_Close"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEdit), buttonClose);
  gtk_box_pack_start (GTK_BOX (vboxEdit), hButtonBoxEdit, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowKey), vboxEdit);
  gpa_widget_show (windowKey, windowPublic, _("keys_openPublic_editKey.tip"));
}				/* keys_openPublic_editKey */

void
keys_openPublic_send_key (gpointer data, gpointer userData)
{
/* var */
  gchar *keyID;
  GtkWidget *windowPublic;
  GpapaPublicKey *key;
/* commands */
  keyID = (gchar *) data;
  windowPublic = (GtkWidget *) userData;
  key = gpapa_get_public_key_by_ID (keyID, gpa_callback, windowPublic);
  gpapa_public_key_send_to_server (key, global_keyserver, gpa_callback,
				   windowPublic);
}				/* keys_openPublic_send_key */

void
keys_openPublic_send (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *windowPublic;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  windowPublic = (GtkWidget *) localParam[1];
  if (!*keysSelected)
    {
      gpa_window_error (_("No keys selected for sending."), windowPublic);
      return;
    }				/* if */
  g_list_foreach (*keysSelected, keys_openPublic_send_key, windowPublic);
  gpa_window_message (_("Keys sent to server."), windowPublic);
}				/* keys_openPublic_send */

void
keys_openPublic_receive_receive (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *entryKey;
  GpaWindowKeeper *keeperReceive;
  gchar *keyID;
  gpointer paramClose[2];
/* commands */
  localParam = (gpointer *) param;
  entryKey = (GtkWidget *) localParam[0];
  keeperReceive = (GpaWindowKeeper *) localParam[1];
  keyID = gtk_entry_get_text (GTK_ENTRY (entryKey));
  global_lastCallbackResult = GPAPA_ACTION_NONE;
  gpapa_receive_public_key_from_server (keyID, global_keyserver, gpa_callback,
					keeperReceive->window);
  if (global_lastCallbackResult == GPAPA_ACTION_ERROR)
    {
      gpa_window_error (_
			("An error occured while receiving\nthe requested key from the keyserver."),
keeperReceive->window);
      return;
    }				/* if */
  paramClose[0] = keeperReceive;
  paramClose[1] = _("keys_openPublic.tip");
  gpa_window_destroy (paramClose);
}				/* keys_openPublic_receive_receive */

void
keys_openPublic_receive (gpointer param)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkWidget *windowPublic;
  GtkAccelGroup *accelGroup;
  gpointer *paramClose;
  gpointer *paramReceive;
/* objects */
  GtkWidget *windowReceive;
  GtkWidget *vboxReceive;
  GtkWidget *hboxKey;
  GtkWidget *labelKey;
  GtkWidget *entryKey;
  GtkWidget *hButtonBoxReceive;
  GtkWidget *buttonCancel;
  GtkWidget *buttonReceive;
/* commands */
  windowPublic = (GtkWidget *) param;
  keeper = gpa_windowKeeper_new ();
  windowReceive = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowReceive);
  gtk_window_set_title (GTK_WINDOW (windowReceive),
			_("Receive key from server"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowReceive), accelGroup);
  vboxReceive = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxReceive), 5);
  hboxKey = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxKey), 5);
  labelKey = gtk_label_new (_(""));
  gtk_box_pack_start (GTK_BOX (hboxKey), labelKey, FALSE, FALSE, 0);
  entryKey = gtk_entry_new ();
  paramReceive = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramReceive);
  paramReceive[0] = entryKey;
  paramReceive[1] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (entryKey), "activate",
			     GTK_SIGNAL_FUNC
			     (keys_openPublic_receive_receive),
			     (gpointer) paramReceive);
  gtk_box_pack_start (GTK_BOX (hboxKey), entryKey, TRUE, TRUE, 0);
  gpa_connect_by_accelerator (GTK_LABEL (labelKey), entryKey, accelGroup,
			      _("_Key ID: "));
  gtk_box_pack_start (GTK_BOX (vboxReceive), hboxKey, TRUE, TRUE, 0);
  hButtonBoxReceive = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxReceive),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxReceive), 10);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = _("keys_openPublic.tip");
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxReceive), buttonCancel);
  buttonReceive = gpa_button_new (accelGroup, _("_Receive"));
  gtk_signal_connect_object (GTK_OBJECT (buttonReceive), "clicked",
			     GTK_SIGNAL_FUNC
			     (keys_openPublic_receive_receive),
			     (gpointer) paramReceive);
  gtk_container_add (GTK_CONTAINER (hButtonBoxReceive), buttonReceive);
  gtk_box_pack_start (GTK_BOX (vboxReceive), hButtonBoxReceive, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowReceive), vboxReceive);
  gpa_widget_show (windowReceive, windowPublic,
		   _("keys_openPublic_receive.tip"));
  gtk_widget_grab_focus (entryKey);
}				/* keys_openPublic_receive */

void
keys_openPublic_exportTrust_export (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *checkerArmor;
  GpaWindowKeeper *keeperTrust;
  GpapaArmor armor;
  gpointer paramClose[2];
/* commands */
  localParam = (gpointer *) param;
  checkerArmor = (GtkWidget *) localParam[0];
  keeperTrust = (GpaWindowKeeper *) localParam[1];
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerArmor)))
    armor = GPAPA_ARMOR;
  else
    armor = GPAPA_NO_ARMOR;
  gpapa_export_ownertrust (gtk_file_selection_get_filename
			   (GTK_FILE_SELECTION (keeperTrust->window)), armor,
			   gpa_callback, keeperTrust->window);
  paramClose[0] = keeperTrust;
  paramClose[1] = _("keys_openPublic.tip");
  gpa_window_destroy (paramClose);
}				/* keys_openPublic_exportTrust_export */

void
keys_openPublic_exportTrust (gpointer param)
{
/* var */
  GpaWindowKeeper *keeper;
  gpointer *localParam;
  GtkWidget *checkerArmor;
  GtkWidget *windowPublic;
  gpointer *paramExport;
  gpointer *paramClose;
/* objects */
  GtkWidget *selectTrust;
/* commands */
  localParam = (gpointer *) param;
  checkerArmor = (GtkWidget *) localParam[0];
  windowPublic = (GtkWidget *) localParam[1];
  keeper = gpa_windowKeeper_new ();
  selectTrust = gtk_file_selection_new (_("Export ownertrust to file"));
  gpa_windowKeeper_set_window (keeper, selectTrust);
  paramExport = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramExport);
  paramExport[0] = checkerArmor;
  paramExport[1] = keeper;
  gtk_signal_connect_object (GTK_OBJECT
			     (GTK_FILE_SELECTION (selectTrust)->ok_button),
			     "clicked",
			     GTK_SIGNAL_FUNC
			     (keys_openPublic_exportTrust_export),
			     (gpointer) paramExport);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = _("keys_openPublic.tip");
  gtk_signal_connect_object (GTK_OBJECT
			     (GTK_FILE_SELECTION (selectTrust)->
			      cancel_button), "clicked",
			     GTK_SIGNAL_FUNC (gpa_window_destroy),
			     (gpointer) paramClose);
  gpa_widget_show (selectTrust, windowPublic,
		   _("keys_openPublic_exportTrust.tip"));
}				/* keys_openPublic_exportTrust */

void
keys_openPublic_fillClistKeys (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *clistKeys;
  GtkWidget *toggleShow;
  GtkWidget *windowPublic;
  gint contentsCountKeys;
  gchar *contentsKeys[5];
  GpapaPublicKey *key;
  GDate *expiryDate;
/* commands */
  localParam = (gpointer *) param;
  clistKeys = (GtkWidget *) localParam[0];
  toggleShow = (GtkWidget *) localParam[1];
  windowPublic = (GtkWidget *) localParam[2];
  gtk_clist_clear (GTK_CLIST (clistKeys));
  contentsCountKeys = gpapa_get_public_key_count (gpa_callback, windowPublic);
  while (contentsCountKeys > 0)
    {
      contentsCountKeys--;
      key =
	gpapa_get_public_key_by_index (contentsCountKeys, gpa_callback,
				       windowPublic);
      contentsKeys[0] =
	gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, windowPublic);
      contentsKeys[1] =
	getStringForKeytrust (gpapa_public_key_get_keytrust
			      (key, gpa_callback, windowPublic));
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggleShow)))
	contentsKeys[2] =
	  getStringForOwnertrust (gpapa_public_key_get_ownertrust
				  (key, gpa_callback, windowPublic));
      else
	contentsKeys[2] = _("");
      expiryDate =
	gpapa_key_get_expiry_date (GPAPA_KEY (key), gpa_callback,
				   windowPublic);
      contentsKeys[3] = getStringForExpiryDate (expiryDate);
      contentsKeys[4] =
	gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
				  windowPublic);
      gtk_clist_append (GTK_CLIST (clistKeys), contentsKeys);
    }				/* while */
}				/* keys_openPublic_fillClistKeys */

void
keys_openPublic_toggleClistKeys (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *clistKeys;
  GtkWidget *toggleShow;
  GtkWidget *windowPublic;
  gint contentsCountKeys;
  gint i;
  GpapaPublicKey *key;
  gchar *contents;
	GtkStyle *style;
	GdkPixmap *icon;
	GdkBitmap *mask;
/* commands */
  localParam = (gpointer *) param;
  clistKeys = (GtkWidget *) localParam[0];
  toggleShow = (GtkWidget *) localParam[1];
  windowPublic = (GtkWidget *) localParam[2];

	style = gtk_widget_get_style(windowPublic);

	contentsCountKeys = gpapa_get_public_key_count (gpa_callback, windowPublic);
	for (i = 0; i < contentsCountKeys; i++) {
		key = gpapa_get_public_key_by_index (i, gpa_callback, windowPublic);

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggleShow))) {
			contents = getStringForOwnertrust (gpapa_public_key_get_ownertrust
				(key, gpa_callback, windowPublic));

			icon = gdk_pixmap_create_from_xpm_d(windowPublic->window,&mask,
				&style->bg[GTK_STATE_NORMAL],
				(gchar **)getIconForOwnertrust(
					gpapa_public_key_get_ownertrust(key, gpa_callback,
						windowPublic)));
			gtk_clist_set_pixtext (GTK_CLIST (clistKeys), i, 2, contents,
				8, icon, mask);
		} else {
			contents = _("");
			gtk_clist_set_text (GTK_CLIST (clistKeys), i, 2, contents);
		}

    }				/* while */
}				/* keys_openPublic_toggleClistKeys */

gboolean
keys_openPublic_evalMouse (GtkWidget * clistKeys, GdkEventButton * event,
			   gpointer param)
{
  if (event->type == GDK_2BUTTON_PRESS)
    keys_openPublic_editKey (param);
  return (TRUE);
}				/* keys_openPublic_evalMouse */

void
keys_ring_selectKey (GtkWidget * clistKeys, gint row, gint column,
		     GdkEventButton * event, gpointer param)
{
/* var */
  gpointer *localParam;
  GList **rowsSelected;
/* commands */
  localParam = (gpointer *) param;
  rowsSelected = (GList **) localParam[3];
  keys_selectKey (clistKeys, row, column, event, param);
  gpa_selectRecipient (clistKeys, row, column, event, rowsSelected);
}				/* keys_ring_selectKey */

void
keys_ring_unselectKey (GtkWidget * clistKeys, gint row, gint column,
		       GdkEventButton * event, gpointer param)
{
/* var */
  gpointer *localParam;
  GList **rowsSelected;
/* commands */
  localParam = (gpointer *) param;
  rowsSelected = (GList **) localParam[3];
  keys_unselectKey (clistKeys, row, column, event, param);
  gpa_unselectRecipient (clistKeys, row, column, event, rowsSelected);
}				/* keys_ring_unselectKey */

void
keys_openPublic (void)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gchar *titlesKeys[] = {
    N_("Key owner"), N_("Key trust"), N_("Ownertrust"),
    N_("Expiry date"), N_("Key ID")
  };
  GList **keysSelected = NULL;
  GList **rowsSelected = NULL;
  gint i;
  static gint columnKeyID = 4;
  gpointer *paramKeys;
  gpointer *paramEdit;
  gpointer *paramSign;
  gpointer *paramSend;
  gpointer *paramExport;
  gpointer *paramDelete;
  gpointer *paramTrust;
  gpointer *paramShow;
  gpointer *paramExportTrust;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowPublic;
  GtkWidget *vboxPublic;
  GtkWidget *vboxKeys;
  GtkWidget *labelJfdRingname;
  GtkWidget *labelRingname;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;
  GtkWidget *hboxAction;
  GtkWidget *tableKey;
  GtkWidget *buttonEditKey;
  GtkWidget *buttonSign, *buttonSignBox;
  GtkWidget *buttonSend;
  GtkWidget *buttonReceive;
  GtkWidget *buttonExportKey;
  GtkWidget *buttonDelete, *buttonDeleteBox;
  GtkWidget *vboxLocally;
  GtkWidget *checkerLocally;
  GtkWidget *tableTrust;
  GtkWidget *toggleShow;
  GtkWidget *buttonEditTrust;
  GtkWidget *buttonExportTrust;
  GtkWidget *vboxTrust;
  GtkWidget *checkerArmor;
  GtkWidget *hSeparatorPublic;
  GtkWidget *hButtonBoxPublic;
  GtkWidget *buttonClose;
/* commands */
  keeper = gpa_windowKeeper_new ();
  windowPublic = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowPublic);
  gtk_window_set_title (GTK_WINDOW (windowPublic),
			_("Public key ring editor"));
  gtk_widget_set_usize (windowPublic, 572, 400);
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowPublic), accelGroup);
  vboxPublic = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxPublic), 5);
  vboxKeys = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxKeys), 5);
  labelRingname = gtk_label_new (_(""));
  labelJfdRingname =
    gpa_widget_hjustified_new (labelRingname, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxKeys), labelJfdRingname, FALSE, FALSE, 0);
  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  clistKeys = gtk_clist_new_with_titles (5, titlesKeys);
  gtk_clist_set_selection_mode (GTK_CLIST (clistKeys),
				GTK_SELECTION_EXTENDED);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 0, 185);
  for (i = 1; i < 4; i++)
    {
      gtk_clist_set_column_width (GTK_CLIST (clistKeys), i, 100);
      gtk_clist_set_column_justification (GTK_CLIST (clistKeys), i,
					  GTK_JUSTIFY_CENTER);
    }				/* for */
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 4, 120);
  for (i = 0; i < 5; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistKeys), i);
  gpa_connect_by_accelerator (GTK_LABEL (labelRingname), clistKeys,
			      accelGroup, _("_Public key Ring"));
  keysSelected = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, keysSelected);
  *keysSelected = NULL;
  rowsSelected = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, rowsSelected);
  *rowsSelected = NULL;
  paramKeys = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramKeys);
  paramKeys[0] = keysSelected;
  paramKeys[1] = &columnKeyID;
  paramKeys[2] = windowPublic;
  paramKeys[3] = rowsSelected;
  gtk_signal_connect (GTK_OBJECT (clistKeys), "select-row",
		      GTK_SIGNAL_FUNC (keys_ring_selectKey),
		      (gpointer) paramKeys);
  gtk_signal_connect (GTK_OBJECT (clistKeys), "unselect-row",
		      GTK_SIGNAL_FUNC (keys_ring_unselectKey),
		      (gpointer) paramKeys);
  paramEdit = (gpointer *) xmalloc (2 * sizeof (paramEdit));
  gpa_windowKeeper_add_param (keeper, paramEdit);
  paramEdit[0] = keysSelected;
  paramEdit[1] = windowPublic;
  gtk_signal_connect (GTK_OBJECT (clistKeys), "button-press-event",
		      GTK_SIGNAL_FUNC (keys_openPublic_evalMouse),
		      (gpointer) paramEdit);
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gtk_box_pack_start (GTK_BOX (vboxKeys), scrollerKeys, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxPublic), vboxKeys, TRUE, TRUE, 0);
  hboxAction = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxAction), 5);
  tableKey = gtk_table_new (3, 2, TRUE);
  buttonEditKey = gpa_button_new (accelGroup, _("_Edit key"));
  gtk_signal_connect_object (GTK_OBJECT (buttonEditKey), "clicked",
			     GTK_SIGNAL_FUNC (keys_openPublic_editKey),
			     (gpointer) paramEdit);
  gtk_table_attach (GTK_TABLE (tableKey), buttonEditKey, 0, 1, 0, 1, GTK_FILL,
		    GTK_FILL, 0, 0);

  /* the 'sign key' button */
  buttonSign = gtk_button_new();
  gtk_table_attach (GTK_TABLE (tableKey), buttonSign, 1, 2, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  buttonSend = gpa_button_new (accelGroup, _("Se_nd keys"));
  paramSend = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSend);
  paramSend[0] = keysSelected;
  paramSend[1] = windowPublic;
  gtk_signal_connect_object (GTK_OBJECT (buttonSend), "clicked",
			     GTK_SIGNAL_FUNC (keys_openPublic_send),
			     (gpointer) paramSend);
  gtk_table_attach (GTK_TABLE (tableKey), buttonSend, 0, 1, 1, 2, GTK_FILL,
		    GTK_FILL, 0, 0);
  /* associate this button with an icon */
  buttonSignBox = gpa_xpm_label_box(windowPublic, gpa_sign_small_xpm,
				  _("_Sign keys"), buttonSign, accelGroup);
  gtk_container_add (GTK_CONTAINER (buttonSign), buttonSignBox);

  buttonReceive = gpa_button_new (accelGroup, _("_Receive keys"));
  gtk_signal_connect_object (GTK_OBJECT (buttonReceive), "clicked",
			     GTK_SIGNAL_FUNC (keys_openPublic_receive),
			     (gpointer) windowPublic);
  gtk_table_attach (GTK_TABLE (tableKey), buttonReceive, 1, 2, 1, 2, GTK_FILL,
		    GTK_FILL, 0, 0);
  buttonExportKey = gpa_button_new (accelGroup, _("E_xport keys"));
  paramExport = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramExport);
  paramExport[0] = keysSelected;
  paramExport[1] = _("keys_openPublic.tip");
  paramExport[2] = keys_openPublic_export_export;
  paramExport[3] = windowPublic;
  gtk_signal_connect_object (GTK_OBJECT (buttonExportKey), "clicked",
			     GTK_SIGNAL_FUNC (keys_export_dialog),
			     (gpointer) paramExport);
  gtk_table_attach (GTK_TABLE (tableKey), buttonExportKey, 0, 1, 2, 3,
		    GTK_FILL, GTK_FILL, 0, 0);

  /* the 'delete key' button */
  buttonDelete = gtk_button_new();
  paramDelete = (gpointer *) xmalloc (5 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramDelete);
  paramDelete[0] = keysSelected;
  paramDelete[1] = clistKeys;
  paramDelete[2] = &columnKeyID;
  paramDelete[3] = rowsSelected;
  paramDelete[4] = windowPublic;
  gtk_signal_connect_object (GTK_OBJECT (buttonDelete), "clicked",
			     GTK_SIGNAL_FUNC (keys_openPublic_delete),
			     (gpointer) paramDelete);
  gtk_table_attach (GTK_TABLE (tableKey), buttonDelete, 1, 2, 2, 3, GTK_FILL,
		    GTK_FILL, 0, 0);
  /* associate this button with an icon */
  buttonDeleteBox = gpa_xpm_label_box(windowPublic, trash_xpm,
				  _("_Delete keys"), buttonDelete, accelGroup);
  gtk_container_add (GTK_CONTAINER (buttonDelete), buttonDeleteBox);

  gtk_box_pack_start (GTK_BOX (hboxAction), tableKey, FALSE, FALSE, 0);
  vboxLocally = gtk_vbox_new (FALSE, 0);
  checkerLocally = gpa_check_button_new (accelGroup, _("sign only _locally"));
  gtk_box_pack_start (GTK_BOX (vboxLocally), checkerLocally, FALSE, FALSE, 0);
  paramSign = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSign);
  paramSign[0] = keysSelected;
  paramSign[1] = checkerLocally;
  paramSign[2] = _("keys_openPublic.tip");
  paramSign[3] = windowPublic;
  gtk_signal_connect_object (GTK_OBJECT (buttonSign), "clicked",
			     GTK_SIGNAL_FUNC (keys_openPublic_sign),
			     (gpointer) paramSign);
  gtk_box_pack_start (GTK_BOX (hboxAction), vboxLocally, FALSE, FALSE, 5);
  vboxTrust = gtk_vbox_new (FALSE, 0);
  checkerArmor = gpa_check_button_new (accelGroup, _("ex_port armored"));
  gtk_box_pack_end (GTK_BOX (vboxTrust), checkerArmor, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hboxAction), vboxTrust, FALSE, FALSE, 0);
  tableTrust = gtk_table_new (3, 1, TRUE);
  toggleShow = gpa_toggle_button_new (accelGroup, _("S_how ownertrust"));
  paramShow = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramShow);
  paramShow[0] = clistKeys;
  paramShow[1] = toggleShow;
  paramShow[2] = windowPublic;
  gtk_signal_connect_object (GTK_OBJECT (toggleShow), "clicked",
			     GTK_SIGNAL_FUNC
			     (keys_openPublic_toggleClistKeys),
			     (gpointer) paramShow);
  gtk_table_attach (GTK_TABLE (tableTrust), toggleShow, 0, 1, 0, 1, GTK_FILL,
		    GTK_FILL, 0, 0);
  keys_openPublic_fillClistKeys (paramShow);
  buttonEditTrust = gpa_button_new (accelGroup, _("Edit _ownertrust"));
  paramTrust = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramTrust);
  paramTrust[0] = keysSelected;
  paramTrust[1] = windowPublic;
  paramTrust[2] = _("keys_openPublic.tip");
  gtk_signal_connect_object (GTK_OBJECT (buttonEditTrust), "clicked",
			     GTK_SIGNAL_FUNC (keys_openPublic_editTrust),
			     (gpointer) paramTrust);
  gtk_table_attach (GTK_TABLE (tableTrust), buttonEditTrust, 0, 1, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  buttonExportTrust = gpa_button_new (accelGroup, _("Export o_wnertrust"));
  paramExportTrust = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramExportTrust);
  paramExportTrust[0] = checkerArmor;
  paramExportTrust[1] = windowPublic;
  gtk_signal_connect_object (GTK_OBJECT (buttonExportTrust), "clicked",
			     GTK_SIGNAL_FUNC (keys_openPublic_exportTrust),
			     (gpointer) paramExportTrust);
  gtk_table_attach (GTK_TABLE (tableTrust), buttonExportTrust, 0, 1, 2, 3,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_end (GTK_BOX (hboxAction), tableTrust, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vboxPublic), hboxAction, FALSE, FALSE, 0);
  hSeparatorPublic = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vboxPublic), hSeparatorPublic, FALSE, FALSE,
		      0);
  hButtonBoxPublic = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxPublic),
			     GTK_BUTTONBOX_END);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxPublic), 5);
  buttonClose = gpa_button_new (accelGroup, _("_Close"));
  gtk_widget_add_accelerator (buttonClose, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keysSelected;
  paramClose[1] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (buttonClose), "clicked",
			     GTK_SIGNAL_FUNC (keys_ringEditor_close),
			     (gpointer) paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxPublic), buttonClose);
  gtk_box_pack_start (GTK_BOX (vboxPublic), hButtonBoxPublic, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowPublic), vboxPublic);
  gpa_widget_show (windowPublic, global_windowMain, _("keys_openPublic.tip"));
  if (!gpapa_get_public_key_count)
    gpa_window_error (_("No public keys available yet."), windowPublic);
}				/* keys_openPublic */

void
keys_openSecret_export_export (gpointer param)
{
/* var */
  gpointer *localParam;
  GpaWindowKeeper *keeperExport;
  gchar *tip;
  GList **keysSelected;
  GtkWidget *entryFilename;
  GtkWidget *checkerArmor;
  gchar *fileID;
  GpapaArmor armor;
  GList *indexKey;
  GpapaSecretKey *key;
  gpointer paramClose[2];
/* commands */
  localParam = (gpointer *) param;
  keeperExport = (GpaWindowKeeper *) localParam[0];
  tip = (gchar *) localParam[1];
  keysSelected = (GList **) localParam[2];
  entryFilename = (GtkWidget *) localParam[3];
  checkerArmor = (GtkWidget *) localParam[4];
  fileID = gtk_entry_get_text (GTK_ENTRY (entryFilename));
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerArmor)))
    armor = GPAPA_ARMOR;
  else
    armor = GPAPA_NO_ARMOR;
  indexKey = g_list_first (*keysSelected);
  while (indexKey)
    {
      global_lastCallbackResult = GPAPA_ACTION_NONE;
      key = gpapa_get_secret_key_by_ID (
					(gchar *) indexKey->data,
					gpa_callback, keeperExport->window);
      if (global_lastCallbackResult == GPAPA_ACTION_ERROR)
	{
	  gpa_window_error (_
			    ("An error occurred while\nexporting secret keys."),
keeperExport->window);
	  return;
	}			/* if */
      gpapa_secret_key_export (key, fileID, armor, gpa_callback,
			       keeperExport->window);
      if (global_lastCallbackResult == GPAPA_ACTION_ERROR)
	{
	  gpa_window_error (_
			    ("An error occurred while\nexporting secret keys."),
keeperExport->window);
	  return;
	}			/* if */
      indexKey = g_list_next (indexKey);
    }				/* while */
  paramClose[0] = keeperExport;
  paramClose[1] = tip;
  gpa_window_message (_("Keys exported."), keeperExport->window);
  gpa_window_destroy (paramClose);
}				/* keys_openSecret_export_export */

void
keys_openSecret_delete (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *clistKeys;
  gint *columnKeyID;
  GList **rowsSelected;
  GtkWidget *windowSecret;
  GList *indexRow, *previousRow;
  gint row;
  gint foundKeyID;
  gchar *keyID;
  GpapaSecretKey *key;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  clistKeys = (GtkWidget *) localParam[1];
  columnKeyID = (gint *) localParam[2];
  rowsSelected = (GList **) localParam[3];
  windowSecret = (GtkWidget *) localParam[4];
  if (!*keysSelected)
    {
      gpa_window_error (_("No keys selected to delete."), windowSecret);
      return;
    }				/* if */
  if (!*rowsSelected)
    {
      gpa_window_error (_("!FATAL ERROR: Invalid key selection info!\n"),
			windowSecret);
      return;
    }				/* if */
  indexRow = g_list_last (*rowsSelected);
  while (indexRow)
    {
      previousRow = g_list_previous (indexRow);
      row = *(gint *) indexRow->data;
      foundKeyID =
	gtk_clist_get_text (GTK_CLIST (clistKeys), row, *columnKeyID, &keyID);
      key = gpapa_get_secret_key_by_ID (keyID, gpa_callback, windowSecret);
      gtk_clist_remove (GTK_CLIST (clistKeys), row);
      gpapa_secret_key_delete (key, gpa_callback, windowSecret);
      indexRow = previousRow;
    }				/* while */
  *rowsSelected = NULL;
  g_list_free (*keysSelected);
  *keysSelected = NULL;
}				/* keys_openSecret_delete */

void
keys_openSecret_revocation (gpointer param)
{
/* var */
  gpointer *localParam;
  GpapaSecretKey *key;
  GtkWidget *windowEdit;
/* commands */
  localParam = (gpointer *) param;
  key = (GpapaSecretKey *) localParam[0];
  windowEdit = (GtkWidget *) localParam[1];
  global_lastCallbackResult = GPAPA_ACTION_NONE;
  gpapa_secret_key_create_revocation (key, gpa_callback, windowEdit);
  if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
    gpa_window_message (_("Revocation certificate created."), windowEdit);
}				/* keys_openSecret_revocation */

void
keys_openSecret_editKey_close (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *radioDont;
  GtkWidget *radioAfter;
  GtkWidget *radioAt;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *entryAt;
  GtkWidget *entryPasswd;
  GtkWidget *entryRepeat;
  GpapaSecretKey *key;
  GpaWindowKeeper *keeperEdit;
  gint i;
  gchar unit;
  GDate *date;
  gpointer paramClose[2];
/* commands */
  localParam = (gpointer *) param;
  radioDont = (GtkWidget *) localParam[0];
  radioAfter = (GtkWidget *) localParam[1];
  radioAt = (GtkWidget *) localParam[2];
  entryAfter = (GtkWidget *) localParam[3];
  comboAfter = (GtkWidget *) localParam[4];
  entryAt = (GtkWidget *) localParam[5];
  entryPasswd = (GtkWidget *) localParam[6];
  entryRepeat = (GtkWidget *) localParam[7];
  key = (GpapaSecretKey *) localParam[8];
  keeperEdit = (GpaWindowKeeper *) localParam[9];
  if (strcmp (gtk_entry_get_text (GTK_ENTRY (entryPasswd)),
	      gtk_entry_get_text (GTK_ENTRY (entryRepeat))) != 0)
    {
      gpa_window_error (_
			("In \"Password\" and \"Repeat Password\",\nyou must enter the same password."),
keeperEdit->window);
      return;
    }				/* if */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radioDont)))
    gpapa_key_set_expiry_date (GPAPA_KEY (key), NULL, gpa_callback,
			       keeperEdit->window);
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radioAfter)))
    {
      i = atoi (gtk_entry_get_text (GTK_ENTRY (entryAfter)));
      unit =
	getTimeunitForString (gtk_entry_get_text
			      (GTK_ENTRY (GTK_COMBO (comboAfter)->entry)));
      gpapa_key_set_expiry_time (GPAPA_KEY (key), i, unit, gpa_callback,
				 keeperEdit->window);
    }				/* else if */
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radioAt)))
    {
      date = g_date_new ();
      g_date_set_parse (date, gtk_entry_get_text (GTK_ENTRY (entryAt)));
      gpapa_key_set_expiry_date (GPAPA_KEY (key), date, gpa_callback,
				 keeperEdit->window);
      free (date);
    }				/* else if */
  else
    {
      gpa_window_error (_
			("!FATAL ERROR!\nInvalid insert mode for expiry date."),
keeperEdit->window);
      return;
    }				/* if */
  gpapa_secret_key_set_passphrase (key,
				   gtk_entry_get_text (GTK_ENTRY
						       (entryPasswd)),
				   gpa_callback, keeperEdit->window);
  paramClose[0] = keeperEdit;
  paramClose[1] = _("keys_openSecret.tip");
  gpa_window_destroy (paramClose);
}				/* keys_openSecret_editKey_close */

void
keys_openSecret_editKey (gpointer param)
{
/* var */
  GpaWindowKeeper *keeper;
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *windowSecret;
  gchar *keyID;
  GpapaSecretKey *key;
  GList **keyEdited;
  GDate **expiryDate;
  GtkAccelGroup *accelGroup;
  gpointer *paramExport;
  gpointer *paramRevoc;
  gpointer *paramCancel;
  gpointer *paramSave;
/* objects */
  GtkWidget *windowEdit;
  GtkWidget *vboxEdit;
  GtkWidget *tableKey;
  GtkWidget *tablePasswd;
  GtkWidget *labelJfdPasswd;
  GtkWidget *labelPasswd;
  GtkWidget *entryPasswd;
  GtkWidget *labelJfdRepeat;
  GtkWidget *labelRepeat;
  GtkWidget *entryRepeat;
  GtkWidget *frameExpire;
  GtkWidget *hButtonBoxEdit;
  GtkWidget *buttonRevoc;
  GtkWidget *buttonExport;
  GtkWidget *buttonCancel;
  GtkWidget *buttonSave;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  windowSecret = (GtkWidget *) localParam[1];
  if (!*keysSelected)
    {
      gpa_window_error (_("No key selected for editing."), windowSecret);
      return;
    }				/* if */
  keyID = (gchar *) g_list_last (*keysSelected)->data;
  key = gpapa_get_secret_key_by_ID (keyID, gpa_callback, windowSecret);
  keeper = gpa_windowKeeper_new ();
  windowEdit = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowEdit);
  keyEdited = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, keyEdited);
  *keyEdited = NULL;
  *keyEdited = g_list_append (*keyEdited, keyID);
  gtk_window_set_title (GTK_WINDOW (windowEdit), _("Secret key editor"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowEdit), accelGroup);
  vboxEdit = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxEdit), 5);
  tableKey = gpa_tableKey_new (GPAPA_KEY (key), windowEdit);
  gtk_container_set_border_width (GTK_CONTAINER (tableKey), 5);
  gtk_box_pack_start (GTK_BOX (vboxEdit), tableKey, FALSE, FALSE, 0);
  tablePasswd = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (tablePasswd), 5);
  labelPasswd = gtk_label_new (_(""));
  labelJfdPasswd = gpa_widget_hjustified_new (labelPasswd, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tablePasswd), labelJfdPasswd, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryPasswd = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entryPasswd), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelPasswd), entryPasswd,
			      accelGroup, _("_Password: "));
  gtk_table_attach (GTK_TABLE (tablePasswd), entryPasswd, 1, 2, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  labelRepeat = gtk_label_new (_(""));
  labelJfdRepeat = gpa_widget_hjustified_new (labelRepeat, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tablePasswd), labelJfdRepeat, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryRepeat = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entryRepeat), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelRepeat), entryRepeat,
			      accelGroup, _("_Repeat password: "));
  gtk_table_attach (GTK_TABLE (tablePasswd), entryRepeat, 1, 2, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_signal_connect_object (GTK_OBJECT (entryPasswd), "activate",
			     GTK_SIGNAL_FUNC (gtk_widget_grab_focus),
			     (gpointer) entryRepeat);
  gtk_box_pack_start (GTK_BOX (vboxEdit), tablePasswd, TRUE, TRUE, 0);
  expiryDate = (GDate **) xmalloc (sizeof (GDate *));
  *expiryDate =
    gpapa_key_get_expiry_date (GPAPA_KEY (key), gpa_callback, windowEdit);
  paramSave = (gpointer *) xmalloc (10 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSave);
  frameExpire =
    gpa_frameExpire_new (accelGroup, expiryDate, keeper, paramSave);
  gtk_container_set_border_width (GTK_CONTAINER (frameExpire), 5);
  gtk_box_pack_start (GTK_BOX (vboxEdit), frameExpire, FALSE, FALSE, 0);
  hButtonBoxEdit = gtk_hbutton_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxEdit), 5);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxEdit),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxEdit), 10);
  buttonRevoc = gpa_button_new (accelGroup, _("Create Re_vocation"));
  paramRevoc = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramRevoc);
  paramRevoc[0] = key;
  paramRevoc[1] = windowEdit;
  gtk_signal_connect_object (GTK_OBJECT (buttonRevoc), "clicked",
			     GTK_SIGNAL_FUNC (keys_openSecret_revocation),
			     (gpointer) paramRevoc);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEdit), buttonRevoc);
  buttonExport = gpa_button_new (accelGroup, _("E_xport key"));
  paramExport = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramExport);
  paramExport[0] = keyEdited;
  paramExport[1] = _("keys_openSecret_editKey.tip");
  paramExport[2] = keys_openSecret_export_export;
  paramExport[3] = windowEdit;
  gtk_signal_connect_object (GTK_OBJECT (buttonExport), "clicked",
			     GTK_SIGNAL_FUNC (keys_export_dialog),
			     (gpointer) paramExport);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEdit), buttonExport);
  paramCancel = (gpointer *) xmalloc (2 * sizeof (gpointer));
  paramCancel[0] = keeper;
  paramCancel[1] = _("keys_openSecret.tip");
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramCancel);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEdit), buttonCancel);
  paramSave[6] = entryPasswd;
  paramSave[7] = entryRepeat;
  paramSave[8] = key;
  paramSave[9] = keeper;
  buttonSave = gpa_button_new (accelGroup, _("_Save and exit"));
  gtk_signal_connect_object (GTK_OBJECT (buttonSave), "clicked",
			     GTK_SIGNAL_FUNC (keys_openSecret_editKey_close),
			     (gpointer) paramSave);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEdit), buttonSave);
  gtk_signal_connect_object (GTK_OBJECT (entryRepeat), "activate",
			     GTK_SIGNAL_FUNC (gtk_widget_grab_focus),
			     (gpointer) buttonSave);
  gtk_box_pack_start (GTK_BOX (vboxEdit), hButtonBoxEdit, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowEdit), vboxEdit);
  gpa_widget_show (windowEdit, windowSecret,
		   _("keys_openSecret_editKey.tip"));
}				/* keys_openSecret_editKey */

gboolean
keys_openSecret_evalMouse (GtkWidget * clistKeys, GdkEventButton * event,
			   gpointer param)
{
  if (event->type == GDK_2BUTTON_PRESS)
    keys_openSecret_editKey (param);
  return (TRUE);
}				/* keys_openSecret_evalMouse */

void
keys_openSecret (void)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gint contentsCountKeys;
  gchar *titlesKeys[2] = { N_("User identity / role"), N_("Key ID") };
  gint i;
  GpapaSecretKey *key;
  gchar *contentsKeys[2];
  GList **keysSelected = NULL;
  GList **rowsSelected = NULL;
  static gint columnKeyID = 1;
  gpointer *paramKeys;
  gpointer *paramExport;
  gpointer *paramDelete;
  gpointer *paramEdit;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowSecret;
  GtkWidget *vboxSecret;
  GtkWidget *vboxKeys;
  GtkWidget *labelJfdRingname;
  GtkWidget *labelRingname;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;
  GtkWidget *hButtonBoxSecret;
  GtkWidget *buttonExport;
  GtkWidget *buttonDelete;
  GtkWidget *buttonEditKey;
  GtkWidget *buttonClose;
/* commands */
  keeper = gpa_windowKeeper_new ();
  windowSecret = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowSecret);
  gtk_window_set_title (GTK_WINDOW (windowSecret),
			_("Secret key ring editor"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowSecret), accelGroup);
  vboxSecret = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxSecret), 5);
  vboxKeys = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxKeys), 5);
  labelRingname = gtk_label_new (_(""));
  labelJfdRingname =
    gpa_widget_hjustified_new (labelRingname, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxKeys), labelJfdRingname, FALSE, FALSE, 0);
  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerKeys, 400, 280);
  clistKeys = gtk_clist_new_with_titles (2, titlesKeys);
  gtk_clist_set_selection_mode (GTK_CLIST (clistKeys),
				GTK_SELECTION_EXTENDED);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 0, 320);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 1, 120);
  for (i = 0; i < 2; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistKeys), i);
  keysSelected = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, keysSelected);
  *keysSelected = NULL;
  rowsSelected = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, rowsSelected);
  *rowsSelected = NULL;
  paramKeys = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramKeys);
  paramKeys[0] = keysSelected;
  paramKeys[1] = &columnKeyID;
  paramKeys[2] = windowSecret;
  paramKeys[3] = rowsSelected;
  gtk_signal_connect (GTK_OBJECT (clistKeys), "select-row",
		      GTK_SIGNAL_FUNC (keys_ring_selectKey),
		      (gpointer) paramKeys);
  gtk_signal_connect (GTK_OBJECT (clistKeys), "unselect-row",
		      GTK_SIGNAL_FUNC (keys_ring_unselectKey),
		      (gpointer) paramKeys);
  paramEdit = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramEdit);
  paramEdit[0] = keysSelected;
  paramEdit[1] = windowSecret;
  gtk_signal_connect (GTK_OBJECT (clistKeys), "button-press-event",
		      GTK_SIGNAL_FUNC (keys_openSecret_evalMouse),
		      (gpointer) paramEdit);
  gpa_connect_by_accelerator (GTK_LABEL (labelRingname), clistKeys,
			      accelGroup, _("_Secret key ring"));
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gtk_box_pack_start (GTK_BOX (vboxKeys), scrollerKeys, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxSecret), vboxKeys, TRUE, TRUE, 0);
  hButtonBoxSecret = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxSecret),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxSecret), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxSecret), 5);
  buttonExport = gpa_button_new (accelGroup, _("E_xport keys"));
  paramExport = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramExport);
  paramExport[0] = keysSelected;
  paramExport[1] = _("keys_openSecret.tip");
  paramExport[2] = keys_openSecret_export_export;
  paramExport[3] = windowSecret;
  gtk_signal_connect_object (GTK_OBJECT (buttonExport), "clicked",
			     GTK_SIGNAL_FUNC (keys_export_dialog),
			     (gpointer) paramExport);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSecret), buttonExport);
  buttonDelete = gpa_button_new (accelGroup, _("_Delete keys"));
  paramDelete = (gpointer *) xmalloc (5 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramDelete);
  paramDelete[0] = keysSelected;
  paramDelete[1] = clistKeys;
  paramDelete[2] = &columnKeyID;
  paramDelete[3] = rowsSelected;
  paramDelete[4] = windowSecret;
  gtk_signal_connect_object (GTK_OBJECT (buttonDelete), "clicked",
			     GTK_SIGNAL_FUNC (keys_openSecret_delete),
			     (gpointer) paramDelete);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSecret), buttonDelete);
  buttonEditKey = gpa_button_new (accelGroup, _("_Edit key"));
  gtk_signal_connect_object (GTK_OBJECT (buttonEditKey), "clicked",
			     GTK_SIGNAL_FUNC (keys_openSecret_editKey),
			     (gpointer) paramEdit);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSecret), buttonEditKey);
  buttonClose = gpa_button_new (accelGroup, _("_Close"));
  gtk_widget_add_accelerator (buttonClose, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keysSelected;
  paramClose[1] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (buttonClose), "clicked",
			     GTK_SIGNAL_FUNC (keys_ringEditor_close),
			     (gpointer) paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSecret), buttonClose);
  gtk_box_pack_start (GTK_BOX (vboxSecret), hButtonBoxSecret, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowSecret), vboxSecret);
  gpa_widget_show (windowSecret, global_windowMain, _("keys_openSecret.tip"));
  contentsCountKeys = gpapa_get_secret_key_count (gpa_callback, windowSecret);
  if (contentsCountKeys)
    do
      {
	contentsCountKeys--;
	key =
	  gpapa_get_secret_key_by_index (contentsCountKeys, gpa_callback,
					 windowSecret);
	contentsKeys[0] =
	  gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, windowSecret);
	contentsKeys[1] =
	  gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
				    windowSecret);
	gtk_clist_prepend (GTK_CLIST (clistKeys), contentsKeys);
      }
    while (contentsCountKeys);
  else
    gpa_window_error (_("No secret keys available yet."), windowSecret);
}				/* keys_openSecret */

void
keys_generateKey_generate (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *radioDont;
  GtkWidget *radioAfter;
  GtkWidget *radioAt;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *entryAt;
  GtkWidget *comboAlgorithm;
  GtkWidget *comboKeysize;
  GtkWidget *entryUserID;
  GtkWidget *entryEmail;
  GtkWidget *entryComment;
  GtkWidget *entryPasswd;
  GtkWidget *entryRepeat;
  GtkWidget *checkerRevoc;
  GtkWidget *checkerSend;
  GpaWindowKeeper *keeperGenerate;
  GpapaAlgo algo;
  gint keysize;
  GDate *expiryDate;
  gint i;
  gchar unit;
  gchar *userID, *email, *comment;
  GpapaPublicKey *publicKey;
  GpapaSecretKey *secretKey;
  GpapaKey *dummyKey;		/*!!! */
  gpointer paramDone[2];
/* commands */
  localParam = (gpointer *) param;
  radioDont = (GtkWidget *) localParam[0];
  radioAfter = (GtkWidget *) localParam[1];
  radioAt = (GtkWidget *) localParam[2];
  entryAfter = (GtkWidget *) localParam[3];
  comboAfter = (GtkWidget *) localParam[4];
  entryAt = (GtkWidget *) localParam[5];
  comboAlgorithm = (GtkWidget *) localParam[6];
  comboKeysize = (GtkWidget *) localParam[7];
  entryUserID = (GtkWidget *) localParam[8];
  entryEmail = (GtkWidget *) localParam[9];
  entryComment = (GtkWidget *) localParam[10];
  entryPasswd = (GtkWidget *) localParam[11];
  entryRepeat = (GtkWidget *) localParam[12];
  checkerRevoc = (GtkWidget *) localParam[13];
  checkerSend = (GtkWidget *) localParam[14];
  keeperGenerate = (GpaWindowKeeper *) localParam[15];
  dummyKey = gpapa_key_new (_("DUMMY"), gpa_callback, keeperGenerate->window);	/*!!! */
  publicKey = (GpapaPublicKey *) xmalloc (sizeof (GpapaPublicKey));	/*!!! */
  publicKey->key = dummyKey;	/*!!! */
  secretKey = (GpapaSecretKey *) xmalloc (sizeof (GpapaSecretKey));	/*!!! */
  secretKey->key = dummyKey;	/*!!! */
  algo =
    getAlgorithmForString (gtk_entry_get_text
			   (GTK_ENTRY (GTK_COMBO (comboAlgorithm)->entry)));
  keysize =
    atoi (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (comboKeysize)->entry)));
  userID = gtk_entry_get_text (GTK_ENTRY (entryUserID));
  email = gtk_entry_get_text (GTK_ENTRY (entryEmail));
  comment = gtk_entry_get_text (GTK_ENTRY (entryComment));
  if (strcmp (gtk_entry_get_text (GTK_ENTRY (entryPasswd)),
	      gtk_entry_get_text (GTK_ENTRY (entryRepeat))) != 0)
    {
      gpa_window_error (_
			("In \"Password\" and \"Repeat Password\",\nyou must enter the same password."),
keeperGenerate->window);
      return;
    }				/* if */
  gpapa_create_key_pair (&publicKey, &secretKey,
			 gtk_entry_get_text (GTK_ENTRY (entryPasswd)),
			 algo, keysize, userID, email, comment,
			 gpa_callback, keeperGenerate->window);
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radioDont)))
    {
      gpapa_key_set_expiry_date (GPAPA_KEY (publicKey), NULL, gpa_callback,
				 keeperGenerate->window);
      gpapa_key_set_expiry_date (GPAPA_KEY (secretKey), NULL, gpa_callback,
				 keeperGenerate->window);
    }				/* if */
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radioAfter)))
    {
      i = atoi (gtk_entry_get_text (GTK_ENTRY (entryAfter)));
      unit =
	getTimeunitForString (gtk_entry_get_text
			      (GTK_ENTRY (GTK_COMBO (comboAfter)->entry)));
      gpapa_key_set_expiry_time (GPAPA_KEY (publicKey), i, unit, gpa_callback,
				 keeperGenerate->window);
      gpapa_key_set_expiry_time (GPAPA_KEY (secretKey), i, unit, gpa_callback,
				 keeperGenerate->window);
    }				/* else if */
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radioAt)))
    {
      expiryDate = g_date_new ();
      g_date_set_parse (expiryDate,
			gtk_entry_get_text (GTK_ENTRY (entryAfter)));
      gpapa_key_set_expiry_date (GPAPA_KEY (publicKey), expiryDate,
				 gpa_callback, keeperGenerate->window);
      gpapa_key_set_expiry_date (GPAPA_KEY (secretKey), expiryDate,
				 gpa_callback, keeperGenerate->window);
      free (expiryDate);
    }				/* if */
  else
    {
      gpa_window_error (_
			("!FATAL ERROR!\nInvalid insert mode for expiry date."),
keeperGenerate->window);
      return;
    }				/* else */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerRevoc)))
    gpapa_secret_key_create_revocation (secretKey, gpa_callback,
					keeperGenerate->window);
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkerSend)))
    gpapa_public_key_send_to_server (publicKey, global_keyserver,
				     gpa_callback, keeperGenerate->window);
  gpapa_key_release (dummyKey, gpa_callback, keeperGenerate->window);	/*!!! */
  free (publicKey);		/*!!! */
  free (secretKey);		/*!!! */
  paramDone[0] = keeperGenerate;
  paramDone[1] = NULL;
  gpa_window_destroy (paramDone);
}				/* keys_generateKey_generate */

void
keys_generateKey (void)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  GList *contentsAlgorithm = NULL;
  GpapaAlgo algo;
  GList *contentsKeysize = NULL;
  GDate **expiryDate = NULL;
  gpointer *paramSave;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowGenerate;
  GtkWidget *vboxGenerate;
  GtkWidget *tableTop;
  GtkWidget *labelJfdAlgorithm;
  GtkWidget *labelAlgorithm;
  GtkWidget *comboAlgorithm;
  GtkWidget *labelJfdKeysize;
  GtkWidget *labelKeysize;
  GtkWidget *comboKeysize;
  GtkWidget *frameExpire;
  GtkWidget *tableMisc;
  GtkWidget *labelJfdUserID;
  GtkWidget *labelUserID;
  GtkWidget *entryUserID;
  GtkWidget *labelJfdEmail;
  GtkWidget *labelEmail;
  GtkWidget *entryEmail;
  GtkWidget *labelJfdComment;
  GtkWidget *labelComment;
  GtkWidget *entryComment;
  GtkWidget *labelJfdPasswd;
  GtkWidget *labelPasswd;
  GtkWidget *entryPasswd;
  GtkWidget *labelJfdRepeat;
  GtkWidget *labelRepeat;
  GtkWidget *entryRepeat;
  GtkWidget *vboxMisc;
  GtkWidget *checkerRevoc;
  GtkWidget *checkerSend;
  GtkWidget *hButtonBoxGenerate;
  GtkWidget *buttonCancel;
  GtkWidget *buttonGenerate;
/* commands */
  keeper = gpa_windowKeeper_new ();
  windowGenerate = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowGenerate);
  gtk_window_set_title (GTK_WINDOW (windowGenerate), _("Generate key"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowGenerate), accelGroup);
  vboxGenerate = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxGenerate), 5);
  tableTop = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (tableTop), 5);
  labelAlgorithm = gtk_label_new (_(""));
  labelJfdAlgorithm =
    gpa_widget_hjustified_new (labelAlgorithm, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableTop), labelJfdAlgorithm, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  comboAlgorithm = gtk_combo_new ();
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (comboAlgorithm)->entry),
			     FALSE);
  for (algo = GPAPA_ALGO_FIRST; algo <= GPAPA_ALGO_LAST; algo++)
    contentsAlgorithm =
      g_list_append (contentsAlgorithm, writtenAlgorithm[algo]);
  gtk_combo_set_popdown_strings (GTK_COMBO (comboAlgorithm),
				 contentsAlgorithm);
  gpa_connect_by_accelerator (GTK_LABEL (labelAlgorithm),
			      GTK_COMBO (comboAlgorithm)->entry, accelGroup,
			      _("_Encryption algorithm: "));
  gtk_table_attach (GTK_TABLE (tableTop), comboAlgorithm, 1, 2, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  labelKeysize = gtk_label_new (_(""));
  labelJfdKeysize =
    gpa_widget_hjustified_new (labelKeysize, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableTop), labelJfdKeysize, 0, 1, 1, 2,
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
  gtk_box_pack_start (GTK_BOX (vboxGenerate), tableTop, FALSE, FALSE, 0);
  expiryDate = (GDate **) xmalloc (sizeof (GDate *));
  gpa_windowKeeper_add_param (keeper, expiryDate);
  *expiryDate = NULL;
  paramSave = (gpointer *) xmalloc (16 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSave);
  frameExpire =
    gpa_frameExpire_new (accelGroup, expiryDate, keeper, paramSave);
  gtk_container_set_border_width (GTK_CONTAINER (frameExpire), 5);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), frameExpire, FALSE, FALSE, 0);
  tableMisc = gtk_table_new (5, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (tableMisc), 5);
  labelUserID = gtk_label_new (_(""));
  labelJfdUserID = gpa_widget_hjustified_new (labelUserID, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableMisc), labelJfdUserID, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryUserID = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelUserID), entryUserID,
			      accelGroup, _("_User ID: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryUserID, 1, 2, 0, 1, GTK_FILL,
		    GTK_SHRINK, 0, 0);
  labelEmail = gtk_label_new (_(""));
  labelJfdEmail = gpa_widget_hjustified_new (labelEmail, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableMisc), labelJfdEmail, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryEmail = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelEmail), entryEmail, accelGroup,
			      _("E-_Mail: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryEmail, 1, 2, 1, 2, GTK_FILL,
		    GTK_SHRINK, 0, 0);
  labelComment = gtk_label_new (_(""));
  labelJfdComment =
    gpa_widget_hjustified_new (labelComment, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableMisc), labelJfdComment, 0, 1, 2, 3,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryComment = gtk_entry_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelComment), entryComment,
			      accelGroup, _("C_omment: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryComment, 1, 2, 2, 3, GTK_FILL,
		    GTK_SHRINK, 0, 0);
  labelPasswd = gtk_label_new (_(""));
  labelJfdPasswd = gpa_widget_hjustified_new (labelPasswd, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableMisc), labelJfdPasswd, 0, 1, 3, 4,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryPasswd = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entryPasswd), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelPasswd), entryPasswd,
			      accelGroup, _("_Password: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryPasswd, 1, 2, 3, 4, GTK_FILL,
		    GTK_SHRINK, 0, 0);
  labelRepeat = gtk_label_new (_(""));
  labelJfdRepeat = gpa_widget_hjustified_new (labelRepeat, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableMisc), labelJfdRepeat, 0, 1, 4, 5,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryRepeat = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entryRepeat), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelRepeat), entryRepeat,
			      accelGroup, _("_Repeat password: "));
  gtk_table_attach (GTK_TABLE (tableMisc), entryRepeat, 1, 2, 4, 5,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), tableMisc, TRUE, FALSE, 0);
  vboxMisc = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxMisc), 5);
  checkerRevoc =
    gpa_check_button_new (accelGroup, _("generate re_vocation certificate"));
  gtk_box_pack_start (GTK_BOX (vboxMisc), checkerRevoc, FALSE, FALSE, 0);
  checkerSend = gpa_check_button_new (accelGroup, _("_send to key server"));
  gtk_box_pack_start (GTK_BOX (vboxMisc), checkerSend, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), vboxMisc, FALSE, FALSE, 0);
  hButtonBoxGenerate = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxGenerate),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxGenerate), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxGenerate), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = NULL;
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxGenerate), buttonCancel);
  buttonGenerate = gpa_button_new (accelGroup, _("_Generate key"));
  paramSave[6] = comboAlgorithm;
  paramSave[7] = comboKeysize;
  paramSave[8] = entryUserID;
  paramSave[9] = entryEmail;
  paramSave[10] = entryComment;
  paramSave[11] = entryPasswd;
  paramSave[12] = entryRepeat;
  paramSave[13] = checkerRevoc;
  paramSave[14] = checkerSend;
  paramSave[15] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (buttonGenerate), "clicked",
			     GTK_SIGNAL_FUNC (keys_generateKey_generate),
			     (gpointer) paramSave);
  gtk_container_add (GTK_CONTAINER (hButtonBoxGenerate), buttonGenerate);
  gtk_box_pack_start (GTK_BOX (vboxGenerate), hButtonBoxGenerate, FALSE,
		      FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowGenerate), vboxGenerate);
  gpa_widget_show (windowGenerate, global_windowMain,
		   _("keys_generateKey.tip"));
}				/* keys_generateKey */

void
keys_generateRevocation_generate_exec (gpointer data, gpointer userData)
{
/* var */
  gchar *keyID;
  GtkWidget *windowGenerate;
  GpapaSecretKey *key;
/* commands */
  keyID = (gchar *) data;
  windowGenerate = (GtkWidget *) userData;
  key = gpapa_get_secret_key_by_ID (keyID, gpa_callback, windowGenerate);
  gpapa_secret_key_create_revocation (key, gpa_callback, windowGenerate);
}				/* keys_generateRevocation_generate_exec */

void
keys_generateRevocation_generate (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **keysSelected;
  GpaWindowKeeper *keeperGenerate;
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  keeperGenerate = (GpaWindowKeeper *) localParam[1];
  if (!*keysSelected)
    {
      gpa_window_error (_
			("No keys selected to create revocation certificate for."),
keeperGenerate->window);
      return;
    }				/* if */
  g_list_foreach (*keysSelected,
		  keys_generateRevocation_generate_exec,
		  keeperGenerate->window);
  keys_ringEditor_close (param);
}				/* keys_generateRevocation_generate */

void
keys_generateRevocation (void)
{
/* var */
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gint contentsCountKeys;
  gchar *titlesKeys[2] = { N_("User identity / role"), N_("Key ID") };
  gint i;
  gchar *contentsKeys[2];
  GpapaSecretKey *key;
  GList **keysSelected = NULL;
  static gint columnKeyID = 1;
  gpointer *paramKeys;
  gpointer *paramGenerate;
  gpointer *paramClose;
/* objects */
  GtkWidget *windowRevoc;
  GtkWidget *vboxRevoc;
  GtkWidget *vboxKeys;
  GtkWidget *labelJfdKeys;
  GtkWidget *labelKeys;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;
  GtkWidget *hButtonBoxRevoc;
  GtkWidget *buttonCancel;
  GtkWidget *buttonGenerate;
/* commands */
  contentsCountKeys =
    gpapa_get_secret_key_count (gpa_callback, global_windowMain);
  if (!contentsCountKeys)
    gpa_window_error (_("No secret keys available yet."), global_windowMain);
  keeper = gpa_windowKeeper_new ();
  windowRevoc = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowRevoc);
  gtk_window_set_title (GTK_WINDOW (windowRevoc),
			_("Generate revocation certificate"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowRevoc), accelGroup);
  vboxRevoc = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxRevoc), 5);
  vboxKeys = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxKeys), 5);
  labelKeys = gtk_label_new (_(""));
  labelJfdKeys = gpa_widget_hjustified_new (labelKeys, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxKeys), labelJfdKeys, FALSE, FALSE, 0);
  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerKeys, 280, 200);
  clistKeys = gtk_clist_new_with_titles (2, titlesKeys);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 0, 180);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 1, 120);
  for (i = 0; i < 2; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistKeys), i);
  gtk_clist_set_selection_mode (GTK_CLIST (clistKeys),
				GTK_SELECTION_EXTENDED);
  while (contentsCountKeys)
    {
      contentsCountKeys--;
      key =
	gpapa_get_secret_key_by_index (contentsCountKeys, gpa_callback,
				       global_windowMain);
      contentsKeys[0] =
	gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, global_windowMain);
      contentsKeys[1] =
	gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
				  global_windowMain);
      gtk_clist_prepend (GTK_CLIST (clistKeys), contentsKeys);
    }				/* while */
  keysSelected = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, keysSelected);
  *keysSelected = NULL;
  paramKeys = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramKeys);
  paramKeys[0] = keysSelected;
  paramKeys[1] = &columnKeyID;
  paramKeys[2] = windowRevoc;
  gtk_signal_connect (GTK_OBJECT (clistKeys), "select-row",
		      GTK_SIGNAL_FUNC (keys_selectKey), (gpointer) paramKeys);
  gtk_signal_connect (GTK_OBJECT (clistKeys), "unselect-row",
		      GTK_SIGNAL_FUNC (keys_unselectKey),
		      (gpointer) paramKeys);
  gpa_connect_by_accelerator (GTK_LABEL (labelKeys), clistKeys, accelGroup,
			      _("_Secret keys"));
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gtk_box_pack_start (GTK_BOX (vboxKeys), scrollerKeys, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxRevoc), vboxKeys, TRUE, TRUE, 0);
  hButtonBoxRevoc = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxRevoc),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxRevoc), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxRevoc), 5);
  buttonCancel = gpa_button_new (accelGroup, _("_Cancel"));
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keysSelected;
  paramClose[1] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (buttonCancel), "clicked",
			     GTK_SIGNAL_FUNC (keys_ringEditor_close),
			     (gpointer) paramClose);
  gtk_widget_add_accelerator (buttonCancel, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  gtk_container_add (GTK_CONTAINER (hButtonBoxRevoc), buttonCancel);
  buttonGenerate = gpa_button_new (accelGroup, _("_Generate"));
  paramGenerate = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramGenerate);
  paramGenerate[0] = keysSelected;
  paramGenerate[1] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (buttonGenerate), "clicked",
			     GTK_SIGNAL_FUNC
			     (keys_generateRevocation_generate),
			     (gpointer) paramGenerate);
  gtk_container_add (GTK_CONTAINER (hButtonBoxRevoc), buttonGenerate);
  gtk_box_pack_start (GTK_BOX (vboxRevoc), hButtonBoxRevoc, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowRevoc), vboxRevoc);
  gpa_widget_show (windowRevoc, global_windowMain,
		   _("keys_generateRevocation.tip"));
}				/* keys_generateRevocation */

void
keys_import_ok (GpaWindowKeeper * keeperImport)
{
/* var */
  gchar *fileID;
  gpointer paramDone[2];
/* commands */
  fileID =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION
				     (keeperImport->window));
  gpapa_import_keys (fileID, gpa_callback, global_windowMain);
  paramDone[0] = keeperImport;
  paramDone[1] = NULL;
  gpa_window_destroy (paramDone);
}				/* keys_import_ok */

void
keys_import (void)
{
/* objects */
  GpaWindowKeeper *keeper;
  GtkWidget *windowImport;
  gpointer *paramClose;
/* commands */
  keeper = gpa_windowKeeper_new ();
  windowImport = gtk_file_selection_new (_("Import keys"));
  gpa_windowKeeper_set_window (keeper, windowImport);
  gtk_signal_connect_object (GTK_OBJECT
			     (GTK_FILE_SELECTION (windowImport)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (keys_import_ok),
			     (gpointer) keeper);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = NULL;
  gtk_signal_connect_object (GTK_OBJECT
			     (GTK_FILE_SELECTION (windowImport)->
			      cancel_button), "clicked",
			     GTK_SIGNAL_FUNC (gpa_window_destroy),
			     (gpointer) paramClose);
  gpa_widget_show (windowImport, global_windowMain, _("keys_import.tip"));
}				/* keys_import */

void
keys_importOwnertrust_ok (GpaWindowKeeper * keeperImport)
{
/* var */
  gchar *fileID;
  gpointer paramDone[2];
/* commands */
  fileID =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION
				     (keeperImport->window));
  gpapa_import_ownertrust (fileID, gpa_callback, global_windowMain);
  paramDone[0] = keeperImport;
  paramDone[1] = NULL;
  gpa_window_destroy (paramDone);
}				/* keys_importOwnertrust_ok */

void
keys_importOwnertrust (void)
{
/* objects */
  GpaWindowKeeper *keeper;
  GtkWidget *windowImport;
  gpointer *paramClose;
/* commands */
  keeper = gpa_windowKeeper_new ();
  windowImport = gtk_file_selection_new (_("Import ownertrust"));
  gpa_windowKeeper_set_window (keeper, windowImport);
  gtk_signal_connect_object (GTK_OBJECT
			     (GTK_FILE_SELECTION (windowImport)->ok_button),
			     "clicked",
			     GTK_SIGNAL_FUNC (keys_importOwnertrust_ok),
			     (gpointer) keeper);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = NULL;
  gtk_signal_connect_object (GTK_OBJECT
			     (GTK_FILE_SELECTION (windowImport)->
			      cancel_button), "clicked",
			     GTK_SIGNAL_FUNC (gpa_window_destroy),
			     (gpointer) paramClose);
  gpa_widget_show (windowImport, global_windowMain,
		   _("keys_importOwnertrust.tip"));
}				/* keys_importOwnertrust */

void
keys_updateTrust (void)
{
  gpapa_update_trust_database (gpa_callback, global_windowMain);
  gpa_window_message (_("Trust database updated."), global_windowMain);
}				/* keys_updateTrust */
