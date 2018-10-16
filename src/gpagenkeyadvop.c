/* gpagenkeyadvop.c - The GpaGenKeyAdvancedOperation object.
 *	Copyright (C) 2003, Miguel Coca.
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
#include "gpagenkeyadvop.h"
#include "keygendlg.h"

static GObjectClass *parent_class = NULL;

static void gpa_gen_key_advanced_operation_done_cb (GpaContext *context,
						    gpg_error_t err,
						    GpaGenKeyAdvancedOperation *op);
static void gpa_gen_key_advanced_operation_done_error_cb (GpaContext *context,
							  gpg_error_t err,
							  GpaGenKeyAdvancedOperation *op);


static gboolean
gpa_gen_key_advanced_operation_idle_cb (gpointer data);

/* GObject boilerplate */

static void
gpa_gen_key_advanced_operation_init (GpaGenKeyAdvancedOperation *op)
{
  op->progress_dialog = NULL;
}

static void
gpa_gen_key_advanced_operation_finalize (GObject *object)
{
  GpaGenKeyAdvancedOperation *op = GPA_GEN_KEY_ADVANCED_OPERATION (object);

  gtk_widget_destroy (op->progress_dialog);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject*
gpa_gen_key_advanced_operation_constructor (GType type,
					    guint n_construct_properties,
					    GObjectConstructParam *
					    construct_properties)
{
  GObject *object;
  GpaGenKeyAdvancedOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_GEN_KEY_ADVANCED_OPERATION (object);

  /* Create progress dialog */
  op->progress_dialog = gpa_progress_dialog_new (GPA_OPERATION (op)->window,
						 GPA_OPERATION (op)->context);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG (op->progress_dialog),
				 _("Generating Key..."));

  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_gen_key_advanced_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_gen_key_advanced_operation_done_cb), op);

  /* Begin working when we are back into the main loop */
  g_idle_add (gpa_gen_key_advanced_operation_idle_cb, op);

  return object;
}

static void
gpa_gen_key_advanced_operation_class_init (GpaGenKeyAdvancedOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_gen_key_advanced_operation_constructor;
  object_class->finalize = gpa_gen_key_advanced_operation_finalize;
}

GType
gpa_gen_key_advanced_operation_get_type (void)
{
  static GType operation_type = 0;

  if (!operation_type)
    {
      static const GTypeInfo operation_info =
      {
        sizeof (GpaGenKeyAdvancedOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_gen_key_advanced_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaGenKeyAdvancedOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_gen_key_advanced_operation_init,
      };

      operation_type = g_type_register_static (GPA_GEN_KEY_OPERATION_TYPE,
					       "GpaGenKeyAdvancedOperation",
					       &operation_info, 0);
    }

  return operation_type;
}

/* API */

GpaGenKeyAdvancedOperation*
gpa_gen_key_advanced_operation_new (GtkWidget *window)
{
  GpaGenKeyAdvancedOperation *op;

  op = g_object_new (GPA_GEN_KEY_ADVANCED_OPERATION_TYPE,
		     "window", window, NULL);

  return op;
}

/* Internal */

static gboolean
gpa_gen_key_advanced_operation_idle_cb (gpointer data)
{
  GpaGenKeyAdvancedOperation *op = data;
  gpg_error_t err;
  gpa_keygen_para_t *parms;

  parms = gpa_key_gen_run_dialog (GPA_OPERATION (op)->window, NULL);
  if (!parms)
    g_signal_emit_by_name (op, "completed", gpg_error (GPG_ERR_CANCELED));
  else
    {
      err = gpa_generate_key_start (GPA_OPERATION (op)->context->ctx, parms);
      if (err)
        {
          gpa_gpgme_warning (err);
          g_signal_emit_by_name (op, "completed", err);
        }
      else
        gtk_widget_show_all (op->progress_dialog);
    }
  return FALSE;
}


static void
gpa_gen_key_advanced_operation_done_cb (GpaContext *context,
					gpg_error_t err,
					GpaGenKeyAdvancedOperation *op)
{
  if (! err)
    {
      gpgme_genkey_result_t result = gpgme_op_genkey_result (context->ctx);
      g_signal_emit_by_name (op, "generated_key", result->fpr);
    }
  g_signal_emit_by_name (op, "completed", err);
}


static void
gpa_gen_key_advanced_operation_done_error_cb (GpaContext *context,
					      gpg_error_t err,
					      GpaGenKeyAdvancedOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;
    default:
      gpa_gpgme_warn (err, NULL, GPA_OPERATION (op)->context);
      break;
    }
}
