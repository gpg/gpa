/* gpa-key-window.h - per-key window
 *      Copyright (C) 2009 g10 Code GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.  
 */

#ifndef GPA_KEY_WINDOW_H
#define GPA_KEY_WINDOW_H

#include <glib.h>
#include <glib-object.h>

#include <gpgme.h>
#include "gpa-key-details.h"

#define GPA_KEY_WINDOW_TYPE            (gpa_key_window_get_type ())
#define GPA_KEY_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_KEY_WINDOW_TYPE, GpaKeyWindow))
#define GPA_KEY_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_KEY_WINDOW_TYPE, GpaKeyWindowClass))
#define IS_GPA_KEY_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_KEY_WINDOW_TYPE))
#define IS_GPA_KEY_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_KEY_WINDOW_TYPE))

typedef struct _GpaKeyWindow      GpaKeyWindow;
typedef struct _GpaKeyWindowClass GpaKeyWindowClass;

struct _GpaKeyWindow
{
  GtkDialog parent;

  GtkUIManager *ui_manager;
  GtkWidget *icon;
  GtkWidget *title_label;
  GtkWidget *key_details;
  gpgme_key_t key;
};

struct _GpaKeyWindowClass
{
  GtkDialogClass parent_class;
};

GType gpa_key_window_get_type (void) G_GNUC_CONST;

#if 0
#define GPA_KEY_DETAILS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_KEY_DETAILS_TYPE, GpaKeyDetailsClass))
#endif



/*
 * Public API.
 */

/* Create and return a new GpaKeyWindow widget. */
GtkWidget *gpa_key_window_new    (void);

/* Update the key window WIDGET with the key KEY. */
void       gpa_key_window_update (GtkWidget *key_window, gpgme_key_t key);

#endif /* GPA_KEY_WINDOW_H */
