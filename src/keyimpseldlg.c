/* keyimpseldlg.c - The GNU Privacy Assistant - Key import selection dialog
 * Copyright (C) 2002 G-N-U GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY - without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GPA. If not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "keyimpseldlg.h"

typedef struct _GPAKeyImportSelectionDialog
{
  GtkWidget *window;
  const gchar *keyserver;
  GtkCList *clist_which;
  GList *keys, *imported_keys;
}
GPAKeyImportSelectionDialog;

static void
key_import_selection_do_import (GPAKeyImportSelectionDialog *dialog)
{
  if (dialog->clist_which->selection)
    {
      gint row = GPOINTER_TO_INT (dialog->clist_which->selection->data);
      gchar *key_id = gtk_clist_get_row_data (dialog->clist_which, row);
      gchar *full_key_id = g_strconcat ("0x", key_id, NULL);
#if 0
      /* FIXME: Reimplement this when gpgme supports searches */
      gpapa_receive_public_key_from_server (full_key_id, dialog->keyserver,
		                            gpa_callback, dialog->window);
#endif
      g_free (full_key_id);
    }
}

static void
key_import_selection_ok (gpointer param)
{
  GPAKeyImportSelectionDialog *dialog = param;

  if (!dialog->clist_which->selection)
    {
      gpa_window_error (_("No keys selected"), dialog->window);
      return;
    }
  key_import_selection_do_import (dialog);
  gtk_widget_destroy (dialog->window);
}

static void
key_import_selection_cancel (gpointer param)
{
  GPAKeyImportSelectionDialog *dialog = param;

  dialog->imported_keys = NULL;
  gtk_widget_destroy (dialog->window);
}

static void
key_import_selection_destroy (GtkWidget *widget, gpointer param)
{
  gtk_main_quit ();
}

/* Run the key import selection dialog and import the selected keys
 * from the given keyserver.
 */
void
gpa_key_import_selection_dialog_run (GtkWidget *parent,
                                     GList *keys, const gchar *keyserver)
{
  GPAKeyImportSelectionDialog dialog;
  GtkAccelGroup *accelGroup;
  GtkWidget *windowSelect;
  GtkWidget *vboxSelect;
  GtkWidget *vboxWhich;
  GtkWidget *labelWhich;
  GtkWidget *scrollerWhich;
  GtkWidget *clistWhich;
  GtkWidget *hButtonBoxSelect;
  GtkWidget *buttonCancel;
  GtkWidget *buttonSelect;

  dialog.keys = keys;
  dialog.imported_keys = NULL;
  dialog.keyserver = keyserver;

  if (g_list_length (dialog.keys) == 0)
    {
      gpa_window_error (_("Keyserver did not return any matching keys."),
			parent);
      return;
    }

  windowSelect = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (windowSelect), _("Select keys to import"));
  dialog.window = windowSelect;
  gtk_signal_connect (GTK_OBJECT (windowSelect), "destroy",
		      GTK_SIGNAL_FUNC (key_import_selection_destroy), NULL);

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowSelect), accelGroup);

  vboxSelect = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxSelect), 5);

  vboxWhich = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxWhich), 5);
  gtk_box_pack_start (GTK_BOX (vboxSelect), vboxWhich, TRUE, TRUE, 0);

  labelWhich = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelWhich), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vboxWhich), labelWhich, FALSE, TRUE, 0);

  scrollerWhich = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerWhich, 260, 75);
  gtk_box_pack_start (GTK_BOX (vboxWhich), scrollerWhich, TRUE, TRUE, 0);

  clistWhich = gpa_key_list_new_from_glist (parent, dialog.keys);
  dialog.clist_which = GTK_CLIST (clistWhich);
  gtk_container_add (GTK_CONTAINER (scrollerWhich), clistWhich);
  gpa_connect_by_accelerator (GTK_LABEL (labelWhich), clistWhich, accelGroup,
			      _("_Import"));

  hButtonBoxSelect = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vboxSelect), hButtonBoxSelect, FALSE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxSelect),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxSelect), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxSelect), 5);
  buttonSelect = gpa_button_new (accelGroup, "_OK");
  gtk_signal_connect_object (GTK_OBJECT (buttonSelect), "clicked",
			     GTK_SIGNAL_FUNC (key_import_selection_ok),
			     (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSelect), buttonSelect);

  buttonCancel = gpa_button_cancel_new (accelGroup, _("_Cancel"),
					(GtkSignalFunc) key_import_selection_cancel, (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSelect), buttonCancel);
  gtk_container_add (GTK_CONTAINER (windowSelect), vboxSelect);
  gpa_window_show_centered (windowSelect, parent);
  gtk_window_set_modal (GTK_WINDOW (windowSelect), TRUE);

  gtk_main ();
}
