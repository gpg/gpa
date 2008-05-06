/* gpakeytrustop.c - The GpaKeyTrustOperation object.
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
#include "gpakeytrustop.h"
#include "ownertrustdlg.h"
#include "gpgmeedit.h"
#include "gtktools.h"

/* Internal functions */
static gboolean gpa_key_trust_operation_idle_cb (gpointer data);
static void gpa_key_trust_operation_done_error_cb (GpaContext *context, 
						    gpg_error_t err,
						    GpaKeyTrustOperation *op);
static void gpa_key_trust_operation_done_cb (GpaContext *context, 
					      gpg_error_t err,
					      GpaKeyTrustOperation *op);

/* GObject */

static GObjectClass *parent_class = NULL;

static void
gpa_key_trust_operation_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_key_trust_operation_init (GpaKeyTrustOperation *op)
{
  op->modified_keys = 0;
}

static GObject*
gpa_key_trust_operation_constructor (GType type,
				      guint n_construct_properties,
				      GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaKeyTrustOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_KEY_TRUST_OPERATION (object);
  /* Initialize */

  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_key_trust_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_key_trust_operation_done_cb), op);
  /* Start with the first key after going back into the main loop */
  g_idle_add (gpa_key_trust_operation_idle_cb, op);

  return object;
}

static void
gpa_key_trust_operation_class_init (GpaKeyTrustOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_key_trust_operation_constructor;
  object_class->finalize = gpa_key_trust_operation_finalize;
}

GType
gpa_key_trust_operation_get_type (void)
{
  static GType key_trust_operation_type = 0;
  
  if (!key_trust_operation_type)
    {
      static const GTypeInfo key_trust_operation_info =
      {
        sizeof (GpaKeyTrustOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_key_trust_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaKeyTrustOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_key_trust_operation_init,
      };
      
      key_trust_operation_type = g_type_register_static 
	(GPA_KEY_OPERATION_TYPE, "GpaKeyTrustOperation",
	 &key_trust_operation_info, 0);
    }
  
  return key_trust_operation_type;
}

/* API */

/* Creates a new key deletion operation.
 */
GpaKeyTrustOperation*
gpa_key_trust_operation_new (GtkWidget *window, GList *keys)
{
  GpaKeyTrustOperation *op;
  
  op = g_object_new (GPA_KEY_TRUST_OPERATION_TYPE,
		     "window", window,
		     "keys", keys,
		     NULL);

  return op;
}

/* Internal */

static gpg_error_t
gpa_key_trust_operation_start (GpaKeyTrustOperation *op)
{ 
  gpg_error_t err;
  gpgme_key_t key;
  gpgme_validity_t trust;

  key = gpa_key_operation_current_key (GPA_KEY_OPERATION (op));

  if (! gpa_ownertrust_run_dialog (key, GPA_OPERATION (op)->window, &trust))
    return gpg_error (GPG_ERR_CANCELED);

  err = gpa_gpgme_edit_trust_start (GPA_OPERATION(op)->context, key,trust);
  if (err)
    {
      gpa_gpgme_warning (err);
      return err;
    }

  return 0;
}


static gboolean
gpa_key_trust_operation_idle_cb (gpointer data)
{
  GpaKeyTrustOperation *op = data;
  gpg_error_t err;

  err = gpa_key_trust_operation_start (op);
  if (err)
    g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);

  return FALSE;
}


static void
gpa_key_trust_operation_next (GpaKeyTrustOperation *op)
{
  gpg_error_t err = 0;

  if (GPA_KEY_OPERATION (op)->current)
    {
      err = gpa_key_trust_operation_start (op);
      if (! err)
	return;
    }

  if (op->modified_keys > 0)
    g_signal_emit_by_name (GPA_OPERATION (op), "changed_wot");
  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}


static void gpa_key_trust_operation_done_error_cb (GpaContext *context, 
						  gpg_error_t err,
						  GpaKeyTrustOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
      op->modified_keys++;
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;
    default:
      gpa_gpgme_warning (err);
      break;
    }
}

static void gpa_key_trust_operation_done_cb (GpaContext *context, 
					      gpg_error_t err,
					      GpaKeyTrustOperation *op)
{
  GPA_KEY_OPERATION (op)->current = g_list_next
    (GPA_KEY_OPERATION (op)->current);
  gpa_key_trust_operation_next (op);
}
