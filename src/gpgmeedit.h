/* gpgmeedit.h - The GNU Privacy Assistant
 *      Copyright (C) 2002, Miguel Coca.
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

/* Wrappers around gpgme_op_edit for common tasks */

#ifndef GPGMEEDIT_H
#define GPGMEEDIT_H

#include "gpa.h"
#include <glib.h>
#include <gpgme.h>

/* Change the ownertrust of a key */
gpgme_error_t gpa_gpgme_edit_trust (gpgme_ctx_t ctx, gpgme_key_t key,
				    gpgme_validity_t ownertrust);

/* Change the expiry date of a key */
gpgme_error_t gpa_gpgme_edit_expire (gpgme_ctx_t ctx, gpgme_key_t key, 
				     GDate *date);

/* Sign this key with the given private key. If local is true, make a local
 * signature. */
gpgme_error_t gpa_gpgme_edit_sign (gpgme_ctx_t ctx, gpgme_key_t key,
				   gpgme_key_t secret_key,
				   gboolean local);

/* Change the key's passphrase.
 */
gpgme_error_t gpa_gpgme_edit_passwd (gpgme_ctx_t ctx, gpgme_key_t key);

#endif
