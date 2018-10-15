/* gtktools.c  -  The GNU Privacy Assistant
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2008, 2014 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* NOTE: Here are a lot of old GTK+ functions and wrappers.  They
   should be replaced by modern GTK+ code and some of the wrappers are
   not needed anymore. */

#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gpa.h"
#include "gtktools.h"
#include "gpawindowkeeper.h"
#include "icons.h"

/* Deprecated - use gpa_show_warning instead.  */
void
gpa_window_error (const gchar *message, GtkWidget *messenger)
{
  gpa_show_warn (messenger, NULL, "%s", message);
}


/* Deprecated - use gpa_show_info instead.  */
void
gpa_window_message (const gchar *message, GtkWidget * messenger)
{
  gpa_show_info (messenger, "%s", message);
}


static void
show_gtk_message (GtkWidget *parent, GtkMessageType mtype, GpaContext *ctx,
                  const char *format, va_list arg_ptr)
{
  GtkWidget *dialog;
  char *buffer;

  buffer = g_strdup_vprintf (format, arg_ptr);
  dialog = gtk_message_dialog_new (parent? GTK_WINDOW (parent):NULL,
                                   GTK_DIALOG_MODAL,
                                   mtype,
                                   GTK_BUTTONS_CLOSE,
                                   "%s", buffer);
  g_free (buffer);
  buffer = NULL;
  if (ctx)
    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            _("_Details"), GTK_RESPONSE_HELP,
                            NULL);

  gtk_widget_show_all (dialog);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_HELP && ctx)
    {
      /* If requested and possible get diagnostics from GPGME.  */
      buffer = gpa_context_get_diag (ctx);
      if (!buffer)
        gpa_show_info (parent, "No diagnostic data available");
      else
        {
          gpa_show_info (parent, "Diagnostics:\n%s", buffer);
          g_free (buffer);
        }
    }

  gtk_widget_destroy (dialog);
}


/* Show a modal info message. */
void
gpa_show_info (GtkWidget *parent, const char *format, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, format);
  show_gtk_message (parent, GTK_MESSAGE_INFO, NULL, format, arg_ptr);
  va_end (arg_ptr);
}


/* Show a modal warning message.  PARENT is the parent windows, CTX is
 * eitehr NULL or a related GPGME context to be used to allow shoing
 * additional information. */
void
gpa_show_warn (GtkWidget *parent, GpaContext *ctx, const char *format, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, format);
  show_gtk_message (parent, GTK_MESSAGE_WARNING, ctx, format, arg_ptr);
  va_end (arg_ptr);
}


/* Set a tooltip TEXT to WIDGET.  TEXT and WIDGET may both be NULL.
   This function is useful so that GPA can be build with older GTK+
   versions.  */
void
gpa_add_tooltip (GtkWidget *widget, const char *text)
{
#if GTK_CHECK_VERSION (2, 12, 0)
  if (widget && text && *text)
    gtk_widget_set_tooltip_text (widget, text);
#endif
}

/* Set the title of COLUMN to TITLE and also set TOOLTIP. */
void
gpa_set_column_title (GtkTreeViewColumn *column,
                      const char *title, const char *tooltip)
{
  GtkWidget *label;

  label = gtk_label_new (title);
  /* We need to show the label before setting the widget.  */
  gtk_widget_show (label);
  gtk_tree_view_column_set_widget (column, label);
  if (tooltip)
    gpa_add_tooltip (gtk_tree_view_column_get_widget (column), tooltip);
}


static void
set_homogeneous (GtkWidget *widget, gpointer data)
{
  gboolean *is_hom_p = data;

  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), *is_hom_p);
}


/* Set the homogeneous property for all children of TOOLBAR to IS_HOM.  */
void
gpa_toolbar_set_homogeneous (GtkToolbar *toolbar, gboolean is_hom)
{
  gtk_container_foreach (GTK_CONTAINER (toolbar),
			 (GtkCallback) set_homogeneous, &is_hom);
}


/* Customized set title function.  */
void
gpa_window_set_title (GtkWindow *window, const char *string)
{
  const char *prefix = GPA_LONG_NAME;
  char *buffer;

  if (!string || !*string)
    {
      gtk_window_set_title (window, prefix);
    }
  else
    {
      buffer = g_strdup_printf ("%s - %s", prefix, string);
      gtk_window_set_title (window, buffer);
      g_free (buffer);
    }
}
