/* gpapaintern.c  -  The GNU Privacy Assistant Pipe Access  -  internal routines
 *	  Copyright (C) 2000 Free Software Foundation, Inc.
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

#include <config.h>
#include <glib.h>
#include "gpapa.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef __MINGW32__
#include <windows.h>
#else
#include <sys/wait.h>
#endif
#include <fcntl.h>
#include "stringhelp.h"

gboolean
gpapa_line_begins_with (gchar * line, gchar * keyword)
{
  if (strncmp (line, keyword, strlen (keyword)) == 0)
    return (TRUE);
  else
    return (FALSE);
}				/* gpapa_line_begins_with */

void
gpapa_linecallback_dummy (char *line, gpointer data, gboolean status)
{
  /* empty */
}				/* gpapa_linecallback_dummy */

static gboolean
status_check (gchar * buffer,
	      GpapaCallbackFunc callback, gpointer calldata,
	      gchar * keyword, gchar * message)
{
  gchar *checkval = xstrcat2 ("[GNUPG:] ", keyword);
  gboolean result = FALSE;
  if (strncmp (buffer, checkval, strlen (checkval)) == 0)
    {
      result = TRUE;
      callback (GPAPA_ACTION_ERROR, message, calldata);
    }
  free (checkval);
  return (result);
}				/* status_check */




/* user_args must be a NULL-terminated array of argument strings.
 */
