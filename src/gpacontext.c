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

#include <glib.h>
#include <gpgme.h>
#include "gpgmetools.h"
#include "gpacontext.h"

/* GObject type functions */

static void gpa_context_init (GpaContext *context);
static void gpa_context_class_init (GpaContextClass *klass);
static void gpa_context_finalize (GObject *object);


/* Default signal handlers */

static void gpa_context_start (GpaContext *context);
static void gpa_context_done (GpaContext *context, GpgmeError err);
static void gpa_context_next_key (GpaContext *context, GpgmeKey key);
static void gpa_context_next_trust_item (GpaContext *context,
                                         GpgmeTrustItem item);

/* The GPGME I/O callbacks */

static GpgmeError gpa_context_register_cb (void *data, int fd, int dir,
                                           GpgmeIOCb fnc, void *fnc_data, 
                                           void **tag);
static void gpa_context_remove_cb (void *tag);
static void gpa_context_event_cb (void *data, GpgmeEventIO type,
                                  void *type_data);

enum
{
  START,
  DONE,
  NEXT_KEY,
  NEXT_TRUST_ITEM,
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
}

static void
gpa_context_init (GpaContext *context)
{
  GpgmeError err;

  context->busy = FALSE;

  /* The callback queue */
  context->cbs = NULL;

  /* The context itself */
  err = gpgme_new (&context->ctx);
  if (err != GPGME_No_Error)
    gpa_gpgme_error (err);

  /* Set the appropiate callbacks */
  gpgme_set_passphrase_cb (context->ctx, gpa_passphrase_cb, context->ctx);
  /* Fill the CB structure */
  context->io_cbs = g_malloc (sizeof (struct GpgmeIOCbs));
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

/* Destroy the GpaContext object */
void
gpa_context_destroy (GpaContext *context)
{
  g_object_run_dispose (G_OBJECT (context));
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
  GpgmeIOCb fnc;
  void *fnc_data;
  gint watch;
  GpaContext *context;
};

/* This function is called by GLib. It's a wrapper for the callback
 * gpgme provided, whose prototype does not match the one needed
 * by g_io_add_watch_full
 */
static gboolean
gpa_io_cb (GIOChannel *source, GIOCondition condition, gpointer data)
{
  struct gpa_io_cb_data *cb =  data;

  cb->fnc (cb->fnc_data, g_io_channel_unix_get_fd (source));

  return TRUE;
}

/* Register a GPGME callback with GLib.
 */
static void
register_callback (struct gpa_io_cb_data *cb)
{
  GIOChannel *channel;
 
  channel = g_io_channel_unix_new (cb->fd);
  cb->watch = g_io_add_watch_full (channel, G_PRIORITY_DEFAULT, 
				   cb->dir ? READ_CONDITION : WRITE_CONDITION,
				   gpa_io_cb, cb, NULL);
  g_io_channel_unref (channel);
}

/* Queuing callbacks until the START event arrives */

/* Add a callback to the queue, until the START event arrives
 */
static void
queue_callback (GpaContext *context, struct gpa_io_cb_data *cb)
{ 
  context->cbs = g_list_append (context->cbs, cb);
}

/* Register with GLib and remove from the queue all callbacks GPGME
 * registered.
 */
static void
register_queued_callbacks (GpaContext *context)
{
  struct gpa_io_cb_data *cb;
  GList *list;

  
  for (list = context->cbs; list; list = g_list_next (list))
    {
      cb = list->data;
      register_callback (cb);
    }
  g_list_free (context->cbs);
  context->cbs = NULL;
}

/* The real GPGME callbacks */

/* Register a callback.
 */
static GpgmeError
gpa_context_register_cb (void *data, int fd, int dir, GpgmeIOCb fnc,
                         void *fnc_data, void **tag)
{
  GpaContext *context = data;
  struct gpa_io_cb_data *cb = g_malloc (sizeof (struct gpa_io_cb_data));

  cb->fd = fd;
  cb->dir = dir;  
  cb->fnc = fnc;
  cb->fnc_data = fnc_data;
  cb->context = context;
  /* If the context is busy, we already have a START event, and can
   * register GLib callbacks freely. Else, we queue them until a START
   * comes. */
  if (context->busy)
    {
      register_callback (cb);
    }
  else
    {
      queue_callback (context, cb);
    }
  *tag = cb;

  return GPGME_No_Error;
}

/* Remove a callback.
 */
static void
gpa_context_remove_cb (void *tag)
{
  struct gpa_io_cb_data *cb = tag;

  if (cb->context->busy)
    {
      g_source_remove (cb->watch);
    }
  else
    {
      cb->context->cbs = g_list_remove (cb->context->cbs, cb);
    }

  g_free (cb);
}

/* The event callback. This just emits signals for the GpaContext. The signal
 * handlers do the real job.
 */
static void
gpa_context_event_cb (void *data, GpgmeEventIO type, void *type_data)
{
  GpaContext *context = data;
  GpgmeError *err;
  
  switch (type)
    {
    case GPGME_EVENT_START:
      g_signal_emit (context, signals[START], 0);
      break;
    case GPGME_EVENT_DONE:
      err = type_data;
      g_signal_emit (context, signals[DONE], 0, *err);
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
  fprintf (stderr, "START\n");
  context->busy = TRUE;
  /* We have START, register all queued callbacks */
  register_queued_callbacks (context);
}

static void
gpa_context_done (GpaContext *context, GpgmeError err)
{
  fprintf (stderr, "DONE\n");
  context->busy = FALSE;
}

static void
gpa_context_next_key (GpaContext *context, GpgmeKey key)
{
  /* Do nothing yet */
}

static void
gpa_context_next_trust_item (GpaContext *context, GpgmeTrustItem item)
{
  /* Do nothing yet */
}
