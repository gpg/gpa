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

typedef int GpgStatusCode;

typedef enum
{
  GPAPA_ACTION_NONE,
  GPAPA_ACTION_ERROR,
  GPAPA_ACTION_PASSPHRASE,
  GPAPA_ACTION_ABORTED,
  GPAPA_ACTION_FINISHED
}
GpapaAction;

#define GPAPA_ACTION_FIRST GPAPA_ACTION_NONE
#define GPAPA_ACTION_LAST GPAPA_ACTION_FINISHED

typedef enum
{
  GPAPA_NO_ARMOR,
  GPAPA_ARMOR
}
GpapaArmor;

#define GPAPA_ARMOR_FIRST GPAPA_NO_ARMOR
#define GPAPA_ARMOR_LAST GPAPA_ARMOR

typedef enum
{
  GPAPA_SIGN_NORMAL,
  GPAPA_SIGN_CLEAR,
  GPAPA_SIGN_DETACH
}
GpapaSignType;

#define GPAPA_SIGN_FIRST GPAPA_SIGN_NORMAL
#define GPAPA_SIGN_LAST GPAPA_SIGN_DETACH

typedef enum
{
  GPAPA_KEY_SIGN_NORMAL,
  GPAPA_KEY_SIGN_LOCALLY,
}
GpapaKeySignType;

#define GPAPA_KEY_SIGN_FIRST GPAPA_KEY_SIGN_NORMAL
#define GPAPA_KEY_SIGN_LAST GPAPA_KEY_SIGN_LOCALLY

typedef enum
{
  GPAPA_SIG_UNKNOWN,
  GPAPA_SIG_INVALID,
  GPAPA_SIG_VALID
}
GpapaSigValidity;

#define GPAPA_SIG_FIRST GPAPA_SIG_UNKNOWN
#define GPAPA_SIG_LAST GPAPA_SIG_VALID

typedef enum
{
  GPAPA_ALGO_BOTH,
  GPAPA_ALGO_DSA,
  GPAPA_ALGO_ELG_BOTH,
  GPAPA_ALGO_ELG
}
GpapaAlgo;

#define GPAPA_ALGO_FIRST GPAPA_ALGO_BOTH
#define GPAPA_ALGO_LAST GPAPA_ALGO_ELG

typedef void (*GpapaCallbackFunc) (GpapaAction action, gpointer actiondata,
				   gpointer calldata);

typedef struct
{
  gchar *identifier;
  gchar *name;
  GList *sigs;
  unsigned status_flags;
}
GpapaFile;

typedef enum
{
  GPAPA_FILE_UNKNOWN,
  GPAPA_FILE_CLEAR,
  GPAPA_FILE_ENCRYPTED,
  GPAPA_FILE_PROTECTED,
  GPAPA_FILE_SIGNED,
  GPAPA_FILE_CLEARSIGNED,
  GPAPA_FILE_DETACHED_SIGNATURE
}
GpapaFileStatus;

#define GPAPA_FILE_FIRST GPAPA_FILE_UNKNOWN
#define GPAPA_FILE_LAST GPAPA_FILE_DETACHED_SIGNATURE

/* For internal use.
 */
#define GPAPA_FILE_STATUS_LITERAL       0x0001
#define GPAPA_FILE_STATUS_ENCRYPTED     0x0002
#define GPAPA_FILE_STATUS_PUBKEY        0x0004
#define GPAPA_FILE_STATUS_SYMKEY        0x0008
#define GPAPA_FILE_STATUS_COMPRESSED    0x0010
#define GPAPA_FILE_STATUS_SIGNATURE     0x0020
#define GPAPA_FILE_STATUS_NODATA        0x0040

typedef struct
{
  GpapaFile *file;
  GpapaCallbackFunc callback;
  gpointer calldata;
}
FileData;

extern GpapaFile *gpapa_file_new (gchar * fileID, GpapaCallbackFunc callback,
				  gpointer calldata);

extern gchar *gpapa_file_get_identifier (GpapaFile * file,
					 GpapaCallbackFunc callback,
					 gpointer calldata);

extern gchar *gpapa_file_get_name (GpapaFile * file,
				   GpapaCallbackFunc callback,
				   gpointer calldata);

extern GpapaFileStatus gpapa_file_get_status (GpapaFile *file,
					      GpapaCallbackFunc callback,
					      gpointer calldata);

extern gint gpapa_file_get_signature_count (GpapaFile *file,
					    GpapaCallbackFunc callback,
					    gpointer calldata);

extern GList *gpapa_file_get_signatures (GpapaFile *file,
					 GpapaCallbackFunc callback,
					 gpointer calldata);

extern void gpapa_file_sign (GpapaFile *file, const gchar *targetFileID,
			     const gchar *keyID, const gchar *PassPhrase,
			     GpapaSignType SignType, GpapaArmor Armor,
			     GpapaCallbackFunc callback, gpointer calldata);

extern void gpapa_file_encrypt (GpapaFile *file, const gchar *targetFileID,
				GList *rcptKeyID, GpapaArmor Armor,
				GpapaCallbackFunc callback,
				gpointer calldata);

