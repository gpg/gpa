/* gpa_gtktools.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000 Free Software Foundation, Inc.
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

#include <gtk/gtk.h>

GtkWidget *gpa_space_new () {
  return gtk_label_new ( "" );
} /* gpa_space_new */

GtkWidget *gpa_widget_hjustified_new (
  GtkWidget *widget, GtkJustification jtype
) {
/* objects */
  GtkWidget *result;
    GtkWidget *space;
/* commands */
  switch ( jtype ) {
    case GTK_JUSTIFY_LEFT:
      result = gtk_hbox_new ( FALSE, 0 );
      gtk_box_pack_start ( GTK_BOX ( result ), widget, FALSE, FALSE, 0 );
      space = gpa_space_new ();
      gtk_box_pack_start ( GTK_BOX ( result ), space, TRUE, TRUE, 0 );
      break;
    case GTK_JUSTIFY_RIGHT:
      result = gtk_hbox_new ( FALSE, 0 );
      space = gpa_space_new ();
      gtk_box_pack_start ( GTK_BOX ( result ), space, TRUE, TRUE, 0 );
      gtk_box_pack_start ( GTK_BOX ( result ), widget, FALSE, FALSE, 0 );
      break;
    default: result = widget;
  } /* switch */
  return result;
} /* gpa_widget_hjustified_new */
