/* gpakeydeleteop.c - The GpaKeyDeleteOperation object.
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

#include <glib.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#else
#include <io.h>
#endif

#include "gpa.h"
#include "gpakeydeleteop.h"

/* Internal functions */
static gboolean gpa_key_delete_operation_idle_cb (gpointer data);
static void gpa_key_delete_operation_done_error_cb (GpaContext *context,
						    gpg_error_t err,
						    GpaKeyDeleteOperation *op);
static void gpa_key_delete_operation_done_cb (GpaContext *context,
					      gpg_error_t err,
					      GpaKeyDeleteOperation *op);

/* GObject */

static GObjectClass *parent_class = NULL;

static void
gpa_key_delete_operation_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_key_delete_operation_init (GpaKeyDeleteOperation *op)
{
}

static GObject*
gpa_key_delete_operation_constructor (GType type,
				      guint n_construct_properties,
				      GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaKeyDeleteOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_KEY_DELETE_OPERATION (object);
  /* Initialize */

  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_key_delete_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_key_delete_operation_done_cb), op);
  /* Start with the first key after going back into the main loop */
  g_idle_add (gpa_key_delete_operation_idle_cb, op);

  return object;
}

static void
gpa_key_delete_operation_class_init (GpaKeyDeleteOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_key_delete_operation_constructor;
  object_class->finalize = gpa_key_delete_operation_finalize;
}

GType
gpa_key_delete_operation_get_type (void)
{
  static GType key_delete_operation_type = 0;

  if (!key_delete_operation_type)
    {
      static const GTypeInfo key_delete_operation_info =
      {
        sizeof (GpaKeyDeleteOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_key_delete_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaKeyDeleteOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_key_delete_operation_init,
      };

      key_delete_operation_type = g_type_register_static
	(GPA_KEY_OPERATION_TYPE, "GpaKeyDeleteOperation",
	 &key_delete_operation_info, 0);
    }

  return key_delete_operation_type;
}

/* API */

/* Creates a new key deletion operation.
 */
GpaKeyDeleteOperation*
gpa_key_delete_operation_new (GtkWidget *window, GList *keys)
{
  GpaKeyDeleteOperation *op;

  op = g_object_new (GPA_KEY_DELETE_OPERATION_TYPE,
		     "window", window,
		     "keys", keys,
		     NULL);

  return op;
}

/* Internal */

static gpg_error_t
gpa_key_delete_operation_start (GpaKeyDeleteOperation *op)
{
  gpg_error_t err;
  gpgme_key_t key;

  key = gpa_key_operation_current_key (GPA_KEY_OPERATION (op));
  g_return_val_if_fail (key, gpg_error (GPG_ERR_CANCELED));

  if (! gpa_delete_dialog_run (GPA_OPERATION (op)->window, key))
    return gpg_error (GPG_ERR_CANCELED);

  gpgme_set_protocol (GPA_OPERATION (op)->context->ctx, key->protocol);
  err = gpgme_op_delete_start (GPA_OPERATION(op)->context->ctx, key, TRUE);
  if (err)
    {
      gpa_gpgme_warning (err);
      return err;
    }

  return 0;
}

static gboolean
gpa_key_delete_operation_idle_cb (gpointer data)
{
  gpg_error_t err;
  GpaKeyDeleteOperation *op = data;

  err = gpa_key_delete_operation_start (op);
  if (err)
    g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);

  return FALSE;
}


static void
gpa_key_delete_operation_next (GpaKeyDeleteOperation *op)
{
  gpg_error_t err = 0;

  if (GPA_KEY_OPERATION (op)->current)
    {
      err = gpa_key_delete_operation_start (op);
      if (! err)
	return;
    }

  g_signal_emit_by_name (GPA_OPERATION (op), "changed_wot");
  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}

static void gpa_key_delete_operation_done_error_cb (GpaContext *context,
						    gpg_error_t err,
						    GpaKeyDeleteOperation *op)
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

static void gpa_key_delete_operation_done_cb (GpaContext *context,
					      gpg_error_t err,
					      GpaKeyDeleteOperation *op)
{
  GPA_KEY_OPERATION (op)->current = g_list_next
    (GPA_KEY_OPERATION (op)->current);
  gpa_key_delete_operation_next (op);
}
