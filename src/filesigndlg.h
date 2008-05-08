/* filesigndlg.h  -  The GNU Privacy Assistant
   Copyright (C) 2000 G-N-U GmbH.

   This file is part of GPA.

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


#ifndef FILESIGNDLG_H
#define FILESIGNDLG_H

#include <gtk/gtk.h>
#include <gpgme.h>

#include "options.h"

/* GObject stuff */
#define GPA_FILE_SIGN_DIALOG_TYPE	  (gpa_file_sign_dialog_get_type ())
#define GPA_FILE_SIGN_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_FILE_SIGN_DIALOG_TYPE, GpaFileSignDialog))
#define GPA_FILE_SIGN_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_FILE_SIGN_DIALOG_TYPE, GpaFileSignDialogClass))
#define GPA_IS_FILE_SIGN_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_FILE_SIGN_DIALOG_TYPE))
#define GPA_IS_FILE_SIGN_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_FILE_SIGN_DIALOG_TYPE))
#define GPA_FILE_SIGN_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_FILE_SIGN_DIALOG_TYPE, GpaFileSignDialogClass))

typedef struct _GpaFileSignDialog GpaFileSignDialog;
typedef struct _GpaFileSignDialogClass GpaFileSignDialogClass;

struct _GpaFileSignDialog
{
  GtkDialog parent;

  GtkWidget *frame_mode;
  GtkWidget *radio_comp;
  GtkWidget *radio_sign;
  GtkWidget *radio_sep;

  GtkWidget *check_armor;
  GtkWidget *clist_who;

  gboolean force_armor;
  gboolean force_sig_mode;
};

struct _GpaFileSignDialogClass
{
  GtkDialogClass parent_class;
};

GType gpa_file_sign_dialog_get_type (void) G_GNUC_CONST;

/* API */

GtkWidget *gpa_file_sign_dialog_new (GtkWidget *parent);

GList *gpa_file_sign_dialog_signers (GpaFileSignDialog *dialog);

gboolean gpa_file_sign_dialog_get_force_armor (GpaFileSignDialog *dialog);
void gpa_file_sign_dialog_set_force_armor (GpaFileSignDialog *dialog,
					   gboolean force_armor);

gboolean gpa_file_sign_dialog_get_armor (GpaFileSignDialog *dialog);
void gpa_file_sign_dialog_set_armor (GpaFileSignDialog *dialog,
				     gboolean armor);

gboolean gpa_file_sign_dialog_get_force_sig_mode (GpaFileSignDialog *dialog);
void gpa_file_sign_dialog_set_force_sig_mode (GpaFileSignDialog *dialog,
					   gboolean force_sig_mode);

gpgme_sig_mode_t gpa_file_sign_dialog_get_sig_mode (GpaFileSignDialog *dialog);
void gpa_file_sign_dialog_set_sig_mode (GpaFileSignDialog *dialog,
					gpgme_sig_mode_t mode);


#endif /* FILESIGNDLG_H */
