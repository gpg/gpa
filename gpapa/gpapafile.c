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

GpapaFileStatus gpapa_file_get_status (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
return ( GPAPA_FILE_CLEAR ); /*!!!*/
} /* gpapa_file_get_status */

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
	    gpgargv, TRUE, FALSE,
	    linecallback_get_signatures, &data,
	    callback, calldata
	  );
	}
      return ( file -> sigs );
    }
} /* gpapa_file_get_signatures */

void gpapa_file_release (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
  free ( file -> identifier );
  free ( file -> name );
  free ( file );
} /* gpapa_file_release */

static void linecallback_dummy ( char *line, gpointer data, gboolean status ) {
  /* empty */
} /* linecallback_dummy */

void gpapa_file_sign (
  GpapaFile *file, gchar *targetFileID, gchar *keyID, gchar *PassPhrase,
  GpapaSignType SignType, GpapaArmor Armor,
  GpapaCallbackFunc callback, gpointer calldata
) {
  if ( file == NULL )
    callback ( GPAPA_ACTION_ERROR, "trying to sign NULL file", calldata );
  else
    {
      gchar *full_keyID;
      char *gpgargv [ 8 ];
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
      if ( keyID == NULL )
	{
	  callback ( GPAPA_ACTION_ERROR, "missing key ID for signing", calldata );
	  return;
	}
      full_keyID = xstrcat2 ( "0x", keyID );
      gpgargv [ i++ ] = full_keyID;
      if ( Armor == GPAPA_ARMOR )
	gpgargv [ i++ ] = "--armor";
      if ( targetFileID != NULL )
	{
	  gpgargv [ i++ ] = "-o";
	  gpgargv [ i++ ] = targetFileID;
	}
      gpgargv [ i++ ] = file -> identifier;
      gpgargv [ i ] = NULL;
      gpapa_call_gnupg
	(
	  gpgargv, TRUE, PassPhrase,
	  linecallback_dummy, NULL,
	  callback, calldata
	);
      free ( full_keyID );
    }
} /* gpapa_file_sign */
