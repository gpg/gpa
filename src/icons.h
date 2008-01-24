/* icons.h  -  Icons for GPA
   Copyright (C) 2000 G-N-U GmbH.
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

#ifndef ICONS_H
#define ICONS_H

GtkWidget *gpa_create_icon_widget (GtkWidget *window, const char *name);

GdkPixmap *gpa_create_icon_pixmap (GtkWidget *window, const char *name,
                                   GdkBitmap **mask);

GdkPixbuf *gpa_create_icon_pixbuf (const char *name);
     
#endif /*ICONS_H*/
