/* gpapa.c - The GNU Privacy Assistant Pipe Access
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

#include "gpapa.h"

char *global_keyServer;

static char *gpg_program;


/* Key management.
 */

/* The public and secret key ring. We only allow for one of each.
 */
static GList *PubRing = NULL, *SecRing = NULL;

/* Release a public key ring G. This is the only place where
 * gpapa_public_key_release() is called.
 */
static void
release_public_keyring (GList *g)
{
  while (g)
    {
      gpapa_public_key_release (g->data);
      g = g_list_next (g);
    }
  g_list_free (g);
}

/* Release a secret key ring G. This is the only place where
 * gpapa_secret_key_release() is called.
 */
static void
release_secret_keyring (GList *g)
{
  while (g)
    {
      gpapa_secret_key_release (g->data);
      g = g_list_next (g);
    }
  g_list_free (g);
}

/* Sort public keys alphabetically.
 */
static int
compare_public_keys (gconstpointer a, gconstpointer b)
{
  const GpapaPublicKey *k1 = a, *k2 = b;
  if (k1->key && k2->key && k1->key->UserID && k2->key->UserID)
    return (strcasecmp (k1->key->UserID, k2->key->UserID));
  else
    return (0);
}

/* Sort secret keys alphabetically.
 */
static int
compare_secret_keys (gconstpointer a, gconstpointer b)
{
  const GpapaPublicKey *k1 = a, *k2 = b;
  if (k1->key && k2->key && k1->key->UserID && k2->key->UserID)
    return (strcasecmp (k1->key->UserID, k2->key->UserID));
  else
    return (0);
}

/* Extract a key fingerprint out of the string BUFFER and return
 * a newly allocated buffer with the fingerprint according to
 * the algorithm ALGORITHM.
 */
char *
gpapa_extract_fingerprint (char *line, int algorithm,
                           GpapaCallbackFunc callback, gpointer calldata)
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
        callback (GPAPA_ACTION_ERROR,
                  "too many fields in GPG colon output", calldata);
    }
  fields = i;
  if (fields != 10)
    {
      callback (GPAPA_ACTION_ERROR,
                "invalid number of fields in GPG colon output", calldata);
      return (NULL);
    }
  else
    {
      if (algorithm == 1)  /* RSA */
        {
          char *fpraw = field[9];
          char *fp = xmalloc (strlen (fpraw) + 16 + 1);
          char *r = fpraw, *q = fp;
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
          return fp;
        }
      else
        {
          char *fpraw = field[9];
          char *fp = xmalloc (strlen (fpraw) + 10 + 1);
          char *r = fpraw, *q = fp;
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
          return fp;
        }
    }
}

/* Extract a string BUFFER like "2001-05-31" to a GDate structure;
 * return NULL on error.
 */
GDate *
gpapa_extract_date (char *buffer)
{
  char *p, *year, *month, *day;
  if (buffer == NULL)
    return (NULL);
  p = buffer;
  year = p;
  while (*p >= '0' && *p <= '9')
    p++;
  if (*p)
    {
      *p = 0;
      p++;
    }
  month = p;
  while (*p >= '0' && *p <= '9')
    p++;
  if (*p)
    {
      *p = 0;
      p++;
    }
  day = p;
  if (year && *year && month && *month && day && *day)
    return (g_date_new_dmy (atoi (day), atoi (month), atoi (year)));
  else
    return (NULL);
}

/* Extract a line LINE of gpg's colon output to a GpapaKey;
 * return NULL on error.
 */
