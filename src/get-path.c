/* get-path.c - Find a system path.
   Copyright (C) 2000-2002 G-N-U GmbH.
   Copyright (C) 2005, 2008, 2013 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GPA; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>
#include <string.h>
#include <gpgme.h>

#include "gpa.h"
#include "get-path.h"


#ifdef G_OS_WIN32

#include <unistd.h>

#include <windows.h>

#include "w32reg.h"

#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a
#endif
#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x001c
#endif
#ifndef CSIDL_FLAG_CREATE
#define CSIDL_FLAG_CREATE 0x8000
#endif

#define RTLD_LAZY 0

#define DIM(v)	(sizeof (v) / sizeof ((v)[0]))


const char *
w32_strerror (int w32_errno)
{
  static char strerr[256];
  int ec = (int) GetLastError ();

  if (w32_errno == 0)
    w32_errno = ec;
  FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, NULL, w32_errno,
                 MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                 strerr, DIM (strerr) - 1, NULL);
  return strerr;
}


static __inline__ void *
dlopen (const char *name, int flag)
{
  void *hd = LoadLibrary (name);
  return hd;
}


static __inline__ void *
dlsym (void *hd, const char *sym)
{
  if (hd && sym)
    {
      void *fnc = GetProcAddress (hd, sym);
      if (! fnc)
        return NULL;
      return fnc;
    }
  return NULL;
}


static __inline__ const char *
dlerror (void)
{
  return w32_strerror (0);
}


static __inline__ int
dlclose (void *hd)
{
  if (hd)
    {
      FreeLibrary (hd);
      return 0;
    }
  return -1;
}


static HRESULT
w32_shgetfolderpath (HWND a, int b, HANDLE c, DWORD d, LPSTR e)
{
  static int initialized;
  static HRESULT (WINAPI *func) (HWND, int, HANDLE, DWORD, LPSTR);

  if (! initialized)
    {
      static char *dllnames[] = { "shell32.dll", "shfolder.dll", NULL };
      void *handle;
      int i;

      initialized = 1;

      for (i = 0, handle = NULL; ! handle && dllnames[i]; i++)
        {
          handle = dlopen (dllnames[i], RTLD_LAZY);
          if (handle)
            {
              func = dlsym (handle, "SHGetFolderPathA");
              if (! func)
                {
                  dlclose (handle);
                  handle = NULL;
                }
            }
        }
    }

  if (func)
    return func (a, b, c, d, e);
  else
    return -1;
}

#endif	/* G_OS_WIN32 */



/* Get the path to the default home directory.  */
gchar *
default_homedir (void)
{
  const char *s;
  gchar *dir = NULL;

  s = gpgme_get_dirinfo ("homedir");
  if (s)
    return g_strdup (s);

  /* No gpgconf installed.  That is we are using GnuPG-1.  */

  /* g_getenv returns string in filename encoding.  */
  dir = (gchar *) g_getenv ("GNUPGHOME");
  if (dir && dir[0])
    dir = g_strdup (dir);

#ifdef G_OS_WIN32

  if (! dir)
    {
      dir = read_w32_registry_string (NULL, "Software\\GNU\\GnuPG", "HomeDir");
      if (dir && ! dir[0])
	{
	  g_free (dir);
	  dir = NULL;
	}
    }

  if (! dir)
    {
      char path[MAX_PATH];

      /* It might be better to use LOCAL_APPDATA because this is
         defined as "non roaming" and thus more likely to be kept
         locally.  For private keys this is desired.  However, given
         that many users copy private keys anyway forth and back,
         using a system roaming serives might be better than to let
         them do it manually.  A security conscious user will anyway
         use the registry entry to have better control.  */
      if (w32_shgetfolderpath (NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
                               NULL, 0, path) >= 0)
        {
	  dir = g_build_filename (path, "gnupg", NULL);

          /* Try to create the directory if it does not yet exists.
             NOTE: This relies on the fact that the homedir is a
             sub-directory of an existing directory (APPDATA).  */
          if (access (dir, F_OK))
            CreateDirectory (dir, NULL);
        }
    }

  if (!dir)
    dir = g_strdup ("C:\\gnupg");

#else

  if (! dir)
    {
      const gchar *home;

      home = g_getenv ("HOME");
      dir = g_build_filename (home ? home : "/", ".gnupg", NULL);
    }

#endif	/* ! G_OS_WIN32 */

  return dir;
}
