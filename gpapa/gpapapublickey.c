/* gpapapublickey.c  -	The GNU Privacy Assistant Pipe Access
 *	  Copyright (C) 2000 G-N-U GmbH.
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "gpapa.h"

static void
linecallback_fingerprint (gchar * line, gpointer data, GpgStatusCode status)
{
  if (status == NO_STATUS && line && strncmp (line, "fpr", 3) == 0)
    {
      PublicKeyData *d = data;
      gchar *field[GPAPA_MAX_GPG_KEY_FIELDS];
      gchar *p = line;
      gint i = 0;
      gint fields;

      while (*p)
	{
	  field[i] = p;
	  while (*p && *p != ':')
	    p++;
	  if (*p == ':')
	    {
	      *p = 0;
	      p++;
	    }
	  i++;
	  if (i >= GPAPA_MAX_GPG_KEY_FIELDS)
	    d->callback (GPAPA_ACTION_ERROR,
			 "too many fields in output of `gpg --fingerprint'",
			 d->calldata);
	}
      fields = i;
      if (fields != 10)
	d->callback (GPAPA_ACTION_ERROR,
		     "invalid number of fields in output of `gpg --fingerprint'",
		     d->calldata);
      else if (d->key->key->algorithm == 1)	/* RSA */
	{
	  gchar *fpraw = field[9];
	  gchar *fp = xmalloc (strlen (fpraw) + 16 + 1);
	  gchar *r = fpraw, *q = fp;
	  gint c = 0;
	  while (*r)
	    {
	      *q++ = *r++;
	      c++;
	      if (c < 32)
		{
		  if (c % 2 == 0)
		    *q++ = ' ';
		  if (c % 16 == 0)
		    *q++ = ' ';
		}
	    }
	  *q = 0;
	  d->key->fingerprint = fp;
	}
      else
	{
	  gchar *fpraw = field[9];
	  gchar *fp = xmalloc (strlen (fpraw) + 10 + 1);
	  gchar *r = fpraw, *q = fp;
	  gint c = 0;
	  while (*r)
	    {
	      *q++ = *r++;
	      c++;
	      if (c < 40)
		{
		  if (c % 4 == 0)
		    *q++ = ' ';
		  if (c % 20 == 0)
		    *q++ = ' ';
		}
	    }
	  *q = 0;
	  d->key->fingerprint = fp;
	}
    }
} /* linecallback_fingerprint */

gchar *
gpapa_public_key_get_fingerprint (GpapaPublicKey * key,
				  GpapaCallbackFunc callback,
				  gpointer calldata)
{
  if (key == NULL)
    return (NULL);
  else
    {
      if (key->fingerprint == NULL && key->key != NULL)
	{
	  PublicKeyData data = { key, callback, calldata };
	  char *key_id = xstrcat2 ("0x", key->key->KeyID);
	  char *gpgargv[4];
	  gpgargv[0] = "--fingerprint";
	  gpgargv[1] = "--with-colons";
	  gpgargv[2] = key_id;
	  gpgargv[3] = NULL;
	  gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
			    linecallback_fingerprint, &data,
			    callback, calldata);
	  free (key_id);
	}
      return (key->fingerprint);
    }
} /* gpapa_public_key_get_fingerprint */

GpapaKeytrust
gpapa_public_key_get_keytrust (GpapaPublicKey * key,
			       GpapaCallbackFunc callback, gpointer calldata)
{
  if (key == NULL || key->key == NULL)
    return (GPAPA_KEYTRUST_UNKNOWN);
  else
    switch (key->key->KeyTrust)
      {
      case 'n':
	return (GPAPA_KEYTRUST_DISTRUST);
      case 'm':
	return (GPAPA_KEYTRUST_MARGINALLY);
      case 'f':
	return (GPAPA_KEYTRUST_FULLY);
      case 'q':
	return (GPAPA_KEYTRUST_UNKNOWN);
      default:
	return (GPAPA_KEYTRUST_UNKNOWN);
      }
} /* gpapa_public_key_get_keytrust */

GpapaOwnertrust
gpapa_public_key_get_ownertrust (GpapaPublicKey * key,
				 GpapaCallbackFunc callback,
				 gpointer calldata)
{
  if (key == NULL || key->key == NULL)
    return (GPAPA_OWNERTRUST_UNKNOWN);
  else
    switch (key->key->OwnerTrust)
      {
      case 'n':
	return (GPAPA_OWNERTRUST_DISTRUST);
      case 'm':
	return (GPAPA_OWNERTRUST_MARGINALLY);
      case 'f':
	return (GPAPA_OWNERTRUST_FULLY);
      case 'q':
	return (GPAPA_KEYTRUST_UNKNOWN);
      default:
	return (GPAPA_OWNERTRUST_UNKNOWN);
      }
} /* gpapa_public_key_get_ownertrust */

