/* keyring.c  -	 The GNU Privacy Assistant
 *	Copyright (C) 2000 G-N-U GmbH.
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
#include <gpapa.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpapastrings.h"

/*
 *	Some GUI construction functions
 */

/* FIXME: this function should be in a separate module */
extern GtkWidget *
gpa_tableKey_new (GpapaKey * key, GtkWidget * window);



/*
 *	Owner Trust dialog
 */

struct _GPAOwnertrustDialog {
    GpapaOwnertrust trust;
    gboolean result;
    GtkWidget *combo;
    GtkWidget *window;
};
typedef struct _GPAOwnertrustDialog GPAOwnertrustDialog;


static void
ownertrust_ok (gpointer param)
{
  GPAOwnertrustDialog * dialog = param;
  GpapaOwnertrust trust;
  gchar * trust_text;

  dialog->result = TRUE;

  trust_text = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dialog->combo)->entry));
  trust = gpa_ownertrust_from_string (trust_text);
  if (trust > GPAPA_OWNERTRUST_LAST)
    {
      gpa_window_error (_("Invalid ownertrust level."), dialog->window);
      return;
    }				/* if */
  dialog->trust = trust;
  gtk_main_quit ();
} /* ownertrust_ok */

static void
ownertrust_cancel (gpointer param)
{
  GPAOwnertrustDialog * dialog = param;

  dialog->result = FALSE;
  gtk_main_quit ();
} /* ownertrust_cancel */

static gboolean
ownertrust_delete_event (GtkWidget *widget, GdkEvent *event, gpointer param)
{
  ownertrust_cancel (param);
  return TRUE;
} /* ownertrust_delete_event */


gboolean
gpa_ownertrust_run_dialog (GpapaPublicKey *key, GtkWidget *parent, gchar* tip,
			   GpapaOwnertrust * trust)
{
  GtkAccelGroup *accelGroup;
  GList *valueLevel = NULL;
  GpapaOwnertrust ownertrust;
  GtkWidget *windowTrust;
  GtkWidget *vboxTrust;
  GtkWidget *tableKey;
  GtkWidget *hboxLevel;
  GtkWidget *labelLevel;
  GtkWidget *comboLevel;
  GtkWidget *hButtonBoxTrust;
  GtkWidget *buttonCancel;
  GtkWidget *buttonAccept;
  GPAOwnertrustDialog dialog;

  dialog.trust = gpapa_public_key_get_ownertrust (key, gpa_callback, parent);
  dialog.result = FALSE;

  windowTrust = dialog.window = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (windowTrust), _("Change key ownertrust"));
  gtk_signal_connect_object (GTK_OBJECT (windowTrust), "delete_event",
			     GTK_SIGNAL_FUNC (ownertrust_delete_event),
			     (gpointer) &dialog);

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowTrust), accelGroup);

  vboxTrust = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxTrust), 5);

  tableKey = gpa_tableKey_new (GPAPA_KEY (key), windowTrust);
  gtk_container_set_border_width (GTK_CONTAINER (tableKey), 5);
  gtk_box_pack_start (GTK_BOX (vboxTrust), tableKey, FALSE, FALSE, 0);

  hboxLevel = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxLevel), 5);

  labelLevel = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hboxLevel), labelLevel, FALSE, FALSE, 0);

  comboLevel = dialog.combo = gtk_combo_new ();
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (comboLevel)->entry),
			     FALSE);
  for (ownertrust = GPAPA_OWNERTRUST_FIRST;
       ownertrust <= GPAPA_OWNERTRUST_LAST; ownertrust++)
    valueLevel =
      g_list_append (valueLevel, gpa_ownertrust_string (ownertrust));
  gtk_combo_set_popdown_strings (GTK_COMBO (comboLevel), valueLevel);
  gpa_connect_by_accelerator (GTK_LABEL (labelLevel),
			      GTK_COMBO (comboLevel)->entry, accelGroup,
			      _("_Ownertrust level: "));
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboLevel)->entry),
		      gpa_ownertrust_string (dialog.trust));
  gtk_box_pack_start (GTK_BOX (hboxLevel), comboLevel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxTrust), hboxLevel, TRUE, TRUE, 0);

  hButtonBoxTrust = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxTrust),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxTrust), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxTrust), 5);

  buttonCancel = gpa_button_cancel_new (accelGroup, _("_Cancel"),
					ownertrust_cancel, &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxTrust), buttonCancel);
  buttonAccept = gpa_button_new (accelGroup, _("_Accept"));
  gtk_signal_connect_object (GTK_OBJECT (buttonAccept), "clicked",
			     GTK_SIGNAL_FUNC (ownertrust_ok),
			     (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxTrust), buttonAccept);
  gtk_box_pack_start (GTK_BOX (vboxTrust), hButtonBoxTrust, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowTrust), vboxTrust);
  gpa_widget_show (windowTrust, parent, _("keyring_openPublic_editTrust.tip"));

  gtk_grab_add (windowTrust);
  gtk_main ();
  gtk_grab_remove (windowTrust);
  gtk_widget_destroy (windowTrust);

  if (dialog.result)
    *trust = dialog.trust;

  return dialog.result;
}				/* gpa_ownertrust_run_dialog */
