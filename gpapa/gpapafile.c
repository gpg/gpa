/* gpapafile.c - The GNU Privacy Assistant Pipe Access - file object
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

static void
linecallback_check_gpg_status (char *line, gpointer data, GpgStatusCode status)
{
  FileData *d = data;
  gpapa_report_error_status (status, d->callback, d->calldata);
}

GpapaFile *
gpapa_file_new (char *fileID, GpapaCallbackFunc callback, gpointer calldata)
{
  GpapaFile *fileNew;
  fileNew = (GpapaFile *) xmalloc (sizeof (GpapaFile));
  memset (fileNew, 0, sizeof (GpapaFile));
  fileNew->identifier = (char *) xstrdup (fileID);
  fileNew->name = (char *) xstrdup (fileID);   /*!!! */
  return (fileNew);
}

char *
gpapa_file_get_identifier (GpapaFile *file, GpapaCallbackFunc callback,
                           gpointer calldata)
{
  if (file != NULL)
    return (file->identifier);
  else
    return (NULL);
}

char *
gpapa_file_get_name (GpapaFile *file, GpapaCallbackFunc callback,
                     gpointer calldata)
{
  if (file != NULL)
    return (file->name);
  else
    return (NULL);
}

static void
linecallback_get_status (char *line, gpointer data, GpgStatusCode status)
{
  FileData *d = data;
  /* No error reporting!
   * We are checking if something works; errors are natural.
   */
  if (line && data)
    {
      if (status == STATUS_NODATA)
        d->file->status_flags |= GPAPA_FILE_STATUS_NODATA;
      else if (status == NO_STATUS)
        {
          if (gpapa_line_begins_with (line, ":literal data packet:"))
            d->file->status_flags |= GPAPA_FILE_STATUS_LITERAL;
          if (gpapa_line_begins_with (line, ":encrypted data packet:"))
            d->file->status_flags |= GPAPA_FILE_STATUS_ENCRYPTED;
          if (gpapa_line_begins_with (line, ":pubkey enc packet:"))
            d->file->status_flags |= GPAPA_FILE_STATUS_PUBKEY;
          if (gpapa_line_begins_with (line, ":symkey enc packet:"))
            d->file->status_flags |= GPAPA_FILE_STATUS_SYMKEY;
          if (gpapa_line_begins_with (line, ":compressed packet:"))
            d->file->status_flags |= GPAPA_FILE_STATUS_COMPRESSED;
          if (gpapa_line_begins_with (line, ":onepass_sig packet:"))
            d->file->status_flags |= GPAPA_FILE_STATUS_SIGNATURE;
          if (gpapa_line_begins_with (line, ":signature packet:"))
            d->file->status_flags |= GPAPA_FILE_STATUS_SIGNATURE;
        }
    }
}

