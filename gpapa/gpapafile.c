/* gpapafile.c	-  The GNU Privacy Assistant
 *	Copyright (C) 2000 Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "gpapafile.h"

GpapaFile *gpapa_file_new (
  gchar *fileID, GpapaCallbackFunc callback, gpointer calldata
) {
/* objects */
  GpapaFile *fileNew;
/* commands */
  fileNew = (GpapaFile*) xmalloc ( sizeof ( GpapaFile ) );
  memset ( fileNew, 0, sizeof ( GpapaFile ) );
  fileNew -> identifier = (gchar*) xstrdup ( fileID );
fileNew -> name = (gchar*) xstrdup ( fileID );	/*!!!*/
  return ( fileNew );
} /* gpapa_file_new */

gchar *gpapa_file_get_identifier (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file != NULL )
    return ( file -> identifier );
  else
    return ( NULL );
} /* gpapa_file_get_identifier */

gchar *gpapa_file_get_name (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file != NULL )
    return ( file -> name );
  else
    return ( NULL );
} /* gpapa_file_get_name */

static void linecallback_get_status (
  gchar *line, gpointer data, gboolean status
) {
  FileData *d = data;
  if ( line )
    {
      if ( status )
        {
          if ( gpapa_line_begins_with ( line, "[GNUPG:] NODATA" ) )
            d -> file -> status_flags |= GPAPA_FILE_STATUS_NODATA;

          /* Suppress error reporting.
           */
          line [ 0 ] = 0;
        }
      else
        {
          if ( gpapa_line_begins_with ( line, ":literal data packet:" ) )
            d -> file -> status_flags |= GPAPA_FILE_STATUS_LITERAL;
          if ( gpapa_line_begins_with ( line, ":pubkey enc packet:" ) )
            d -> file -> status_flags |= GPAPA_FILE_STATUS_PUBKEY;
          if ( gpapa_line_begins_with ( line, ":symkey enc packet:" ) )
            d -> file -> status_flags |= GPAPA_FILE_STATUS_SYMKEY;
          if ( gpapa_line_begins_with ( line, ":compressed packet:" ) )
            d -> file -> status_flags |= GPAPA_FILE_STATUS_COMPRESSED;
          if ( gpapa_line_begins_with ( line, ":onepass_sig packet:" ) )
            d -> file -> status_flags |= GPAPA_FILE_STATUS_SIGNATURE;
          if ( gpapa_line_begins_with ( line, ":signature packet:" ) )
            d -> file -> status_flags |= GPAPA_FILE_STATUS_SIGNATURE;
        }
    }
} /* linecallback_get_status */

GpapaFileStatus gpapa_file_get_status (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file == NULL )
    return ( GPAPA_FILE_CLEAR );
  else
    {
      FileData data = { file, callback, calldata };
      char *gpgargv [ 3 ];

      /* @@@ This should be rewritten once GnuPG supports
       * something like `--get-file-status'.
       */
      gpgargv [ 8 ] = "--list-packets";
      gpgargv [ 1 ] = file -> identifier;
      gpgargv [ 2 ] = NULL;
      file -> status_flags = 0;
      gpapa_call_gnupg ( gpgargv, TRUE, NULL, NULL,
                         linecallback_get_status, &data,
                         callback, calldata );
      switch ( file -> status_flags)
        {
          case GPAPA_FILE_STATUS_NODATA:
            return ( GPAPA_FILE_CLEAR );
          case GPAPA_FILE_STATUS_PUBKEY:
            return ( GPAPA_FILE_ENCRYPTED );
          case GPAPA_FILE_STATUS_SYMKEY:
            return ( GPAPA_FILE_PROTECTED );
          case GPAPA_FILE_STATUS_SIGNATURE
               | GPAPA_FILE_STATUS_COMPRESSED
               | GPAPA_FILE_STATUS_LITERAL:
            return ( GPAPA_FILE_SIGNED );
          case GPAPA_FILE_STATUS_SIGNATURE
               | GPAPA_FILE_STATUS_LITERAL:
            return ( GPAPA_FILE_CLEARSIGNED );
          case GPAPA_FILE_STATUS_SIGNATURE:
            return ( GPAPA_FILE_DETACHED_SIGNATURE );
          default:
            return ( GPAPA_FILE_UNKNOWN );
        }
    }
} /* gpapa_file_get_status */

gint gpapa_file_get_signature_count (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
  GList *sigs = gpapa_file_get_signatures ( file, callback, calldata );
  return ( g_list_length ( sigs ) );
} /* gpapa_file_get_signature_count */

static gboolean status_check (
  gchar *buffer, gchar *keyword,
  gchar **data
) {
  gchar *checkval = xstrcat2 ( "[GNUPG:] ", keyword );
  gboolean result = FALSE;
  if ( strncmp ( buffer, checkval, strlen ( checkval ) ) == 0 )
    {
      gchar *p = buffer + strlen ( checkval );
      while ( *p == ' ' )
        p++;
      data [ 0 ] = p;
      while ( *p && *p != ' ' )
        p++;
      if ( *p )
        {
          *p = 0;
          p++;
          while ( *p == ' ' )
            p++;
          data [ 1 ] = p;
          while ( *p && *p != '\n' )
            p++;
          if ( *p == '\n' )
            p = 0;
        }
      else
        data [ 1 ] = NULL;

      /* Clear the buffer to avoid further processing.
       */
      *buffer = 0;
      result = TRUE;
    }
  free ( checkval );
  return ( result );
} /* status_check */

