/* gpacontext.c -  The GpaContext object.
 *	Copyright (C) 2003 Miguel Coca.
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
#include <gpgme.h>
#include "gpa.h"
#include "gpgmetools.h"
#include "gpacontext.h"

/* GObject type functions */

static void gpa_context_init (GpaContext *context);
static void gpa_context_class_init (GpaContextClass *klass);
static void gpa_context_finalize (GObject *object);


/* Default signal handlers */

static void gpa_context_start (GpaContext *context);
static void gpa_context_done (GpaContext *context, gpg_error_t err);
static void gpa_context_next_key (GpaContext *context, gpgme_key_t key);
static void gpa_context_next_trust_item (GpaContext *context,
                                         gpgme_trust_item_t item);
static void gpa_context_progress (GpaContext *context, int current, int total);

/* The GPGME I/O callbacks */

static gpg_error_t gpa_context_register_cb (void *data, int fd, int dir,
                                           gpgme_io_cb_t fnc, void *fnc_data,
                                           void **tag);
static void gpa_context_remove_cb (void *tag);
static void gpa_context_event_cb (void *data, gpgme_event_io_t type,
                                  void *type_data);

static gpg_error_t
gpa_context_passphrase_cb (void *hook, const char *uid_hint,
			   const char *passphrase_info, int prev_was_bad,
			   int fd);
static void
gpa_context_progress_cb (void *opaque, const char *what,
			 int type, int current, int total);