GpapaFileStatus
gpapa_file_get_status (GpapaFile *file, GpapaCallbackFunc callback,
                       gpointer calldata)
{
  if (file == NULL)
    return (GPAPA_FILE_CLEAR);
  else
    {
      FileData data = { file, callback, calldata };
      const gchar *gpgargv[3];
      gchar *quoted_filename = NULL;
      struct stat statbuf;

      /* First check explicitly whether the file exists and is a
       * regular file.
       */
      if (stat (file->identifier, &statbuf) != 0)
        {
          callback (GPAPA_ACTION_ERROR, _("Error accessing file"), calldata);
          file->status_flags = GPAPA_FILE_STATUS_NODATA;
          return (GPAPA_FILE_UNKNOWN);
        }
      else if (! S_ISREG (statbuf.st_mode))
        {
          callback (GPAPA_ACTION_ERROR, _("Not a regular file"), calldata);
          file->status_flags = GPAPA_FILE_STATUS_NODATA;
          return (GPAPA_FILE_UNKNOWN);
        }

      /* @@@ This should be rewritten once GnuPG supports
       * something like `--get-file-status'.
       */
      gpgargv[0] = "--list-packets";
#ifdef HAVE_DOSISH_SYSTEM
      quoted_filename = g_strconcat ("\"", file->identifier, "\"", NULL);
#else
      quoted_filename = file->identifier;
#endif
      gpgargv[1] = quoted_filename;
      gpgargv[2] = NULL;
      file->status_flags = 0;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, NULL,
                        linecallback_get_status, &data, callback, calldata);
#ifdef HAVE_DOSISH_SYSTEM
      g_free (quoted_filename);
#endif
      if (file->status_flags == GPAPA_FILE_STATUS_NODATA)
        return (GPAPA_FILE_CLEAR);
      else if (file->status_flags & GPAPA_FILE_STATUS_PUBKEY)
        return (GPAPA_FILE_ENCRYPTED);
      else if (file->status_flags & GPAPA_FILE_STATUS_SYMKEY)
        return (GPAPA_FILE_PROTECTED);
      else if ((file->status_flags & GPAPA_FILE_STATUS_SIGNATURE)
               && (file->status_flags & GPAPA_FILE_STATUS_LITERAL))
        {
          if (file->status_flags & GPAPA_FILE_STATUS_COMPRESSED)
            return (GPAPA_FILE_SIGNED);
          else
            return (GPAPA_FILE_CLEARSIGNED);
        }
      else if (file->status_flags & GPAPA_FILE_STATUS_SIGNATURE)
        return (GPAPA_FILE_DETACHED_SIGNATURE);
      else
        return (GPAPA_FILE_UNKNOWN);
    }
}

gint
gpapa_file_get_signature_count (GpapaFile *file, GpapaCallbackFunc callback,
                                gpointer calldata)
{
  GList *sigs = gpapa_file_get_signatures (file, callback, calldata);
  return (g_list_length (sigs));
}

static gboolean
status_check (char *buffer, GpgStatusCode code, GpgStatusCode status, char **data)
{
  gboolean result = FALSE;

  if (status == code)
    {
      char *p = buffer;
      /* Parse the buffer.
       * First string: key ID
       */
      while (*p == ' ')
        p++;
      data[0] = p;
      while (*p && *p != ' ')
        p++;
      if (*p)
        {
          *p = 0;
          p++;
          while (*p == ' ')
            p++;
          data[1] = p;
          while (*p && *p != '\n')
            p++;
          if (*p == '\n')
            p = 0;
        }
      else
        data[1] = NULL;
      result = TRUE;
    }
  return (result);
}

static void
linecallback_get_signatures (char *line, gpointer data, GpgStatusCode status)
{
  FileData *d = data;
  if (status != STATUS_NODATA)  /* not signed */
    gpapa_report_error_status (status, d->callback, d->calldata);
  if (status != NO_STATUS)
    {
      char *sigdata[2] = { NULL, NULL };
      if (status_check (line, STATUS_GOODSIG, status, sigdata))
        {
          GpapaSignature *sig = gpapa_signature_new (d->callback, d->calldata);
          if (sigdata[0] == NULL || strlen(sigdata[0]) <= 8)
            sig->KeyID = NULL;
          else
            sig->KeyID = xstrdup (sigdata[0] + 8);
          sig->validity = GPAPA_SIG_VALID;
          sig->UserID = xstrdup (sigdata[1]);
          d->file->sigs = g_list_append (d->file->sigs, sig);
        }
      else if (status_check (line, STATUS_BADSIG, status, sigdata))
        {
          GpapaSignature *sig = gpapa_signature_new (d->callback, d->calldata);
          if (sigdata[0] == NULL || strlen(sigdata[0]) <= 8)
            sig->KeyID = NULL;
          else
            sig->KeyID = xstrdup (sigdata[0] + 8);
          sig->validity = GPAPA_SIG_INVALID;
          d->file->sigs = g_list_append (d->file->sigs, sig);
        }
      else if (status_check (line, STATUS_ERRSIG, status, sigdata))
        {
          GpapaSignature *sig = gpapa_signature_new (d->callback, d->calldata);
          if (sigdata[0] == NULL || strlen(sigdata[0]) <= 8)
            sig->KeyID = NULL;
          else
            sig->KeyID = xstrdup (sigdata[0] + 8);
          sig->validity = GPAPA_SIG_UNKNOWN;
          d->file->sigs = g_list_append (d->file->sigs, sig);
        }
    }
}

