/* gparecvkeydlg.h - The GpaReceiveKeyDialog object.
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

#ifndef GPA_RECEIVE_KEY_DIALOG_H
#define GPA_RECEIVE_KEY_DIALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gpacontext.h"

/* GObject stuff */
#define GPA_RECEIVE_KEY_DIALOG_TYPE	  (gpa_receive_key_dialog_get_type ())
#define GPA_RECEIVE_KEY_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_RECEIVE_KEY_DIALOG_TYPE, GpaReceiveKeyDialog))
#define GPA_RECEIVE_KEY_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_RECEIVE_KEY_DIALOG_TYPE, GpaReceiveKeyDialogClass))
#define GPA_IS_RECEIVE_KEY_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_RECEIVE_KEY_DIALOG_TYPE))
#define GPA_IS_RECEIVE_KEY_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_RECEIVE_KEY_DIALOG_TYPE))
#define GPA_RECEIVE_KEY_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_RECEIVE_KEY_DIALOG_TYPE, GpaReceiveKeyDialogClass))

typedef struct _GpaReceiveKeyDialog GpaReceiveKeyDialog;
typedef struct _GpaReceiveKeyDialogClass GpaReceiveKeyDialogClass;

struct _GpaReceiveKeyDialog {
  GtkDialog parent;

  GtkWidget *entry;
};

struct _GpaReceiveKeyDialogClass {
  GtkDialogClass parent_class;
};

GType gpa_receive_key_dialog_get_type (void) G_GNUC_CONST;

/* API */

/* Create a new receive key dialog.
 */
GtkWidget*
gpa_receive_key_dialog_new (GtkWidget *parent);

/* Retrieve the selected key ID.
 */
const gchar*
gpa_receive_key_dialog_get_id (GpaReceiveKeyDialog *dialog);

#endif
