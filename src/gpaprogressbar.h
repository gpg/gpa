/* gpaprogressbar.h - The GpaProgressBar object.
   Copyright (C) 2003 Miguel Coca.
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.  */

#ifndef GPA_PROGRESS_BAR_H
#define GPA_PROGRESS_BAR_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gpacontext.h"

/* GObject stuff */
#define GPA_PROGRESS_BAR_TYPE (gpa_progress_bar_get_type ())
#define GPA_PROGRESS_BAR(obj) \
 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_PROGRESS_BAR_TYPE, GpaProgressBar))
#define GPA_PROGRESS_BAR_CLASS(klass) \
 (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_PROGRESS_BAR_TYPE, GpaProgressBarClass))
#define GPA_IS_PROGRESS_BAR(obj) \
 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_PROGRESS_BAR_TYPE))
#define GPA_IS_PROGRESS_BAR_CLASS(klass) \
 (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_PROGRESS_BAR_TYPE))
#define GPA_PROGRESS_BAR_GET_CLASS(obj) \
 (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_PROGRESS_BAR_TYPE, GpaProgressBarClass))


typedef struct _GpaProgressBar GpaProgressBar;
typedef struct _GpaProgressBarClass GpaProgressBarClass;


struct _GpaProgressBar
{
  GtkProgressBar parent;

  GpaContext *context;

  /* Private.  */
  gulong sig_id_progress;
  gulong sig_id_start;
  gulong sig_id_done;
};


struct _GpaProgressBarClass
{
  GtkProgressBarClass parent_class;
};


GType gpa_progress_bar_get_type (void) G_GNUC_CONST;


/* API */

/* Create a new progress bar.  */
GtkWidget *gpa_progress_bar_new (void);

/* Create a new progress bar for the given context.  */
GtkWidget *gpa_progress_bar_new_with_context (GpaContext *context);

/* Set the context.  */
GpaContext *gpa_progress_bar_get_context (GpaProgressBar *pbar);

/* Get the context.  */
void gpa_progress_bar_set_context (GpaProgressBar *pbar, GpaContext *context);

#endif
