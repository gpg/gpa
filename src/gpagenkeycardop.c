/* gpagenkeycardop.c - The GpaGenKeyCardOperation object.
 *	Copyright (C) 2003 Miguel Coca.
 *	Copyright (C) 2008, 2009 g10 Code GmbH
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
#include "gpa.h"
#include "i18n.h"
#include "gtktools.h"
#include "gpagenkeycardop.h"
#include "keygendlg.h"
#include "gpgmeedit.h"

static GObjectClass *parent_class = NULL;

static void gpa_gen_key_card_operation_done_cb (GpaContext *context,
						    gpg_error_t err,
						    GpaGenKeyCardOperation *op);

static void gpa_gen_key_card_operation_done_error_cb (GpaContext *context,
							  gpg_error_t err,
							  GpaGenKeyCardOperation *op);


static gboolean
gpa_gen_key_card_operation_idle_cb (gpointer data);

/* GObject boilerplate */

static void
gpa_gen_key_card_operation_init (GpaGenKeyCardOperation *op)
{
  op->progress_dialog = NULL;
}

static void
gpa_gen_key_card_operation_finalize (GObject *object)
{
  GpaGenKeyCardOperation *op = GPA_GEN_KEY_CARD_OPERATION (object);

  xfree (op->key_attributes);
  op->key_attributes = NULL;
  gtk_widget_destroy (op->progress_dialog);
  gpa_keygen_para_free (op->parms);
  op->parms = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject*
gpa_gen_key_card_operation_constructor (GType type,
					    guint n_construct_properties,
					    GObjectConstructParam *
					    construct_properties)
{
  GObject *object;
  GpaGenKeyCardOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_GEN_KEY_CARD_OPERATION (object);

  /* Create progress dialog */
  op->progress_dialog = gpa_progress_dialog_new (GPA_OPERATION (op)->window,
						 GPA_OPERATION (op)->context);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG (op->progress_dialog),
				 _("Generating Key..."));

  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_gen_key_card_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_gen_key_card_operation_done_cb), op);

  /* Begin working when we are back into the main loop */
  g_idle_add (gpa_gen_key_card_operation_idle_cb, op);

  return object;
}

static void
gpa_gen_key_card_operation_class_init (GpaGenKeyCardOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_gen_key_card_operation_constructor;
  object_class->finalize = gpa_gen_key_card_operation_finalize;
}

GType
gpa_gen_key_card_operation_get_type (void)
{
  static GType operation_type = 0;

  if (!operation_type)
    {
      static const GTypeInfo operation_info =
      {
        sizeof (GpaGenKeyCardOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_gen_key_card_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaGenKeyCardOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_gen_key_card_operation_init,
      };

      operation_type = g_type_register_static (GPA_GEN_KEY_OPERATION_TYPE,
					       "GpaGenKeyCardOperation",
					       &operation_info, 0);
    }

  return operation_type;
}

/* API */

GpaGenKeyCardOperation*
gpa_gen_key_card_operation_new (GtkWidget *window, const char *keyattr)
{
  GpaGenKeyCardOperation *op;

  op = g_object_new (GPA_GEN_KEY_CARD_OPERATION_TYPE,
		     "window", window, NULL);
  op->key_attributes = g_strdup (keyattr);

  return op;
}

/* Internal */

static gboolean
gpa_gen_key_card_operation_idle_cb (gpointer data)
{
  GpaGenKeyCardOperation *op = data;
  gpg_error_t err;

  op->parms = gpa_key_gen_run_dialog
    (GPA_OPERATION (op)->window, op->key_attributes? op->key_attributes : "");

  if (!op->parms)
    g_signal_emit_by_name (op, "completed", gpg_error (GPG_ERR_CANCELED));
  else
    {
      err = gpa_gpgme_card_edit_genkey_start (GPA_OPERATION (op)->context,
                                              op->parms);
      if (err)
        {
          gpa_gpgme_warning (err);
          g_signal_emit_by_name (op, "completed", err);
        }
      else
        gtk_widget_show_all (op->progress_dialog);
    }
  return FALSE;  /* Remove this idle function.  */
}


static void
gpa_gen_key_card_operation_done_cb (GpaContext *context,
				    gpg_error_t err,
				    GpaGenKeyCardOperation *op)
{
  if (! err)
    {
      /* This is not a gpgme_op_genkey operation, thus we cannot use
	 gpgme_op_genkey_result to retrieve result information about
	 the generated key. */
      g_signal_emit_by_name (op, "generated_key", NULL);
    }
  g_signal_emit_by_name (op, "completed", err);
}


static void
gpa_gen_key_card_operation_done_error_cb (GpaContext *context,
					  gpg_error_t err,
					  GpaGenKeyCardOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;

    case GPG_ERR_BAD_PIN:
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                     "%s", gpg_strerror (err));
      break;

    default:
      gpa_gpgme_warn (err, op->parms? op->parms->r_error_desc : NULL,
                      GPA_OPERATION (op)->context);
      break;
    }
}
