/* gpapasecretkey.h  -  The GNU Privacy Assistant Pipe Access
 *        Copyright (C) 2000 Free Software Foundation, Inc.
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

#ifndef __GPAPASECRETKEY_H__
#define __GPAPASECRETKEY_H__

#include <glib.h>
#include "gpapatypedefs.h"

typedef struct {
  GpapaKey *key;
} GpapaSecretKey;

typedef enum {
  GPAPA_ALGO_BOTH,
  GPAPA_ALGO_DSA,
  GPAPA_ALGO_ELG_BOTH,
  GPAPA_ALGO_ELG
} GpapaAlgo;

#define GPAPA_ALGO_FIRST GPAPA_ALGO_BOTH
#define GPAPA_ALGO_LAST GPAPA_ALGO_ELG

extern void gpapa_secret_key_create_revocation (
  GpapaSecretKey *key, GpapaCallbackFunc callback, gpointer calldata
);

#endif /* __GPAPASECRETKEY_H__ */
