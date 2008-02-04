/* clipboard.h  -  The GNU Privacy Assistant
 * Copyright (C) 2000 G-N-U GmbH.
 * Copyright (C) 2007, 2008 g10 Code GmbH
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

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <gtk/gtk.h>

/* Declare the Object. */
typedef struct _GpaClipboard GpaClipboard;
typedef struct _GpaClipboardClass GpaClipboardClass;

GType gpa_clipboard_get_type (void) G_GNUC_CONST;

#define GPA_CLIPBOARD_TYPE	  (gpa_clipboard_get_type ())

#define GPA_CLIPBOARD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_CLIPBOARD_TYPE, GpaClipboard))

#define GPA_CLIPBOARD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                            GPA_CLIPBOARD_TYPE, GpaClipboardClass))

#define GPA_IS_CLIPBOARD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_CLIPBOARD_TYPE))

#define GPA_IS_CLIPBOARD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_CLIPBOARD_TYPE))

#define GPA_CLIPBOARD_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_CLIPBOARD_TYPE, GpaClipboardClass))



/*  Our own API.  */

GtkWidget *gpa_clipboard_get_instance (void);

gboolean gpa_clipboard_is_open (void);

#endif /*CLIPBOARD_H*/