static GpapaKey *
extract_key (char *line, GpapaCallbackFunc callback, gpointer calldata)
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
        callback (GPAPA_ACTION_ERROR,
                  "too many fields in GPG colon output", calldata);
    }
  fields = i;
  if (fields != 10)
    {
      callback (GPAPA_ACTION_ERROR,
                "invalid number of fields in GPG colon output", calldata);
      return (NULL);
    }
  else
    {
      char *quoted;
      GpapaKey *key = gpapa_key_new (field[7], callback, calldata);
      key->KeyTrust = field[1][0];
      key->bits = atoi (field[2]);
      key->algorithm = atoi (field[3]);
      key->KeyID = xstrdup (field[4]);
      key->CreationDate = gpapa_extract_date (field[5]);
      key->ExpirationDate = gpapa_extract_date (field[6]);
      key->OwnerTrust = field[8][0];
      key->UserID = xstrdup (field[9]);

      /* Special case (really?): a quoted colon.
       * Un-quote it manually.
       */
      while ((quoted = strstr (key->UserID, "\\x3a")) != NULL)
        {
          quoted[0] = ':';
          quoted++;
          while (quoted[3])
            {
              quoted[0] = quoted[3];
              quoted++;
            }
          quoted[0] = 0;
        }
      return (key);
    }
}

static void
linecallback_refresh_pub (char *line, gpointer data, GpgStatusCode status)
{
  static GpapaPublicKey *key = NULL;
  PublicKeyData *d = data;
  gpapa_report_error_status (status, d->callback, d->calldata);
  if (status == NO_STATUS && line)
    {
      if (strncmp (line, "pub:", 4) == 0)
        {
#ifdef DEBUG
          fprintf (stderr, "extracting key: %s\n", line);
#endif
          key = (GpapaPublicKey *) xmalloc (sizeof (GpapaPublicKey));
          memset (key, 0, sizeof (GpapaPublicKey));
          key->key = extract_key (line, d->callback, d->calldata);
          PubRing = g_list_append (PubRing, key);
        }
      else if (strncmp (line, "fpr:", 4) == 0 && key && key->key)
        {
#ifdef DEBUG
          fprintf (stderr, "extracting fingerprint: %s\n", line);
#endif
          key->fingerprint = gpapa_extract_fingerprint (line, key->key->algorithm,
                                                        d->callback, d->calldata);
        }
    }
}

void
gpapa_refresh_public_keyring (GpapaCallbackFunc callback, gpointer calldata)
{
  PublicKeyData data = { NULL, callback, calldata };
  char *gpgargv[4];
  if (PubRing)
    {
      release_public_keyring (PubRing);
      PubRing = NULL;
    }
  gpgargv[0] = "--list-keys";
  gpgargv[1] = "--with-colons";
  gpgargv[2] = "--with-fingerprint";
  gpgargv[3] = NULL;
  gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
		    linecallback_refresh_pub, &data, callback, calldata);
  PubRing = g_list_sort (PubRing, compare_public_keys);
}

gint
gpapa_get_public_key_count (GpapaCallbackFunc callback, gpointer calldata)
{
  if (! PubRing)
    gpapa_refresh_public_keyring (callback, calldata);
  return (g_list_length (PubRing));
}

GpapaPublicKey *
gpapa_get_public_key_by_index (gint idx, GpapaCallbackFunc callback,
			       gpointer calldata)
{
  if (! PubRing)
    gpapa_refresh_public_keyring (callback, calldata);
  return (g_list_nth_data (PubRing, idx));
}

GpapaPublicKey *
gpapa_get_public_key_by_ID (char *keyID, GpapaCallbackFunc callback,
			    gpointer calldata)
{
  GList *g;
  GpapaPublicKey *p = NULL;
  if (! PubRing)
    gpapa_refresh_public_keyring (callback, calldata);
  g = PubRing;
  while (g && g->data
           && (p = (GpapaPublicKey *) g->data) != NULL
           && p->key
           && p->key->KeyID
           && strcmp (p->key->KeyID, keyID) != 0)
    g = g_list_next (g);
  if (g && p)
    return p;
  else
    return NULL;
}

static void
linecallback_id_pub (char *line, gpointer data, GpgStatusCode status)
{
  PublicKeyData *d = data;
  gpapa_report_error_status (status, d->callback, d->calldata);
  if (status == NO_STATUS && line && strncmp (line, "pub:", 4) == 0)
    {
      d->key = (GpapaPublicKey *) xmalloc (sizeof (GpapaPublicKey));
      memset (d->key, 0, sizeof (GpapaPublicKey));
      d->key->key = extract_key (line, d->callback, d->calldata);
    }
}

