/* gpapaintern.h  -  header for internal functions
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

#ifndef __GPAPAINTERN_H__
#define __GPAPAINTERN_H__

#include <glib.h>
#include "gpapa.h"

typedef void (*GpapaLineCallbackFunc) (
  gchar *line, gpointer data, gboolean status
);

extern void gpapa_call_gnupg (
  gchar **argv, gboolean do_wait, gchar *passphrase,
  GpapaLineCallbackFunc linecallback, gpointer linedata,
  GpapaCallbackFunc callback, gpointer calldata
);

#endif /* __GPAPAINTERN_H__ */
