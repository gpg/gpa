/* gpacontext.h - The GpaContext object.
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

#ifndef GPA_CONTEXT_H
#define GPA_CONTEXT_H

#include <glib.h>
#include <glib-object.h>
#include <gpgme.h>

/* GObject stuff */
#define GPA_CONTEXT_TYPE	  (gpa_context_get_type ())
#define GPA_CONTEXT(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_CONTEXT_TYPE, GpaContext))
#define GPA_CONTEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_CONTEXT_TYPE, GpaContextClass))
#define GPA_IS_CONTEXT(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_CONTEXT_TYPE))
#define GPA_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_CONTEXT_TYPE))
#define GPA_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_CONTEXT_TYPE, GpaContextClass))

typedef struct _GpaContext GpaContext;
typedef struct _GpaContextClass GpaContextClass;

struct _GpaContext {
  GObject parent;

  /* public: */

  /* The real GpgmeCtx */
  GpgmeCtx ctx;
  /* Whether there is an operation currently in course */
  gboolean busy;

  /* private: */

  /* Queued I/O callbacks */
  GList *cbs;
  /* The IO callback structure */
  struct GpgmeIOCbs *io_cbs;
};

struct _GpaContextClass {
  GObjectClass parent_class;

  /* Signal handlers */
  void (*start) (GpaContext *context);
  void (*done) (GpaContext *context, GpgmeError err);
  void (*next_key) (GpaContext *context, GpgmeKey key);
  void (*next_trust_item) (GpaContext *context, GpgmeTrustItem item);
};

GType gpa_context_get_type (void) G_GNUC_CONST;

/* API */

/* Create a new GpaContext object.
 */
GpaContext *gpa_context_new (void);

/* Destroy the context.
 */
void gpa_context_destroy (GpaContext *context);

#endif

