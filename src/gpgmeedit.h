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
GpgmeError gpa_gpgme_edit_trust (GpgmeCtx ctx, GpgmeKey key,
				 GpgmeValidity ownertrust);

/* Change the expiry date of a key */
GpgmeError gpa_gpgme_edit_expire (GpgmeCtx ctx, GpgmeKey key, GDate *date);

/* Sign this key with the given private key. If local is true, make a local
 * signature. */
GpgmeError gpa_gpgme_edit_sign (GpgmeCtx ctx, GpgmeKey key,
				const gchar *private_key_fpr, gboolean local);

/* Change the key's passphrase.
 */
GpgmeError gpa_gpgme_edit_passwd (GpgmeCtx ctx, GpgmeKey key);

#endif