GList *
gpapa_file_get_signatures (GpapaFile *file, GpapaCallbackFunc callback,
                           gpointer calldata)
{
  if (file == NULL)
    return (NULL);
  else
    {
      if (file->sigs == NULL && file->identifier != NULL)
        {
          FileData data = { file, callback, calldata };
          const gchar *gpgargv[3];
#ifdef HAVE_DOSISH_SYSTEM
          gchar *quoted_filename = g_strconcat ("\"", file->identifier, "\"", NULL);
#else
          gchar *quoted_filename = file->identifier;
#endif
          gpgargv[0] = "--verify";
          gpgargv[1] = quoted_filename;
          gpgargv[2] = NULL;
          gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, NULL,
                            linecallback_get_signatures, &data,
                            callback, calldata);
#ifdef HAVE_DOSISH_SYSTEM
          g_free (quoted_filename);
#endif
        }
      return (file->sigs);
    }
}

void
gpapa_file_sign (GpapaFile *file, const gchar *targetFileID, const gchar *keyID,
                 const gchar *PassPhrase, GpapaSignType SignType, GpapaArmor Armor,
                 GpapaCallbackFunc callback, gpointer calldata)
{
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, _("Missing file name"), calldata);
  else if (keyID == NULL)
    callback (GPAPA_ACTION_ERROR, _("Missing private key ID for signing"),
              calldata);
  else
    {
      FileData data = { file, callback, calldata };
      gchar *full_keyID;
      gchar *quoted_source_filename = NULL, *quoted_target_filename = NULL;
      const gchar *gpgargv[10];
      int i = 0;
      switch (SignType)
        {
          case GPAPA_SIGN_NORMAL:
            gpgargv[i++] = "--sign";
            break;
          case GPAPA_SIGN_CLEAR:
            gpgargv[i++] = "--clearsign";
            break;
          case GPAPA_SIGN_DETACH:
            gpgargv[i++] = "--detach-sign";
            break;
          default:
            callback (GPAPA_ACTION_ERROR, _("Invalid signature type"), calldata);
            return;
        }
      gpgargv[i++] = "-u";
      full_keyID = xstrcat2 ("0x", keyID);
      gpgargv[i++] = full_keyID;
      if (Armor == GPAPA_ARMOR)
        gpgargv[i++] = "--armor";
      if (targetFileID != NULL)
        {
#ifdef HAVE_DOSISH_SYSTEM
          quoted_target_filename = g_strconcat ("\"", targetFileID, "\"", NULL);
#else
          quoted_target_filename = (gchar*) targetFileID;
#endif
          gpgargv[i++] = "-o";
          gpgargv[i++] = quoted_target_filename;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
      gpgargv[i++] = "--no-tty";
#ifdef HAVE_DOSISH_SYSTEM
      quoted_source_filename = g_strconcat ("\"", file->identifier, "\"", NULL);
#else
      quoted_source_filename = file->identifier;
#endif
      gpgargv[i++] = quoted_source_filename;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, PassPhrase,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
      free (full_keyID);
#ifdef HAVE_DOSISH_SYSTEM
      g_free (quoted_source_filename);
      if (quoted_target_filename)
        g_free (quoted_target_filename);
#endif
    }
}

