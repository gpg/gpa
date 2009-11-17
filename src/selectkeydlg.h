/* selectkeydlg.h - A dialog to select a key.
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

#ifndef SELECTKEYDLG_H
#define SELECTKEYDLG_H

/*
   This dialog allow the selection of a single public or secret key.
   The visual layout is:

      +-----------------------+ +------+
      |                       | |Search|
      +-----------------------+ +------+
       (x) PGP  (x) x509  
      +--------------------------------+
      | List of keys                   |
      |                                |
      |                                |
      |                                |
      |                                |
      |                                |
      |                                |
      |                                |
      +--------------------------------+
                        +------++------+
                        |Select||Cancel|
                        +------++------+

    Initially the search box is empty and the list of keys is filled
    with the supplied keys.  The caller may choose whether PGP, X.509
    or both keys are displayed.  The default is both.  If the caller
    specified one mode, the user is not allowed to override it and the
    list will be filtered for displaying only supplied keys of the
    specfied type.  If none are specified, the user may restrict the
    search to one protocol.

    Usage:

       WORK IN PROGRESS


 */








/* Definitions to define the object.  */
#define SELECT_KEY_DLG_TYPE \
          (select_key_dlg_get_type ())

#define SELECT_KEY_DLG(obj) \
          (G_TYPE_CHECK_INSTANCE_CAST \
            ((obj), SELECT_KEY_DLG_TYPE,\
              SelectKeyDlg))

#define SELECT_KEY_DLG_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_CAST \
            ((klass), SELECT_KEY_DLG_TYPE, \
              SelectKeyDlgClass))

#define IS_SELECT_KEY_DLG(obj) \
          (G_TYPE_CHECK_INSTANCE_TYPE \
            ((obj), SELECT_KEY_DLG_TYPE))

#define IS_SELECT_KEY_DLG_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_TYPE \
            ((klass), SELECT_KEY_DLG_TYPE))

#define SELECT_KEY_DLG_GET_CLASS(obj) \
          (G_TYPE_INSTANCE_GET_CLASS \
            ((obj), SELECT_KEY_DLG_TYPE, \
              SelectKeyDlgClass))

typedef struct _SelectKeyDlg SelectKeyDlg;
typedef struct _SelectKeyDlgClass SelectKeyDlgClass;


GType select_key_dlg_get_type (void) G_GNUC_CONST;


/************************************
 ************ Public API ************
 ************************************/

SelectKeyDlg *select_key_dlg_new (GtkWidget *parent);
SelectKeyDlg *select_key_dlg_new_with_keys (GtkWidget *parent,
                                            gpgme_protocol_t protocol,
                                            gpgme_key_t *keys,
                                            const char *pattern);

gpgme_key_t select_key_dlg_get_key (SelectKeyDlg *dialog);
gpgme_key_t *select_key_dlg_get_keys (SelectKeyDlg *dialog);



#endif /*SELECTKEYDLG_H*/
