/* encryptdlg.h  -  The GNU Privacy Assistant
 * Copyright (C) 2000 G-N-U GmbH.
 * Copyright (C) 2008 g10 Code GmbH.
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

#ifndef ENCRYPTDLG_H
#define ENCRYPTDLG_H

#include <gtk/gtk.h>

/* GObject stuff */
#define GPA_FILE_ENCRYPT_DIALOG_TYPE	  (gpa_file_encrypt_dialog_get_type ())
#define GPA_FILE_ENCRYPT_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_FILE_ENCRYPT_DIALOG_TYPE, GpaFileEncryptDialog))
#define GPA_FILE_ENCRYPT_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_FILE_ENCRYPT_DIALOG_TYPE, GpaFileEncryptDialogClass))
#define GPA_IS_FILE_ENCRYPT_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_FILE_ENCRYPT_DIALOG_TYPE))
#define GPA_IS_FILE_ENCRYPT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_FILE_ENCRYPT_DIALOG_TYPE))
#define GPA_FILE_ENCRYPT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_FILE_ENCRYPT_DIALOG_TYPE, GpaFileEncryptDialogClass))

typedef struct _GpaFileEncryptDialog GpaFileEncryptDialog;
typedef struct _GpaFileEncryptDialogClass GpaFileEncryptDialogClass;

struct _GpaFileEncryptDialog {
  GtkDialog parent;

  GtkWidget *clist_keys;
  GtkWidget *check_sign;
  GtkWidget *check_armor;
  GtkWidget *clist_who;
  /* FIXME: See comment in encryptdlg.h.  */
  GtkWidget *scroller_who;

  gboolean force_armor;
};

struct _GpaFileEncryptDialogClass {
  GtkDialogClass parent_class;
};

GType gpa_file_encrypt_dialog_get_type (void) G_GNUC_CONST;

/* API */

GtkWidget *gpa_file_encrypt_dialog_new (GtkWidget *parent,
					gboolean force_armor);

GList *gpa_file_encrypt_dialog_recipients (GpaFileEncryptDialog *dialog);

gboolean gpa_file_encrypt_dialog_get_sign (GpaFileEncryptDialog *dialog);
void gpa_file_encrypt_dialog_set_sign (GpaFileEncryptDialog *dialog,
				       gboolean sign);

GList *gpa_file_encrypt_dialog_signers (GpaFileEncryptDialog *dialog);

gboolean gpa_file_encrypt_dialog_get_armor (GpaFileEncryptDialog *dialog);
void gpa_file_encrypt_dialog_set_armor (GpaFileEncryptDialog *dialog,
					gboolean armor);

#endif /* ENCRYPTDLG_H */
