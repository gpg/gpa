/* gpg-stuff.h - Code taken from GnuPG.
 * Copyright (C) 2008 g10 Code GmbH.
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GPG_STUFF_H
#define GPG_STUFF_H

#include "strlist.h"

struct keyserver_spec
{
  char *uri;
  char *scheme;
  char *auth;
  char *host;
  char *port;
  char *path;
  char *opaque;
  strlist_t options;
  struct
  {
    unsigned int direct_uri:1;
  } flags;
  struct keyserver_spec *next;
};
typedef struct keyserver_spec *keyserver_spec_t;


void free_keyserver_spec (struct keyserver_spec *keyserver);
keyserver_spec_t parse_keyserver_uri (const char *string,
                                      int require_scheme,
                                      const char *configname,
                                      unsigned int configlineno);



/* Linked list of methods to find a key.  */
struct akl
{
  enum
    {
      AKL_NODEFAULT,
      AKL_LOCAL,
      AKL_CERT, 
      AKL_PKA, 
      AKL_LDAP,
      AKL_KEYSERVER,
      AKL_SPEC
    } type;
  struct keyserver_spec *spec;
  struct akl *next;
};
typedef struct akl *akl_t;


void gpg_release_akl (akl_t akl);

/* Note: VALUE will be changed on return from the fucntion.  */
akl_t gpg_parse_auto_key_locate (char *value);



#endif /*GPG_STUFF_H*/
