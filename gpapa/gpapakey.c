/* gpapakey.c  -  The GNU Privacy Assistant Pipe Access
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

GpapaKey *gpapa_key_new (
  gchar *keyID, GpapaCallbackFunc callback, gpointer calldata
) {
  GpapaKey *key = (GpapaKey *) xmalloc ( sizeof ( GpapaKey ) );
  memset ( key, 0, sizeof ( GpapaKey ) );
  key -> KeyID = xstrdup ( keyID );
  return ( key );
} /* gpapa_key_new */

gchar *gpapa_key_get_identifier (
  GpapaKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key == NULL )
    return ( NULL );
  else
    return ( key -> KeyID );
} /* gpapa_key_get_identifier */

gchar *gpapa_key_get_name (
  GpapaKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key == NULL )
    return ( NULL );
  else
    return ( key -> UserID );
} /* gpapa_key_get_name */

GDate *gpapa_key_get_expiry_date (
  GpapaKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key == NULL )
    return ( NULL );
  else
    return ( key -> ExpirationDate );
} /* gpapa_key_get_expiry_date */

void gpapa_key_set_expiry_date (
  GpapaKey *key, GDate *date,
  GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key )
    printf ( "Setting expiry date of key 0x%s.\n", key -> KeyID );
} /* gpapa_key_set_expiry_date */

void gpapa_key_set_expiry_time (
  GpapaKey *key, gint number, gchar unit,
  GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key )
    printf ( "Setting expiry time of key 0x%s to %d %c.\n",
             key -> KeyID, number, unit );
} /* gpapa_key_set_expiry_time */

void gpapa_key_release (
  GpapaKey *key, GpapaCallbackFunc callback, gpointer calldata
) {
  if ( key != NULL )
    {
      free ( key -> KeyID );
      free ( key -> LocalID );
      free ( key -> UserID );
      g_list_free ( key -> uids );
      g_list_free ( key -> subs );
      if ( key -> CreationDate != NULL )
	g_date_free ( key -> CreationDate );
      if ( key -> ExpirationDate != NULL )
	g_date_free ( key -> ExpirationDate );
      free ( key );
    }
} /* gpapa_key_release */
