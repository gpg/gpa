/* gpapapublickey.c - The GNU Privacy Assistant Pipe Access - public key object
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "gpapa.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __USE_HKP__
#include <keyserver.h>
#endif

static void
linecallback_fingerprint (gchar *line, gpointer data, GpgStatusCode status)
{
  PublicKeyData *d = data;
  gpapa_report_error_status (status, d->callback, d->calldata);
  if (status == NO_STATUS && line && strncmp (line, "fpr:", 4) == 0)
    d->key->fingerprint = gpapa_extract_fingerprint (line, d->key->key->algorithm,
                                                     d->callback, d->calldata);
}

char *
gpapa_public_key_get_fingerprint (GpapaPublicKey *key,
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
          gchar *key_id = xstrcat2 ("0x", key->key->KeyID);
          const gchar *gpgargv[4];
          gpgargv[0] = "--fingerprint";
          gpgargv[1] = "--with-colons";
          gpgargv[2] = key_id;
          gpgargv[3] = NULL;
          gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, NULL,
                            linecallback_fingerprint, &data,
                            callback, calldata);
          free (key_id);
        }
      return (key->fingerprint);
    }
}

GpapaKeytrust
gpapa_public_key_get_keytrust (GpapaPublicKey *key,
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
        case 'u':
          return (GPAPA_KEYTRUST_ULTIMATE);
        case 'r':
          return (GPAPA_KEYTRUST_REVOKED);
        case 'e':
          return (GPAPA_KEYTRUST_EXPIRED);
        case 'i':
          return (GPAPA_KEYTRUST_INVALID);
        case 'd':
          return (GPAPA_KEYTRUST_DISABLED);
        case 'q':
          return (GPAPA_KEYTRUST_UNKNOWN);
        default:
          return (GPAPA_KEYTRUST_UNKNOWN);
      }
}

GpapaOwnertrust
gpapa_public_key_get_ownertrust (GpapaPublicKey *key,
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
}

extern void
gpapa_public_key_set_ownertrust (GpapaPublicKey *key, GpapaOwnertrust trust,
                                 GpapaCallbackFunc callback,
                                 gpointer calldata)
{
  if (key)
    {
      const gchar *gpgargv[3];
      gchar *commands;
      gchar *commands_sprintf_str;
      int trust_int;
      gchar trust_char;
      switch (trust)
        {
          case GPAPA_OWNERTRUST_DISTRUST:
            trust_int = 2;
            trust_char = 'n';
            break;
          case GPAPA_OWNERTRUST_MARGINALLY:
            trust_int = 3;
            trust_char = 'm';
            break;
          case GPAPA_OWNERTRUST_FULLY:
            trust_int = 4;
            trust_char = 'f';
            break;
          default:
            trust_int = 1;
            trust_char = 'q';
            break;
        }
      commands_sprintf_str = "trust\n%u\nquit\n";
      commands = g_strdup_printf (commands_sprintf_str, trust_int);
      gpgargv[0] = "--edit-key";
      gpgargv[1] = key->key->KeyID;
      gpgargv[2] = NULL; 
      gpapa_call_gnupg (gpgargv, TRUE, commands, NULL, NULL,
                        NULL, NULL, callback, calldata); 
      g_free (commands);
      key->key->OwnerTrust = trust_char;
    }
}

static GpapaSignature *
extract_sig (char *line, GpapaCallbackFunc callback, gpointer calldata)
{
  char *field[GPAPA_MAX_GPG_KEY_FIELDS];
  char *p = line;
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
          break;
    }
  fields = i;
  if (fields < 11)
    {
      callback (GPAPA_ACTION_ERROR,
                "invalid number of fields in output of `gpg --check-sigs'",
                calldata);
      return (NULL);
    }
  else
    {
      GpapaSignature *sig = gpapa_signature_new (callback, calldata);
      if (strcmp (field[1], "!") == 0)
        sig->validity = GPAPA_SIG_VALID;
      else if (strcmp (field[1], "-") == 0)
        sig->validity = GPAPA_SIG_INVALID;
      else
        sig->validity = GPAPA_SIG_UNKNOWN;
      if (field[4] == NULL || strlen(field[4]) <= 8)
        sig->KeyID = NULL;
      else
        sig->KeyID = xstrdup (field[4] + 8);
      sig->CreationDate = gpapa_extract_date (field[5]);
      sig->UserID = field[9][0] ? xstrdup (field[9]) : NULL;
      return (sig);
    }
}

static void
linecallback_get_signatures (char *line, gpointer data, GpgStatusCode status)
{
  PublicKeyData *d = data;
  gpapa_report_error_status (status, d->callback, d->calldata);
  if (status == NO_STATUS && line && strncmp (line, "sig", 3) == 0)
    {
      GpapaSignature *sig = extract_sig (line, d->callback, d->calldata);
      if (sig)
        d->key->sigs = g_list_append (d->key->sigs, sig);
    }
} 

GList *
gpapa_public_key_get_signatures (GpapaPublicKey *key,
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
          gchar *key_id = xstrcat2 ("0x", key->key->KeyID);
          const gchar *gpgargv[4];
          gpgargv[0] = "--check-sigs";
          gpgargv[1] = "--with-colons";
          gpgargv[2] = key_id;
          gpgargv[3] = NULL;
          gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, NULL,
                            linecallback_get_signatures, &data,
                            callback, calldata);
          free (key_id);
        }
      return (key->sigs);
    }
}

void
gpapa_public_key_export (GpapaPublicKey *key, const gchar *targetFileID,
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
      gchar *quoted_filename = NULL;
      const gchar *gpgargv[7];
      int i = 0;
      full_keyID = xstrcat2 ("0x", key->key->KeyID);
      quoted_filename = g_strconcat ("\"", targetFileID, "\"", NULL);
      gpgargv[i++] = "-o";
      gpgargv[i++] = quoted_filename;
      gpgargv[i++] = "--yes";  /* overwrite the file */
      if (Armor == GPAPA_ARMOR)
        gpgargv[i++] = "--armor";
      gpgargv[i++] = "--export";
      gpgargv[i++] = full_keyID;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, NULL,
                        NULL, NULL, callback, calldata);
      free (quoted_filename);
      free (full_keyID);
    }
}

