/* launch-gpa.c - Wrapper to start GPA under Windows.
 * Copyright (C) 2014 g10 Code GmbH
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

/* This wrapper merely starts gpa in detached mode.  Building gpa as
   console program has the advantage that debugging is easier and we
   get a nice help menu on the command line.  Tricks building as
   windows subsystem and using AllocConsole don't work well because
   the shell might have already created a console window before we get
   control.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static char *
build_commandline (char **argv)
{
  int i;
  int n = 0;
  char *buf;
  char *p;

  /* We have to quote some things because under Windows the program
     parses the commandline and does some unquoting.  We enclose the
     whole argument in double-quotes, and escape literal double-quotes
     as well as backslashes with a backslash.  We end up with a
     trailing space at the end of the line, but that is harmless.  */
  for (i = 0; argv[i]; i++)
    {
      p = argv[i];
      /* The leading double-quote.  */
      n++;
      while (*p)
	{
	  /* An extra one for each literal that must be escaped.  */
	  if (*p == '\\' || *p == '"')
	    n++;
	  n++;
	  p++;
	}
      /* The trailing double-quote and the delimiter.  */
      n += 2;
    }
  /* And a trailing zero.  */
  n++;

  buf = p = malloc (n);
  if (!buf)
    return NULL;
  for (i = 0; argv[i]; i++)
    {
      char *argvp = argv[i];

      *(p++) = '"';
      while (*argvp)
	{
	  if (*argvp == '\\' || *argvp == '"')
	    *(p++) = '\\';
	  *(p++) = *(argvp++);
	}
      *(p++) = '"';
      *(p++) = ' ';
    }
  *(p++) = 0;

  return buf;
}


int
main (int argc, char **argv)
{
  char me[256];
  int i;
  char pgm[MAX_PATH+100];
  char *p, *p0, *p1, *arg0;
  char *arg_string;
  SECURITY_ATTRIBUTES sec_attr;
  STARTUPINFOA si;
  int cr_flags;
  PROCESS_INFORMATION pi =
    {
      NULL,      /* returns process handle */
      0,         /* returns primary thread handle */
      0,         /* returns pid */
      0          /* returns tid */
    };

  p = argc?argv[0]:"?";
  p0 = strrchr (p, '/');
  p1 = strrchr (p, '\\');
  if (p0 && p1)
    p = p0 > p1? p0+1 : p1+1;
  else if (p0)
    p = p0+1;
  else if (p1)
    p = p1+1;

  for (i=0; *p && i < sizeof me - 1; i++)
    me[i] = *p++;
  me[i] = 0;
  p = strchr (me, '.');
  if (p)
    *p = 0;
  strlwr (me);

  if (!GetModuleFileNameA (NULL, pgm, sizeof (pgm) - 1))
    {
      fprintf (stderr, "%s: error getting my own name: rc=%d\n",
               me, (int)GetLastError ());
      return 2;
    }

  /* Remove the "launch-" part from the module name.  */
  p0 = strrchr (pgm, '\\');
  if (!p0)
    goto leave;
  p0++;
  arg0 = p0;
  if (strnicmp (p0, "launch-", 7))
    goto leave;
  for (p = p0+7; *p; p++)
    *p0++ = *p;
  *p0 = 0;

  /* Hack to output our own version along with the real file name
     before the actual, we require that the --version option is given
     twice.  Not very useful for a -mwindows program, though.  */
  if (argc > 2
      && !strcmp(argv[1], "--version")
      && !strcmp(argv[2], "--version"))
    {
      printf (stdout, "%s %s ;%s\n", me, PACKAGE_VERSION, pgm);
      fflush (stdout);
    }

  p = argv[0];
  argv[0] = arg0;
  arg_string = build_commandline (argv);
  argv[0] = p;
  if (!arg_string)
    {
      fprintf (stderr, "%s: error building command line\n", me);
      return 2;
    }

  memset (&sec_attr, 0, sizeof sec_attr);
  sec_attr.nLength = sizeof sec_attr;
  sec_attr.bInheritHandle = FALSE;

  memset (&si, 0, sizeof si);
  si.cb = sizeof (si);
  si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  si.hStdInput = INVALID_HANDLE_VALUE;
  si.hStdOutput = INVALID_HANDLE_VALUE;
  si.hStdError = INVALID_HANDLE_VALUE;

  cr_flags  = CREATE_DEFAULT_ERROR_MODE;
  cr_flags |= DETACHED_PROCESS;
  cr_flags |= GetPriorityClass (GetCurrentProcess ());
  if (!CreateProcessA (pgm,
		       arg_string,
		       NULL,          /* process security attributes */
		       NULL,          /* thread security attributes */
		       FALSE,         /* inherit handles */
		       cr_flags,      /* creation flags */
		       NULL,          /* environment */
		       NULL,          /* use current drive/directory */
		       &si,           /* startup information */
		       &pi))          /* returns process information */
    {
      fprintf (stderr, "%s: executing `%s' failed: rc=%d\n",
	       me, pgm, (int) GetLastError ());
      free (arg_string);
      return 2;
     }

  free (arg_string);
  return 0;

 leave:
  fprintf (stderr, "%s: internal error parsing my own name `%s'\n",
           me, pgm);
  return 2;
}
