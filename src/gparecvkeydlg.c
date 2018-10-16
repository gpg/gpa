/* gparecvkeydlg.c  -  The GNU Privacy Assistant
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

#include <string.h>
#include <errno.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gpa.h"
#include "gparecvkeydlg.h"
#include "gtktools.h"


/* Properties.  */
enum
{
  PROP_0,
  PROP_WINDOW,
};

static GObjectClass *parent_class = NULL;

static void
gpa_receive_key_dialog_get_property (GObject *object, guint prop_id,
				     GValue *value, GParamSpec *pspec)
{
  GpaReceiveKeyDialog *dialog = GPA_RECEIVE_KEY_DIALOG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value,
			  gtk_window_get_transient_for (GTK_WINDOW (dialog)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_receive_key_dialog_set_property (GObject *object, guint prop_id,
				     const GValue *value, GParamSpec *pspec)
{
  GpaReceiveKeyDialog *dialog = GPA_RECEIVE_KEY_DIALOG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      gtk_window_set_transient_for (GTK_WINDOW (dialog),
				    g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_receive_key_dialog_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_receive_key_dialog_init (GpaReceiveKeyDialog *dialog)
{
  GtkWidget *label;
  GtkWidget *hbox;

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			  GTK_STOCK_OK, GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),10);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  label = gtk_label_new (is_gpg_version_at_least ("2.1.0")?
                         _("Which key do you want to import?") :
                         _("Which key do you want to import? (The key must "
			   "be specified by key ID)."));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, FALSE,
		      TRUE, 10);

  dialog->entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (dialog->entry), TRUE);
  if (is_gpg_version_at_least ("2.1.0"))
    {
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                          dialog->entry, FALSE, TRUE, 10);
    }
  else
    {
      hbox = gtk_hbox_new (0, FALSE);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE,
                          TRUE, 10);
      label = gtk_label_new_with_mnemonic (_("Key _ID:"));
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->entry);
      gtk_box_pack_start_defaults (GTK_BOX (hbox), label);
      gtk_box_pack_start_defaults (GTK_BOX (hbox), dialog->entry);
    }

}


static void
gpa_receive_key_dialog_class_init (GpaReceiveKeyDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gpa_receive_key_dialog_finalize;
  object_class->set_property = gpa_receive_key_dialog_set_property;
  object_class->get_property = gpa_receive_key_dialog_get_property;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_WINDOW,
				   g_param_spec_object
				   ("window", "Parent window",
				    "Parent window", GTK_TYPE_WIDGET,
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}


GType
gpa_receive_key_dialog_get_type (void)
{
  static GType verify_dialog_type = 0;

  if (!verify_dialog_type)
    {
      static const GTypeInfo verify_dialog_info =
	{
	  sizeof (GpaReceiveKeyDialogClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gpa_receive_key_dialog_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (GpaReceiveKeyDialog),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) gpa_receive_key_dialog_init,
	};

      verify_dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
						    "GpaReceiveKeyDialog",
						    &verify_dialog_info, 0);
    }

  return verify_dialog_type;
}

/* API */

/* Create a new receive key dialog.  */
GtkWidget*
gpa_receive_key_dialog_new (GtkWidget *parent)
{
  GpaReceiveKeyDialog *dialog;

  dialog = g_object_new (GPA_RECEIVE_KEY_DIALOG_TYPE,
			 "window", parent, NULL);

  return GTK_WIDGET(dialog);
}

/* Retrieve the selected key ID.  */
const gchar*
gpa_receive_key_dialog_get_id (GpaReceiveKeyDialog *dialog)
{
  return gtk_entry_get_text (GTK_ENTRY (dialog->entry));
}

