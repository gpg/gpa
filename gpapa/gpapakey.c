/* gpapakey.c - The GNU Privacy Assistant Pipe Access - abstract key object
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

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "gpapa.h"

GpapaKey *
gpapa_key_new (char *keyID, GpapaCallbackFunc callback, gpointer calldata)
{
  GpapaKey *key = (GpapaKey *) xmalloc (sizeof (GpapaKey));
  memset (key, 0, sizeof (GpapaKey));
  key->KeyID = xstrdup (keyID);
  return (key);
}

char *
gpapa_key_get_identifier (GpapaKey *key, GpapaCallbackFunc callback,
			  gpointer calldata)
{
  if (key == NULL)
    return (NULL);
  else
    return (key->KeyID);
}

char *
gpapa_key_get_name (GpapaKey *key, GpapaCallbackFunc callback,
		    gpointer calldata)
{
  if (key == NULL)
    return (NULL);
  else
    return (key->UserID);
}

GDate *
gpapa_key_get_expiry_date (GpapaKey *key, GpapaCallbackFunc callback,
			   gpointer calldata)
{
  if (key == NULL)
    return (NULL);
  else
    return (key->ExpirationDate);
}

GDate *
gpapa_key_get_creation_date (GpapaKey *key, GpapaCallbackFunc callback,
			     gpointer calldata)
{
  if (key == NULL)
    return (NULL);
  else
    return (key->CreationDate);
}

void
gpapa_key_set_expiry_date (GpapaKey *key, GDate *date, char *passphrase, 
			   GpapaCallbackFunc callback, gpointer calldata)
{
  if (key)
    {
      char *gpgargv[3];
      char *commands;
      char *commands_sprintf_str;
      if (date)
	{
	  commands_sprintf_str = "expire\n%04u-%02u-%02u\nsave\n";
	  commands = g_strdup_printf (commands_sprintf_str,
                                      date->year, date->month, date->day);
	}
      else
        commands = "expire\n0\nsave\n";
      gpgargv[0] = "--edit-key";
      gpgargv[1] = key->KeyID;
      gpgargv[2] = NULL; 
      gpapa_call_gnupg (gpgargv, TRUE, commands, passphrase,
                        NULL, NULL, callback, calldata); 
      if (key->ExpirationDate)
        g_date_free (key->ExpirationDate);
      if (date)
        {
          g_free (commands);
          key->ExpirationDate = g_date_new_dmy (date->day, date->month, date->year);
        }
      else
        key->ExpirationDate = NULL;
    }
}

void
gpapa_key_set_expiry_time (GpapaKey *key, gint number, char unit,
			   GpapaCallbackFunc callback, gpointer calldata)
{
  if (key)
    printf ("Setting expiry time of key 0x%s to %d %c.\n",
	    key->KeyID, number, unit);
}

void
gpapa_key_release (GpapaKey *key)
{
  if (key != NULL)
    {
      free (key->KeyID);
      free (key->LocalID);
      free (key->UserID);
      g_list_free (key->uids);
      g_list_free (key->subs);
      if (key->CreationDate != NULL)
	g_date_free (key->CreationDate);
      if (key->ExpirationDate != NULL)
	g_date_free (key->ExpirationDate);
      free (key);
    }
}
