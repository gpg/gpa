/* gpapa.h - The GNU Privacy Assistant Pipe Access - main header
 * Copyright (C) 2000, 2001 G-N-U GmbH, http://www.g-n-u.de
 *
 * This file is part of GPAPA.
 *
 * GPAPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPAPA is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GPAPA; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GPAPA_H__
#define __GPAPA_H__

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xmalloc.h"
#include "../src/i18n.h"

#include <glib.h>

#include "gpapatypedefs.h"
#include "gpapafile.h"
#include "gpapakey.h"
#include "gpapapublickey.h"
#include "gpapasecretkey.h"
#include "gpapasignature.h"
#include "gpapaintern.h"

#define GPAPA_MAX_GPG_KEY_FIELDS 20

extern char *global_keyServer;

#if defined(__MINGW32__) || defined(HAVE_DOSISH_SYSTEM)
/* Defining __USE_HKP__ means to connect directly to keyservers
 * instead of running `gpg --recv-keys'.
 */
#define __USE_HKP__
extern gchar *hkp_errtypestr[];
#endif

/* Key management.
 */

extern GDate *gpapa_extract_date (char *buffer);

extern char *gpapa_extract_fingerprint (char *line, int algorithm,
                                        GpapaCallbackFunc callback, gpointer calldata);

extern void gpapa_refresh_public_keyring (GpapaCallbackFunc callback,
					  gpointer calldata);

extern gint gpapa_get_public_key_count (GpapaCallbackFunc callback,
					gpointer calldata);

extern GpapaPublicKey *gpapa_get_public_key_by_index (gint idx,
						      GpapaCallbackFunc
						      callback,
						      gpointer calldata);

extern GpapaPublicKey *gpapa_get_public_key_by_ID (const gchar *keyID,
						   GpapaCallbackFunc callback,
						   gpointer calldata);

extern GpapaPublicKey *gpapa_get_public_key_by_userID (const gchar *userID,
						   GpapaCallbackFunc callback,
						   gpointer calldata);

extern void gpapa_report_hkp_error (int rc, GpapaCallbackFunc callback,
                                            gpointer calldata);

extern GpapaPublicKey *gpapa_receive_public_key_from_server (const gchar *keyID,
                                                             const gchar *ServerName,
                                                             GpapaCallbackFunc callback,
                                                             gpointer calldata);

extern GList *gpapa_search_public_keys_on_server (const gchar *keyID,
                                                  const gchar *ServerName,
                                                  GpapaCallbackFunc callback,
                                                  gpointer calldata);

extern void gpapa_release_public_key (GpapaPublicKey *key,
				      GpapaCallbackFunc callback,
				      gpointer calldata);

extern void gpapa_refresh_secret_keyring (GpapaCallbackFunc callback,
					  gpointer calldata);

extern gint gpapa_get_secret_key_count (GpapaCallbackFunc callback,
					gpointer calldata);

extern GpapaSecretKey *gpapa_get_secret_key_by_index (gint idx,
						      GpapaCallbackFunc
						      callback,
						      gpointer calldata);

extern GpapaSecretKey *gpapa_get_secret_key_by_ID (const gchar *keyID,
						   GpapaCallbackFunc callback,
						   gpointer calldata);

extern GpapaSecretKey *gpapa_get_secret_key_by_userID (const gchar *userID,
						   GpapaCallbackFunc callback,
						   gpointer calldata);

extern void gpapa_release_secret_key (GpapaSecretKey *key,
				      GpapaCallbackFunc callback,
				      gpointer calldata);

extern void gpapa_create_key_pair (GpapaPublicKey **publicKey,
				   GpapaSecretKey **secretKey,
				   char *passphrase, GpapaAlgo anAlgo,
				   gint aKeysize, char *aUserID,
				   char *anEmail, char *aComment,
				   GpapaCallbackFunc callback,
				   gpointer calldata);

extern void gpapa_export_ownertrust (const gchar *targetFileID, GpapaArmor Armor,
				     GpapaCallbackFunc callback,
				     gpointer calldata);

extern void gpapa_import_ownertrust (const gchar *sourceFileID,
				     GpapaCallbackFunc callback,
				     gpointer calldata);

extern void gpapa_update_trust_database (GpapaCallbackFunc callback,
					 gpointer calldata);

extern void gpapa_import_keys (const gchar *sourceFileID,
			       GpapaCallbackFunc callback, gpointer calldata);

extern void gpapa_import_keys_from_clipboard (GpapaCallbackFunc callback,
                                              gpointer calldata);

/* Miscellaneous.
 */

extern void gpapa_init (const char *gpg);

extern void gpapa_fini (void);

extern void gpapa_idle (void);

#endif /* __GPAPA_H__ */