void
gpapa_call_gnupg (char **user_args, gboolean do_wait, gchar * commands,
		  gchar * passphrase, GpapaLineCallbackFunc linecallback,
		  gpointer linedata, GpapaCallbackFunc callback,
		  gpointer calldata)
{
#ifdef __MINGW32__
  /* Hmm, let's see how we can tackle this for the W32 API:
   *
   */

  SECURITY_ATTRIBUTES sec_attr;
  PROCESS_INFORMATION pi = { NULL,	/* returns process handle */
    0,				/* returns primary thread handle */
    0,				/* returns pid */
    0				/* returns tid */
  };
  STARTUPINFO si = { 0, NULL, NULL, NULL,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    NULL, NULL, NULL, NULL
  };
  char *envblock = NULL;
  int cr_flags = CREATE_DEFAULT_ERROR_MODE
    | GetPriorityClass (GetCurrentProcess ());
  int rc;
  HANDLE save_stdout;
  HANDLE outputfd[2], statusfd[2], inputfd[2];
  gboolean missing_passphrase = FALSE;
  char *arg_string = NULL;
  char *standard_args[] = {
    "--status-fd=2",
    "--no-options",
    "--batch",
    NULL
  };

  sec_attr.nLength = sizeof (sec_attr);
  sec_attr.bInheritHandle = FALSE;
  sec_attr.lpSecurityDescriptor = NULL;


  /* Open output pipe. */
  if (!CreatePipe (outputfd + 0, outputfd + 1, NULL, 0))
    {
      callback (GPAPA_ACTION_ERROR, "could not open output pipe", calldata);
      return;
    }

  /* Open status pipe. */
  if (!CreatePipe (statusfd + 0, statusfd + 1, NULL, 0))
    {
      callback (GPAPA_ACTION_ERROR, "could not open status pipe", calldata);
      CloseHandle (outputfd[0]);
      CloseHandle (outputfd[1]);
      return;
    }

  /* fixme: open passphrase pipe - Hmmm: Should we really use it?? */

  /* fixme: setup input pipe */


  /* Construct the command line. */
  fprintf (stderr, "+++ constructing commandline\n");
  fflush (stderr);
  {
    int i, n = 0;
    char *p;

    for (i = 0; standard_args[i]; i++)
      n += strlen (standard_args[i]) + 1;
    for (i = 0; user_args[i]; i++)
      n += strlen (user_args[i]) + 1;
    n += 5;			/* "gpg " */
    p = arg_string = xmalloc (n);
    p = stpcpy (p, "gpg");
    for (i = 0; standard_args[i]; i++)
      p = stpcpy (stpcpy (p, " "), standard_args[i]);
    for (i = 0; user_args[i]; i++)
      p = stpcpy (stpcpy (p, " "), user_args[i]);

  }

  fprintf (stderr, "+++ setting handles\n");
  fflush (stderr);
  si.cb = sizeof (si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
  si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
  si.hStdError = GetStdHandle (STD_ERROR_HANDLE);
  si.hStdOutput = outputfd[1];
  si.hStdError = statusfd[1];
  if (!SetHandleInformation (si.hStdOutput, HANDLE_FLAG_INHERIT,
			     HANDLE_FLAG_INHERIT))
    {
      fprintf (stderr, "** SHI 1 failed: ec=%d\n", (int) GetLastError ());
    }
  if (!SetHandleInformation (si.hStdError, HANDLE_FLAG_INHERIT,
			     HANDLE_FLAG_INHERIT))
    {
      fprintf (stderr, "** SHI 2 failed: ec=%d\n", (int) GetLastError ());
    }


  fputs ("** CreateProcess ...\n", stderr);
  fprintf (stderr, "** args=`%s'\n", arg_string);
  fflush (stderr);
  rc = CreateProcessA ("c:\\gnupg\\gpg.exe", arg_string, &sec_attr,	/* process security attributes */
		       &sec_attr,	/* thread security attributes */
		       TRUE,	/* inherit handles */
		       cr_flags,	/* creation flags */
		       envblock,	/* environment */
		       NULL,	/* use current drive/directory */
		       &si,	/* startup information */
		       &pi	/* returns process information */
    );

  if (!rc)
    {
      fprintf (stderr, "** CreateProcess failed: ec=%d\n",
	       (int) GetLastError ());
      fflush (stderr);
      callback (GPAPA_ACTION_ERROR, "CreateProcess failed", calldata);
      return;
    }

  CloseHandle (outputfd[1]);
  CloseHandle (statusfd[1]);

  fprintf (stderr, "** CreateProcess ready\n");
  fprintf (stderr, "**   hProcess=%p  hThread=%p\n", pi.hProcess, pi.hThread);
  fprintf (stderr, "**   dwProcessID=%d dwThreadId=%d\n",
	   (int) pi.dwProcessId, (int) pi.dwThreadId);
  fflush (stderr);


  fprintf (stderr, "** start waiting for this processs ...\n");
  fflush (stderr);

  SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_HIGHEST);

  if (do_wait)
    {
      int ready, status, retpid;
      size_t bufsize = 8192;
      size_t pendingsize = 0;
      char *buffer = (char *) xmalloc (bufsize);
      char *pendingbuffer = NULL;

      for (ready = 0; !ready;)
	{
	  HANDLE waitbuf[3] = { pi.hProcess, outputfd[0], statusfd[0] };
	  int nwait = 3;
	  int nread, ncount;

	  ncount = WaitForMultipleObjects (nwait, waitbuf, FALSE, 0);
	  if (ncount >= WAIT_OBJECT_0 && ncount < WAIT_OBJECT_0 + 2)
	    {
	      int check_again;
	      /* Hmm: I don't know how this should work: We always get
	       * an event for the outputfd but most of the time the pipe
	       * is empty.  I have never seen an event for the status_fd.
	       * The workaround is to check on any event for data on one
	       * of these pipes.
	       */
	      do
		{
		  DWORD navail;
		  check_again = 0;

		  /* check outputfd */
		  if (!PeekNamedPipe (outputfd[0], NULL, 0, NULL,
				      &navail, NULL))
		    {
		      fprintf (stderr, "** PeekPipe failed: ec=%d\n",
			       (int) GetLastError ());
		      navail = 0;
		    }
		  nread = 0;
		  if (navail && !ReadFile (outputfd[0], buffer, bufsize,
					   &nread, NULL))
		    {
		      fprintf (stderr, "** ReadFile failed: ec=%d\n",
			       (int) GetLastError ());
		      nread = 0;
		    }
		  while (nread > 0)
		    {		/* process this data */
		      int q = 0;

		      check_again = 1;
		      fprintf (stderr, "outputfd has %d bytes\n", nread);
		      while (q < nread && buffer[q] != '\n')
			q++;
		      if (q < nread)
			{
			  if (pendingsize > 0)
			    {
			      int newpendingsize = pendingsize + q + 1;
			      pendingbuffer = xrealloc (pendingbuffer,
							newpendingsize);
			      memcpy (pendingbuffer + pendingsize, buffer, q);
			      pendingbuffer[newpendingsize - 1] = 0;
			      linecallback (pendingbuffer, linedata, FALSE);
			      free (pendingbuffer);
			      pendingbuffer = NULL;
			      pendingsize = 0;
			    }
			  else
			    {
			      buffer[q] = 0;
			      linecallback (buffer, linedata, FALSE);
			    }
			  memmove (buffer, buffer + q + 1, nread - q - 1);
			  nread -= q + 1;
			}
		      else
			{
			  int newpendingsize = pendingsize + nread;
			  pendingbuffer = xrealloc (pendingbuffer,
						    newpendingsize);
			  memcpy (pendingbuffer + pendingsize, buffer, nread);
			  pendingsize = newpendingsize;
			  nread = 0;
			}
		    }

		  /* check statusfd */
		  if (!PeekNamedPipe (statusfd[0], NULL, 0, NULL,
				      &navail, NULL))
		    {
		      fprintf (stderr, "** PeekPipe failed: ec=%d\n",
			       (int) GetLastError ());
		      navail = 0;
		    }
		  nread = 0;
		  if (navail && !ReadFile (statusfd[0], buffer, bufsize,
					   &nread, NULL))
		    {
		      fprintf (stderr, "** ReadFile failed: ec=%d\n",
			       (int) GetLastError ());
		      nread = 0;
		    }
		  if (nread > 0)
		    {
		      char *bufptr = buffer;
		      check_again = 1;
		      fprintf (stderr, "statusfd has %d bytes\n", nread);

		      buffer[MIN (bufsize - 1, nread)] = 0;
		      while (*bufptr)
			{
			  char *qq = bufptr;
			  char nlsave;
			  while (*qq && *qq != '\n')
			    qq++;
			  nlsave = *qq;
			  *qq = 0;
			  linecallback (bufptr, linedata, TRUE);
			  status_check (bufptr, callback, calldata, "NODATA",
					"no data found");
			  status_check (bufptr, callback, calldata,
					"BADARMOR",
					"ASCII armor is corrupted");
			  if (status_check
			      (bufptr, callback, calldata,
			       "MISSING_PASSPHRASE", "missing passphrase"))
			    missing_passphrase = TRUE;
			  if (!missing_passphrase)
			    status_check (bufptr, callback, calldata,
					  "BAD_PASSPHRASE", "bad passphrase");
			  status_check (bufptr, callback, calldata,
					"DECRYPTION_FAILED",
					"decryption failed");
			  status_check (bufptr, callback, calldata,
					"NO_PUBKEY",
					"public key not available");
			  status_check (bufptr, callback, calldata,
					"NO_SECKEY",
					"secret key not available");
			  *qq = nlsave;
			  if (*qq)
			    qq++;
			  bufptr = qq;
			}
		    }
		}
	      while (check_again);
	    }			/* end read pipes */

	  switch (ncount)
	    {
	    case WAIT_TIMEOUT:
	      fprintf (stderr, "WFMO timed out after signal");
	      if (WaitForSingleObject (pi.hProcess, 0) != WAIT_OBJECT_0)
		{
		  fprintf (stderr, "subprocess still alive after signal");
		  break;
		}
	      fprintf (stderr, "subprocess exited after signal");
	      /* fall thru */
	    case WAIT_OBJECT_0:
	      {
		DWORD res;
		fprintf (stderr, "subprocess exited");
		if (!GetExitCodeProcess (pi.hProcess, &res))
		  fprintf (stderr, "GetExitCodeProcess failed\n");
		else
		  fprintf (stderr, "** exit code=%lu\n", (unsigned long) res);
		ready = 1;
	      }
	      break;

	    case WAIT_OBJECT_0 + 1:
	    case WAIT_OBJECT_0 + 2:
	      /* already handled */
	      break;

	    case WAIT_FAILED:
	      {
		DWORD r;
		fprintf ("waitbuf[0] %p = %d", waitbuf[0],
			 GetHandleInformation (waitbuf[0], &r));
	      }
	      break;

	    default:
	      fprintf (stderr, "WFMO returned invalid value\n");
	      break;
	    }

	  /* Handle remaining output data.
	   */
	  if (pendingsize > 0)
	    {
	      int newpendingsize = pendingsize + 1;
	      pendingbuffer =
		(char *) xrealloc (pendingbuffer, newpendingsize);
	      pendingbuffer[newpendingsize - 1] = 0;
	      linecallback (pendingbuffer, linedata, FALSE);
	      free (pendingbuffer);
	      pendingbuffer = NULL;
	      pendingsize = 0;
	    }
#if 0
	  if (WIFEXITED (status) && WEXITSTATUS (status) != 0)
	    {
	      char msg[80];
	      sprintf (msg, "GnuPG execution terminated with error code %d",
		       WEXITSTATUS (status));
	      callback (GPAPA_ACTION_ABORTED, msg, calldata);
	    }
	  else if (WIFSIGNALED (status) && WTERMSIG (status) != 0)
	    {
	      char msg[80];
	      sprintf (msg,
		       "GnuPG execution terminated by uncaught signal %d",
		       WTERMSIG (status));
	      callback (GPAPA_ACTION_ABORTED, msg, calldata);
	    }
	  else
	    callback (GPAPA_ACTION_FINISHED,
		      "GnuPG execution terminated normally", calldata);
#endif
	}

      free (buffer);
    }
  else
    {
      /* @@ To be implemented:
       *
       * Install linecallback() in a way that gpapa_idle()
       * will call it for each line coming from this pipe.
       */
    }
  CloseHandle (outputfd[0]);
  CloseHandle (statusfd[0]);

  fprintf (stderr, "** gpapa_call_gnupg ready\n");
#else /* Here is the code for something we call a real operating system */
  int pid;
  int outputfd[2], statusfd[2], passfd[2], inputfd[2], devnull;
  char **argv;
  char statusfd_str[10], passfd_str[10];
  int argc, user_args_counter, standard_args_counter, i, j;

  char *standard_args[] = {
    "--status-fd",
    statusfd_str,
    "--no-options",
    "--batch",
    NULL
  };

  /* Open /dev/null for redirecting stderr.
   */
  devnull = open ("/dev/null", O_RDWR);
  if (devnull == -1)
    {
      callback (GPAPA_ACTION_ERROR, "could not open `/dev/null'", calldata);
      return;
    }

  /* Open output pipe.
   */
  if (pipe (outputfd) == -1)
    {
      callback (GPAPA_ACTION_ERROR, "could not open output pipe", calldata);
      return;
    }

  /* Open status pipe.
   */
  if (pipe (statusfd) == -1)
    {
      callback (GPAPA_ACTION_ERROR, "could not open status pipe", calldata);
      close (outputfd[0]);
      close (outputfd[1]);
      return;
    }
  sprintf (statusfd_str, "%d", statusfd[1]);

  if (passphrase)
    {
      /* Open passphrase pipe.
       */
      if (pipe (passfd) == -1)
	{
	  callback (GPAPA_ACTION_ERROR, "could not open passphrase pipe",
		    calldata);
	  close (outputfd[0]);
	  close (outputfd[1]);
	  close (statusfd[0]);
	  close (statusfd[1]);
	  return;
	}
      sprintf (passfd_str, "%d", passfd[0]);
      write (passfd[1], passphrase, strlen (passphrase));
      write (passfd[1], "\n", 1);
    }

  if (commands)
    {
      /* Open input pipe.
       */
      if (pipe (inputfd) == -1)
	{
	  callback (GPAPA_ACTION_ERROR, "could not open input pipe",
		    calldata);
	  close (outputfd[0]);
	  close (outputfd[1]);
	  close (statusfd[0]);
	  close (statusfd[1]);
	  if (passphrase)
	    {
	      close (passfd[0]);
	      close (passfd[1]);
	    }
	  return;
	}
      write (inputfd[1], commands, strlen (commands));
    }

  /* Construct the command line.
   */
  user_args_counter = 0;
  while (user_args[user_args_counter] != NULL)
    user_args_counter++;
  standard_args_counter = 0;
  while (standard_args[standard_args_counter] != NULL)
    standard_args_counter++;
  argc = 1 + standard_args_counter + user_args_counter;
  if (passphrase)
    argc += 2;
  argv = (char **) xmalloc ((argc + 1) * sizeof (char *));
  i = 0;
  argv[i++] = "/usr/local/bin/gpg";
  for (j = 0; j < standard_args_counter; j++)
    argv[i++] = standard_args[j];
  if (passphrase)
    {
      argv[i++] = "--passphrase-fd";
      argv[i++] = passfd_str;
    }
  for (j = 0; j < user_args_counter; j++)
    argv[i++] = user_args[j];
  argv[i] = NULL;

#ifdef DEBUG
  for (j = 0; j < i; j++)
    fprintf (stderr, "%s ", argv[j]);
  fprintf (stderr, "\n");
#endif

  pid = fork ();
  if (pid == -1)		/* error */
    callback (GPAPA_ACTION_ERROR, "fork() failed", calldata);
  else if (pid == 0)		/* child */
    {
      close (outputfd[0]);
      close (statusfd[0]);
      dup2 (outputfd[1], 1);
      close (outputfd[1]);
      if (commands)
	{
	  dup2 (inputfd[0], 0);
	  close (inputfd[0]);
	}
      else
	dup2 (devnull, 0);
#ifndef DEBUG
      dup2 (devnull, 2);
#endif
      close (devnull);
      execv (argv[0], argv);

      /* Reached only if execution failed.
       */
      callback (GPAPA_ACTION_ERROR, "GnuPG execution failed", calldata);
    }
  else				/* parent */
    {
      if (do_wait)
	{
	  int status, retpid, dataavail;
	  size_t bufsize = MIN (8192, SSIZE_MAX);
	  size_t pendingsize = 0;
	  char *buffer = (char *) xmalloc (bufsize);
	  char *pendingbuffer = NULL;
	  fd_set readfds;
	  int maxfd = MAX (outputfd[0], statusfd[0]);
	  struct timeval timeout = { 0, 0 };
	  gboolean missing_passphrase = FALSE;

	  /* Check for data in the pipe and whether the GnuPG
	   * process is still running.
	   */
	  retpid = waitpid (pid, &status, WNOHANG);
	  FD_ZERO (&readfds);
	  FD_SET (outputfd[0], &readfds);
	  FD_SET (statusfd[0], &readfds);
	  dataavail = select (maxfd + 1, &readfds, NULL, NULL, &timeout);

	  while (retpid == 0 || dataavail != 0)
	    {
	      int p, q;

	      /* Handle output data.
	       */
	      if (FD_ISSET (outputfd[0], &readfds))
		p = read (outputfd[0], buffer, bufsize);
	      else
		p = 0;
	      while (p > 0)
		{
		  q = 0;
		  while (q < p && buffer[q] != '\n')
		    q++;
		  if (q < p)
		    {
		      if (pendingsize > 0)
			{
			  int newpendingsize = pendingsize + q + 1;
			  pendingbuffer =
			    (char *) xrealloc (pendingbuffer, newpendingsize);
			  memcpy (pendingbuffer + pendingsize, buffer, q);
			  pendingbuffer[newpendingsize - 1] = 0;
			  linecallback (pendingbuffer, linedata, FALSE);
			  free (pendingbuffer);
			  pendingbuffer = NULL;
			  pendingsize = 0;
			}
		      else
			{
			  buffer[q] = 0;
			  linecallback (buffer, linedata, FALSE);
			}
		      memmove (buffer, buffer + q + 1, p - q - 1);
		      p -= q + 1;
		    }
		  else
		    {
		      int newpendingsize = pendingsize + p;
		      pendingbuffer =
			(char *) xrealloc (pendingbuffer, newpendingsize);
		      memcpy (pendingbuffer + pendingsize, buffer, p);
		      pendingsize = newpendingsize;
		      p = 0;
		    }
		}

	      /* Handle status data.
	       */
	      if (FD_ISSET (statusfd[0], &readfds))
		{
		  char *bufptr = buffer;
		  p = read (statusfd[0], buffer, bufsize);
		  buffer[MIN (bufsize - 1, p)] = 0;
		  if (p)
		    {
#ifdef DEBUG
		      fprintf (stderr, "status data: %s\n", buffer);
#endif
		      while (*bufptr)
			{
			  char *qq = bufptr;
			  char nlsave;
			  while (*qq && *qq != '\n')
			    qq++;
			  nlsave = *qq;
			  *qq = 0;
			  linecallback (bufptr, linedata, TRUE);
			  status_check (bufptr, callback, calldata, "NODATA",
					"no data found");
			  status_check (bufptr, callback, calldata,
					"BADARMOR",
					"ASCII armor is corrupted");
			  if (status_check
			      (bufptr, callback, calldata,
			       "MISSING_PASSPHRASE", "missing passphrase"))
			    missing_passphrase = TRUE;
			  if (!missing_passphrase)
			    status_check (bufptr, callback, calldata,
					  "BAD_PASSPHRASE", "bad passphrase");
			  status_check (bufptr, callback, calldata,
					"DECRYPTION_FAILED",
					"decryption failed");
			  status_check (bufptr, callback, calldata,
					"NO_PUBKEY",
					"public key not available");
			  status_check (bufptr, callback, calldata,
					"NO_SECKEY",
					"secret key not available");
			  *qq = nlsave;
			  if (*qq)
			    qq++;
			  bufptr = qq;
			}
		    }
		}

	      /* Check for more data in the pipes and whether the GnuPG
	       * process is still running.
	       */
	      if (retpid == 0)
		retpid = waitpid (pid, &status, WNOHANG);
	      FD_ZERO (&readfds);
	      FD_SET (outputfd[0], &readfds);
	      FD_SET (statusfd[0], &readfds);
	      dataavail = select (maxfd + 1, &readfds, NULL, NULL, &timeout);
	    }

	  /* Handle remaining output data.
	   */
	  if (pendingsize > 0)
	    {
	      int newpendingsize = pendingsize + 1;
	      pendingbuffer =
		(char *) xrealloc (pendingbuffer, newpendingsize);
	      pendingbuffer[newpendingsize - 1] = 0;
	      linecallback (pendingbuffer, linedata, FALSE);
	      free (pendingbuffer);
	      pendingbuffer = NULL;
	      pendingsize = 0;
	    }
	  if (WIFEXITED (status) && WEXITSTATUS (status) != 0)
	    {
	      char msg[80];
	      sprintf (msg, "GnuPG execution terminated with error code %d",
		       WEXITSTATUS (status));
	      callback (GPAPA_ACTION_ABORTED, msg, calldata);
	    }
	  else if (WIFSIGNALED (status) && WTERMSIG (status) != 0)
	    {
	      char msg[80];
	      sprintf (msg,
		       "GnuPG execution terminated by uncaught signal %d",
		       WTERMSIG (status));
	      callback (GPAPA_ACTION_ABORTED, msg, calldata);
	    }
	  else
	    callback (GPAPA_ACTION_FINISHED,
		      "GnuPG execution terminated normally", calldata);
	  free (buffer);
	}
      else
	{
	  /* @@ To be implemented:
	   *
	   * Install linecallback() in a way that gpapa_idle()
	   * will call it for each line coming from this pipe.
	   */
	}
      close (outputfd[0]);
      close (statusfd[0]);
      if (passphrase)
	close (passfd[1]);
      if (commands)
	close (inputfd[1]);
      close (outputfd[1]);	/* @@ Shouldn't this be done earlier? */
      close (statusfd[1]);
      if (passphrase)
	close (passfd[0]);
      if (commands)
	close (inputfd[0]);
      close (devnull);
    }
  free (argv);
#endif /* real operating system */
}				/* gpapa_call_gnupg */
