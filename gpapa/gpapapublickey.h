/* gpapapublickey.h  -  The GNU Privacy Assistant Pipe Access
 *        Copyright (C) 2000 Free Software Foundation, Inc.
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

#ifndef __GPAPAPUBLICKEY_H__
#define __GPAPAPUBLICKEY_H__

#include <glib.h>
#include "gpapatypedefs.h"
#include "gpapakey.h"

typedef struct {
  GpapaKey *key;
  GList *uids, *sigs;
  gchar *fingerprint;
} GpapaPublicKey;

typedef struct {
  GpapaPublicKey *key;
  GpapaCallbackFunc callback;
  gpointer calldata;
} PublicKeyData;

typedef enum {
  GPAPA_OWNERTRUST_UNKNOWN,
  GPAPA_OWNERTRUST_DISTRUST,
  GPAPA_OWNERTRUST_MARGINALLY,
  GPAPA_OWNERTRUST_FULLY
} GpapaOwnertrust;

#define GPAPA_OWNERTRUST_FIRST GPAPA_OWNERTRUST_UNKNOWN
#define GPAPA_OWNERTRUST_LAST GPAPA_OWNERTRUST_FULLY

typedef enum {
  GPAPA_KEYTRUST_UNKNOWN,
  GPAPA_KEYTRUST_DISTRUST,
  GPAPA_KEYTRUST_MARGINALLY,
  GPAPA_KEYTRUST_FULLY
} GpapaKeytrust;

#define GPAPA_KEYTRUST_FIRST GPAPA_KEYTRUST_UNKNOWN
#define GPAPA_KEYTRUST_LAST GPAPA_KEYTRUST_FULLY

extern gchar *gpapa_public_key_get_fingerprint (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
);

extern GpapaKeytrust gpapa_public_key_get_keytrust (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
);

extern GpapaOwnertrust gpapa_public_key_get_ownertrust (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
);

extern GList *gpapa_public_key_get_signatures (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_public_key_send_to_server (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
);

#endif /* __GPAPAPUBLICKEY_H__ */
