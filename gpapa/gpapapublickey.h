/* gpapapublickey.h - The GNU Privacy Assistant Pipe Access - public key object header
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

#ifndef __GPAPAPUBLICKEY_H__
#define __GPAPAPUBLICKEY_H__

#include <glib.h>
#include "gpapatypedefs.h"
#include "gpapakey.h"

typedef struct
{
  GpapaKey *key;
  GList *uids, *sigs;
  char *fingerprint;
}
GpapaPublicKey;

typedef struct
{
  GpapaPublicKey *key;
  GpapaCallbackFunc callback;
  gpointer calldata;
}
PublicKeyData;

typedef enum
{
  GPAPA_OWNERTRUST_UNKNOWN,
  GPAPA_OWNERTRUST_DISTRUST,
  GPAPA_OWNERTRUST_MARGINALLY,
  GPAPA_OWNERTRUST_FULLY
}
GpapaOwnertrust;

#define GPAPA_OWNERTRUST_FIRST GPAPA_OWNERTRUST_UNKNOWN
#define GPAPA_OWNERTRUST_LAST GPAPA_OWNERTRUST_FULLY

typedef enum
{
  GPAPA_KEYTRUST_UNKNOWN,
  GPAPA_KEYTRUST_DISTRUST,
  GPAPA_KEYTRUST_MARGINALLY,
  GPAPA_KEYTRUST_FULLY,
  GPAPA_KEYTRUST_ULTIMATE,
  GPAPA_KEYTRUST_REVOKED,
  GPAPA_KEYTRUST_EXPIRED,
  GPAPA_KEYTRUST_INVALID,
  GPAPA_KEYTRUST_DISABLED
}
GpapaKeytrust;

#define GPAPA_KEYTRUST_FIRST GPAPA_KEYTRUST_UNKNOWN
#define GPAPA_KEYTRUST_LAST GPAPA_KEYTRUST_DISABLED

extern char *gpapa_public_key_get_fingerprint (GpapaPublicKey *key,
                                               GpapaCallbackFunc callback,
                                               gpointer calldata);

extern GpapaKeytrust gpapa_public_key_get_keytrust (GpapaPublicKey *key,
                                                    GpapaCallbackFunc
                                                    callback,
                                                    gpointer calldata);

extern GpapaOwnertrust gpapa_public_key_get_ownertrust (GpapaPublicKey *key,
                                                        GpapaCallbackFunc
                                                        callback,
                                                        gpointer calldata);

extern void gpapa_public_key_set_ownertrust (GpapaPublicKey *key,
                                             GpapaOwnertrust trust,
                                             GpapaCallbackFunc callback,
                                             gpointer calldata);

extern GList *gpapa_public_key_get_signatures (GpapaPublicKey *key,
                                               GpapaCallbackFunc callback,
                                               gpointer calldata);

extern void gpapa_public_key_export (GpapaPublicKey *key,
                                     char *targetFileID, GpapaArmor Armor,
                                     GpapaCallbackFunc callback,
                                     gpointer calldata);

extern void gpapa_public_key_delete (GpapaPublicKey *key,
                                     GpapaCallbackFunc callback,
                                     gpointer calldata);

extern void gpapa_public_key_send_to_server (GpapaPublicKey *key,
                                             const char *ServerName,
                                             GpapaCallbackFunc callback,
                                             gpointer calldata);

extern void gpapa_public_key_sign (GpapaPublicKey *key, char *keyID,
                                   char *PassPhrase,
                                   GpapaKeySignType SignType,
                                   GpapaCallbackFunc callback,
                                   gpointer calldata);

extern void gpapa_public_key_release_sigs (GpapaPublicKey *key);

extern void gpapa_public_key_release (GpapaPublicKey *key);

#endif /* __GPAPAPUBLICKEY_H__ */
