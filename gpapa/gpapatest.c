/* gpapatest.c	-  The GNU Privacy Assistant Pipe Access  -  test program
 *	  Copyright (C) 2000 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <glib.h>
#include "gpapa.h"

char *calldata;

void callback (
  GpapaAction action, gpointer actiondata, gpointer calldata
) {
  fprintf ( stderr, "%s: %s\n", (char *) calldata, (char *) actiondata );
} /* callback */

void linecallback (
  gchar *line, gpointer data
) {
  printf ( "---> %s <---%s\n", line, (gchar *) data );
} /* linecallback */

void versiontest ( void )
{
  char *gpgargv [ 2 ];
  gpgargv [ 0 ] = "--version";
  gpgargv [ 1 ] = NULL;
  gpapa_call_gnupg ( gpgargv, TRUE, NULL, linecallback, "pruzzel", callback, NULL );
}

int main ( int argc, char **argv )
{
  int i, seccount, pubcount;
  GpapaSecretKey *S;
  GpapaPublicKey *P;
  calldata = argv [ 0 ];
  seccount = gpapa_get_secret_key_count ( callback, calldata );
  printf ( "Secret keys: %d\n", seccount );
  printf ( "\n" );
  for ( i = MAX ( 0, seccount - 2 ); i < seccount; i++ )
    {
      GDate *expiry_date;
      char buffer [ 256 ];
      S = gpapa_get_secret_key_by_index ( i, callback, calldata );
      printf ( "Secret key #%d: %s\n", i,
	       gpapa_key_get_name ( GPAPA_KEY ( S ), callback, calldata ) );
      expiry_date = gpapa_key_get_expiry_date ( GPAPA_KEY ( S ), callback, calldata );
      if ( expiry_date )
	g_date_strftime ( buffer, 256, "%d.%m.%Y", expiry_date );
      else
	strcpy ( buffer, "never" );
      printf ( "Expires: %s\n", buffer );
      printf ( "\n" );
      gpapa_release_secret_key ( S, callback, calldata );
    }
  S = gpapa_get_secret_key_by_ID ( "983465DB21439422", callback, calldata);
  if ( S != NULL )
    {
      GpapaFile *F;
      gchar *PassPhrase;
      printf ( "Secret key 983465DB21439422: %s\n",
	       gpapa_key_get_name ( GPAPA_KEY ( S ), callback, calldata ) );
      gpapa_release_secret_key ( S, callback, calldata );
      printf ( "\n" );
      PassPhrase = getpass ( "Please enter passphrase: " );
      printf ( "Signing file `test.txt' ... " );
      F = gpapa_file_new ( "test.txt", callback, calldata );
      gpapa_file_sign ( F, NULL, "983465DB21439422", PassPhrase,
			GPAPA_SIGN_CLEAR, GPAPA_ARMOR,
			callback, calldata );
      printf ( "done.\n\n" );
    }
  else
    printf ( "Secret key 983465DB21439422: not available\n\n" );
  pubcount = gpapa_get_public_key_count ( callback, calldata );
  printf ( "Public keys: %d\n", pubcount );
  printf ( "\n" );
  for ( i = MAX ( 0, pubcount - 2 ); i < pubcount; i++ )
    {
      GDate *expiry_date;
      char buffer [ 256 ];
      P = gpapa_get_public_key_by_index ( i, callback, calldata );
      printf ( "Public key #%d: %s\n", i,
	       gpapa_key_get_name ( GPAPA_KEY ( P ), callback, calldata ) );
      printf ( "Key trust: %d\n",
	       gpapa_public_key_get_keytrust ( P, callback, calldata ) );
      printf ( "Owner trust: %d\n",
	       gpapa_public_key_get_ownertrust ( P, callback, calldata ) );
      printf ( "Fingerprint: %s\n",
	       gpapa_public_key_get_fingerprint ( P, callback, calldata ) );
      expiry_date = gpapa_key_get_expiry_date ( GPAPA_KEY ( P ), callback, calldata );
      if ( expiry_date )
	g_date_strftime ( buffer, 256, "%d.%m.%Y", expiry_date );
      else
	strcpy ( buffer, "never" );
      printf ( "Expires: %s\n", buffer );
      printf ( "\n" );
      gpapa_release_public_key ( P, callback, calldata );
    }
  P = gpapa_get_public_key_by_ID ( "983465DB21439422", callback, calldata );
  if ( P != NULL )
    {
      GList *g;
      printf ( "Public key 983465DB21439422: %s\n",
	       gpapa_key_get_name ( GPAPA_KEY ( P ), callback, calldata ) );
      printf ( "Signatures:\n" );
      g = gpapa_public_key_get_sigs ( P, callback, calldata );
      while ( g )
	{
	  GpapaSignature *sig = g -> data;
	  char *validity;
	  if ( sig -> validity == GPAPA_SIG_UNKNOWN )
	    validity = "unknown";
	  else if ( sig -> validity == GPAPA_SIG_VALID )
	    validity = "valid";
	  else if ( sig -> validity == GPAPA_SIG_INVALID )
	    validity = "invalid";
	  else
	    validity = "BROKEN";
	  printf ( "0x%s %s (%s)\n",
		   gpapa_signature_get_identifier ( sig, callback, calldata ),
		   sig -> UserID,
		   validity );
	  g = g_list_next ( g );
	}
      gpapa_release_public_key ( P, callback, calldata );
    }
  else
    printf ( "Public key 983465DB21439422: not available\n" );
  return ( 0 );
} /* main */