GpapaPublicKey *
gpapa_get_public_key_by_userID (char *userID, GpapaCallbackFunc callback,
			    gpointer calldata)
{
  PublicKeyData data = { NULL, callback, calldata };
  char *gpgargv[4];
  char *uid = xstrdup (userID);
  uid = strcpy (uid, userID);
  gpgargv[0] = "--list-keys";
  gpgargv[1] = "--with-colons";
  gpgargv[2] = uid;
  gpgargv[3] = NULL;
  gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
		    linecallback_id_pub, &data, callback, calldata);
  free (uid);
  if (data.key)
    {
      GpapaPublicKey *result;
      if (data.key->key)
        result = gpapa_get_public_key_by_ID (data.key->key->KeyID,
                                             callback, calldata);
      else
        result = NULL;
      gpapa_public_key_release (data.key);
      return result;
    }
  else
    return NULL;
}

GpapaPublicKey *
gpapa_receive_public_key_from_server (char *keyID, char *ServerName,
				      GpapaCallbackFunc callback,
				      gpointer calldata)
{
  if (keyID && ServerName)
    {
      char *gpgargv[5];
      char *id = xstrcat2 ("0x", keyID);
      gpgargv[0] = "--keyserver";
      gpgargv[1] = ServerName;
      gpgargv[2] = "--recv-keys";
      gpgargv[3] = id;
      gpgargv[4] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
			NULL, NULL, callback, calldata);
      free (id);
      gpapa_refresh_public_keyring (callback, calldata);
    }
  return (gpapa_get_public_key_by_ID (keyID, callback, calldata));
}

static void
linecallback_refresh_sec (char *line, gpointer data, GpgStatusCode status)
{
  SecretKeyData *d = data;
  gpapa_report_error_status (status, d->callback, d->calldata);
  if (status == NO_STATUS && line && strncmp (line, "sec", 3) == 0)
    {
      GpapaSecretKey *key =
	(GpapaSecretKey *) xmalloc (sizeof (GpapaSecretKey));
      memset (key, 0, sizeof (GpapaSecretKey));
      key->key = extract_key (line, d->callback, d->calldata);
      if (key->key && key->key->KeyID)
        {
          GpapaPublicKey *pubkey
            = gpapa_get_public_key_by_ID (key->key->KeyID,
                                          d->callback, d->calldata);
          if (pubkey)
            {
              gpapa_key_release (key->key);
              key->key = pubkey->key;
            }
        }
      SecRing = g_list_append (SecRing, key);
    }
}

void
gpapa_refresh_secret_keyring (GpapaCallbackFunc callback, gpointer calldata)
{
  SecretKeyData data = { NULL, callback, calldata };
  char *gpgargv[3];
  if (SecRing != NULL)
    {
      g_list_free (SecRing);
      SecRing = NULL;
    }
  gpgargv[0] = "--list-secret-keys";
  gpgargv[1] = "--with-colons";
  gpgargv[2] = NULL;
  gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
		    linecallback_refresh_sec, &data, callback, calldata);
  SecRing = g_list_sort (SecRing, compare_secret_keys);
}

gint
gpapa_get_secret_key_count (GpapaCallbackFunc callback, gpointer calldata)
{
  if (! SecRing)
    gpapa_refresh_secret_keyring (callback, calldata);
  return (g_list_length (SecRing));
}

GpapaSecretKey *
gpapa_get_secret_key_by_index (gint idx, GpapaCallbackFunc callback,
			       gpointer calldata)
{
  if (SecRing == NULL)
    gpapa_refresh_secret_keyring (callback, calldata);
  return (g_list_nth_data (SecRing, idx));
}

GpapaSecretKey *
gpapa_get_secret_key_by_ID (char *keyID, GpapaCallbackFunc callback,
			    gpointer calldata)
{
  GList *g;
  GpapaSecretKey *s = NULL;
  if (! SecRing)
    gpapa_refresh_secret_keyring (callback, calldata);
  g = SecRing;
  while (g && g->data
           && (s = (GpapaSecretKey *) g->data) != NULL
           && s->key
           && s->key->KeyID
           && strcmp (s->key->KeyID, keyID) != 0)
    g = g_list_next (g);
  if (g && s)
    return s;
  else
    return NULL;
}
 
