/* gpawindowkeeper.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
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
#include <gtk/gtk.h>
#include "gpa.h"
#include "gpawindowkeeper.h"

static GList *tempWindows = NULL;

GpaWindowKeeper *
gpa_windowKeeper_new (void)
{
/* var */
  GpaWindowKeeper *keeper;
/* commands */
  keeper = (GpaWindowKeeper *) g_malloc (sizeof (GpaWindowKeeper));
  keeper->window = NULL;
  keeper->listParam = NULL;
  tempWindows = g_list_append (tempWindows, keeper);
  return (keeper);
}				/* GpaWindowKeeper */

void
gpa_windowKeeper_set_window (GpaWindowKeeper * keeper, GtkWidget * window)
{
  keeper->window = window;
}				/* gpa_windowKeeper_set_window */

void
gpa_windowKeeper_add_param (GpaWindowKeeper * keeper, gpointer param)
{
  keeper->listParam = g_list_append (keeper->listParam, param);
}				/* gpa_windowKeeper_add_param */

void
gpa_windowKeeper_release_exec (gpointer data, gpointer userData)
{
  if (data)
    g_free (data);
}				/* gpa_windowKeeper_release_exec */

void
gpa_windowKeeper_release (GpaWindowKeeper * keeper)
{
  gtk_widget_destroy (keeper->window);
  g_list_foreach (keeper->listParam, gpa_windowKeeper_release_exec, NULL);
  tempWindows = g_list_remove (tempWindows, keeper);
  g_free (keeper);
}				/* gpa_windowKeeper_release */