void
gpapa_file_encrypt (GpapaFile *file, const gchar *targetFileID,
                    GList *rcptKeyIDs, GpapaArmor Armor,
                    GpapaCallbackFunc callback, gpointer calldata) {
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, _("Missing file name"), calldata);
  else if (rcptKeyIDs == NULL)
    callback (GPAPA_ACTION_ERROR, _("Missing public key ID(s) for encrypting"),
              calldata);
  else
    {
      FileData data = { file, callback, calldata };
      GList *R;
      int i = 0, l = g_list_length (rcptKeyIDs);
      gchar *quoted_source_filename = NULL, *quoted_target_filename = NULL;
      gchar **gpgargv = xmalloc ((7 + 2 * l) * sizeof (char *));
      R = rcptKeyIDs;
      while (R)
        {
          gpgargv[i++] = "-r";
          gpgargv[i++] = xstrcat2 ("0x", (char *) R->data);
          R = g_list_next (R);
        }
      gpgargv[i++] = "--encrypt";
      if (Armor == GPAPA_ARMOR)
        gpgargv[i++] = "--armor";
      if (targetFileID != NULL)
        {
#ifdef HAVE_DOSISH_SYSTEM
          quoted_target_filename = g_strconcat ("\"", targetFileID, "\"", NULL);
#else
          quoted_target_filename = (gchar*) targetFileID;
#endif
          gpgargv[i++] = "-o";
          gpgargv[i++] = quoted_target_filename;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
#ifdef HAVE_DOSISH_SYSTEM
      quoted_source_filename = g_strconcat ("\"", file->identifier, "\"", NULL);
#else
      quoted_source_filename = file->identifier;
#endif
      gpgargv[i++] = quoted_source_filename;
      gpgargv[i] = NULL;
      gpapa_call_gnupg ((const gchar **) gpgargv, TRUE, NULL, NULL, NULL,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
      for (i = 1; i < 2 * l; i += 2)
        free (gpgargv[i]);
#ifdef HAVE_DOSISH_SYSTEM
      g_free (quoted_source_filename);
      if (quoted_target_filename)
        g_free (quoted_target_filename);
#endif
      free (gpgargv);
    }
}

void
gpapa_file_encrypt_and_sign (GpapaFile *file, const gchar *targetFileID,
                             GList *rcptKeyIDs, char *keyID,
                             const gchar *PassPhrase, GpapaSignType SignType,
                             GpapaArmor Armor, GpapaCallbackFunc callback,
                             gpointer calldata)
{
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, _("Missing file name"), calldata);
  else if (rcptKeyIDs == NULL)
    callback (GPAPA_ACTION_ERROR, _("Missing public key ID(s) for encrypting"),
              calldata);
  else
    {
      FileData data = { file, callback, calldata };
      GList *R;
      int i = 0, l = g_list_length (rcptKeyIDs);
      char *full_keyID;
      gchar *quoted_source_filename = NULL, *quoted_target_filename = NULL;
      char **gpgargv = xmalloc ((11 + 2 * l) * sizeof (char *));
      R = rcptKeyIDs;
      while (R)
        {
          gpgargv[i++] = "-r";
          gpgargv[i++] = xstrcat2 ("0x", (char *) R->data);
          R = g_list_next (R);
        }
      gpgargv[i++] = "-u";
      full_keyID = xstrcat2 ("0x", keyID);
      gpgargv[i++] = full_keyID;
      if (Armor == GPAPA_ARMOR)
        gpgargv[i++] = "--armor";
      gpgargv[i++] = "--encrypt";
      gpgargv[i++] = "--sign";
      if (Armor == GPAPA_ARMOR)
        gpgargv[i++] = "--armor";
      if (targetFileID != NULL)
        {
#ifdef HAVE_DOSISH_SYSTEM
          quoted_target_filename = g_strconcat ("\"", targetFileID, "\"", NULL);
#else
          quoted_target_filename = (gchar*) targetFileID;
#endif
          gpgargv[i++] = "-o";
          gpgargv[i++] = quoted_target_filename;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
      gpgargv[i++] = "--no-tty";
#ifdef HAVE_DOSISH_SYSTEM
      quoted_source_filename = g_strconcat ("\"", file->identifier, "\"", NULL);
#else
      quoted_source_filename = file->identifier;
#endif
      gpgargv[i++] = quoted_source_filename;
      gpgargv[i] = NULL;
      gpapa_call_gnupg ((const gchar **) gpgargv, TRUE, NULL, NULL, PassPhrase,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
      for (i = 1; i < 2 * l; i += 2)
        free (gpgargv[i]);
      free (gpgargv);
#ifdef HAVE_DOSISH_SYSTEM
      g_free (quoted_source_filename);
      if (quoted_target_filename)
        g_free (quoted_target_filename);
#endif
      free (full_keyID);
    }
}

void
gpapa_file_protect (GpapaFile *file, const gchar *targetFileID,
                    const gchar *PassPhrase, GpapaArmor Armor,
                    GpapaCallbackFunc callback, gpointer calldata)
{
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, _("Missing file name"), calldata);
  else if (PassPhrase == NULL)
    callback (GPAPA_ACTION_ERROR,
              _("Missing passphrase for symmetric encrypting"), calldata);
  else
    {
      FileData data = { file, callback, calldata };
      const gchar *gpgargv[7];
      gchar *quoted_source_filename = NULL, *quoted_target_filename = NULL;
      int i = 0;
      gpgargv[i++] = "--symmetric";
      if (Armor == GPAPA_ARMOR)
        gpgargv[i++] = "--armor";
      if (targetFileID != NULL)
        {
#ifdef HAVE_DOSISH_SYSTEM
          quoted_target_filename = g_strconcat ("\"", targetFileID, "\"", NULL);
#else
          quoted_target_filename = (gchar*) targetFileID;
#endif
          gpgargv[i++] = "-o";
          gpgargv[i++] = quoted_target_filename;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
#ifdef HAVE_DOSISH_SYSTEM
      quoted_source_filename = g_strconcat ("\"", file->identifier, "\"", NULL);
#else
      quoted_source_filename = file->identifier;
#endif
      gpgargv[i++] = quoted_source_filename;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, PassPhrase,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
#ifdef HAVE_DOSISH_SYSTEM
      g_free (quoted_source_filename);
      if (quoted_target_filename)
        g_free (quoted_target_filename);
#endif
    }
}

void
gpapa_file_decrypt (GpapaFile *file, char *targetFileID,
                    const gchar *PassPhrase, GpapaCallbackFunc callback,
                    gpointer calldata)
{
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, _("Missing file name"), calldata);
  else if (PassPhrase == NULL)
    callback (GPAPA_ACTION_ERROR,
              _("Missing passphrase for decrypting"), calldata);
  else
    {
      FileData data = { file, callback, calldata };
      const gchar *gpgargv[6];
      gchar *quoted_source_filename, *quoted_target_filename = NULL;
      int i = 0;
      gpgargv[i++] = "--decrypt";
      if (targetFileID != NULL)
        {
#ifdef HAVE_DOSISH_SYSTEM
          quoted_target_filename = g_strconcat ("\"", targetFileID, "\"", NULL);
#else
          quoted_target_filename = targetFileID;
#endif
          gpgargv[i++] = "-o";
          gpgargv[i++] = quoted_target_filename;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
#ifdef HAVE_DOSISH_SYSTEM
      quoted_source_filename = g_strconcat ("\"", file->identifier, "\"", NULL);
#else
      quoted_source_filename = file->identifier;
#endif
      gpgargv[i++] = quoted_source_filename;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL, PassPhrase,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
#ifdef HAVE_DOSISH_SYSTEM
      g_free (quoted_source_filename);
      if (quoted_target_filename)
        g_free (quoted_target_filename);
#endif
    }
}

void
gpapa_file_release (GpapaFile *file, GpapaCallbackFunc callback,
                    gpointer calldata)
{
  free (file->identifier);
  free (file->name);
  if (file->sigs)
    g_list_free (file->sigs);
  free (file);
}
