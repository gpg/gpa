/* gpabackupop.c - The GpaBackupOperation object.
   Copyright (C) 2003 Miguel Coca.
   Copyright (C) 2005, 2008 g10 Code GmbH.

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


/* Return the filename in filename encoding.  */
static gchar*
gpa_backup_operation_dialog_run (GtkWidget *parent, const gchar *key_id)
{
  GtkWidget *dialog;
  GtkResponseType response;
  gchar *default_comp;
  gchar *default_file;
  gchar *id_text;
  GtkWidget *id_label;
  gchar *filename = NULL;

  dialog = gtk_file_chooser_dialog_new
    (_("Backup key to file"), GTK_WINDOW (parent),
     GTK_FILE_CHOOSER_ACTION_SAVE,  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
     GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
						  TRUE);

  /* Set the default file name.  */
  default_comp = g_strdup_printf ("secret-key-%s.asc", key_id);
  default_file = g_build_filename (g_get_home_dir (), default_comp, NULL);
  g_free (default_comp);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
				       g_get_home_dir ());
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), default_file);
  g_free (default_file);

  /* Set the label with more explanations.  */
  id_text = g_strdup_printf (_("Generating backup of key: %s"), key_id);
  id_label = gtk_label_new (id_text);
  g_free (id_text);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), id_label);

  /* Run the dialog until there is a valid response.  */
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_OK)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      if (filename)
	g_strdup (filename);
    }

  gtk_widget_destroy (dialog);

  return filename;
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
