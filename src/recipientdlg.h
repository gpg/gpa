/* recipientdlg.h - A dialog to select a mail recipient.
 * Copyright (C) 2008 g10 Code GmbH.
 *
 * This file is part of GPA
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

#ifndef RECIPIENTDLG_H
#define RECIPIENTDLG_H


/* Definitions to define the object.  */
#define RECIPIENT_DLG_TYPE \
          (recipient_dlg_get_type ())

#define RECIPIENT_DLG(obj) \
          (G_TYPE_CHECK_INSTANCE_CAST \
            ((obj), RECIPIENT_DLG_TYPE,\
              RecipientDlg))

#define RECIPIENT_DLG_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_CAST \
            ((klass), RECIPIENT_DLG_TYPE, \
              RecipientDlgClass))

#define IS_RECIPIENT_DLG(obj) \
          (G_TYPE_CHECK_INSTANCE_TYPE \
            ((obj), RECIPIENT_DLG_TYPE))

#define IS_RECIPIENT_DLG_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_TYPE \
            ((klass), RECIPIENT_DLG_TYPE))

#define RECIPIENT_DLG_GET_CLASS(obj) \
          (G_TYPE_INSTANCE_GET_CLASS \
            ((obj), RECIPIENT_DLG_TYPE, \
              RecipientDlgClass))

typedef struct _RecipientDlg RecipientDlg;
typedef struct _RecipientDlgClass RecipientDlgClass;


GType recipient_dlg_get_type (void) G_GNUC_CONST;


/************************************
 ************ Public API ************
 ************************************/

RecipientDlg *recipient_dlg_new (GtkWidget *parent);
void recipient_dlg_set_recipients (RecipientDlg *dialog, GSList *recipients,
                                   gpgme_protocol_t protocol);
gpgme_key_t *recipient_dlg_get_keys (RecipientDlg *dialog,
                                     gpgme_protocol_t *r_protocol);



#endif /*RECIPIENTDLG_H*/
