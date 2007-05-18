/* fileman.h  -  The GNU Privacy Assistant
 *	Copyright (C) 2000 G-N-U GmbH.
 *      Copyright (C) 2007 g10 Code GmbH
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef FILEMAN_H
#define FILEMAN_H

#include <gtk/gtk.h>

/* Declare the Object. */
typedef struct _GpaFileManager GpaFileManager;
typedef struct _GpaFileManagerClass GpaFileManagerClass;

GType gpa_file_manager_get_type (void) G_GNUC_CONST;

#define GPA_FILE_MANAGER_TYPE	  (gpa_file_manager_get_type ())

#define GPA_FILE_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_FILE_MANAGER_TYPE, GpaFileManager))

#define GPA_FILE_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                            GPA_FILE_MANAGER_TYPE, GpaFileManagerClass))

#define GPA_IS_FILE_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_FILE_MANAGER_TYPE))

#define GPA_IS_FILE_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_FILE_MANAGER_TYPE))

#define GPA_FILE_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_FILE_MANAGER_TYPE, GpaFileManagerClass))



/*  Our own API.  */

GtkWidget *gpa_file_manager_get_instance (void);

gboolean gpa_file_manager_is_open (void);

void gpa_file_manager_open_file (GpaFileManager *fileman,
				 const char *filename);


#endif /*FILEMAN_H*/