/* Signals */
enum
{
  START,
  DONE,
  NEXT_KEY,
  NEXT_TRUST_ITEM,
  PROGRESS,
  LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint signals [LAST_SIGNAL] = { 0 };

GType
gpa_context_get_type (void)
{
  static GType context_type = 0;

  if (!context_type)
    {
      static const GTypeInfo context_info =
      {
        sizeof (GpaContextClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_context_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaContext),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_context_init,
      };

      context_type = g_type_register_static (G_TYPE_OBJECT,
                                             "GpaContext",
                                             &context_info, 0);
    }

  return context_type;
}

static void
gpa_context_class_init (GpaContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gpa_context_finalize;

  klass->start = gpa_context_start;
  klass->done = gpa_context_done;
  klass->next_key = gpa_context_next_key;
  klass->next_trust_item = gpa_context_next_trust_item;
  klass->progress = gpa_context_progress;

  /* Signals */
  signals[START] =
          g_signal_new ("start",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_FIRST,
                        G_STRUCT_OFFSET (GpaContextClass, start),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);
  signals[DONE] =
          g_signal_new ("done",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_FIRST,
                        G_STRUCT_OFFSET (GpaContextClass, done),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__INT,
                        G_TYPE_NONE, 1,
			G_TYPE_INT);
  signals[NEXT_KEY] =
          g_signal_new ("next_key",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_FIRST,
                        G_STRUCT_OFFSET (GpaContextClass, next_key),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
			G_TYPE_POINTER);
  signals[NEXT_TRUST_ITEM] =
          g_signal_new ("next_trust_item",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_FIRST,
                        G_STRUCT_OFFSET (GpaContextClass, next_trust_item),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
			G_TYPE_POINTER);
  signals[PROGRESS] =
          g_signal_new ("progress",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_FIRST,
                        G_STRUCT_OFFSET (GpaContextClass, progress),
                        NULL, NULL,
			gtk_marshal_VOID__INT_INT,
                        G_TYPE_NONE, 2,
			G_TYPE_INT, G_TYPE_INT);
}

static void
gpa_context_init (GpaContext *context)
{
  gpg_error_t err;

  context->busy = FALSE;
  context->inhibit_gpgme_events = 0;

  /* The callback queue */
  context->cbs = NULL;

  /* The context itself */
  err = gpgme_new (&context->ctx);
  if (err)
    {
      gpa_gpgme_warning (err);
      return;
    }

  /* Set the appropriate callbacks.  Note that we can't set the
     passphrase callback in CMS mode because it is not implemented by
     the CMS backend.  To make things easier we never set in CMS mode
     because we can then assume that a proper GnuPG-2 system (with
     pinentry) is in use and then we don't need that callback for
     OpenPGP either. */
  if (!cms_hack)
    gpgme_set_passphrase_cb (context->ctx, gpa_context_passphrase_cb, context);
  gpgme_set_progress_cb (context->ctx, gpa_context_progress_cb, context);
  /* Fill the CB structure */
  context->io_cbs = g_malloc (sizeof (struct gpgme_io_cbs));
  context->io_cbs->add = gpa_context_register_cb;
  context->io_cbs->add_priv = context;
  context->io_cbs->remove = gpa_context_remove_cb;
  context->io_cbs->event = gpa_context_event_cb;
  context->io_cbs->event_priv = context;
  /* Set the callbacks */
  gpgme_set_io_cbs (context->ctx, context->io_cbs);
}


static void
gpa_context_finalize (GObject *object)
{
  GpaContext *context = GPA_CONTEXT (object);

  gpgme_release (context->ctx);
  g_list_free (context->cbs);
  g_free (context->io_cbs);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* API */

/* Create a new GpaContext object.
 */
GpaContext *
gpa_context_new (void)
{
  GpaContext *context;

  context = g_object_new (GPA_CONTEXT_TYPE, NULL);

  return context;
}

/* TRUE if an operation is in progress.
 */
gboolean
gpa_context_busy (GpaContext *context)
{
  g_return_val_if_fail (context != NULL, FALSE);
  g_return_val_if_fail (GPA_IS_CONTEXT (context), FALSE);

  return context->busy;
}


/* Return a malloced string with the last diagnostic data of the
 * context.  Returns NULL if no diagnostics are available.  */
char *
gpa_context_get_diag (GpaContext *context)
{
#if GPGME_VERSION_NUMBER >= 0x010c00 /* >= 1.12 */
  gpgme_data_t diag;
  char *info, *buffer, *result;
  gpg_error_t err;
  gpgme_engine_info_t engine;
  gpgme_protocol_t proto;
  int need_gpgme_free;

  if (!context)
    return NULL;
  if (gpgme_data_new (&diag))
    return NULL;  /* Ooops.  */

  context->inhibit_gpgme_events++;
  err = gpgme_op_getauditlog (context->ctx, diag, GPGME_AUDITLOG_DIAG);
  context->inhibit_gpgme_events--;
  if (err)
    {
      gpgme_data_release (diag);
      return NULL;  /* No data.  */
    }

  proto = gpgme_get_protocol (context->ctx);
  gpgme_get_engine_info (&engine);
  for (; engine; engine = engine->next)
    if (engine->protocol == proto)
      break;
  info = g_strdup_printf ("[GPA %s, GPGME %s, GnuPG %s]\n",
                          VERSION,
                          gpgme_check_version (NULL),
                          engine? engine->version : "?");

  /* Append a trailing zero and return the string.  */
  gpgme_data_seek (diag, 0, SEEK_END);
  gpgme_data_write (diag, "", 1);
  buffer = gpgme_data_release_and_get_mem (diag, NULL);
  need_gpgme_free = 1;
  /* In case the diags do not return proper utf-8 we transliterate to
   * ascii using the "C" locale which might help in bug tracking.  */
  if (!g_utf8_validate (buffer, -1, NULL))
    {
      result = g_str_to_ascii (buffer, "C");
      gpgme_free (buffer);
      buffer = result;
      need_gpgme_free = 0;
    }
  /* Prefix with the version info. */
  result = g_strconcat (info, buffer, NULL);
  if (need_gpgme_free)
    gpgme_free (buffer);
  else
    g_free (buffer);
  g_free (info);

  return result;
#else
  return NULL;
#endif
}



/*
 * The GPGME I/O callbacks
 */

/* Registering callbacks with GLib */

/* Macros "borrowed" from GDK */
#define READ_CONDITION (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define WRITE_CONDITION (G_IO_OUT | G_IO_ERR)

struct gpa_io_cb_data
{
  int fd;
  int dir;
  gpgme_io_cb_t fnc;
  void *fnc_data;
  gint watch;
  GpaContext *context;
  gboolean registered;
};

/* This function is called by GLib. It's a wrapper for the callback
 * gpgme provided, whose prototype does not match the one needed
 * by g_io_add_watch_full
 */
static gboolean
gpa_io_cb (GIOChannel *source, GIOCondition condition, gpointer data)
{
  struct gpa_io_cb_data *cb =  data;

  /* We have to use the GPGME provided "file descriptor" here.  It may
     not be a system file descriptor after all.  */
  cb->fnc (cb->fnc_data, cb->fd);

  return TRUE;
}

/* Register a GPGME callback with GLib.
 */
static void
register_callback (struct gpa_io_cb_data *cb)
{
  GIOChannel *channel;

#ifdef G_OS_WIN32
  /* We have to ask GPGME for the GIOChannel to use.  The "file
     descriptor" may not be a system file descriptor.  */
  channel = gpgme_get_giochannel (cb->fd);
  g_assert (channel);
#else
  channel = g_io_channel_unix_new (cb->fd);
#endif

  cb->watch = g_io_add_watch_full (channel, G_PRIORITY_DEFAULT,
				   cb->dir ? READ_CONDITION : WRITE_CONDITION,
				   gpa_io_cb, cb, NULL);
  cb->registered = TRUE;

#ifdef G_OS_WIN32
  /* Nothing do to here.  */
#else
  g_io_channel_unref (channel);
#endif
}

/* Queuing callbacks until the START event arrives */

/* Add a callback to the list, until the START event arrives
 */
static void
add_callback (GpaContext *context, struct gpa_io_cb_data *cb)
{
  context->cbs = g_list_append (context->cbs, cb);
}


/* Register with GLib all previously unregistered callbacks.  */
static void
register_all_callbacks (GpaContext *context)
{
  struct gpa_io_cb_data *cb;
  GList *list;

  for (list = context->cbs; list; list = g_list_next (list))
    {
      cb = list->data;
      if (!cb->registered)
	{
	  register_callback (cb);
	}
    }
}

static void
unregister_all_callbacks (GpaContext *context)
{
  struct gpa_io_cb_data *cb;
  GList *list;

  for (list = context->cbs; list; list = g_list_next (list))
    {
      cb = list->data;
      if (cb->registered)
	{
	  g_source_remove (cb->watch);
	  cb->registered = FALSE;
	}
    }
}


/* The real GPGME callbacks */

/* Register a callback.  This is called by GPGME when a crypto
   operation is initiated in this context.  */
static gpg_error_t
gpa_context_register_cb (void *data, int fd, int dir, gpgme_io_cb_t fnc,
                         void *fnc_data, void **tag)
{
  GpaContext *context = data;
  struct gpa_io_cb_data *cb = g_malloc (sizeof (struct gpa_io_cb_data));


  cb->registered = FALSE;
  cb->fd = fd;
  cb->dir = dir;
  cb->fnc = fnc;
  cb->fnc_data = fnc_data;
  cb->context = context;
  /* If the context is busy, we already have a START event, and can
   * register GLib callbacks immediately.  */
  if (context->busy)
    register_callback (cb);

  /* In any case, we add it to the list.   */
  add_callback (context, cb);
  *tag = cb;

  return 0;
}


/* Remove a callback.  This is called by GPGME if a context is to be
   destroyed.  */
static void
gpa_context_remove_cb (void *tag)
{
  struct gpa_io_cb_data *cb = tag;

  if (cb->registered)
    {
      g_source_remove (cb->watch);
    }
  cb->context->cbs = g_list_remove (cb->context->cbs, cb);
  g_free (cb);
}


/* The event callback.  It is called by GPGME to signal an event for
   an operation running in this context.  This fucntion merely emits
   signals for GpaContext; the Glib signal handlers do the real
   job.  */
static void
gpa_context_event_cb (void *data, gpgme_event_io_t type, void *type_data)
{
  GpaContext *context = data;
  gpg_error_t err, op_err;

  if (context->inhibit_gpgme_events)
    return;

  switch (type)
    {
    case GPGME_EVENT_START:
      g_signal_emit (context, signals[START], 0);
      break;
    case GPGME_EVENT_DONE:
      err = ((gpgme_io_event_done_data_t)type_data)->err;
      op_err = ((gpgme_io_event_done_data_t)type_data)->op_err;
      g_debug ("EVENT_DONE: err=%s op_err=%s",
               gpg_strerror (err), gpg_strerror (op_err));
      if (!err)
        err = op_err;
      g_signal_emit (context, signals[DONE], 0, err);
      break;
    case GPGME_EVENT_NEXT_KEY:
      g_signal_emit (context, signals[NEXT_KEY], 0, type_data);
      break;
    case GPGME_EVENT_NEXT_TRUSTITEM:
      g_signal_emit (context, signals[NEXT_TRUST_ITEM], 0,
                     type_data);
      break;
    default:
      /* Ignore unsupported event types */
      break;
    }
}

/* Default signal handlers */

static void
gpa_context_start (GpaContext *context)
{
/*   g_debug ("gpgme event START enter"); */
  context->busy = TRUE;
  /* We have START, register all queued callbacks */
  register_all_callbacks (context);
/*   g_debug ("gpgme event START leave"); */
}

static void
gpa_context_done (GpaContext *context, gpg_error_t err)
{
  context->busy = FALSE;
/*   g_debug ("gpgme event DONE ready"); */
}

static void
gpa_context_next_key (GpaContext *context, gpgme_key_t key)
{
  /* Do nothing yet */
}

static void
gpa_context_next_trust_item (GpaContext *context, gpgme_trust_item_t item)
{
  /* Do nothing yet */
}

static void
gpa_context_progress (GpaContext *context, int current, int total)
{
  /* Do nothing yet */
}

/* The passphrase callback */
static gpg_error_t
gpa_context_passphrase_cb (void *hook, const char *uid_hint,
			   const char *passphrase_info, int prev_was_bad,
			   int fd)
{
  GpaContext *context = hook;
  gpg_error_t err;

  unregister_all_callbacks (context);
  err = gpa_passphrase_cb (NULL, uid_hint, passphrase_info, prev_was_bad, fd);
  register_all_callbacks (context);

  return err;
}

/* The progress callback */
static void
gpa_context_progress_cb (void *opaque, const char *what,
			 int type, int current, int total)
{
  GpaContext *context = opaque;
  g_signal_emit (context, signals[PROGRESS], 0, current, total);
}
