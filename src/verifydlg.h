/* verifydlg.h - The GpaFileVerifyDialog object.
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

#ifndef GPA_FILE_VERIFY_DIALOG_H
#define GPA_FILE_VERIFY_DIALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gpacontext.h"

/* GObject stuff */
#define GPA_FILE_VERIFY_DIALOG_TYPE	  (gpa_file_verify_dialog_get_type ())
#define GPA_FILE_VERIFY_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_FILE_VERIFY_DIALOG_TYPE, GpaFileVerifyDialog))
#define GPA_FILE_VERIFY_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_FILE_VERIFY_DIALOG_TYPE, GpaFileVerifyDialogClass))
#define GPA_IS_FILE_VERIFY_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_FILE_VERIFY_DIALOG_TYPE))
#define GPA_IS_FILE_VERIFY_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_FILE_VERIFY_DIALOG_TYPE))
#define GPA_FILE_VERIFY_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_FILE_VERIFY_DIALOG_TYPE, GpaFileVerifyDialogClass))

typedef struct _GpaFileVerifyDialog GpaFileVerifyDialog;
typedef struct _GpaFileVerifyDialogClass GpaFileVerifyDialogClass;

struct _GpaFileVerifyDialog {
  GtkDialog parent;

  GtkWidget *notebook;
  /* Context for retrieving signature's keys */
  GpaContext *ctx;
};

struct _GpaFileVerifyDialogClass {
  GtkDialogClass parent_class;
};

GType gpa_file_verify_dialog_get_type (void) G_GNUC_CONST;

/* API */

GtkWidget *gpa_file_verify_dialog_new (GtkWidget *parent);

void gpa_file_verify_dialog_set_title (GpaFileVerifyDialog *dialog, 
                                       const char *title);

void gpa_file_verify_dialog_add_file (GpaFileVerifyDialog *dialog,
				      const gchar *filename,
				      const gchar *signed_file,
				      const gchar *signature_file,
				      gpgme_signature_t sigs);

#endif
