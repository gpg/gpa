/* hidewnd.c -  MS-Windows console hiding (W98, ME)
 *	Copyright (C) 2001 Timo Schulz
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

/* This module is only used in this environment */
#if defined(__MINGW32__) || defined(__CYGWIN__)
#include <stdio.h>

#include <windows.h>

static HWND console_window = NULL;

static BOOL CALLBACK
enum_parent_windows( HWND hwnd, LPARAM lparam )
{
  char wndclass[ 512 ];
  char wndtext[ 512 ];
  
  GetWindowText( hwnd, wndtext, sizeof( wndtext ) );
  GetClassName( hwnd, wndclass, sizeof( wndclass ) );
  
  if ( (!strcmp( wndclass, "tty" ) && strstr( wndtext, "gpa" ))
       || (!strcmp ( wndclass, "ConsoleWindowClass" )
           && (strstr (wndtext, "GPA") || strstr (wndtext, "gpa.exe"))))
    {
      console_window = hwnd;
      return FALSE;
    }
  
  return TRUE;
}

int
hide_gpa_console_window( void )
{
	EnumWindows( enum_parent_windows, 0 );
	if ( console_window )
		ShowWindow( console_window, SW_HIDE );
	return 0;
}

#endif /* __MINGW32__ */
