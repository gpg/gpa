/* keysmenu.c  -  The GNU Privacy Assistant
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
#include <gpapa.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include "filemenu.h"
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gpapastrings.h"
#include "gtktools.h"
#include "icons.h"

gchar *unitExpiryTime[4] = {
  N_("days"),
  N_("weeks"),
  N_("months"),
  N_("years")
};

gchar unitTime[4] = { 'd', 'w', 'm', 'y' };


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
  gtk_entry_set_text (GTK_ENTRY (entryAfter), "");
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboAfter)->entry),
		      unitExpiryTime[0]);
  gtk_entry_set_text (GTK_ENTRY (entryAt), "");
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
  gtk_entry_set_text (GTK_ENTRY (entryAt), "");
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
  gchar *dateBuffer;
/* commands */
  if (!gtk_toggle_button_get_active (radioAt))
    return;
  localParam = (gpointer *) param;
  expiryDate = (GDate **) localParam[0];
  entryAfter = (GtkWidget *) localParam[1];
  comboAfter = (GtkWidget *) localParam[2];
  entryAt = (GtkWidget *) localParam[3];
  window = (GtkWidget *) localParam[4];
  gtk_entry_set_text (GTK_ENTRY (entryAfter), "");
  if (*expiryDate)
    {
      dateBuffer = gpa_expiry_date_string (*expiryDate);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboAfter)->entry),
			  unitExpiryTime[0]);
      gtk_entry_set_text (GTK_ENTRY (entryAt), dateBuffer);
      free (dateBuffer);
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
  gchar *dateBuffer;
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
      dateBuffer = gpa_expiry_date_string (*expiryDate);
      gtk_entry_set_text (GTK_ENTRY (entryAt), dateBuffer);
      free (dateBuffer);
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
  labelFilename = gtk_label_new ("");
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
  gpa_window_show_centered (windowExport, parent);
  gtk_widget_grab_focus (entryFilename);
}				/* keys_export_dialog */







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
  gpa_window_show_centered (selectTrust, windowPublic);
}				/* keys_openPublic_exportTrust */




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
    gpapa_key_set_expiry_date (GPAPA_KEY (key), NULL, "", gpa_callback,
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
      gpapa_key_set_expiry_date (GPAPA_KEY (key), date, "", gpa_callback,
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
  labelPasswd = gtk_label_new ("");
  labelJfdPasswd = gpa_widget_hjustified_new (labelPasswd, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tablePasswd), labelJfdPasswd, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryPasswd = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entryPasswd), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelPasswd), entryPasswd,
			      accelGroup, _("_Password: "));
  gtk_table_attach (GTK_TABLE (tablePasswd), entryPasswd, 1, 2, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  labelRepeat = gtk_label_new ("");
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
  gpa_window_show_centered (windowEdit, windowSecret);
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
  labelRingname = gtk_label_new ("");
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
  gpa_window_show_centered (windowSecret, global_windowMain);
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
  labelKeys = gtk_label_new ("");
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
  gpa_window_show_centered (windowRevoc, global_windowMain);
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
  gpa_window_show_centered (windowImport, global_windowMain);
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
  gpa_window_show_centered (windowImport, global_windowMain);
}				/* keys_importOwnertrust */

void
keys_updateTrust (void)
{
  gpapa_update_trust_database (gpa_callback, global_windowMain);
  gpa_window_message (_("Trust database updated."), global_windowMain);
}				/* keys_updateTrust */
