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
      char *gpgargv[3];
      struct stat statbuf;

      /* First check explicitly whether the file exists and is a
       * regular file.
       */
      if (stat (file->identifier, &statbuf) != 0)
        {
          callback (GPAPA_ACTION_ERROR, "error accessing file", calldata);
          file->status_flags = GPAPA_FILE_STATUS_NODATA;
          return (GPAPA_FILE_UNKNOWN);
        }
      else if (! S_ISREG (statbuf.st_mode))
        {
          callback (GPAPA_ACTION_ERROR, "not a regular file", calldata);
          file->status_flags = GPAPA_FILE_STATUS_NODATA;
          return (GPAPA_FILE_UNKNOWN);
        }

      /* @@@ This should be rewritten once GnuPG supports
       * something like `--get-file-status'.
       */
      gpgargv[0] = "--list-packets";
      gpgargv[1] = file->identifier;
      gpgargv[2] = NULL;
      file->status_flags = 0;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
                        linecallback_get_status, &data, callback, calldata);
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
          GpapaSignature *sig =
            gpapa_signature_new (sigdata[0], d->callback, d->calldata);
          sig->validity = GPAPA_SIG_VALID;
          sig->UserID = xstrdup (sigdata[1]);
          d->file->sigs = g_list_append (d->file->sigs, sig);
        }
      else if (status_check (line, STATUS_BADSIG, status, sigdata))
        {
          GpapaSignature *sig =
            gpapa_signature_new (sigdata[0], d->callback, d->calldata);
          sig->validity = GPAPA_SIG_INVALID;
          d->file->sigs = g_list_append (d->file->sigs, sig);
        }
      else if (status_check (line, STATUS_ERRSIG, status, sigdata))
        {
          GpapaSignature *sig =
            gpapa_signature_new (sigdata[0], d->callback, d->calldata);
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
          char *gpgargv[3];
          gpgargv[0] = "--verify";
          gpgargv[1] = file->identifier;
          gpgargv[2] = NULL;
          gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
                            linecallback_get_signatures, &data,
                            callback, calldata);
        }
      return (file->sigs);
    }
}

void
gpapa_file_sign (GpapaFile *file, char *targetFileID, char *keyID,
                 char *PassPhrase, GpapaSignType SignType, GpapaArmor Armor,
                 GpapaCallbackFunc callback, gpointer calldata)
{
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, "missing file name", calldata);
  else if (keyID == NULL)
    callback (GPAPA_ACTION_ERROR, "missing private key ID for signing",
              calldata);
  else
    {
      FileData data = { file, callback, calldata };
      char *full_keyID;
      char *gpgargv[9];
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
            callback (GPAPA_ACTION_ERROR, "invalid signature type", calldata);
            return;
        }
      gpgargv[i++] = "-u";
      full_keyID = xstrcat2 ("0x", keyID);
      gpgargv[i++] = full_keyID;
      if (Armor == GPAPA_ARMOR)
        gpgargv[i++] = "--armor";
      if (targetFileID != NULL)
        {
          gpgargv[i++] = "-o";
          gpgargv[i++] = targetFileID;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
      gpgargv[i++] = file->identifier;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, PassPhrase,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
      free (full_keyID);
    }
}

void
gpapa_file_encrypt (GpapaFile *file, char *targetFileID,
                    GList *rcptKeyIDs, GpapaArmor Armor,
                    GpapaCallbackFunc callback, gpointer calldata)
{
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, "missing file name", calldata);
  else if (rcptKeyIDs == NULL)
    callback (GPAPA_ACTION_ERROR, "missing public key ID(s) for encrypting",
              calldata);
  else
    {
      FileData data = { file, callback, calldata };
      GList *R;
      int i = 0, l = g_list_length (rcptKeyIDs);
      char **gpgargv = xmalloc ((7 + 2 * l) * sizeof (char *));
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
          gpgargv[i++] = "-o";
          gpgargv[i++] = targetFileID;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
      gpgargv[i++] = file->identifier;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, NULL,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
      for (i = 1; i < 2 * l; i += 2)
        free (gpgargv[i]);
      free (gpgargv);
    }
}

void
gpapa_file_encrypt_and_sign (GpapaFile *file, char *targetFileID,
                             GList *rcptKeyIDs, char *keyID,
                             char *PassPhrase, GpapaSignType SignType,
                             GpapaArmor Armor, GpapaCallbackFunc callback,
                             gpointer calldata)
{
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, "missing file name", calldata);
  else if (rcptKeyIDs == NULL)
    callback (GPAPA_ACTION_ERROR, "missing public key ID(s) for encrypting",
              calldata);
  else
    {
      FileData data = { file, callback, calldata };
      GList *R;
      int i = 0, l = g_list_length (rcptKeyIDs);
      char *full_keyID;
      char **gpgargv = xmalloc ((10 + 2 * l) * sizeof (char *));
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
          gpgargv[i++] = "-o";
          gpgargv[i++] = targetFileID;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
      gpgargv[i++] = file->identifier;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, PassPhrase,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
      for (i = 1; i < 2 * l; i += 2)
        free (gpgargv[i]);
      free (gpgargv);
      free (full_keyID);
    }
}

void
gpapa_file_protect (GpapaFile *file, char *targetFileID,
                    char *PassPhrase, GpapaArmor Armor,
                    GpapaCallbackFunc callback, gpointer calldata)
{
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, "missing file name", calldata);
  else if (PassPhrase == NULL)
    callback (GPAPA_ACTION_ERROR,
              "missing passphrase for symmetric encrypting", calldata);
  else
    {
      FileData data = { file, callback, calldata };
      char *gpgargv[7];
      int i = 0;
      gpgargv[i++] = "--symmetric";
      if (Armor == GPAPA_ARMOR)
        gpgargv[i++] = "--armor";
      if (targetFileID != NULL)
        {
          gpgargv[i++] = "-o";
          gpgargv[i++] = targetFileID;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
      gpgargv[i++] = file->identifier;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, PassPhrase,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
    }
}

void
gpapa_file_decrypt (GpapaFile *file, char *targetFileID,
                    char *PassPhrase, GpapaCallbackFunc callback,
                    gpointer calldata)
{
  if (file == NULL)
    callback (GPAPA_ACTION_ERROR, "missing file name", calldata);
  else if (PassPhrase == NULL)
    callback (GPAPA_ACTION_ERROR,
              "missing passphrase for decrypting", calldata);
  else
    {
      FileData data = { file, callback, calldata };
      char *gpgargv[6];
      int i = 0;
      gpgargv[i++] = "--decrypt";
      if (targetFileID != NULL)
        {
          gpgargv[i++] = "-o";
          gpgargv[i++] = targetFileID;
        }
      gpgargv[i++] = "--yes";  /* overwrite the file */
      gpgargv[i++] = file->identifier;
      gpgargv[i] = NULL;
      gpapa_call_gnupg (gpgargv, TRUE, NULL, PassPhrase,
                        linecallback_check_gpg_status, &data,
                        callback, calldata);
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
