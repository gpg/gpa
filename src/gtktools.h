/* gtktools.h  -  The GNU Privacy Assistant
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2008 g10 Code GmbH.


   This file is part of GPA

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


#ifndef GTK_TOOLS_H_
#define GTK_TOOLS_H_

#include <gtk/gtk.h>

extern void gpa_connect_by_accelerator (GtkLabel * label, GtkWidget * widget,
					GtkAccelGroup * accelGroup,
					gchar * labelText);
extern void gpa_window_show_centered (GtkWidget * widget, GtkWidget * parent);
extern void gpa_window_error (const gchar * message, GtkWidget * messenger);
extern void gpa_window_message (gchar * message, GtkWidget * messenger);


/* Set a tooltip TEXT to WIDGET.  TEXT and WIDGET may both be NULL.
   This function is useful so that GPA can be build with older GTK+
   versions.  */ 
void gpa_add_tooltip (GtkWidget *widget, const char *text);

/* Set the title of COLUMN to TITLE and also set TOOLTIP. */
void gpa_set_column_title (GtkTreeViewColumn *column,
                           const char *title, const char *tooltip);


#endif /* GTK_TOOLS_H_ */
