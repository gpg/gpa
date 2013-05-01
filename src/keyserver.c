/* keyserver.c -  keyserver handling
 *	Copyright (C) 2001 g10 Code GmbH
 *
 * This file is part of GPA.
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

#include <config.h>

#include <glib.h>
#if GLIB_CHECK_VERSION (2, 6, 0)
#include <glib/gstdio.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "keyserver.h"

#define DIM(v)		     (sizeof(v)/sizeof((v)[0]))

typedef struct server_name_s *ServerName;
struct server_name_s
{
  ServerName next;
  int selected;
  char name[1];
};


static ServerName serverlist = NULL;

static ServerName
get_server (ServerName list, const char *name)
{
  ServerName x;

  for (x=list; x; x=x->next)
    {
      if (!strcasecmp (x->name, name) )
        return x;
    }

  return NULL;
}


static ServerName
add_server (ServerName *rlist, const char *name)
{
  ServerName x;

  if (rlist && (x=get_server (*rlist, name)) )
    return x; /* already in list */

  x = g_malloc (sizeof *x + strlen (name) );
  strcpy (x->name, name);
  x->selected = 0;
  x->next = *rlist;
  *rlist = x;

  return x;
}

static void
release_server_list (ServerName list)
{
  ServerName x, x2;

  for (x=list; x; x=x2)
    {
      x2 = x->next;
      g_free (x);
    }
}


static int
read_list (const gchar *fname)
{
  FILE *fp;
  char line[256], *p;
  int lnr = 0;
  const char *err = NULL;
  ServerName list = NULL;

  if (!fname)
    return -1;

#if GLIB_CHECK_VERSION (2, 6, 0)
  fp = g_fopen (fname, "r");
#else
  fp = fopen (fname, "r");
#endif
  if (!fp)
    {
/*
      fprintf (stderr, "can't open `%s': %s\n", fname, strerror (errno) );
      fflush (stderr);
 */
      return -1;
    }

  while ( fgets( line, DIM(line)-1, fp ) )
    {
      lnr++;
      if ( *line && line[strlen(line)-1] != '\n' )
        {
          err = "line too long";
          break;
	}


      g_strstrip (line);
      if( !*line || *line == '#' )
        continue; /* comment or empty line */

      for( p=line; *p; p++ )
        {
          if (isspace (*p))
            {
              err = "syntax error (more than one word)";
              break;
            }
        }
      add_server (&list, line);
    }

  if (err)
    fprintf (stderr, "%s:%d: %s\n", fname, lnr, err);
  else if (ferror (fp))
    {
      fprintf (stderr, "%s:%d: read error: %s\n",
	       fname, lnr, strerror (errno));
      err = "";
    }
  fflush (stderr);
  fclose (fp);
  if (err)
    {
      release_server_list (list);
      return -1;
    }

  return 0;
}

/*
 * Read a list of servers from the configuration directory with the name
 * CONFNAME.  Switch to this list if everything is fine.
 * Returns: 0 = okay.
 */
int
keyserver_read_list (const gchar *confname)
{
  int rc;

  rc = read_list (confname);

  if (!serverlist)
    { /* no entries in list - use default values */
      add_server (&serverlist, "hkp://keys.gnupg.net");
      add_server (&serverlist, "hkp://zimmermann.mayfirst.org");
      add_server (&serverlist, "hkp://minsky.surfnet.nl");
      add_server (&serverlist, "hkp://pks.gpg.cz");
      add_server (&serverlist, "hkp://pgp.cns.ualberta.ca");
      add_server (&serverlist, "hkp://keyserver.ubuntu.com");
      add_server (&serverlist, "hkp://keyserver.pramberger.at");

      add_server (&serverlist, "http://gpg-keyserver.de");
      add_server (&serverlist, "http://keyserver.pramberger.at");
    }

  return rc;
}

GList *
keyserver_get_as_glist (void)
{
  ServerName x;
  GList *list = NULL;

  for (x=serverlist; x; x = x->next)
    list = g_list_prepend (list, g_strdup (x->name));

  return list;
}





