/* gpaprogressdlg.h - The GpaProgressDialog object.
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

#include "gpaprogressdlg.h"

/* Internal functions */
static void
gpa_progress_dialog_start_cb (GpaContext *context,
			      GpaProgressDialog *dialog);
static void
gpa_progress_dialog_done_cb (GpaContext *context, gpg_error_t err,
			     GpaProgressDialog *dialog);

/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_CONTEXT
};

static GObjectClass *parent_class = NULL;

static void
gpa_progress_dialog_get_property (GObject     *object,
				  guint        prop_id,
				  GValue      *value,
				  GParamSpec  *pspec)
{
  GpaProgressDialog *dialog = GPA_PROGRESS_DIALOG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value,
			  gtk_window_get_transient_for (GTK_WINDOW (dialog)));
      break;
    case PROP_CONTEXT:
      g_value_set_object (value, dialog->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_progress_dialog_set_property (GObject     *object,
				  guint        prop_id,
				  const GValue      *value,
				  GParamSpec  *pspec)
{
  GpaProgressDialog *dialog = GPA_PROGRESS_DIALOG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      gtk_window_set_transient_for (GTK_WINDOW (dialog),
				    g_value_get_object (value));
      break;
    case PROP_CONTEXT:
      dialog->context = (GpaContext*) g_value_get_object (value);
      /* Context signals */
      g_signal_connect (G_OBJECT (dialog->context), "start",
			G_CALLBACK (gpa_progress_dialog_start_cb), dialog);
      g_signal_connect (G_OBJECT (dialog->context), "done",
			G_CALLBACK (gpa_progress_dialog_done_cb), dialog);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_progress_dialog_finalize (GObject *object)
{  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_progress_dialog_class_init (GpaProgressDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gpa_progress_dialog_finalize;
  object_class->set_property = gpa_progress_dialog_set_property;
  object_class->get_property = gpa_progress_dialog_get_property;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_WINDOW,
				   g_param_spec_object 
				   ("window", "Parent window",
				    "Parent window", GTK_TYPE_WIDGET,
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_CONTEXT,
				   g_param_spec_object 
				   ("context", "context",
				    "context", GPA_CONTEXT_TYPE,
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}

static void
gpa_progress_dialog_init (GpaProgressDialog *dialog)
{
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
				  5);
  /* Elements */
  dialog->label = gtk_label_new (NULL);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			       dialog->label);
  dialog->progress_bar = gtk_progress_bar_new ();
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			       dialog->progress_bar);
  /* Set up the dialog */
  gtk_dialog_add_button (GTK_DIALOG (dialog),
			 GTK_STOCK_CANCEL,
			 GTK_RESPONSE_CANCEL);
}

GType
gpa_progress_dialog_get_type (void)
{
  static GType progress_dialog_type = 0;
  
  if (!progress_dialog_type)
    {
      static const GTypeInfo progress_dialog_info =
      {
        sizeof (GpaProgressDialogClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_progress_dialog_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaProgressDialog),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_progress_dialog_init,
      };
      
      progress_dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
						     "GpaProgressDialog",
						     &progress_dialog_info, 0);
    }
  
  return progress_dialog_type;
}

/* API */

/* Create a new progress dialog for the given context.
 */
GtkWidget*
gpa_progress_dialog_new (GtkWidget *parent, 
			 GpaContext *context)
{
  GpaProgressDialog *dialog;
  
  dialog = g_object_new (GPA_PROGRESS_DIALOG_TYPE,
			 "window", parent,
			 "context", context,
			 NULL);

  return GTK_WIDGET(dialog);
}

/* Set the dialog label.
 */
void
gpa_progress_dialog_set_label (GpaProgressDialog *dialog, const gchar *label)
{
  gtk_label_set_text (GTK_LABEL (dialog->label), label);
}

/* Internal functions */

static gboolean
gpa_progress_dialog_pulse (gpointer data)
{
  GpaProgressDialog *dialog = data;

  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (dialog->progress_bar));

  return TRUE;
}

static void
gpa_progress_dialog_start_cb (GpaContext *context,
			      GpaProgressDialog *dialog)
{
  dialog->timer = gtk_timeout_add (100, gpa_progress_dialog_pulse,
				   dialog);
}

static void
gpa_progress_dialog_done_cb (GpaContext *context, gpg_error_t err,
			     GpaProgressDialog *dialog)
{
  gtk_idle_remove (dialog->timer);
}