extern void
gpapa_public_key_set_ownertrust (GpapaPublicKey * key, GpapaOwnertrust trust,
				 GpapaCallbackFunc callback,
				 gpointer calldata)
{
  if(key)
    {
      gchar *gpgargv[3];
      gchar *commands;
      gchar *commands_sprintf_str;
      int trust_int;
      switch (trust)
	{
	  case GPAPA_OWNERTRUST_DISTRUST:
	    trust_int = 2;
	    break;
	  case GPAPA_OWNERTRUST_MARGINALLY:
	    trust_int = 3;
	    break;
	  case GPAPA_OWNERTRUST_FULLY:
	    trust_int = 4;
	    break;
	  default:
	    trust_int = 1;
	    break;
	}
      commands_sprintf_str = "trust \n%u \nquit \n";
      commands = (char *) (xmalloc (strlen (commands_sprintf_str) + 1));
      sprintf (commands, commands_sprintf_str, trust_int);
      gpgargv[0] = "--edit-key";
      gpgargv[1] = key->key->KeyID;
      gpgargv[2] = NULL; 
      gpapa_call_gnupg (gpgargv, TRUE, commands, NULL,
                        NULL, NULL, callback, calldata); 
      free(commands);
    }
} /* gpapa_public_key_set_ownertrust */

static GpapaSignature *
extract_sig (gchar * line, GpapaCallbackFunc callback, gpointer calldata)
{
  gchar *field[GPAPA_MAX_GPG_KEY_FIELDS];
  gchar *p = line;
  gint i = 0;
  gint fields;

  while (*p)
    {
      field[i] = p;
      while (*p && *p != ':')
	p++;
      if (*p == ':')
	{
	  *p = 0;
	  p++;
	}
      i++;
      if (i >= GPAPA_MAX_GPG_KEY_FIELDS)
	callback (GPAPA_ACTION_ERROR,
		  "too many fields in output of `gpg --check-sigs'",
		  calldata);
    }
  fields = i;
  if (fields != 11)
    {
      callback (GPAPA_ACTION_ERROR,
		"invalid number of fields in output of `gpg --check-sigs'",
		calldata);
      return (NULL);
    }
  else
    {
      GpapaSignature *sig =
	gpapa_signature_new (field[4], callback, calldata);
      if (strcmp (field[1], "!") == 0)
	sig->validity = GPAPA_SIG_VALID;
      else if (strcmp (field[1], "-") == 0)
	sig->validity = GPAPA_SIG_INVALID;
      else
	sig->validity = GPAPA_SIG_UNKNOWN;
      sig->CreationDate = extract_date (field[5]);
      sig->UserID = field[9][0] ? xstrdup (field[9]) : NULL;
      return (sig);
    }
} /* extract_sig */

static void
linecallback_get_signatures (gchar * line, gpointer data, GpgStatusCode status)
{
  if (status == NO_STATUS && line && strncmp (line, "sig", 3) == 0)
    {
      PublicKeyData *d = data;
      GpapaSignature *sig = extract_sig (line, d->callback, d->calldata);
      if (strcmp (d->key->key->KeyID, sig->KeyID) == 0)
	{
	  /* Self-signature.  Only report it if it is not valid.
	   */
	  if (sig->validity == GPAPA_SIG_VALID)
	    {
	      gpapa_signature_release (sig, d->callback, d->calldata);
	      sig = NULL;
	    }
	}
      if (sig)
	d->key->sigs = g_list_append (d->key->sigs, sig);
    }
} /* linecallback_get_signatures */

GList *
gpapa_public_key_get_signatures (GpapaPublicKey * key,
				 GpapaCallbackFunc callback,
				 gpointer calldata)
{
  if (key == NULL)
    return (NULL);
  else
    {
      if (key->sigs == NULL && key->key != NULL)
	{
	  PublicKeyData data = { key, callback, calldata };
	  char *key_id = xstrcat2 ("0x", key->key->KeyID);
	  char *gpgargv[4];
	  gpgargv[0] = "--check-sigs";
	  gpgargv[1] = "--with-colons";
	  gpgargv[2] = key_id;
	  gpgargv[3] = NULL;
	  gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
			    linecallback_get_signatures, &data,
			    callback, calldata);
	  free (key_id);
	}
      return (key->sigs);
    }
} /* gpapa_public_key_get_signatures */

