/* gpapa.c  -  The GNU Privacy Assistant Pipe Access
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
#include <glib.h>

#include "gpapa.h"
#include "gpapafile.h"
#include "gpapakey.h"
#include "gpapapublickey.h"
#include "gpapasecretkey.h"
#include "gpapasignature.h"

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

gchar *global_keyServer;

/* Key management.
 */

static GList *PubRing = NULL, *SecRing = NULL;

GDate *extract_date ( gchar *buffer )
{
  char *p, *year, *month, *day;
  if ( buffer == NULL )
    return ( NULL );
  p = buffer;
  year = p;
  while ( *p >= '0' && *p <= '9' )
    p++;
  if ( *p )
    {
      *p = 0;
      p++;
    }
  month = p;
  while ( *p >= '0' && *p <= '9' )
    p++;
  if ( *p )
    {
      *p = 0;
      p++;
    }
  day = p;
  if ( year && *year && month && *month && day && *day )
    return ( g_date_new_dmy ( atoi ( day ), atoi ( month ), atoi ( year ) ) );
  else
    return ( NULL );
} /* extract_date */

static GpapaKey *extract_key (
  gchar *line,
  GpapaCallbackFunc callback, gpointer calldata
) {
  gchar *field [ GPAPA_MAX_GPG_KEY_FIELDS ];
  gchar *p = line;
  gint i = 0;
  gint fields;

  while ( *p )
    {
      field [ i ] = p;
      while ( *p && *p != ':' )
	p++;
      if ( *p == ':' )
	{
	  *p = 0;
	  p++;
	}
      i++;
      if ( i >= GPAPA_MAX_GPG_KEY_FIELDS )
	callback ( GPAPA_ACTION_ERROR,
		   "too many fields in GPG colon output",
		    calldata );
    }
  fields = i;
  if ( fields != 10 )
    {
      callback ( GPAPA_ACTION_ERROR,
		 "invalid number of fields in GPG colon output",
		 calldata );
      return ( NULL );
    }
  else
    {
      GpapaKey *key = gpapa_key_new ( field [ 7 ], callback, calldata );
      key -> KeyTrust = field [ 1 ] [ 0 ];
      key -> bits = atoi ( field [ 2 ] );
      key -> algorithm = atoi ( field [ 3 ] );
      key -> KeyID = xstrdup ( field [ 4 ] );
      key -> CreationDate = extract_date ( field [ 5 ] );
      key -> ExpirationDate = extract_date ( field [ 6 ] );
      key -> OwnerTrust = field [ 8 ] [ 0 ];
      key -> UserID = xstrdup ( field [ 9 ] );
      return ( key );
    }
} /* extract_key */

static void linecallback_refresh_pub (
  gchar *line, gpointer data, gboolean status
) {
  PublicKeyData *d = data;
  if ( line && strncmp ( line, "pub", 3 ) == 0 )
    {
      GpapaPublicKey *key = (GpapaPublicKey *) xmalloc ( sizeof ( GpapaPublicKey ) );
      memset ( key, 0, sizeof ( GpapaPublicKey ) );
      key -> key = extract_key ( line, d -> callback, d -> calldata );
      PubRing = g_list_append ( PubRing, key );
    }
} /* linecallback_refresh_pub */

static void refresh_public_keyring (
  GpapaCallbackFunc callback, gpointer calldata
) {
  PublicKeyData data = { NULL, callback, calldata };
  char *gpgargv [ 3 ];
  if ( PubRing != NULL )
    {
      g_list_free ( PubRing );
      PubRing = NULL;
    }
  gpgargv [ 0 ] = "--list-keys";
  gpgargv [ 1 ] = "--with-colons";
  gpgargv [ 2 ] = NULL;
  gpapa_call_gnupg (
    gpgargv, TRUE, NULL,
    linecallback_refresh_pub, &data,
    callback, calldata
  );
} /* refresh_public_keyring */

gint gpapa_get_public_key_count (
  GpapaCallbackFunc callback, gpointer calldata
) {
  refresh_public_keyring ( callback, calldata );
  return ( g_list_length ( PubRing ) );
} /* gpapa_get_public_key_count */

GpapaPublicKey *gpapa_get_public_key_by_index (
  gint idx, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( PubRing == NULL )
    refresh_public_keyring ( callback, calldata );
  return ( g_list_nth_data ( PubRing, idx ) );
} /* gpapa_get_public_key_by_index */

static void linecallback_id_pub (
  gchar *line, gpointer data, gboolean status
) {
  PublicKeyData *d = data;
  if ( line && strncmp ( line, "pub", 3 ) == 0 )
    {
      d -> key = (GpapaPublicKey *) xmalloc ( sizeof ( GpapaPublicKey ) );
      memset ( d -> key, 0, sizeof ( GpapaPublicKey ) );
      d -> key -> key = extract_key ( line, d -> callback, d -> calldata );
    }
} /* linecallback_id_pub */

