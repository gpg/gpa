/* gpapasignature.h - The GNU Privacy Assistant Pipe Access - signature object header
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

#ifndef __GPAPASIGNATURE_H__
#define __GPAPASIGNATURE_H__

#include <glib.h>
#include "gpapatypedefs.h"

typedef struct
{
  gchar *identifier;
  gchar *KeyID, *UserID;
  GDate *CreationDate;
  GpapaSigValidity validity;
}
GpapaSignature;

extern GpapaSignature *gpapa_signature_new (gchar *keyID,
					    GpapaCallbackFunc callback,
					    gpointer calldata);

extern gchar *gpapa_signature_get_identifier (GpapaSignature *signature,
					      GpapaCallbackFunc callback,
					      gpointer calldata);

extern gchar *gpapa_signature_get_name (GpapaSignature *signature,
					GpapaCallbackFunc callback,
					gpointer calldata);

extern GpapaSigValidity gpapa_signature_get_validity (GpapaSignature *signature,
						      GpapaCallbackFunc
						      callback,
						      gpointer calldata);

extern void gpapa_signature_release (GpapaSignature *signature);

#endif /* __GPAPASIGNATURE_H__ */
