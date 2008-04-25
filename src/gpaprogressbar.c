/* gpaprogressbar.c - The GpaProgressBar object.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gpaprogressbar.h"
#include "i18n.h"


/* Properties.  */
enum
{
  PROP_0,
  PROP_CONTEXT
};


static GObjectClass *parent_class = NULL;


static void
gpa_progress_bar_get_property (GObject *object, guint prop_id, GValue *value,
			       GParamSpec *pspec)
{
  GpaProgressBar *pbar = GPA_PROGRESS_BAR (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, pbar->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_progress_bar_set_property (GObject *object, guint prop_id,
			       const GValue *value, GParamSpec *pspec)
{
  GpaProgressBar *pbar = GPA_PROGRESS_BAR (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      gpa_progress_bar_set_context (pbar, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_progress_bar_finalize (GObject *object)
{  
  GpaProgressBar *pbar = GPA_PROGRESS_BAR (object);
  GpaContext *context = pbar->context;

  if (context)
    {
      g_signal_handler_disconnect (G_OBJECT (context), pbar->sig_id_start);
      g_signal_handler_disconnect (G_OBJECT (context), pbar->sig_id_done);
      g_signal_handler_disconnect (G_OBJECT (context), pbar->sig_id_progress);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_progress_bar_class_init (GpaProgressBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gpa_progress_bar_finalize;
  object_class->set_property = gpa_progress_bar_set_property;
  object_class->get_property = gpa_progress_bar_get_property;

  /* Properties.  */
  g_object_class_install_property (object_class,
				   PROP_CONTEXT,
				   g_param_spec_object 
				   ("context", "context",
				    "context", GPA_CONTEXT_TYPE,
				    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


static void
gpa_progress_bar_init (GpaProgressBar *pbar)
{
}


GType
gpa_progress_bar_get_type (void)
{
  static GType progress_bar_type = 0;
  
  if (! progress_bar_type)
    {
      static const GTypeInfo progress_bar_info =
      {
        sizeof (GpaProgressBarClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_progress_bar_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaProgressBar),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_progress_bar_init,
      };
      
      progress_bar_type = g_type_register_static (GTK_TYPE_PROGRESS_BAR,
						  "GpaProgressBar",
						  &progress_bar_info, 0);
    }
  
  return progress_bar_type;
}


/* API.  */

/* Create a new progress bar.  */
GtkWidget *
gpa_progress_bar_new (void)
{
  GpaProgressBar *pbar;
  
  pbar = g_object_new (GPA_PROGRESS_BAR_TYPE, NULL);

  return GTK_WIDGET (pbar);
}


/* Create a new progress bar for the given context.  */
GtkWidget *
gpa_progress_bar_new_with_context (GpaContext *context)
{
  GpaProgressBar *pbar;
  
  pbar = g_object_new (GPA_PROGRESS_BAR_TYPE, "context", context, NULL);

  return GTK_WIDGET (pbar);
}


static void
progress_cb (GpaContext *context, int current, int total, GpaProgressBar *pbar)
{
  if (total > 0) 
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar),
				   (gdouble) current / (gdouble) total);
  else
    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (pbar));
}


static void
start_cb (GpaContext *context, GpaProgressBar *pbar)
{
  progress_cb (context, 0, 1, pbar);
}


static void
done_cb (GpaContext *context, gpg_error_t err, GpaProgressBar *pbar)
{
  progress_cb (context, 1, 1, pbar);
}


GpaContext *
gpa_progress_bar_get_context (GpaProgressBar *pbar)
{
  g_return_val_if_fail (GTK_IS_PROGRESS_BAR (pbar), 0);
  return pbar->context;
}


void
gpa_progress_bar_set_context (GpaProgressBar *pbar, GpaContext *context)
{
  g_return_if_fail (GTK_IS_PROGRESS_BAR (pbar));

  if (pbar->context)
    {
      g_signal_handler_disconnect (G_OBJECT (pbar->context),
				   pbar->sig_id_start);
      g_signal_handler_disconnect (G_OBJECT (pbar->context),
				   pbar->sig_id_done);
      g_signal_handler_disconnect (G_OBJECT (pbar->context),
				   pbar->sig_id_progress);
    }
  
  pbar->context = context;
  if (context)
    {
      pbar->sig_id_start = g_signal_connect (G_OBJECT (context), "start",
					     G_CALLBACK (start_cb), pbar);
      pbar->sig_id_done = g_signal_connect (G_OBJECT (context), "done",
					    G_CALLBACK (done_cb), pbar);
      pbar->sig_id_progress = g_signal_connect (G_OBJECT (context), "progress",
						G_CALLBACK (progress_cb), pbar);
    }
}
