/* gpabackupop.c - The GpaBackupOperation object.
   Copyright (C) 2003 Miguel Coca.
   Copyright (C) 2005 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GPA; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA  */

#include <config.h>

#include <gpgme.h>
#include "gpa.h"
#include "i18n.h"
#include "gtktools.h"
#include "gpabackupop.h"

static GObjectClass *parent_class = NULL;

static gboolean gpa_backup_operation_idle_cb (gpointer data);

/* GObject boilerplate.  */

/* Properties.  */
enum
{
  PROP_0,
  PROP_KEY,
  PROP_FINGERPRINT,
};

static void
gpa_backup_operation_get_property (GObject     *object,
				   guint        prop_id,
				   GValue      *value,
				   GParamSpec  *pspec)
{
  GpaBackupOperation *op = GPA_BACKUP_OPERATION (object);

  switch (prop_id)
    {
    case PROP_KEY:
      g_value_set_pointer (value, op->key);
      break;
    case PROP_FINGERPRINT:
      g_value_set_string (value, op->fpr);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_backup_operation_set_property (GObject     *object,
				   guint        prop_id,
				   const GValue      *value,
				   GParamSpec  *pspec)
{
  GpaBackupOperation *op = GPA_BACKUP_OPERATION (object);
  gchar *fpr;
  gpgme_key_t key;

  switch (prop_id)
    {
    case PROP_KEY:
      key = (gpgme_key_t) g_value_get_pointer (value);
      if (key)
	{
	  op->key = key;
	  gpgme_key_ref (op->key);
	  op->fpr = g_strdup (op->key->subkeys->fpr);
	  op->key_id = g_strdup (gpa_gpgme_key_get_short_keyid (op->key));
	}
      break;
    case PROP_FINGERPRINT:
      fpr = (gchar*) g_value_get_pointer (value);
      if (fpr)
	{
	  op->key = NULL;
	  op->fpr = g_strdup (fpr);
	  op->key_id = g_strdup (fpr + strlen (fpr) - 8);
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_backup_operation_finalize (GObject *object)
{
  GpaBackupOperation *op = GPA_BACKUP_OPERATION (object);

  if (op->key)
    {
      gpgme_key_unref (op->key);
    }
  g_free (op->fpr);
  g_free (op->key_id);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_backup_operation_init (GpaBackupOperation *op)
{
  op->key = NULL;
  op->fpr = NULL;
  op->key_id = NULL;
}

static GObject*
gpa_backup_operation_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaBackupOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_BACKUP_OPERATION (object);

  /* Begin working when we are back into the main loop */
  g_idle_add (gpa_backup_operation_idle_cb, op);

  return object;
}

static void
gpa_backup_operation_class_init (GpaBackupOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_backup_operation_constructor;
  object_class->finalize = gpa_backup_operation_finalize;
  object_class->set_property = gpa_backup_operation_set_property;
  object_class->get_property = gpa_backup_operation_get_property;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_KEY,
				   g_param_spec_pointer 
				   ("key", "Key",
				    "Key",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_FINGERPRINT,
				   g_param_spec_pointer 
				   ("fpr", "fpr",
				    "Fingerprint",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}

GType
gpa_backup_operation_get_type (void)
{
  static GType file_operation_type = 0;
  
  if (!file_operation_type)
    {
      static const GTypeInfo file_operation_info =
      {
        sizeof (GpaBackupOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_backup_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaBackupOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_backup_operation_init,
      };
      
      file_operation_type = g_type_register_static (GPA_OPERATION_TYPE,
						    "GpaBackupOperation",
						    &file_operation_info, 0);
    }
  
  return file_operation_type;
}

/* Private functions */

static void
gpa_backup_operation_do_backup (GpaBackupOperation *op, gchar *filename)
{
  gboolean cancelled = FALSE;
  
  if (g_file_test (filename, (G_FILE_TEST_EXISTS)))
    {
      GtkWidget *msgbox = 
	gtk_message_dialog_new (GTK_WINDOW (GPA_OPERATION (op)->window), 
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, 
				_("The file %s already exists.\n"
				  "Do you want to overwrite it?"), filename);
      gtk_dialog_add_buttons (GTK_DIALOG (msgbox),
                              _("_Yes"), GTK_RESPONSE_YES,
                              _("_No"), GTK_RESPONSE_NO, NULL);
      if (gtk_dialog_run (GTK_DIALOG (msgbox)) == GTK_RESPONSE_NO)
        {
          cancelled = TRUE;
        }
      gtk_widget_destroy (msgbox);
    }
  if (!cancelled)
    {
      if (gpa_backup_key (op->fpr, filename))
        {
          gchar *message;
	      message = g_strdup_printf (_("A copy of your secret key has "
					   "been made to the file:\n\n"
					   "\t\"%s\"\n\n"
					   "This is sensitive information, "
					   "and should be stored carefully\n"
					   "(for example, in a floppy disk "
					   "kept in a safe place)."),
					 filename);
	      gpa_window_message (message, GPA_OPERATION (op)->window);
	      g_free (message);
	      gpa_options_set_backup_generated (gpa_options_get_instance (),
						TRUE);
        }
      else
	{
	  gchar *message = g_strdup_printf (_("An error ocurred during the "
					      "backup operation."));
	  gpa_window_error (message, GPA_OPERATION (op)->window);
	}
    }
}

/* Handler for the browse button. Run a modal file dialog and set the
 * text of the file entry widget accordingly.
 */
static void
export_browse (gpointer param)
{
  GtkWidget *entry = param;
  gchar *filename;

  filename = gpa_get_save_file_name (entry, _("Backup key to file"), NULL);
  if (filename)
    {
      gchar *utf8_filename = g_filename_to_utf8 (filename, -1, NULL, NULL, 
						 NULL);
      gtk_entry_set_text (GTK_ENTRY (entry),
			  utf8_filename);
      g_free (utf8_filename);
      g_free (filename);
    }
} /* export_browse */

static gchar*
gpa_backup_operation_dialog_run (GtkWidget *parent, const gchar *key_id)
{
  GtkAccelGroup *accel_group;

  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *id_label;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;

  gchar *id_text, *default_file;

  window = gtk_dialog_new_with_buttons (_("Backup Keys"),
                                        GTK_WINDOW (parent),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        _("_Cancel"),
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);  

  vbox = GTK_DIALOG (window)->vbox;
  gtk_container_add (GTK_CONTAINER (window), vbox);

  table = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 5);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);

  /* Show the ID */
  id_text = g_strdup_printf (_("Generating backup of key: %s"), key_id);
  id_label = gtk_label_new (id_text);
  gtk_table_attach (GTK_TABLE (table), id_label, 0, 3, 0, 1, GTK_FILL, 0, 0, 0);
  g_free (id_text);

  /* File name entry */
  label = gtk_label_new_with_mnemonic (_("_Backup to file:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, 
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  default_file = g_build_filename (g_get_home_dir (), "sec_key.asc", NULL);
  gtk_entry_set_text (GTK_ENTRY (entry), default_file);
  g_free (default_file);
  gtk_widget_grab_focus (entry);

  button = gpa_button_new (accel_group, _("B_rowse..."));
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (export_browse),
			     (gpointer) entry);

  gtk_widget_show_all (window);
  if (gtk_dialog_run (GTK_DIALOG (window)) == GTK_RESPONSE_OK)
    {
      gchar *filename = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
      gtk_widget_destroy (window);
      return filename;
    }
  else
    {
      gtk_widget_destroy (window);
      return NULL;
    }
}

static gboolean
gpa_backup_operation_idle_cb (gpointer data)
{
  GpaBackupOperation *op = data;
  gchar *file;

  if ((file = gpa_backup_operation_dialog_run (GPA_OPERATION (op)->window,
					       op->key_id)))
    {
      gpa_backup_operation_do_backup (op, file);
    }
  
  g_signal_emit_by_name (GPA_OPERATION (op), "completed");

  return FALSE;
}

/* API */

GpaBackupOperation* 
gpa_backup_operation_new (GtkWidget *window, gpgme_key_t key)
{
  GpaBackupOperation *op;
  
  op = g_object_new (GPA_BACKUP_OPERATION_TYPE,
		     "window", window,
		     "key", key,
		     NULL);

  return op;
}

GpaBackupOperation* 
gpa_backup_operation_new_from_fpr (GtkWidget *window, const gchar *fpr)
{
  GpaBackupOperation *op;
  
  op = g_object_new (GPA_BACKUP_OPERATION_TYPE,
		     "window", window,
		     "fpr", fpr,
		     NULL);
  
  return op;
}
