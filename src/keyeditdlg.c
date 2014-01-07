/* keyeditdlg.c  -  The GNU Privacy Assistant
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2003 Miguel Coca.
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA

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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <time.h>

#include "gpa.h"
#include "keyeditdlg.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "convert.h"
#include "keytable.h"
#include "gpakeyexpireop.h"
#include "gpakeypasswdop.h"

/* Signals */
enum
{
  KEY_MODIFIED,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_KEY,
};

static GObjectClass *parent_class = NULL;


/* internal API */
static void gpa_key_edit_change_expiry (GtkWidget *widget,
					GpaKeyEditDialog *dialog);
static void gpa_key_edit_change_passphrase (GtkWidget *widget,
					    GpaKeyEditDialog *dialog);

static void
gpa_key_edit_dialog_get_property (GObject     *object,
				      guint        prop_id,
				      GValue      *value,
				      GParamSpec  *pspec)
{
  GpaKeyEditDialog *dialog = GPA_KEY_EDIT_DIALOG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value,
			  gtk_window_get_transient_for (GTK_WINDOW (dialog)));
      break;
    case PROP_KEY:
      g_value_set_pointer (value, dialog->key);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_key_edit_dialog_set_property (GObject     *object,
				   guint        prop_id,
				   const GValue      *value,
				   GParamSpec  *pspec)
{
  GpaKeyEditDialog *dialog = GPA_KEY_EDIT_DIALOG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      gtk_window_set_transient_for (GTK_WINDOW (dialog),
				    g_value_get_object (value));
      break;
    case PROP_KEY:
      dialog->key = (gpgme_key_t) g_value_get_pointer (value);
      gpgme_key_ref (dialog->key);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_key_edit_dialog_finalize (GObject *object)
{
  GpaKeyEditDialog *dialog = GPA_KEY_EDIT_DIALOG (object);

  if (dialog->key)
    {
      gpgme_key_unref (dialog->key);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject*
gpa_key_edit_dialog_constructor (GType                  type,
				 guint                  n_construct_properties,
				 GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaKeyEditDialog *dialog;

  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *table;

  gchar *date_string;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  dialog = GPA_KEY_EDIT_DIALOG (object);

  /* Initialize */
  gpa_window_set_title (GTK_WINDOW (dialog), _("Edit Key"));
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  _("_Close"),
			  GTK_RESPONSE_CLOSE,
			  NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  vbox = GTK_DIALOG (dialog)->vbox;

  /* info about the key */
  table = gpa_key_info_new (dialog->key);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);

  /* change passphrase */
  button = gtk_button_new_with_mnemonic (_("Change _passphrase"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (gpa_key_edit_change_passphrase), dialog);

  /* change expiry date */
  frame = gtk_frame_new (_("Expiry Date"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 5);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_set_spacing (GTK_BOX (hbox), 10);

  date_string = gpa_expiry_date_string (dialog->key->subkeys->expires);
  label = gtk_label_new (date_string);
  g_free (date_string);
  dialog->expiry = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  button = gtk_button_new_with_mnemonic (_("Change _expiration"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button,(gpa_keytable_lookup_key
			     (gpa_keytable_get_secret_instance(),
			      dialog->key->subkeys->fpr) != NULL));
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (gpa_key_edit_change_expiry), dialog);

  /* Close the dialog in response */
  g_signal_connect (G_OBJECT (dialog), "response",
		    G_CALLBACK (gtk_widget_destroy), dialog);

  return object;
}

static void
gpa_key_edit_dialog_init (GpaKeyEditDialog *dialog)
{
  dialog->key = NULL;
  dialog->expiry = NULL;
}

static void
gpa_key_edit_dialog_class_init (GpaKeyEditDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_key_edit_dialog_constructor;
  object_class->finalize = gpa_key_edit_dialog_finalize;
  object_class->set_property = gpa_key_edit_dialog_set_property;
  object_class->get_property = gpa_key_edit_dialog_get_property;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_WINDOW,
				   g_param_spec_object
				   ("window", "Parent window",
				    "Parent window", GTK_TYPE_WIDGET,
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_KEY,
				   g_param_spec_pointer
				   ("key", "Key",
				    "Key",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

  /* Signals */
  signals[KEY_MODIFIED] =
    g_signal_new ("key_modified",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GpaKeyEditDialogClass, key_modified),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
}

GType
gpa_key_edit_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (!dialog_type)
    {
      static const GTypeInfo dialog_info =
	{
	  sizeof (GpaKeyEditDialogClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gpa_key_edit_dialog_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (GpaKeyEditDialog),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) gpa_key_edit_dialog_init,
	};

      dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
					    "GpaKeyEditDialog",
					    &dialog_info, 0);
    }

  return dialog_type;
}

/* Internal functions */

static void
gpa_key_edit_dialog_new_expiration (GpaKeyOperation *op,
				    gpgme_key_t key,
				    GDate *new_date,
				    gpointer data)
{
  GpaKeyEditDialog *dialog = GPA_KEY_EDIT_DIALOG (data);
  struct tm tm;
  gchar *date_string;

  if (new_date)
    {
      g_date_to_struct_tm (new_date, &tm);
      date_string = gpa_expiry_date_string (mktime(&tm));
    }
  else
    {
      date_string = gpa_expiry_date_string (0);
    }
  gtk_label_set_text (GTK_LABEL (dialog->expiry), date_string);
  g_free (date_string);
}

static void
gpa_key_edit_dialog_changed_wot_cb (GpaKeyOperation *op, gpointer data)
{
  GpaKeyEditDialog *dialog = GPA_KEY_EDIT_DIALOG (data);

  g_signal_emit_by_name (dialog, "key_modified", dialog->key);
}

/* signal handler for the expiry date change button. */
static void
gpa_key_edit_change_expiry(GtkWidget * widget, GpaKeyEditDialog *dialog)
{
  /* All key operations want a list of key, so we give them one */
  GList *keys = g_list_append (NULL, dialog->key);
  GpaKeyExpireOperation *op =
    gpa_key_expire_operation_new (GTK_WIDGET (dialog), keys);

  g_signal_connect (G_OBJECT (op), "new_expiration",
		    G_CALLBACK (gpa_key_edit_dialog_new_expiration),
		    dialog);
  g_signal_connect (G_OBJECT (op), "changed_wot",
		    G_CALLBACK (gpa_key_edit_dialog_changed_wot_cb),
		    dialog);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), dialog);
}

/* signal handler for the change passphrase button. */
static void
gpa_key_edit_change_passphrase (GtkWidget *widget, GpaKeyEditDialog *dialog)
{
  /* All key operations want a list of key, so we give them one */
  GList *keys = g_list_append (NULL, dialog->key);
  GpaKeyPasswdOperation *op =
    gpa_key_passwd_operation_new (GTK_WIDGET (dialog), keys);

  g_signal_connect (G_OBJECT (op), "changed_wot",
		    G_CALLBACK (gpa_key_edit_dialog_changed_wot_cb),
		    dialog);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), dialog);
}

/* API */

/* Create a new "edit key" dialog.
 */
GtkWidget*
gpa_key_edit_dialog_new (GtkWidget *parent, gpgme_key_t key)
{
  GpaKeyEditDialog *dialog;

  dialog = g_object_new (GPA_KEY_EDIT_DIALOG_TYPE,
			 "window", parent,
			 "key", key, NULL);

  return GTK_WIDGET(dialog);
}

