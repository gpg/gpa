/* keyring.c  -	 The GNU Privacy Assistant
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
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
#include "gpa.h"
#include "gpawidgets.h"
#include "gtktools.h"
#include "gpapastrings.h"

/*
 *	Some GUI construction functions
 */

/* FIXME: this function should be in a separate module */
extern GtkWidget *
gpa_tableKey_new (GpgmeKey key, GtkWidget * window);



/*
 *	Owner Trust dialog
 */

/* Run the owner trust dialog modally. */
gboolean
gpa_ownertrust_run_dialog (GpgmeKey key, GtkWidget *parent,
			   GpgmeValidity * trust)
{
  GtkAccelGroup *accelGroup;
  GList *valueLevel = NULL;
  GtkWidget *windowTrust;
  GtkWidget *vboxTrust;
  GtkWidget *tableKey;
  GtkWidget *hboxLevel;
  GtkWidget *labelLevel;
  GtkWidget *comboLevel;

  windowTrust = gtk_dialog_new_with_buttons (_("Change key ownertrust"),
					     GTK_WINDOW (parent), 
					     GTK_DIALOG_MODAL, 
					     "_Set", GTK_RESPONSE_OK,
					     GTK_STOCK_CANCEL, 
					     GTK_RESPONSE_CANCEL, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (windowTrust), GTK_RESPONSE_OK);
  gtk_container_set_border_width (GTK_CONTAINER (windowTrust), 5);
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowTrust), accelGroup);

  vboxTrust = GTK_DIALOG (windowTrust)->vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vboxTrust), 5);

  tableKey = gpa_key_info_new (key, windowTrust);
  gtk_container_set_border_width (GTK_CONTAINER (tableKey), 5);
  gtk_box_pack_start (GTK_BOX (vboxTrust), tableKey, FALSE, FALSE, 0);

  hboxLevel = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxLevel), 5);

  labelLevel = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hboxLevel), labelLevel, FALSE, FALSE, 0);

  comboLevel = gtk_combo_new ();
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (comboLevel)->entry),
			     FALSE);
  /* Not all values are used, and we can't be sure we can iterate over them,
   * so we hardcode the validity values */
  valueLevel = g_list_append (valueLevel, 
                              gpa_trust_string (GPGME_VALIDITY_UNDEFINED));
  valueLevel = g_list_append (valueLevel, 
                              gpa_trust_string (GPGME_VALIDITY_NEVER));
  valueLevel = g_list_append (valueLevel, 
                              gpa_trust_string (GPGME_VALIDITY_MARGINAL));
  valueLevel = g_list_append (valueLevel, 
                              gpa_trust_string (GPGME_VALIDITY_FULL));
  gtk_combo_set_popdown_strings (GTK_COMBO (comboLevel), valueLevel);
  gpa_connect_by_accelerator (GTK_LABEL (labelLevel),
			      GTK_COMBO (comboLevel)->entry, accelGroup,
			      _("_Ownertrust level: "));
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboLevel)->entry),
		      gpa_trust_string (gpgme_key_get_ulong_attr
					(key, GPGME_ATTR_VALIDITY, NULL,0)));
  gtk_box_pack_start (GTK_BOX (hboxLevel), comboLevel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxTrust), hboxLevel, TRUE, TRUE, 0);

  gtk_widget_show_all (windowTrust);
  if (gtk_dialog_run (GTK_DIALOG (windowTrust)) == GTK_RESPONSE_OK)
    {
      gchar *trust_text;
      
      trust_text = (gchar *) gtk_entry_get_text
	(GTK_ENTRY(GTK_COMBO(comboLevel)->entry));
      *trust = gpa_ownertrust_from_string (trust_text);
      gtk_widget_destroy (windowTrust);
      return TRUE;
    }
  else
    {
      gtk_widget_destroy (windowTrust);
      return FALSE;
    }
} /* gpa_ownertrust_run_dialog */
