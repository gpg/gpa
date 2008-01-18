/* w32reg.c - MS-Windows Registry access
   Copyright (C) 1999, 2002, 2005 Free Software Foundation, Inc.
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <windows.h>


static HKEY
get_root_key (const char *root)
{
  HKEY root_key;
  
  if (!root)
    root_key = HKEY_CURRENT_USER;
  else if (!strcmp( root, "HKEY_CLASSES_ROOT"))
    root_key = HKEY_CLASSES_ROOT;
  else if (!strcmp( root, "HKEY_CURRENT_USER"))
    root_key = HKEY_CURRENT_USER;
  else if (!strcmp( root, "HKEY_LOCAL_MACHINE"))
    root_key = HKEY_LOCAL_MACHINE;
  else if (!strcmp( root, "HKEY_USERS"))
    root_key = HKEY_USERS;
  else if (!strcmp( root, "HKEY_PERFORMANCE_DATA"))
    root_key = HKEY_PERFORMANCE_DATA;
  else if (!strcmp( root, "HKEY_CURRENT_CONFIG"))
    root_key = HKEY_CURRENT_CONFIG;
  else
    return NULL;
  
  return root_key;
}


/* Return a string from the Win32 Registry or NULL in case of error.
   Caller must release the return value with g_free ().  A NULL for
   root is an alias for HKEY_CURRENT_USER with a fallback for
   HKEY_LOCAL_MACHINE.  */
char *
read_w32_registry_string (const char *root, const char *dir, const char *name)
{
  HKEY root_key;
  HKEY key_handle;
  DWORD n1;
  DWORD nbytes;
  DWORD type;
  char *result = NULL;

  root_key = get_root_key (root);
  if (! root_key)
    return NULL;

  if (RegOpenKeyEx (root_key, dir, 0, KEY_READ, &key_handle))
    {
      if (root)
	/* No need for a RegClose, so return directly.  */
	return NULL;
      
      /* It seems to be common practise to fall back to HKLM.  */
      if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, dir, 0, KEY_READ, &key_handle))
	/* Still no need for a RegClose, so return directly.  */
	return NULL;
    }

  nbytes = 1;
  if (RegQueryValueEx (key_handle, name, 0, NULL, NULL, &nbytes))
    {
      if (root)
	goto leave;

      /* Try to fallback to HKLM also for a missing value.  */
      RegCloseKey (key_handle);
      if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, dir, 0, KEY_READ, &key_handle))
	return NULL;

      if (RegQueryValueEx( key_handle, name, 0, NULL, NULL, &nbytes))
	goto leave;
    }
  n1 = nbytes + 1;
  result = g_malloc (n1);
  if (RegQueryValueEx (key_handle, name, 0, &type, result, &n1))
    {
      g_free (result);
      result = NULL;
      goto leave;
    }

  /* Make sure it is really a string.  */
  result[nbytes] = 0;
  if (type == REG_EXPAND_SZ && strchr (result, '%'))
    {
      char *tmp;
        
      n1 += 1000;
      tmp = g_malloc (n1 + 1);
      nbytes = ExpandEnvironmentStrings (result, tmp, n1);
      if (nbytes && nbytes > n1)
	{
	  free (tmp);
	  n1 = nbytes;
	  tmp = g_malloc (n1 + 1);
	  nbytes = ExpandEnvironmentStrings (result, tmp, n1);
	  if (nbytes && nbytes > n1)
	    {
	      /* Oops: truncated, better don't expand at all.  */
	      free (tmp);
	      goto leave;
            }
	  tmp[nbytes] = 0;
	  g_free (result);
	  result = tmp;
        }
      else if (nbytes)
	{
	  /* Okay, reduce the length.  */
	  tmp[nbytes] = 0;
	  free (result);
	  result = g_malloc (strlen (tmp) + 1);
	  strcpy (result, tmp);
	  g_free (tmp);
	}
      else
	{
	  /* Error - don't expand.  */
	  g_free (tmp);
        }
    }
  
 leave:
  RegCloseKey (key_handle);
  return result;
}


/* Write string VALUE into the W32 Registry ROOT, DIR and NAME.  If
   NAME does not exist, it is created.  */
int
write_w32_registry_string (const char *root, const char *dir, 
                           const char *name, const char *value)
{
  HKEY root_key;
  HKEY reg_key;
  
  root_key = get_root_key (root);
  if (! root_key)
    return -1;

  if (RegOpenKeyEx (root_key, dir, 0, KEY_WRITE, &reg_key))
    return -1;
	
  if (RegSetValueEx (reg_key, name, 0, REG_SZ, (BYTE *)value, 
		     strlen( value ) ) != ERROR_SUCCESS)
    {
      if (RegCreateKey (root_key, name, &reg_key))
	{
	  RegCloseKey (reg_key);
	  return -1;
        }

      if (RegSetValueEx (reg_key, name, 0, REG_SZ, (BYTE *) value,
			 strlen (value)))
	{
	  RegCloseKey(reg_key);
	  return -1;
        }
    }

  RegCloseKey (reg_key);
  
  return 0;
}
