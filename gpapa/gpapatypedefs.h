/* gpapatypedefs.h  -  The GNU Privacy Assistant Pipe Access
 *      Copyright (C) 2000 Free Software Foundation, Inc.
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

#ifndef __GPAPATYPEDEFS_H__
#define __GPAPATYPEDEFS_H__
                  
typedef enum {
  GPAPA_ACTION_NONE,
  GPAPA_ACTION_ERROR,
  GPAPA_ACTION_PASSPHRASE,
  GPAPA_ACTION_ABORTED,
  GPAPA_ACTION_FINISHED
} GpapaAction;

#define GPAPA_ACTION_FIRST GPAPA_ACTION_NONE
#define GPAPA_ACTION_LAST GPAPA_ACTION_FINISHED

typedef enum {
  GPAPA_NO_ARMOR,
  GPAPA_ARMOR
} GpapaArmor;

#define GPAPA_ARMOR_FIRST GPAPA_NO_ARMOR
#define GPAPA_ARMOR_LAST GPAPA_ARMOR

typedef enum {
  GPAPA_SIGN_NORMAL,
  GPAPA_SIGN_CLEAR,
  GPAPA_SIGN_DETACH
} GpapaSignType;

#define GPAPA_SIGN_FIRST GPAPA_SIGN_NORMAL
#define GPAPA_SIGN_LAST GPAPA_SIGN_DETACH

typedef enum {
  GPAPA_KEY_SIGN_NORMAL,
  GPAPA_KEY_SIGN_LOCALLY,
} GpapaKeySignType;

#define GPAPA_KEY_SIGN_FIRST GPAPA_KEY_SIGN_NORMAL
#define GPAPA_KEY_SIGN_LAST GPAPA_KEY_SIGN_LOCALLY

typedef enum {
  GPAPA_SIG_UNKNOWN,
  GPAPA_SIG_INVALID,
  GPAPA_SIG_VALID
} GpapaSigValidity;

#define GPAPA_SIG_FIRST GPAPA_SIG_UNKNOWN
#define GPAPA_SIG_LAST GPAPA_SIG_VALID

typedef enum {
  GPAPA_ALGO_BOTH,
  GPAPA_ALGO_DSA,
  GPAPA_ALGO_ELG_BOTH,
  GPAPA_ALGO_ELG
} GpapaAlgo;

#define GPAPA_ALGO_FIRST GPAPA_ALGO_BOTH
#define GPAPA_ALGO_LAST GPAPA_ALGO_ELG

typedef void (*GpapaCallbackFunc) (
  GpapaAction action, gpointer actiondata, gpointer calldata
);

#endif /* __GPAPATYPEDEFS_H__ */