static void
linecallback_id_sec (char *line, gpointer data, GpgStatusCode status)
{
  SecretKeyData *d = data;
  gpapa_report_error_status (status, d->callback, d->calldata);
  if (status == NO_STATUS && line && strncmp (line, "sec", 3) == 0)
    {
      d->key = (GpapaSecretKey *) xmalloc (sizeof (GpapaSecretKey));
      memset (d->key, 0, sizeof (GpapaSecretKey));
      d->key->key = extract_key (line, d->callback, d->calldata);
    }
}

GpapaSecretKey *
gpapa_get_secret_key_by_userID (char *userID, GpapaCallbackFunc callback,
			    gpointer calldata)
{
  SecretKeyData data = { NULL, callback, calldata };
  char *gpgargv[4];
  char *uid = xstrdup (userID);
  uid = strcpy (uid, userID);
  gpgargv[0] = "--list-secret-keys";
  gpgargv[1] = "--with-colons";
  gpgargv[2] = uid;
  gpgargv[3] = NULL;
  gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
		    linecallback_id_sec, &data, callback, calldata);
  free (uid);
  if (data.key)
    {
      GpapaSecretKey *result;
      if (data.key->key)
        result = gpapa_get_secret_key_by_ID (data.key->key->KeyID,
                                             callback, calldata);
      else
        result = NULL;
      gpapa_secret_key_release (data.key);
      return result;
    }
  else
    return NULL;
}

void
gpapa_create_key_pair (GpapaPublicKey **publicKey,
		       GpapaSecretKey **secretKey, char *passphrase,
		       GpapaAlgo anAlgo, gint aKeysize, char *aUserID,
		       char *anEmail, char *aComment,
		       GpapaCallbackFunc callback, gpointer calldata)
{
  if (aKeysize && aUserID && anEmail && aComment)
    {
      char *gpgargv[3];
      char *commands = NULL;
      char *commands_sprintf_str;
      char *Algo, *Sub_Algo, *Name_Comment;
      if (aComment && *aComment)
        Name_Comment = g_strdup_printf ("Name-Comment: %s\n", aComment);
      else
        Name_Comment = "";
      if (anAlgo == GPAPA_ALGO_DSA
          || anAlgo == GPAPA_ALGO_ELG_BOTH
          || anAlgo == GPAPA_ALGO_ELG)
	{
	  if (anAlgo == GPAPA_ALGO_DSA) 
	    Algo = "DSA";
	  else if (anAlgo == GPAPA_ALGO_ELG_BOTH) 
	    Algo = "ELG";
	  else /* anAlgo == GPAPA_ALGO_ELG */
	    Algo = "ELG-E";
	  commands_sprintf_str = "Key-Type: %s\n"
	                         "Key-Length: %d\n"
	                         "Name-Real: %s\n"
                                 "%s"                /* Name_Comment */
                                 "Name-Email: %s\n"
                                 "Expire-Date: 0\n"
                                 "Passphrase: %s\n"
                                 "%%commit\n";
	  commands = g_strdup_printf (commands_sprintf_str,
                                      Algo, aKeysize, 
		                      aUserID, Name_Comment, anEmail,
                                      passphrase);
	}
      else if (anAlgo == GPAPA_ALGO_BOTH)
	{
	  Algo = "DSA";
	  Sub_Algo = "ELG-E";
	  commands_sprintf_str = "Key-Type: %s\n"
                                 "Key-Length: %d\n"
                                 "Subkey-Type: %s\n"
                                 "Subkey-Length: %d\n"
                                 "Name-Real: %s\n"
                                 "%s"                /* Name_Comment */
                                 "Name-Email: %s\n"
                                 "Expire-Date: 0\n"
                                 "Passphrase: %s\n"
                                 "%%commit\n";
	  commands = g_strdup_printf (commands_sprintf_str,
                                      Algo, aKeysize, Sub_Algo, aKeysize,
                                      aUserID, Name_Comment, anEmail,
                                      passphrase);
	}
      else 
	callback (GPAPA_ACTION_ERROR, 
		     "specified algorithm not supported", calldata); 
      gpgargv[0] = "--gen-key";
      gpgargv[1] = "--batch";  /* not automatically added due to commands */
      gpgargv[2] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, commands, passphrase, 
			NULL, NULL, callback, calldata);
      free (commands);
      gpapa_refresh_public_keyring (callback, calldata);
      gpapa_refresh_secret_keyring (callback, calldata);
      *publicKey = gpapa_get_public_key_by_userID (aUserID, callback, calldata);
      *secretKey = gpapa_get_secret_key_by_userID (aUserID, callback, calldata);
    }
}