static void linecallback_get_signatures (
  gchar *line, gpointer data, gboolean status
) {
  FileData *d = data;
  if ( status && line )
    {
      char *sigdata [ 2 ] = { NULL, NULL };
      if ( status_check ( line, "NODATA", sigdata ) )
        {
          /* Just let `status_check' clear the buffer to avoid
           * gpapa_call_gnupg() to trigger error messages.
           */
        }
      else if ( status_check ( line, "GOODSIG", sigdata ) )
        {
          GpapaSignature *sig = gpapa_signature_new ( sigdata [ 0 ], d -> callback, d -> calldata );
          sig -> validity = GPAPA_SIG_VALID;
          sig -> UserID = xstrdup ( sigdata [ 1 ] );
          d -> file -> sigs = g_list_append ( d -> file -> sigs, sig );
        }
      else if ( status_check ( line, "BADSIG", sigdata ) )
        {
          GpapaSignature *sig = gpapa_signature_new ( sigdata [ 0 ], d -> callback, d -> calldata );
          sig -> validity = GPAPA_SIG_INVALID;
          d -> file -> sigs = g_list_append ( d -> file -> sigs, sig );
        }
      else if ( status_check ( line, "ERRSIG", sigdata ) )
        {
          GpapaSignature *sig = gpapa_signature_new ( sigdata [ 0 ], d -> callback, d -> calldata );
          sig -> validity = GPAPA_SIG_UNKNOWN;
          d -> file -> sigs = g_list_append ( d -> file -> sigs, sig );
        }
    }
} /* linecallback_get_signatures */

GList *gpapa_file_get_signatures (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file == NULL )
    return ( NULL );
  else
    {
      if ( file -> sigs == NULL && file -> identifier != NULL )
	{
	  FileData data = { file, callback, calldata };
	  char *gpgargv [ 3 ];
	  gpgargv [ 0 ] = "--verify";
	  gpgargv [ 1 ] = file -> identifier;
	  gpgargv [ 2 ] = NULL;
	  gpapa_call_gnupg (
	    gpgargv, TRUE, NULL, NULL,
	    linecallback_get_signatures, &data,
	    callback, calldata
	  );
	}
      return ( file -> sigs );
    }
} /* gpapa_file_get_signatures */