void
gpapa_public_key_export (GpapaPublicKey * key, gchar * targetFileID,
			 GpapaArmor Armor, GpapaCallbackFunc callback,
			 gpointer calldata)
{
  if (!key)
    callback (GPAPA_ACTION_ERROR, "no valid public key specified", calldata);
  if (!targetFileID)
    callback (GPAPA_ACTION_ERROR, "target file not specified", calldata);
  if (key && targetFileID)
    {
      gchar *full_keyID;
      char *gpgargv[7];
      int i = 0;
      full_keyID = xstrcat2 ("0x", key->key->KeyID);
      gpgargv[i++] = "-o";
      gpgargv[i++] = targetFileID;
      gpgargv[i++] = "--yes";	/* overwrite the file */
      if (Armor == GPAPA_ARMOR)
	gpgargv[i++] = "--armor";
      gpgargv[i++] = "--export";
      gpgargv[i++] = full_keyID;
      gpgargv[i] = NULL;
      gpapa_call_gnupg
	(gpgargv, TRUE, NULL, NULL,
	 NULL, NULL, callback, calldata);
      free (full_keyID);
    }
} /* gpapa_public_key_export */

void
gpapa_public_key_delete (GpapaPublicKey * key, GpapaCallbackFunc callback,
			 gpointer calldata)
{
  if (!key)
    callback (GPAPA_ACTION_ERROR, "no valid public key specified", calldata);
  else
    {
      gchar *full_keyID;
      char *gpgargv[4];
      full_keyID = xstrcat2 ("0x", key->key->KeyID);
      gpgargv[0] = "--yes";
      gpgargv[1] = "--delete-key";
      gpgargv[2] = full_keyID;
      gpgargv[3] = NULL;
      gpapa_call_gnupg
	(gpgargv, TRUE, "\n\n", NULL,
	 NULL, NULL, callback, calldata);
      free (full_keyID);
      gpapa_refresh_public_keyring (callback, calldata);
    }
} /* gpapa_public_key_delete */ ;

void
gpapa_public_key_send_to_server (GpapaPublicKey * key, gchar * ServerName,
				 GpapaCallbackFunc callback,
				 gpointer calldata)
{
  if (!key)
    callback (GPAPA_ACTION_ERROR, "no valid public key specified", calldata);
  if (!ServerName)
    callback (GPAPA_ACTION_ERROR, "keyserver not specified", calldata);
  if (key && ServerName)
    {
      gchar *full_keyID;
      char *gpgargv[5];
      full_keyID = xstrcat2 ("0x", key->key->KeyID);
      gpgargv[0] = "--keyserver";
      gpgargv[1] = ServerName;
      gpgargv[2] = "--send-keys";
      gpgargv[3] = full_keyID;
      gpgargv[4] = NULL;
      gpapa_call_gnupg
	(gpgargv, TRUE, NULL, NULL,
	 NULL, NULL, callback, calldata);
      free (full_keyID);
    }
} /* gpapa_public_key_send_to_server */

void
gpapa_public_key_sign (GpapaPublicKey * key, gchar * keyID,
		       gchar * PassPhrase, GpapaKeySignType SignType,
		       GpapaCallbackFunc callback, gpointer calldata)
{
  if (!key)
    callback (GPAPA_ACTION_ERROR, "no valid public key specified", calldata);
  if (!keyID)
    callback (GPAPA_ACTION_ERROR, "no valid secret key specified", calldata);
  if (!PassPhrase)
    callback (GPAPA_ACTION_ERROR, "no valid PassPhrase specified", calldata);
  if (key && keyID && PassPhrase)
    {
      gchar *full_keyID;
      char commands[] = "\n";
      char *gpgargv[6];
      full_keyID = xstrcat2 ("", key->key->UserID);
      gpgargv[0] = "--yes";
      gpgargv[1] = "--local-user";
      gpgargv[2] = keyID;
      if (SignType == GPAPA_KEY_SIGN_NORMAL)
	gpgargv[3] = "--sign-key";
      else
	gpgargv[3] = "--lsign-key";
      gpgargv[4] = full_keyID;
      gpgargv[5] = NULL;
      gpapa_call_gnupg
	(gpgargv, TRUE, commands, PassPhrase, NULL, NULL,
	 callback, calldata);
      free (full_keyID);
    }    
} /* gpapa_public_key_sign */