GpapaPublicKey *gpapa_get_public_key_by_ID (
  gchar *keyID, GpapaCallbackFunc callback, gpointer calldata
) {
  PublicKeyData data = { NULL, callback, calldata };
  char *gpgargv [ 4 ];
  char *id = xstrcat2 ( "0x", keyID );
  gpgargv [ 0 ] = "--list-keys";
  gpgargv [ 1 ] = "--with-colons";
  gpgargv [ 2 ] = id;
  gpgargv [ 3 ] = NULL;
  gpapa_call_gnupg (
    gpgargv, TRUE, NULL,
    linecallback_id_pub, &data,
    callback, calldata
  );
  free ( id );
  return ( data.key );
} /* gpapa_get_public_key_by_ID */

/* This is intentionally a global function, not a method of
 * GpapaPublicKey.
 */
void gpapa_release_public_key (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  /* Do nothing.
   * Public keys will be released with the PubRing.
   */
} /* gpapa_release_public_key */

static void linecallback_refresh_sec (
  gchar *line, gpointer data, gboolean status
) {
  SecretKeyData *d = data;
  if ( line && strncmp ( line, "sec", 3 ) == 0 )
    {
      GpapaSecretKey *key = (GpapaSecretKey *) xmalloc ( sizeof ( GpapaSecretKey ) );
      memset ( key, 0, sizeof ( GpapaSecretKey ) );
      key -> key = extract_key ( line, d -> callback, d -> calldata );
      SecRing = g_list_append ( SecRing, key );
    }
} /* linecallback_refresh_sec */

static void refresh_secret_keyring (
  GpapaCallbackFunc callback, gpointer calldata
) {
  SecretKeyData data = { NULL, callback, calldata };
  char *gpgargv [ 3 ];
  if ( SecRing != NULL )
    {
      g_list_free ( SecRing );
      SecRing = NULL;
    }
  gpgargv [ 0 ] = "--list-secret-keys";
  gpgargv [ 1 ] = "--with-colons";
  gpgargv [ 2 ] = NULL;
  gpapa_call_gnupg (
    gpgargv, TRUE, NULL,
    linecallback_refresh_sec, &data,
    callback, calldata
  );
} /* refresh_secret_keyring */

gint gpapa_get_secret_key_count (
  GpapaCallbackFunc callback, gpointer calldata
) {
  refresh_secret_keyring ( callback, calldata );
  return ( g_list_length ( SecRing ) );
} /* gpapa_get_secret_key_count */

GpapaSecretKey *gpapa_get_secret_key_by_index (
  gint idx, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( SecRing == NULL )
    refresh_secret_keyring ( callback, calldata );
  return ( g_list_nth_data ( SecRing, idx ) );
} /* gpapa_get_secret_key_by_index */

static void linecallback_id_sec (
  gchar *line, gpointer data, gboolean status
) {
  SecretKeyData *d = data;
  if ( line && strncmp ( line, "sec", 3 ) == 0 )
    {
      d -> key = (GpapaSecretKey *) xmalloc ( sizeof ( GpapaSecretKey ) );
      memset ( d -> key, 0, sizeof ( GpapaSecretKey ) );
      d -> key -> key = extract_key ( line, d -> callback, d -> calldata );
    }
} /* linecallback_id_sec */

GpapaSecretKey *gpapa_get_secret_key_by_ID (
  gchar *keyID, GpapaCallbackFunc callback, gpointer calldata
) {
  SecretKeyData data = { NULL, callback, calldata };
  char *gpgargv [ 4 ];
  char *id = xstrcat2 ( "0x", keyID );
  gpgargv [ 0 ] = "--list-secret-keys";
  gpgargv [ 1 ] = "--with-colons";
  gpgargv [ 2 ] = id;
  gpgargv [ 3 ] = NULL;
  gpapa_call_gnupg (
    gpgargv, TRUE, NULL,
    linecallback_id_sec, &data,
    callback, calldata
  );
  free ( id );
  return ( data.key );
} /* gpapa_get_secret_key_by_ID */

void gpapa_release_secret_key (
  GpapaSecretKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  /* Do nothing.
   * Secret keys will be released with the SecRing.
   */
} /* gpapa_release_secret_key */

void gpapa_create_key_pair (
  GpapaAlgo anAlgo, gint aKeysize, long anExpiryDate,
  gchar *aUserID, gchar *anEmail, gchar *aComment,
  GpapaCallbackFunc callback, gpointer calldata
) {
} /* gpapa_create_key_pair */

/* Miscellaneous.
 */

void gpapa_init ( void )
{
  /* For the moment, do nothing.
   */
} /* gpapa_init */

void gpapa_fini ( void )
{
  if ( PubRing != NULL )
    {
      g_list_free ( PubRing );
      PubRing = NULL;
    }
  if ( SecRing != NULL )
    {
      g_list_free ( SecRing );
      SecRing = NULL;
    }
} /* gpapa_fini */

void gpapa_idle ( void )
{
  /* In the future, we will call a non-blocking select() and
   * waitpid() here to read data from an open pipe and see
   * whether gpg is still running.
   *
   * Right now, just do nothing.
   */
} /* gpapa_idle */