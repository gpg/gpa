/* gpapapublickey.c  -	The GNU Privacy Assistant Pipe Access
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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "gpapa.h"

static void linecallback_fingerprint (
  gchar *line, gpointer data, gboolean status
) {
  PublicKeyData *d = data;
  if ( line && strncmp ( line, "fpr", 3 ) == 0 )
    {
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
	    d -> callback ( GPAPA_ACTION_ERROR,
			    "too many fields in output of `gpg --fingerprint'",
			    d -> calldata );
	}
      fields = i;
      if ( fields != 10 )
	d -> callback ( GPAPA_ACTION_ERROR,
			"invalid number of fields in output of `gpg --fingerprint'",
			d -> calldata );
      else if ( d -> key -> key -> algorithm == 1 )  /* RSA */
	{
	  gchar *fpraw = field [ 9 ];
	  gchar *fp = xmalloc ( strlen ( fpraw ) + 16 + 1 );
	  gchar *r = fpraw, *q = fp;
	  gint c = 0;
	  while ( *r )
	    {
	      *q++ = *r++;
	      c++;
	      if ( c < 32 )
		{
		  if ( c % 2 == 0 )
		    *q++ = ' ';
		  if ( c % 16 == 0 )
		    *q++ = ' ';
		}
	    }
	  *q = 0;
	  d -> key -> fingerprint = fp;
	}
      else
	{
	  gchar *fpraw = field [ 9 ];
	  gchar *fp = xmalloc ( strlen ( fpraw ) + 10 + 1 );
	  gchar *r = fpraw, *q = fp;
	  gint c = 0;
	  while ( *r )
	    {
	      *q++ = *r++;
	      c++;
	      if ( c < 40 )
		{
		  if ( c % 4 == 0 )
		    *q++ = ' ';
		  if ( c % 20 == 0 )
		    *q++ = ' ';
		}
	    }
	  *q = 0;
	  d -> key -> fingerprint = fp;
	}
    }
} /* linecallback_fingerprint */

gchar *gpapa_public_key_get_fingerprint (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key == NULL )
    return ( NULL );
  else
    {
      if ( key -> fingerprint == NULL && key -> key != NULL )
	{
	  PublicKeyData data = { key, callback, calldata };
	  char *key_id = xstrcat2 ( "0x", key -> key -> KeyID );
	  char *gpgargv [ 4 ];
	  gpgargv [ 0 ] = "--fingerprint";
	  gpgargv [ 1 ] = "--with-colons";
	  gpgargv [ 2 ] = key_id;
	  gpgargv [ 3 ] = NULL;
	  gpapa_call_gnupg (
	    gpgargv, TRUE, FALSE,
	    linecallback_fingerprint, &data,
	    callback, calldata
	  );
	  free ( key_id );
	}
      return ( key -> fingerprint );
    }
} /* gpapa_public_key_get_fingerprint */

GpapaKeytrust gpapa_public_key_get_keytrust (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key == NULL || key -> key == NULL )
    return ( GPAPA_KEYTRUST_UNKNOWN );
  else
    switch ( key -> key -> KeyTrust )
      {
	case 'n': return ( GPAPA_KEYTRUST_DISTRUST );
	case 'm': return ( GPAPA_KEYTRUST_MARGINALLY );
	case 'f':
	case 'u': return ( GPAPA_KEYTRUST_FULLY );
	default:  return ( GPAPA_KEYTRUST_UNKNOWN );
      }
} /* gpapa_public_key_get_keytrust */

GpapaOwnertrust gpapa_public_key_get_ownertrust (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key == NULL || key -> key == NULL )
    return ( GPAPA_OWNERTRUST_UNKNOWN );
  else
    switch ( key -> key -> OwnerTrust )
      {
	case 'n': return ( GPAPA_OWNERTRUST_DISTRUST );
	case 'm': return ( GPAPA_OWNERTRUST_MARGINALLY );
	case 'f':
	case 'u': return ( GPAPA_OWNERTRUST_FULLY );
	default:  return ( GPAPA_OWNERTRUST_UNKNOWN );
      }
} /* gpapa_public_key_get_ownertrust */

static GpapaSignature *extract_sig (
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
		   "too many fields in output of `gpg --check-sigs'",
		    calldata );
    }
  fields = i;
  if ( fields != 11 )
    {
      callback ( GPAPA_ACTION_ERROR,
		 "invalid number of fields in output of `gpg --check-sigs'",
		 calldata );
      return ( NULL );
    }
  else
    {
      GpapaSignature *sig = gpapa_signature_new ( field [ 4 ], callback, calldata );
      if ( strcmp ( field [ 1 ], "!" ) == 0 )
	sig -> validity = GPAPA_SIG_VALID;
      else if ( strcmp ( field [ 1 ], "-" ) == 0 )
	sig -> validity = GPAPA_SIG_INVALID;
      else
	sig -> validity = GPAPA_SIG_UNKNOWN;
      sig -> CreationDate = extract_date ( field [ 5 ] );
      sig -> UserID = field [ 9 ] [ 0 ] ? xstrdup ( field [ 9 ] ) : NULL;
      return ( sig );
    }
} /* extract_sig */

static void linecallback_get_signatures (
  gchar *line, gpointer data, gboolean status
) {
  PublicKeyData *d = data;
  if ( line && strncmp ( line, "sig", 3 ) == 0 )
    {
      GpapaSignature *sig = extract_sig ( line, d -> callback, d -> calldata );
      d -> key -> sigs = g_list_append ( d -> key -> sigs, sig );
    }
} /* linecallback_get_signatures */

GList *gpapa_public_key_get_signatures (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key == NULL )
    return ( NULL );
  else
    {
      if ( key -> sigs == NULL && key -> key != NULL )
	{
	  PublicKeyData data = { key, callback, calldata };
	  char *key_id = xstrcat2 ( "0x", key -> key -> KeyID );
	  char *gpgargv [ 4 ];
	  gpgargv [ 0 ] = "--check-sigs";
	  gpgargv [ 1 ] = "--with-colons";
	  gpgargv [ 2 ] = key_id;
	  gpgargv [ 3 ] = NULL;
	  gpapa_call_gnupg (
	    gpgargv, TRUE, FALSE,
	    linecallback_get_signatures, &data,
	    callback, calldata
	  );
	  free ( key_id );
	}
      return ( key -> sigs );
    }
} /* gpapa_public_key_get_signatures */

void gpapa_public_key_send_to_server (
  GpapaPublicKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
} /* gpapa_public_key_send_to_server */