void gpapa_file_sign (
  GpapaFile *file, gchar *targetFileID, gchar *keyID, gchar *PassPhrase,
  GpapaSignType SignType, GpapaArmor Armor,
  GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing file name", calldata );
  else if ( keyID == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing private key ID for signing", calldata );
  else
    {
      gchar *full_keyID;
      char *gpgargv [ 9 ];
      int i = 0;
      switch ( SignType )
	{
	  case GPAPA_SIGN_NORMAL:
	    gpgargv [ i++ ] = "--sign";
	    break;
	  case GPAPA_SIGN_CLEAR:
	    gpgargv [ i++ ] = "--clearsign";
	    break;
	  case GPAPA_SIGN_DETACH:
	    gpgargv [ i++ ] = "--detach-sign";
	    break;
	  default:
	    callback ( GPAPA_ACTION_ERROR, "invalid signature type", calldata );
	    return;
	}
      gpgargv [ i++ ] = "-u";
      full_keyID = xstrcat2 ( "0x", keyID );
      gpgargv [ i++ ] = full_keyID;
      if ( Armor == GPAPA_ARMOR )
	gpgargv [ i++ ] = "--armor";
      if ( targetFileID != NULL )
	{
          gpgargv [ i++ ] = "-o";
          gpgargv [ i++ ] = targetFileID;
	}
      gpgargv [ i++ ] = "--yes";  /* overwrite the file */
      gpgargv [ i++ ] = file -> identifier;
      gpgargv [ i ] = NULL;
      gpapa_call_gnupg
	(
	  gpgargv, TRUE, NULL, PassPhrase,
	  gpapa_linecallback_dummy, NULL,
	  callback, calldata
	);
      free ( full_keyID );
    }
} /* gpapa_file_sign */

void gpapa_file_encrypt (
  GpapaFile *file, gchar *targetFileID, GList *rcptKeyIDs, GpapaArmor Armor,
  GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing file name", calldata );
  else if ( rcptKeyIDs == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing public key ID(s) for encrypting", calldata );
  else
    {
      GList *R;
      int i = 0, l = g_list_length ( rcptKeyIDs );
      char **gpgargv = xmalloc ( ( 7 + 2 * l ) * sizeof ( char * ) );
      R = rcptKeyIDs;
      while ( R )
        {
          gpgargv [ i++ ] = "-r";
          gpgargv [ i++ ] = xstrcat2 ( "0x", (char *) R -> data );
          R = g_list_next ( R );
        }
      gpgargv [ i++ ] = "--encrypt";
      if ( Armor == GPAPA_ARMOR )
	gpgargv [ i++ ] = "--armor";
      if ( targetFileID != NULL )
	{
          gpgargv [ i++ ] = "-o";
          gpgargv [ i++ ] = targetFileID;
	}
      gpgargv [ i++ ] = "--yes";  /* overwrite the file */
      gpgargv [ i++ ] = file -> identifier;
      gpgargv [ i ] = NULL;
      gpapa_call_gnupg
	(
	  gpgargv, TRUE, NULL, NULL,
	  gpapa_linecallback_dummy, NULL,
	  callback, calldata
	);
      for ( i = 1; i < 2 * l; i += 2 )
        free ( gpgargv [ i ] );
      free ( gpgargv );
    }
} /* gpapa_file_encrypt */

void gpapa_file_encrypt_and_sign (
  GpapaFile *file, gchar *targetFileID, GList *rcptKeyIDs,
  gchar *keyID, gchar *PassPhrase, GpapaSignType SignType, GpapaArmor Armor,
  GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing file name", calldata );
  else if ( rcptKeyIDs == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing public key ID(s) for encrypting", calldata );
  else
    {
      GList *R;
      int i = 0, l = g_list_length ( rcptKeyIDs );
      char *full_keyID;
      char **gpgargv = xmalloc ( ( 10 + 2 * l ) * sizeof ( char * ) );
      R = rcptKeyIDs;
      while ( R )
        {
          gpgargv [ i++ ] = "-r";
          gpgargv [ i++ ] = xstrcat2 ( "0x", (char *) R -> data );
          R = g_list_next ( R );
        }
      gpgargv [ i++ ] = "-u";
      full_keyID = xstrcat2 ( "0x", keyID );
      gpgargv [ i++ ] = full_keyID;
      if ( Armor == GPAPA_ARMOR )
	gpgargv [ i++ ] = "--armor";
      gpgargv [ i++ ] = "--encrypt";
      gpgargv [ i++ ] = "--sign";
      if ( Armor == GPAPA_ARMOR )
	gpgargv [ i++ ] = "--armor";
      if ( targetFileID != NULL )
	{
          gpgargv [ i++ ] = "-o";
          gpgargv [ i++ ] = targetFileID;
	}
      gpgargv [ i++ ] = "--yes";  /* overwrite the file */
      gpgargv [ i++ ] = file -> identifier;
      gpgargv [ i ] = NULL;
      gpapa_call_gnupg
	(
	  gpgargv, TRUE, NULL, PassPhrase,
	  gpapa_linecallback_dummy, NULL,
	  callback, calldata
	);
      for ( i = 1; i < 2 * l; i += 2 )
        free ( gpgargv [ i ] );
      free ( gpgargv );
      free ( full_keyID );
    }
} /* gpapa_file_encrypt_and_sign */

void gpapa_file_protect (
  GpapaFile *file, gchar *targetFileID, gchar *PassPhrase, GpapaArmor Armor,
  GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing file name", calldata );
  else if ( PassPhrase == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing passphrase for symmetric encrypting", calldata );
  else
    {
      char *gpgargv [ 7 ];
      int i = 0;
      gpgargv [ i++ ] = "--symmetric";
      if ( Armor == GPAPA_ARMOR )
	gpgargv [ i++ ] = "--armor";
      if ( targetFileID != NULL )
	{
          gpgargv [ i++ ] = "-o";
          gpgargv [ i++ ] = targetFileID;
	}
      gpgargv [ i++ ] = "--yes";  /* overwrite the file */
      gpgargv [ i++ ] = file -> identifier;
      gpgargv [ i ] = NULL;
      gpapa_call_gnupg
	(
	  gpgargv, TRUE, NULL, PassPhrase,
	  gpapa_linecallback_dummy, NULL,
	  callback, calldata
	);
    }
} /* gpapa_file_protect */

void gpapa_file_decrypt (
  GpapaFile *file, gchar *targetFileID, gchar *PassPhrase,
  GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing file name", calldata );
  else if ( PassPhrase == NULL )
    callback ( GPAPA_ACTION_ERROR, "missing passphrase for symmetric encrypting", calldata );
  else
    {
      char *gpgargv [ 6 ];
      int i = 0;
      gpgargv [ i++ ] = "--decrypt";
      if ( targetFileID != NULL )
	{
          gpgargv [ i++ ] = "-o";
          gpgargv [ i++ ] = targetFileID;
	}
      gpgargv [ i++ ] = "--yes";  /* overwrite the file */
      gpgargv [ i++ ] = file -> identifier;
      gpgargv [ i ] = NULL;
      gpapa_call_gnupg
	(
	  gpgargv, TRUE, NULL, PassPhrase,
	  gpapa_linecallback_dummy, NULL,
	  callback, calldata
	);
    }
} /* gpapa_file_protect */

void gpapa_file_release (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
  free ( file -> identifier );
  free ( file -> name );
  if ( file -> sigs )
    g_list_free ( file -> sigs );
  free ( file );
} /* gpapa_file_release */
