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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <glib.h>

#include "stringhelp.h"
#include "mischelp.h"

#include "xmalloc.h"
#include "options.h"
#include "keyserver.h"

typedef struct server_name_s *ServerName;
struct server_name_s
{
  ServerName next;
  int selected;
  char name[1];
};


static ServerName serverlist;

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

  x = xmalloc (sizeof *x + strlen (name) );
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
      free (x);
    }
}


static int
read_list (const char *confname)
{
  char *fname = make_filename (gpa_options.homedir, confname, NULL);
  FILE *fp;
  char line[256], *p;
  int lnr = 0;
  const char *err = NULL;
  ServerName list = NULL;
  
  fp = fopen (fname, "r");
  if (!fp)       
    {
      fprintf (stderr, "can't open `%s': %s\n", fname, strerror (errno) );
      fflush (stderr);
      free (fname);
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
      
      trim_spaces (line);
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
    free (fname);
    if (err)
      {
        release_server_list (list);
        return -1;
      }
    
    /* fine: switch to new list */
    {
      const char *current = keyserver_get_current (FALSE);
      release_server_list (serverlist);
      serverlist = list;
      keyserver_set_current (current);
    }

    return 0;
}

/*
 * Read a list of servers from the configuration directory with the name
 * CONFNAME.  Switch to this list if everything is fine.
 * Returns: 0 = okay.
 */
int
keyserver_read_list (const char *confname)
{
  int rc;

  rc = read_list (confname);
  if (!serverlist)
    { /* no entries in list - use default values */
      add_server (&serverlist, "blackhole.pca.dfn.de");
      add_server (&serverlist, "horowitz.surfnet.nl");
    }

  return rc;
}

/*
 * Set the standard server to use.  If this one is not yet in the list,
 * it will be added to the list.  With a NAME of NULL a selected server
 * will be unselected.
 */
void
keyserver_set_current (const char *name)
{
  ServerName x;

  for (x=serverlist; x; x = x->next)
    x->selected = 0;
  if (!name)
    return;
  x = add_server (&serverlist, name);
  x->selected = 1;
}

const char *
keyserver_get_current (gboolean non_null)
{
  ServerName x;

  for (x=serverlist; x; x = x->next)
    {
      if (x->selected)
        return x->name;
    }
  if (non_null)
    return "";
  else
    return NULL;
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





