/* gpapafile.h  -  The GNU Privacy Assistant Pipe Access
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
                  
#ifndef __GPAPAFILE_H__
#define __GPAPAFILE_H__

#include <glib.h>
#include "gpapa.h"

typedef struct {
  gchar *identifier;
  gchar *name;
  GList *sigs;
  unsigned status_flags;
} GpapaFile;

typedef enum {
  GPAPA_FILE_UNKNOWN,
  GPAPA_FILE_CLEAR,
  GPAPA_FILE_ENCRYPTED,
  GPAPA_FILE_PROTECTED,
  GPAPA_FILE_SIGNED,
  GPAPA_FILE_CLEARSIGNED,
  GPAPA_FILE_DETACHED_SIGNATURE
} GpapaFileStatus;

#define GPAPA_FILE_FIRST GPAPA_FILE_UNKNOWN
#define GPAPA_FILE_LAST GPAPA_FILE_DETACHED_SIGNATURE

/* For internal use.
 */
#define GPAPA_FILE_STATUS_LITERAL       0x0001
#define GPAPA_FILE_STATUS_PUBKEY        0x0002
#define GPAPA_FILE_STATUS_SYMKEY        0x0004
#define GPAPA_FILE_STATUS_COMPRESSED    0x0008
#define GPAPA_FILE_STATUS_SIGNATURE     0x0010
#define GPAPA_FILE_STATUS_NODATA        0x0020

typedef struct {
  GpapaFile *file;
  GpapaCallbackFunc callback;
  gpointer calldata;
} FileData;

extern GpapaFile *gpapa_file_new (
  gchar *fileID, GpapaCallbackFunc callback, gpointer calldata
);

extern gchar *gpapa_file_get_identifier (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
);

extern gchar *gpapa_file_get_name (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
);

extern GpapaFileStatus gpapa_file_get_status (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
);

extern gint gpapa_file_get_signature_count (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
);

extern GList *gpapa_file_get_signatures (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_file_sign (
  GpapaFile *file, gchar *targetFileID, gchar *keyID, gchar *PassPhrase,
  GpapaSignType SignType, GpapaArmor Armor,
  GpapaCallbackFunc callback, gpointer calldata
);

extern void gpapa_file_release (
  GpapaFile *file, GpapaCallbackFunc callback, gpointer calldata
);

#endif /* __GPAPAFILE_H__ */