void
linecallback_to_clipboard (gchar *line, gpointer data, GpgStatusCode status)
{
#ifdef HAVE_DOSISH_SYSTEM
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif
  if (status == NO_STATUS)  /* `line' may be NULL */
    {
      gchar **pd = data;
      gchar *d0 = *pd;
      if (d0)
        {
          *pd = g_strconcat (d0, NEWLINE, line, NULL);
          g_free (d0);
        }
      else
        *pd = xstrdup (line);
    }
}

#ifdef _WIN32

int
set_w32_clip_text (const gchar *data, gint size)
{
  int rc;
  HANDLE cb;
  char *private_data;

  rc = OpenClipboard( NULL );
  if (!rc)
    return 1;
  EmptyClipboard();
  cb = GlobalAlloc(GHND, size+1);
  if (!cb)
    goto leave;

  private_data = GlobalLock(cb);
  if (!private_data)
    goto leave;
  memcpy(private_data, data, size);
  private_data[size] = '\0';
  SetClipboardData(CF_TEXT, cb);
  GlobalUnlock(cb);

leave:
  CloseClipboard();
  return 0;
} /* set_w32_clip_text */

#endif /* _WIN32 */

void
gpapa_public_key_export_to_clipboard (GpapaPublicKey *key,
                                      GpapaCallbackFunc callback,
                                      gpointer calldata)
{
  if (!key)
    callback (GPAPA_ACTION_ERROR, "no valid public key specified", calldata);
  if (key)
    {
      gchar *full_keyID;
      gchar *clipboard_data = NULL;
      const gchar *gpgargv[4];
      int i = 0;
      full_keyID = xstrcat2 ("0x", key->key->KeyID);
      gpgargv[i++] = "--armor";
      gpgargv[i++] = "--export";
      gpgargv[i++] = full_keyID;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, NULL,
                        linecallback_to_clipboard, &clipboard_data,
                        callback, calldata);
#ifdef _WIN32
      set_w32_clip_text (clipboard_data, strlen (clipboard_data));
#else
      fprintf (stderr, "*** Clipboard ***\n%s\n*** End Clipboard ***\n",
                       clipboard_data);
#endif
      free (full_keyID);
    }
}