static void
linecallback_export_ownertrust (gchar *line, gpointer data, GpgStatusCode status)
{
  FILE *stream = data;
  if (status == NO_STATUS && stream && line)
    fprintf (stream, "%s\n", line);
}

void
gpapa_export_ownertrust (gchar *targetFileID, GpapaArmor Armor,
			 GpapaCallbackFunc callback, gpointer calldata)
{
  if (!targetFileID)
    callback (GPAPA_ACTION_ERROR, "target file not specified", calldata);
  else
    {
      FILE *stream = fopen (targetFileID, "w");
      if (!stream)
	callback (GPAPA_ACTION_ERROR,
		  "could not open target file for writing", calldata);
      else
	{
	  char *gpgargv[3];
	  int i = 0;
	  if (Armor == GPAPA_ARMOR)
	    gpgargv[i++] = "--armor";
	  gpgargv[i++] = "--export-ownertrust";
	  gpgargv[i] = NULL;
	  gpapa_call_gnupg
	    (gpgargv, TRUE, NULL, NULL,
	     linecallback_export_ownertrust, stream, callback, calldata);
	  fclose (stream);
	}
    }
}

void
gpapa_import_ownertrust (gchar *sourceFileID,
			 GpapaCallbackFunc callback, gpointer calldata)
{
  if (!sourceFileID)
    callback (GPAPA_ACTION_ERROR, "source file not specified", calldata);
  else
    {
      char *gpgargv[3];
      gpgargv[0] = "--import-ownertrust";
      gpgargv[1] = sourceFileID;
      gpgargv[2] = NULL;
      gpapa_call_gnupg 	(gpgargv, TRUE, NULL, NULL,
                         NULL, NULL, callback, calldata);
    }
}

void
gpapa_update_trust_database (GpapaCallbackFunc callback, gpointer calldata)
{
  char *gpgargv[2];
  gpgargv[0] = "--update-trustdb";
  gpgargv[1] = NULL;
  gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
                    NULL, NULL, callback, calldata);
}

void
gpapa_import_keys (gchar *sourceFileID,
		   GpapaCallbackFunc callback, gpointer calldata)
{
  if (!sourceFileID)
    callback (GPAPA_ACTION_ERROR, "source file not specified", calldata);
  else
    {
      char *gpgargv[3];
      gpgargv[0] = "--import";
      gpgargv[1] = sourceFileID;
      gpgargv[2] = NULL;
      gpapa_call_gnupg
	(gpgargv, TRUE, NULL, NULL,
	 NULL, NULL, callback, calldata);
    }
}


/* Miscellaneous.
 */

const char *
gpapa_private_get_gpg_program (void)
{
  return gpg_program;
}

void
gpapa_init (const char *gpg)
{
  free (gpg_program);
  gpg_program = xstrdup (gpg ? gpg : "/usr/bin/gpg");
}

void
gpapa_fini (void)
{
  if (PubRing != NULL)
    {
      release_public_keyring (PubRing);
      PubRing = NULL;
    }
  if (SecRing != NULL)
    {
      release_secret_keyring (SecRing);
      SecRing = NULL;
    }
  free (gpg_program);
  gpg_program = NULL;
}

void
gpapa_idle (void)
{
  /* In the future, we will call a non-blocking select() and
   * waitpid() here to read data from an open pipe and see
   * whether gpg is still running.
   *
   * Right now, just do nothing.
   */
}
