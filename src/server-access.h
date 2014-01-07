/* server-access.h - The GNU Privacy Assistant
 *      Copyright (C) 2002, Miguel Coca.
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* A set of functions to access the gpg keyserver helper programs.
 */

#ifndef SERVER_ACCESS_H
#define SERVER_ACCESS_H
#ifdef ENABLE_KEYSERVER_SUPPORT

#include <gtk/gtk.h>
#include <gpgme.h>
#include "gpa.h"

/* Send the keys in NULL terminated array KEYS to the keyserver SERVER.
 * The PARENT window is used as parent for any dialog the function displays.
 */
gboolean server_send_keys (const gchar *server, const gchar *keyid,
		       gpgme_data_t data, GtkWidget *parent);

gboolean server_get_key (const gchar *server, const gchar *keyid,
                         gpgme_data_t *data, GtkWidget *parent);

#endif /*ENABLE_KEYSERVER_SUPPORT*/
#endif /*SERVER_ACCESS_H*/