void
gpapa_public_key_delete (GpapaPublicKey *key, GpapaCallbackFunc callback,
                         gpointer calldata)
{
  if (!key)
    callback (GPAPA_ACTION_ERROR, "no valid public key specified", calldata);
  else
    {
      gchar *full_keyID;
      gchar *commands = "YES\n";
      const gchar *gpgargv[4];
      full_keyID = xstrcat2 ("0x", key->key->KeyID);
      gpgargv[0] = "--yes";
      gpgargv[1] = "--delete-key";
      gpgargv[2] = full_keyID;
      gpgargv[3] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, commands, NULL, NULL,
                        NULL, NULL, callback, calldata);
      free (full_keyID);
      gpapa_refresh_public_keyring (callback, calldata);
    }
}

void
gpapa_public_key_send_to_server (GpapaPublicKey *key,
                                 const char *ServerName,
                                 GpapaCallbackFunc callback,
                                 gpointer calldata)
{
  if (!key)
    callback (GPAPA_ACTION_ERROR, "no valid public key specified", calldata);
  if (!ServerName)
    callback (GPAPA_ACTION_ERROR, "keyserver not specified", calldata);
  if (key && ServerName)
    {
#ifdef __USE_HKP__
      gchar *full_keyID;
      gchar *buffer_data = NULL;
      const gchar *gpgargv[4];
      int rc;
      int i = 0;
      full_keyID = xstrcat2 ("0x", key->key->KeyID);
      gpgargv[i++] = "--armor";
      gpgargv[i++] = "--export";
      gpgargv[i++] = full_keyID;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, NULL,
                        linecallback_to_clipboard, &buffer_data,
                        callback, calldata);
      wsock_init ();
      rc = kserver_sendkey (ServerName, buffer_data, strlen (buffer_data));
      wsock_end ();
      if (rc != 0)
        {
	  if (rc < 1 || rc > 8)
	    rc = 1;
	  if (rc == HKPERR_RECVKEY || rc == HKPERR_SENDKEY)
            callback (GPAPA_ACTION_ERROR, (char *) kserver_strerror (), calldata);
          callback (GPAPA_ACTION_ERROR, hkp_errtypestr[rc - 1], calldata);
	}

      free (full_keyID);
#else /* not __USE_HKP__ */
      gchar *name = xstrdup (ServerName);
      gchar *full_keyID;
      const gchar *gpgargv[5];

      full_keyID = xstrcat2 ("0x", key->key->KeyID);
      gpgargv[0] = "--keyserver";
      gpgargv[1] = name;
      gpgargv[2] = "--send-keys";
      gpgargv[3] = full_keyID;
      gpgargv[4] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, NULL,
                        NULL, NULL, callback, calldata);
      free (full_keyID);
      free (name);
#endif /* not __USE_HKP__ */
    }
}

void
gpapa_public_key_sign (GpapaPublicKey *key, char *keyID,
                       char *PassPhrase, GpapaKeySignType SignType,
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
      gchar *commands = "YES\nYES\n";
      const gchar *gpgargv[6];
      full_keyID = g_strconcat ("\"", key->key->UserID, "\"");
      gpgargv[0] = "--yes";
      gpgargv[1] = "--local-user";
      gpgargv[2] = keyID;
      if (SignType == GPAPA_KEY_SIGN_NORMAL)
        gpgargv[3] = "--sign-key";
      else
        gpgargv[3] = "--lsign-key";
      gpgargv[4] = full_keyID;
      gpgargv[5] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, commands, NULL, PassPhrase,
                        NULL, NULL, callback, calldata);
      free (full_keyID);

      /* Enforce reloading of this key's signatures.
       */
      g_list_free (key->sigs);
      key->sigs = NULL;
    }    
}

void
gpapa_public_key_release_sigs (GpapaPublicKey *key)
{
  if (key != NULL && key->sigs != NULL)
    {
      GList *g = key->sigs;
      while (g)
        {
          if (g->data)
            gpapa_signature_release (g->data);
          g = g_list_next (g);
        }
      g_list_free (key->sigs);
      key->sigs = NULL;
    }
}

void
gpapa_public_key_release (GpapaPublicKey *key)
{
  if (key != NULL)
    {
      gpapa_key_release (key->key);
      g_list_free (key->uids);
      gpapa_public_key_release_sigs (key);
      free (key->fingerprint);
      free (key);
    }
}
