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
#include <glib.h>
#include "gpapafile.h"

GpapaFile *gpapa_file_new (
  gchar *fileID, GpapaCallbackFunc callback, gpointer calldata
) {
/* objects */
  GpapaFile *fileNew;
/* commands */
  fileNew = (GpapaFile*) xmalloc ( sizeof ( GpapaFile ) );
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

GList *gpapa_file_get_signatures (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
return ( NULL ); /*!!!*/
} /* gpapa_file_get_signatures */

void gpapa_file_release (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
) {
  free ( file -> identifier );
  free ( file -> name );
  free ( file );
} /* gpapa_file_release */

static void linecallback_dummy ( char *line, gpointer data ) {
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