extern void gpapa_file_encrypt_and_sign (GpapaFile *file,
					 const gchar *targetFileID,
					 GList *rcptKeyIDs, gchar *keyID,
					 const gchar *PassPhrase,
					 GpapaSignType SignType,
					 GpapaArmor Armor,
					 GpapaCallbackFunc callback,
					 gpointer calldata);

extern void gpapa_file_protect (GpapaFile *file, const gchar *targetFileID,
				const gchar *PassPhrase, GpapaArmor Armor,
				GpapaCallbackFunc callback,
				gpointer calldata);

extern void gpapa_file_decrypt (GpapaFile *file, gchar *targetFileID,
				const gchar *PassPhrase,
				GpapaCallbackFunc callback,
				gpointer calldata);

extern void gpapa_file_release (GpapaFile *file, GpapaCallbackFunc callback,
				gpointer calldata);

#define GPAPA_KEY(obj) ((obj)->key)

typedef struct
{
  char KeyTrust, OwnerTrust;  /* OwnerTrust might get a different type in future versions. */
  gint bits, algorithm;
  GList *uids, *subs;
  char *KeyID, *LocalID, *UserID;
  GDate *CreationDate, *ExpirationDate;
}
GpapaKey;

extern GpapaKey *gpapa_key_new (const gchar *keyID, GpapaCallbackFunc callback,
				gpointer calldata);

extern char *gpapa_key_get_identifier (GpapaKey *key,
                                       GpapaCallbackFunc callback,
                                       gpointer calldata);

extern char *gpapa_key_get_name (GpapaKey *key, GpapaCallbackFunc callback,
				 gpointer calldata);

extern GDate *gpapa_key_get_expiry_date (GpapaKey *key,
					 GpapaCallbackFunc callback,
					 gpointer calldata);

extern GDate *gpapa_key_get_creation_date (GpapaKey *key,
					   GpapaCallbackFunc callback,
					   gpointer calldata);

extern void gpapa_key_set_expiry_date (GpapaKey *key, GDate *date,
				       const gchar *password,
				       GpapaCallbackFunc callback,
				       gpointer calldata);

extern void gpapa_key_set_expiry_time (GpapaKey *key, gint number,
				       char unit, GpapaCallbackFunc callback,
				       gpointer calldata);

extern void gpapa_key_release (GpapaKey *key);

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

extern void linecallback_to_clipboard (gchar *line, gpointer data,
                                       GpgStatusCode status);

extern int set_w32_clip_text (const gchar *data, gint size);

extern GList *gpapa_public_key_get_signatures (GpapaPublicKey *key,
                                               GpapaCallbackFunc callback,
                                               gpointer calldata);

extern void gpapa_public_key_export (GpapaPublicKey *key,
                                     const gchar *targetFileID, GpapaArmor Armor,
                                     GpapaCallbackFunc callback,
                                     gpointer calldata);

extern void gpapa_public_key_export_to_clipboard (GpapaPublicKey *key,
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

typedef struct
{
  GpapaKey *key;
}
GpapaSecretKey;

typedef struct
{
  GpapaSecretKey *key;
  GpapaCallbackFunc callback;
  gpointer calldata;
}
SecretKeyData;

extern void gpapa_secret_key_set_passphrase (GpapaSecretKey *key,
					     const gchar *passphrase,
					     GpapaCallbackFunc callback,
					     gpointer calldata);

extern void gpapa_secret_key_export (GpapaSecretKey * key,
				     gchar * targetFileID, GpapaArmor Armor,
				     GpapaCallbackFunc callback,
				     gpointer calldata);

extern void gpapa_secret_key_export_to_clipboard (GpapaSecretKey *key,
                                                  GpapaCallbackFunc callback,
                                                  gpointer calldata);

extern void gpapa_secret_key_delete (GpapaSecretKey * key,
				     GpapaCallbackFunc callback,
				     gpointer calldata);

extern void gpapa_secret_key_create_revocation (GpapaSecretKey * key,
						GpapaCallbackFunc callback,
						gpointer calldata);

extern void gpapa_secret_key_release (GpapaSecretKey *key);

typedef struct
{
  gchar *identifier;
  gchar *KeyID, *UserID;
  GDate *CreationDate;
  GpapaSigValidity validity;
}
GpapaSignature;

extern GpapaSignature *gpapa_signature_new (GpapaCallbackFunc callback,
					    gpointer calldata);

extern gchar *gpapa_signature_get_identifier (GpapaSignature *signature,
					      GpapaCallbackFunc callback,
					      gpointer calldata);

extern gchar *gpapa_signature_get_name (GpapaSignature *signature,
					GpapaCallbackFunc callback,
					gpointer calldata);

extern GpapaSigValidity gpapa_signature_get_validity (GpapaSignature *signature,
						      GpapaCallbackFunc
						      callback,
						      gpointer calldata);

extern void gpapa_signature_release (GpapaSignature *signature);

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
