/* gpapa.h  -  main header
 *	  Copyright (C) 2000 Free Software Foundation, Inc.
 *
 * This file is part of GPAPA
 *
 * GPAPA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPAPA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef __GPAPA_H__
#define __GPAPA_H__

#include <glib.h>

#include <stdlib.h>
#include "xmalloc.h"

#include "gpapatypedefs.h"
#include "gpapafile.h"
#include "gpapakey.h"
#include "gpapapublickey.h"
#include "gpapasecretkey.h"
#include "gpapasignature.h"
#include "gpapaintern.h"

#define GPAPA_MAX_GPG_KEY_FIELDS 20

extern gchar *global_keyServer;

/* Key management.
 */

extern GDate *extract_date ( gchar *buffer );

extern void gpapa_refresh_public_keyring (
  GpapaCallbackFunc callback, gpointer calldata
);

extern gint gpapa_get_public_key_count (
  GpapaCallbackFunc callback, gpointer calldata
);

extern GpapaPublicKey *gpapa_get_public_key_by_index (
  gint idx, GpapaCallbackFunc callback, gpointer calldata
);

extern GpapaPublicKey *gpapa_get_public_key_by_ID (
  gchar *keyID, GpapaCallbackFunc callback, gpointer calldata
);

extern GpapaPublicKey *gpapa_receive_public_key_from_server (
  gchar *keyID, gchar *ServerName, GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_release_public_key (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_refresh_secret_keyring (
  GpapaCallbackFunc callback, gpointer calldata
);

extern gint gpapa_get_secret_key_count (
  GpapaCallbackFunc callback, gpointer calldata
);

extern GpapaSecretKey *gpapa_get_secret_key_by_index (
  gint idx, GpapaCallbackFunc callback, gpointer calldata
);

extern GpapaSecretKey *gpapa_get_secret_key_by_ID (
  gchar *keyID, GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_release_secret_key (
  GpapaSecretKey *key, GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_create_key_pair (
  GpapaAlgo anAlgo, gint aKeysize, long anExpiryDate,
  gchar *aUserID, gchar *anEmail, gchar *aComment,
  GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_export_ownertrust (
  gchar *targetFileID, GpapaArmor Armor,
  GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_import_ownertrust (
  gchar *sourceFileID,
  GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_update_trust_database (
  GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_import_keys (
  gchar *sourceFileID,
  GpapaCallbackFunc callback, gpointer calldata
);
;

/* Miscellaneous.
 */

extern void gpapa_init ( void );

extern void gpapa_fini ( void );

extern void gpapa_idle ( void );

#endif /* __GPAPA_H__ */
