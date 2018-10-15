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

  /* The real gpgme_ctx_t */
  gpgme_ctx_t ctx;
  /* Whether there is an operation currently in course */
  gboolean busy;

  /* private: */

  /* Queued I/O callbacks */
  GList *cbs;
  /* The IO callback structure */
  struct gpgme_io_cbs *io_cbs;
  /* Hack to block certain events.  */
  int inhibit_gpgme_events;
};

struct _GpaContextClass {
  GObjectClass parent_class;

  /* Signal handlers */
  void (*start) (GpaContext *context);
  void (*done) (GpaContext *context, gpg_error_t err);
  void (*next_key) (GpaContext *context, gpgme_key_t key);
  void (*next_trust_item) (GpaContext *context, gpgme_trust_item_t item);
  void (*progress) (GpaContext *context, int current, int total);
};

GType gpa_context_get_type (void) G_GNUC_CONST;

/* API */

/* Create a new GpaContext object.
 */
GpaContext *gpa_context_new (void);

/* TRUE if an operation is in progress.
 */
gboolean gpa_context_busy (GpaContext *context);

/* Return a string with the diagnostics from gpgme.  */
char *gpa_context_get_diag (GpaContext *context);

#endif /*GPA_CONTEXT_H*/
