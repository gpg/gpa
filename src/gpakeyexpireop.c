/* gpakeyexpireop.c - The GpaKeyExpireOperation object.
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

#include <glib.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#else
#include <io.h>
#endif

#include "gpa.h"
#include "gpakeyexpireop.h"
#include "expirydlg.h"
#include "gpgmeedit.h"
#include "gtktools.h"

/* Internal functions */
static gboolean gpa_key_expire_operation_idle_cb (gpointer data);
static void gpa_key_expire_operation_done_error_cb (GpaContext *context, 
						    gpg_error_t err,
						    GpaKeyExpireOperation *op);
static void gpa_key_expire_operation_done_cb (GpaContext *context, 
					      gpg_error_t err,
					      GpaKeyExpireOperation *op);

/* GObject */

/* Signals */
enum
{
  NEW_EXPIRATION,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;

static void
gpa_key_expire_operation_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_key_expire_operation_init (GpaKeyExpireOperation *op)
{
  op->modified_keys = 0;
}

static GObject*
gpa_key_expire_operation_constructor (GType type,
				      guint n_construct_properties,
				      GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaKeyExpireOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_KEY_EXPIRE_OPERATION (object);
  /* Initialize */

  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_key_expire_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_key_expire_operation_done_cb), op);
  /* Start with the first key after going back into the main loop */
  g_idle_add (gpa_key_expire_operation_idle_cb, op);

  return object;
}

static void
gpa_key_expire_operation_class_init (GpaKeyExpireOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_key_expire_operation_constructor;
  object_class->finalize = gpa_key_expire_operation_finalize;

  /* Signals */
  signals[NEW_EXPIRATION] =
    g_signal_new ("new_expiration",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GpaKeyExpireOperationClass, new_expiration),
		  NULL, NULL,
		  gtk_marshal_VOID__POINTER_POINTER,
		  G_TYPE_NONE, 2,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER);
}

GType
gpa_key_expire_operation_get_type (void)
{
  static GType key_expire_operation_type = 0;
  
  if (!key_expire_operation_type)
    {
      static const GTypeInfo key_expire_operation_info =
      {
        sizeof (GpaKeyExpireOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_key_expire_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaKeyExpireOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_key_expire_operation_init,
      };
      
      key_expire_operation_type = g_type_register_static 
	(GPA_KEY_OPERATION_TYPE, "GpaKeyExpireOperation",
	 &key_expire_operation_info, 0);
    }
  
  return key_expire_operation_type;
}

/* API */

/* Creates a new key deletion operation.
 */
GpaKeyExpireOperation*
gpa_key_expire_operation_new (GtkWidget *window, GList *keys)
{
  GpaKeyExpireOperation *op;
  
  op = g_object_new (GPA_KEY_EXPIRE_OPERATION_TYPE,
		     "window", window,
		     "keys", keys,
		     NULL);

  return op;
}

/* Internal */

static gboolean
gpa_key_expire_operation_start (GpaKeyExpireOperation *op)
{ 
  gpgme_key_t key = gpa_key_operation_current_key (GPA_KEY_OPERATION (op));
  GDate *date;

  if (gpa_expiry_dialog_run (GPA_OPERATION (op)->window, key, &date))
    {
      gpg_error_t err;
      err = gpa_gpgme_edit_expire_start (GPA_OPERATION(op)->context, key,
					 date);
      op->date = date;
      if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
	{
	  gpa_gpgme_warning (err);
	  return FALSE;
	}
    }
  else
    {
      return FALSE;
    }
  return TRUE;
}

static gboolean
gpa_key_expire_operation_idle_cb (gpointer data)
{
  GpaKeyExpireOperation *op = data;

  if (!gpa_key_expire_operation_start (op))
    {
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }

  return FALSE;
}

static void
gpa_key_expire_operation_next (GpaKeyExpireOperation *op)
{
  if (!GPA_KEY_OPERATION (op)->current ||
      !gpa_key_expire_operation_start (op))
    {
      if (op->modified_keys > 0)
	{
	  g_signal_emit_by_name (GPA_OPERATION (op), "changed_wot");
	}
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }
}

static void 
gpa_key_expire_operation_done_error_cb (GpaContext *context, 
                                        gpg_error_t err,
                                        GpaKeyExpireOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
      op->modified_keys++;
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;
    case GPG_ERR_BAD_PASSPHRASE:
      gpa_window_error (_("Wrong passphrase!"), GPA_OPERATION (op)->window);
      break;
    case GPG_ERR_INV_TIME:
      gpa_window_error 
        (_("Invalid time given.\n"
           "(you may not set the expiration time to the past.)"),
         GPA_OPERATION (op)->window);
      break;
    default:
      gpa_gpgme_warning (err);
      break;
    }
}

static void 
gpa_key_expire_operation_done_cb (GpaContext *context,
                                  gpg_error_t err,
                                  GpaKeyExpireOperation *op)
{
  if (gpg_err_code (err) == GPG_ERR_NO_ERROR)
    {
      /* The expiration was changed */
      g_signal_emit_by_name (op, "new_expiration", 
			     GPA_KEY_OPERATION (op)->current->data, op->date);
    }
  /* Clean previous date */
  if (op->date)
    {
      g_date_free (op->date);
      op->date = NULL;
    }
  /* Go to the next key */
  GPA_KEY_OPERATION (op)->current = g_list_next
    (GPA_KEY_OPERATION (op)->current);
  gpa_key_expire_operation_next (op);
}
