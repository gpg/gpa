/* dummy_gpapa.c  -	 The GNU Privacy Assistant
 *	Copyright (C) 2002 Miguel Coca
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* This is a collection of dummy functions to assist during the migration
 * from gpapa to gpgme. Every public gpapa function is available, but does
 * nothing. */

#include <gpapa.h>

extern GDate *
gpapa_extract_date (char *buffer)
{
  return NULL;
}

extern char *
gpapa_extract_fingerprint (char *line, int algorithm,
			   GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern void
gpapa_refresh_public_keyring (GpapaCallbackFunc callback, gpointer calldata)
{
}

extern gint
gpapa_get_public_key_count (GpapaCallbackFunc callback, gpointer calldata)
{
  return 0;
}

extern GpapaPublicKey *
gpapa_get_public_key_by_index (gint idx,
			       GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern GpapaPublicKey *
gpapa_get_public_key_by_ID (const gchar * keyID,
			    GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern GpapaPublicKey *
gpapa_get_public_key_by_userID (const gchar * userID,
				GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern void
gpapa_report_hkp_error (int rc, GpapaCallbackFunc callback, gpointer calldata)
{
}

extern GpapaPublicKey *
gpapa_receive_public_key_from_server (const gchar * keyID,
				      const gchar * ServerName,
				      GpapaCallbackFunc callback,
				      gpointer calldata)
{
  return NULL;
}

extern GList *
gpapa_search_public_keys_on_server (const gchar * keyID,
				    const gchar * ServerName,
				    GpapaCallbackFunc callback,
				    gpointer calldata)
{
  return NULL;
}

extern void
gpapa_release_public_key (GpapaPublicKey * key,
			  GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_refresh_secret_keyring (GpapaCallbackFunc callback, gpointer calldata)
{
  return;
}

extern gint
gpapa_get_secret_key_count (GpapaCallbackFunc callback, gpointer calldata)
{
  return 0;
}

extern GpapaSecretKey *
gpapa_get_secret_key_by_index (gint idx,
			       GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern GpapaSecretKey *
gpapa_get_secret_key_by_ID (const gchar * keyID,
			    GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern GpapaSecretKey *
gpapa_get_secret_key_by_userID (const gchar * userID,
				GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern void
gpapa_release_secret_key (GpapaSecretKey * key,
			  GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_create_key_pair (GpapaPublicKey ** publicKey,
		       GpapaSecretKey ** secretKey,
		       char *passphrase, GpapaAlgo anAlgo,
		       gint aKeysize, char *aUserID,
		       char *anEmail, char *aComment,
		       GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_export_ownertrust (const gchar * targetFileID, GpapaArmor Armor,
			 GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_import_ownertrust (const gchar * sourceFileID,
			 GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_update_trust_database (GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_import_keys (const gchar * sourceFileID,
		   GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_import_keys_from_clipboard (GpapaCallbackFunc callback,
				  gpointer calldata)
{
}

/* Miscellaneous.
 */

extern void
gpapa_init (const char *gpg)
{
}

extern void
gpapa_fini (void)
{
}

extern void
gpapa_idle (void)
{
}


extern GpapaFile *
gpapa_file_new (gchar * fileID, GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern gchar *
gpapa_file_get_identifier (GpapaFile * file,
			   GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern gchar *
gpapa_file_get_name (GpapaFile * file,
		     GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern GpapaFileStatus
gpapa_file_get_status (GpapaFile * file,
		       GpapaCallbackFunc callback, gpointer calldata)
{
  return 0;
}

extern gint
gpapa_file_get_signature_count (GpapaFile * file,
				GpapaCallbackFunc callback, gpointer calldata)
{
  return 0;
}

extern GList *
gpapa_file_get_signatures (GpapaFile * file,
			   GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern void
gpapa_file_sign (GpapaFile * file, const gchar * targetFileID,
		 const gchar * keyID, const gchar * PassPhrase,
		 GpapaSignType SignType, GpapaArmor Armor,
		 GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_file_encrypt (GpapaFile * file, const gchar * targetFileID,
		    GList * rcptKeyID, GpapaArmor Armor,
		    GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_file_encrypt_and_sign (GpapaFile * file,
			     const gchar * targetFileID,
			     GList * rcptKeyIDs, gchar * keyID,
			     const gchar * PassPhrase,
			     GpapaSignType SignType,
			     GpapaArmor Armor,
			     GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_file_protect (GpapaFile * file, const gchar * targetFileID,
		    const gchar * PassPhrase, GpapaArmor Armor,
		    GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_file_decrypt (GpapaFile * file, gchar * targetFileID,
		    const gchar * PassPhrase,
		    GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_file_release (GpapaFile * file, GpapaCallbackFunc callback,
		    gpointer calldata)
{
}



extern GpapaKey *
gpapa_key_new (const gchar * keyID, GpapaCallbackFunc callback,
	       gpointer calldata)
{
  return NULL;
}

extern char *
gpapa_key_get_identifier (GpapaKey * key,
			  GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern char *
gpapa_key_get_name (GpapaKey * key, GpapaCallbackFunc callback,
		    gpointer calldata)
{
  return NULL;
}

extern GDate *
gpapa_key_get_expiry_date (GpapaKey * key,
			   GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern GDate *
gpapa_key_get_creation_date (GpapaKey * key,
			     GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern void
gpapa_key_set_expiry_date (GpapaKey * key, GDate * date,
			   const gchar * password,
			   GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_key_set_expiry_time (GpapaKey * key, gint number,
			   char unit, GpapaCallbackFunc callback,
			   gpointer calldata)
{
}

extern void
gpapa_key_release (GpapaKey * key)
{
}


extern char *
gpapa_public_key_get_fingerprint (GpapaPublicKey * key,
				  GpapaCallbackFunc callback,
				  gpointer calldata)
{
  return NULL;
}

extern GpapaKeytrust
gpapa_public_key_get_keytrust (GpapaPublicKey * key,
			       GpapaCallbackFunc callback, gpointer calldata)
{
  return 0;
}

extern GpapaOwnertrust
gpapa_public_key_get_ownertrust (GpapaPublicKey * key,
				 GpapaCallbackFunc
				 callback, gpointer calldata)
{
  return 0;
}

extern void
gpapa_public_key_set_ownertrust (GpapaPublicKey * key,
				 GpapaOwnertrust trust,
				 GpapaCallbackFunc callback,
				 gpointer calldata)
{
}

extern void
linecallback_to_clipboard (gchar * line, gpointer data, GpgStatusCode status)
{
}

extern int
set_w32_clip_text (const gchar * data, gint size)
{
  return 0;
}

extern GList *
gpapa_public_key_get_signatures (GpapaPublicKey * key,
				 GpapaCallbackFunc callback,
				 gpointer calldata)
{
  return NULL;
}

extern void
gpapa_public_key_export (GpapaPublicKey * key,
			 const gchar * targetFileID, GpapaArmor Armor,
			 GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_public_key_export_to_clipboard (GpapaPublicKey * key,
				      GpapaCallbackFunc callback,
				      gpointer calldata)
{
}

extern void
gpapa_public_key_delete (GpapaPublicKey * key,
			 GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_public_key_send_to_server (GpapaPublicKey * key,
				 const char *ServerName,
				 GpapaCallbackFunc callback,
				 gpointer calldata)
{
}

extern void
gpapa_public_key_sign (GpapaPublicKey * key, char *keyID,
		       char *PassPhrase,
		       GpapaKeySignType SignType,
		       GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_public_key_release_sigs (GpapaPublicKey * key)
{
}

extern void
gpapa_public_key_release (GpapaPublicKey * key)
{
}


extern void
gpapa_secret_key_set_passphrase (GpapaSecretKey * key,
				 const gchar * passphrase,
				 GpapaCallbackFunc callback,
				 gpointer calldata)
{
}

extern void
gpapa_secret_key_export (GpapaSecretKey * key,
			 gchar * targetFileID, GpapaArmor Armor,
			 GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_secret_key_export_to_clipboard (GpapaSecretKey * key,
				      GpapaCallbackFunc callback,
				      gpointer calldata)
{
}

extern void
gpapa_secret_key_delete (GpapaSecretKey * key,
			 GpapaCallbackFunc callback, gpointer calldata)
{
}

extern void
gpapa_secret_key_create_revocation (GpapaSecretKey * key,
				    GpapaCallbackFunc callback,
				    gpointer calldata)
{
}

extern void
gpapa_secret_key_release (GpapaSecretKey * key)
{
}


extern GpapaSignature *
gpapa_signature_new (GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern gchar *
gpapa_signature_get_identifier (GpapaSignature * signature,
				GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern gchar *
gpapa_signature_get_name (GpapaSignature * signature,
			  GpapaCallbackFunc callback, gpointer calldata)
{
  return NULL;
}

extern GpapaSigValidity
gpapa_signature_get_validity (GpapaSignature * signature,
			      GpapaCallbackFunc callback, gpointer calldata)
{
  return 0;
}

extern void
gpapa_signature_release (GpapaSignature * signature)
{
}
