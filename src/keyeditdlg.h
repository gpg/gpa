/* keyeditdlg.h - The GpaKeyEditDialog object.
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

#ifndef GPA_KEY_EDIT_DIALOG_H
#define GPA_KEY_EDIT_DIALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gpacontext.h"

/* GObject stuff */
#define GPA_KEY_EDIT_DIALOG_TYPE	  (gpa_key_edit_dialog_get_type ())
#define GPA_KEY_EDIT_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_KEY_EDIT_DIALOG_TYPE, GpaKeyEditDialog))
#define GPA_KEY_EDIT_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_KEY_EDIT_DIALOG_TYPE, GpaKeyEditDialogClass))
#define GPA_IS_KEY_EDIT_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_KEY_EDIT_DIALOG_TYPE))
#define GPA_IS_KEY_EDIT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_KEY_EDIT_DIALOG_TYPE))
#define GPA_KEY_EDIT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_KEY_EDIT_DIALOG_TYPE, GpaKeyEditDialogClass))

typedef struct _GpaKeyEditDialog GpaKeyEditDialog;
typedef struct _GpaKeyEditDialogClass GpaKeyEditDialogClass;

struct _GpaKeyEditDialog {
  GtkDialog parent;

  /* Key being modified */
  gpgme_key_t key;

  /* Expiry date label */
  GtkWidget *expiry;
};

struct _GpaKeyEditDialogClass {
  GtkDialogClass parent_class;

  /* "Key modified" signal */
  void (*key_modified) (GpaKeyEditDialog *dialog, gpgme_key_t key);
};

GType gpa_key_edit_dialog_get_type (void) G_GNUC_CONST;

/* API */

/* Create a new "edit key" dialog.
 */
GtkWidget*
gpa_key_edit_dialog_new (GtkWidget *parent, gpgme_key_t key);

#endif
