/* utilis.c -  Utility functions for GPA.
 * Copyright (C) 2007 g10 Code GmbH
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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <glib.h>

#include "gpa.h"


/* We want our usual xmalloc function.  */
void *
xmalloc (size_t n)
{
  if (!n)
    n++;
  return g_malloc (n);
}


/* We want an xcalloc.  The similar g_new0 macro is not correclty
   implemented as it allows an integer overflow.  */
void *
xcalloc (size_t n, size_t m)
{
  size_t nbytes;
  void *p;

  nbytes = n * m;
  if ( m && nbytes / m != n)
    {
      errno = ENOMEM;
      p = NULL;
    }
  else
    p = g_malloc (nbytes);
  if (p)
    memset (p, 0, nbytes);
  else
    {
      g_error ("%s: failed to allocate %lu bytes", 
               G_STRLOC, (unsigned long)nbytes);
      abort (); /* Just in case g_error returns.  */
    }

  return p;
}


char *
xstrdup (const char *str)
{
  char *buf = g_malloc (strlen(str) + 1);
  strcpy (buf, str);
  return buf;
}



/* This function is a NOP for POSIX systems but required under Windows
   as the file handles as returned by OS calls (like CreateFile) are
   different from the libc file descriptors (like open). This function
   translates system file handles to libc file handles.  FOR_WRITE
   gives the direction of the handle.  */
int
translate_sys2libc_fd (assuan_fd_t fd, int for_write)
{
#ifdef HAVE_W32_SYSTEM
  int x;

  if (fd == ASSUAN_INVALID_FD)
    return -1;
  
  /* Note that _open_osfhandle is currently defined to take and return
     a long.  */
  x = _open_osfhandle ((long)fd, for_write ? 1 : 0);
  if (x == -1)
    g_debug ("failed to translate osfhandle %p\n", (void *) fd);
  return x;
#else /*!HAVE_W32_SYSTEM */
  return fd;
#endif
}
