/* gpapasignature.c  -	The GNU Privacy Assistant Pipe Access
 *	  Copyright (C) 2000 G-N-U GmbH.
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

GpapaSignature *
gpapa_signature_new (gchar * keyID, GpapaCallbackFunc callback,
		     gpointer calldata)
{
  GpapaSignature *sig = (GpapaSignature *) xmalloc (sizeof (GpapaSignature));
  memset (sig, 0, sizeof (GpapaSignature));
  sig->KeyID = xstrdup (keyID);
  return (sig);
} /* gpapa_signature_new */

gchar *
gpapa_signature_get_identifier (GpapaSignature * signature,
				GpapaCallbackFunc callback, gpointer calldata)
{
  if (signature == NULL)
    return (NULL);
  else
    return (signature->KeyID);
} /* gpapa_signature_get_identifier */

gchar *
gpapa_signature_get_name (GpapaSignature * signature,
			  GpapaCallbackFunc callback, gpointer calldata)
{
  if (signature == NULL)
    return (NULL);
  else
    return (signature->UserID);
} /* gpapa_signature_get_name */

GpapaSigValidity
gpapa_signature_get_validity (GpapaSignature * signature,
			      GpapaCallbackFunc callback, gpointer calldata)
{
  if (signature == NULL)
    return (GPAPA_SIG_UNKNOWN);
  else
    return (signature->validity);
} /* gpapa_signature_is_valid */

void
gpapa_signature_release (GpapaSignature * signature,
			 GpapaCallbackFunc callback, gpointer calldata)
{
  if (signature != NULL)
    {
      free (signature->KeyID);
      free (signature->UserID);
      if (signature->CreationDate != NULL)
	g_date_free (signature->CreationDate);
      free (signature);
    }
} /* gpapa_signature_release */
