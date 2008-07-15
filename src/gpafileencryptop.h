/* gpafileencryptop.h - The GpaFileEncryptOperation object.
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

#ifndef GPA_FILE_ENCRYPT_OP_H
#define GPA_FILE_ENCRYPT_OP_H

#include <glib.h>
#include <glib-object.h>
#include "gpafileop.h"

/* GObject stuff */
#define GPA_FILE_ENCRYPT_OPERATION_TYPE	\
          (gpa_file_encrypt_operation_get_type ())

#define GPA_FILE_ENCRYPT_OPERATION(obj)	\
          (G_TYPE_CHECK_INSTANCE_CAST   \
            ((obj), GPA_FILE_ENCRYPT_OPERATION_TYPE, GpaFileEncryptOperation))

#define GPA_FILE_ENCRYPT_OPERATION_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_FILE_ENCRYPT_OPERATION_TYPE, \
                                    GpaFileEncryptOperationClass))

#define GPA_IS_FILE_ENCRYPT_OPERATION(obj) \
          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_FILE_ENCRYPT_OPERATION_TYPE))

#define GPA_IS_FILE_ENCRYPT_OPERATION_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_FILE_ENCRYPT_OPERATION_TYPE))

#define GPA_FILE_ENCRYPT_OPERATION_GET_CLASS(obj) \
          (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_FILE_ENCRYPT_OPERATION_TYPE, \
                                      GpaFileEncryptOperationClass))

typedef struct _GpaFileEncryptOperation GpaFileEncryptOperation;
typedef struct _GpaFileEncryptOperationClass GpaFileEncryptOperationClass;


struct _GpaFileEncryptOperation 
{
  GpaFileOperation parent;
  
  GtkWidget *encrypt_dialog;
  gpgme_key_t *rset;
  int cipher_fd, plain_fd;
  gpgme_data_t cipher, plain;

  gboolean force_armor;
};


struct _GpaFileEncryptOperationClass 
{
  GpaFileOperationClass parent_class;
};


GType gpa_file_encrypt_operation_get_type (void) G_GNUC_CONST;

/* API */

/* Creates a new encryption operation. */
GpaFileEncryptOperation *gpa_file_encrypt_operation_new (GtkWidget *window,
							 GList *files,
							 gboolean force_armor);

GpaFileEncryptOperation *gpa_file_encrypt_sign_operation_new
(GtkWidget *window, GList *files, gboolean force_armor);

/* Create a new encryption operaion for the UI server.  */
GpaFileEncryptOperation*
gpa_file_encrypt_operation_new_for_server (GList *files, void *server_ctx);

#endif
