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
#include <gpgme.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include "filemenu.h"
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gpafilesel.h"
#include "gpapastrings.h"
#include "gtktools.h"
#include "icons.h"
#include "keyserver.h"


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
  gchar result = ' ';
  gint i;

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
gpa_tableKey_new (GpgmeKey key, GtkWidget * window)
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
  labelKeyID = gtk_label_new (_("Key ID:"));
  labelJfdKeyID = gpa_widget_hjustified_new (labelKeyID, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableKey), labelJfdKeyID, 0, 1, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryKeyID = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entryKeyID),
                      gpa_gpgme_key_get_short_keyid (key, 0));
  gtk_editable_set_editable (GTK_EDITABLE (entryKeyID), FALSE);
  gtk_table_attach (GTK_TABLE (tableKey), entryKeyID, 1, 2, 1, 2,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  labelKeyname = gtk_label_new (_("User Name:"));
  labelJfdKeyname =
    gpa_widget_hjustified_new (labelKeyname, GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (tableKey), labelJfdKeyname, 0, 1, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entryKeyname = gtk_entry_new ();
  contentsKeyname = gpa_gpgme_key_get_userid (key, 0);
  gtk_widget_ensure_style (entryKeyname);
  gtk_widget_set_usize (entryKeyname,
    gdk_string_width (gtk_style_get_font (entryKeyname->style),
                      contentsKeyname)
    + gdk_string_width (gtk_style_get_font (entryKeyname->style), "  ")
    + my_gtk_style_get_xthickness (entryKeyname->style), 0);
  gtk_entry_set_text (GTK_ENTRY (entryKeyname), contentsKeyname);
  g_free (contentsKeyname);
  gtk_editable_set_editable (GTK_EDITABLE (entryKeyname), FALSE);
  gtk_table_attach (GTK_TABLE (tableKey), entryKeyname, 1, 2, 0, 1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  return (tableKey);
}				/* gpa_tableKey_new */

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
