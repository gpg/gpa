/* gpapaintern.c  -  The GNU Privacy Assistant Pipe Access  -  internal routines
 *	  Copyright (C) 2000 G-N-U GmbH.
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
#include <errno.h>
#ifdef __MINGW32__
#include <windows.h>
#else
#include <sys/wait.h>
#endif
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include "stringhelp.h"

#define DEBUG 1
/* #define TEST_PASSPHRASE "test" */

#ifdef __MINGW32__
struct gpg_object_private {
  int foo;
};
#else
struct gpg_object_private {
  pid_t pid;
  int output_fd;
  int status_fd;
};
#endif

typedef enum {
  STATUS_CHANNEL,
  OUTPUT_CHANNEL
} GPG_CHANNEL;

typedef struct channel {
    int avail;
    int eof;
    size_t bufsize;
    char *buffer;
    size_t readpos;
} CHANNEL;

typedef struct {
  struct gpg_object_private private;
  int running;
  int exit_status;
  int exit_signal;
  CHANNEL status;
  CHANNEL output;
} GPG_OBJECT;


static void
dummy_linecallback ( char *a, void *opaque, int is_status )
{
}


gboolean
gpapa_line_begins_with (gchar * line, gchar * keyword)
{
  if (strncmp (line, keyword, strlen (keyword)) == 0)
    return (TRUE);
  else
    return (FALSE);
}	



static gboolean
status_check (char *line, 
	      GpapaCallbackFunc callback, gpointer calldata,
	      const char *keyword, const char *message)
{
  size_t n = strlen(keyword);

  if ( !strncmp ( line, keyword, n ) && (line[n] == ' ' || !line[n] ) )
    {
      char *p = xstrdup ( message );
      callback (GPAPA_ACTION_ERROR, p, calldata);
      free ( p );
      return 1;
    }
  return 0;
}		




#ifdef __MINGW32__
static GPG_OBJECT * 
spawn_gnupg(char **user_args, gchar * commands,
                GpapaCallbackFunc callback, gpointer calldata)
{
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
      if ( GetLastError() != ERROR_CALL_NOT_IMPLEMENTED )
	fprintf (stderr, "** SHI 1 failed: ec=%d\n", (int) GetLastError ());
    }
  if (!SetHandleInformation (si.hStdError, HANDLE_FLAG_INHERIT,
			     HANDLE_FLAG_INHERIT))
    {
      if ( GetLastError() != ERROR_CALL_NOT_IMPLEMENTED )
	fprintf (stderr, "** SHI 2 failed: ec=%d\n", (int) GetLastError ());
    }


  fputs ("** CreateProcess ...\n", stderr);
  fprintf (stderr, "** args=`%s'\n", arg_string);
  fflush (stderr);
  rc = CreateProcessA ("c:\\gnupg\\gpg.exe", arg_string,
                       &sec_attr,     /* process security attributes */
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

  gpg = xcalloc (1, sizeof *gpg );
  gpg->private.
}

static int /* Windows version */
wait_gnupg ( GPG_OBJECT *gpg, int no_hang )
{
  int rc = 0;
  int status;
  pid_t retpid;
  fd_set readfds;
  int nfds;
  struct timeval timeout = { 0, 0 };
  
  gpg->output.avail = 0;
  gpg->status.avail = 0;
  if ( !gpg->running )
    return rc;  

  /* first have a look at the pipes */
	  HANDLE waitbuf[3] = { pi.hProcess, outputfd[0], statusfd[0] };
	  int nwait = 3;
	  int nread, ncount;
   ncount = WaitForMultipleObjectsEx (nwait, waitbuf, FALSE, INFINITE, TRUE);

	  if ( ncount == WAIT_FAILED ) {
	    fprintf (stderr, "** WFMO failed: ec=%d\n", (int) GetLastError ());
	  }
	  if ( (ncount >= WAIT_OBJECT_0 && ncount < WAIT_OBJECT_0 + 2)
	       || ncount == WAIT_FAILED )
	
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



   

  nfds = select ( nfds, &readfds, NULL, NULL, no_hang? &timeout : NULL);
  if ( nfds > 0 )
    {
      if ( FD_ISSET ( gpg->private.output_fd, &readfds ) )
        gpg->output.avail = 1;
      if ( FD_ISSET ( gpg->private.status_fd, &readfds ) )
        gpg->status.avail = 1;
    }
  else if ( nfds && errno != EINTR )
    { 
      fprintf (stderr, "wait_gpg: select failed: %s\n", strerror (errno) );
      rc = -1;
    }

  /* see whether gpg is still running */
  
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
		DWORD res;
		fprintf (stderr, "wait failed");
		if (!GetExitCodeProcess (pi.hProcess, &res))
		  fprintf (stderr, "GetExitCodeProcess failed\n");
		else
		  fprintf (stderr, "** exit code=%lu\n", (unsigned long) res);
		ready = 1;
	      }
	      break;

	    default:
	      fprintf (stderr, "WFMO returned invalid value\n");
	      break;
	    }


if ( retpid == gpg->private.pid )
    {
      gpg->running = 0;     
      if ( WIFSIGNALED (status) )
        {
          gpg->exit_status = 4;
          gpg->exit_signal = WTERMSIG (status);
        }
      else if ( WIFEXITED (status) )
        {
          gpg->exit_status = WEXITSTATUS (status);
        }
      else /* oops */
        {
          rc = -1;
        }
    }

  return rc;
}

static int
channel_read ( GPG_OBJECT *gpg, GPG_CHANNEL channel,
               void *buffer, size_t bufsize )
{
  int fd=-1, avail=0, nread;

  switch ( channel )
    {
    case STATUS_CHANNEL:
      fd = gpg->private.status_fd;
      avail = gpg->status.avail;
      break;
    case OUTPUT_CHANNEL:
      fd = gpg->private.output_fd;
      avail = gpg->output.avail;
      break;
    }

  if ( !avail )
    return 0;

  do 
    nread = read ( fd, buffer, bufsize );
  while ( nread == -1 && errno == EINTR );

  if ( nread == -1 )
    fprintf ( stderr, "gnupg_read on channel %d failed: %s\n",
              channel, strerror ( errno ) );

  return nread;
}

void
gpapa_gnupg_close ( GPG_OBJECT *gpg )
{
  if ( !gpg )
    return;

  if ( gpg->running )
    {  /* still running? Must send a killer */
      kill ( gpg->private.pid, SIGTERM);
      sleep (2);
      if ( !waitpid (gpg->private.pid, NULL, WNOHANG) )
        {  /* pay the murderer better and then forget about it */
          kill (gpg->private.pid, SIGKILL);
        }
    }

  close ( gpg->private.output_fd );
  close ( gpg->private.status_fd );

  free (gpg);
}

#endif /*__MINGW32__*/


#ifndef __MINGW32__
static GPG_OBJECT * 
spawn_gnupg(char **user_args, gchar * commands,
                GpapaCallbackFunc callback, gpointer calldata)
{
  int pid;
  int outputfd[2], statusfd[2], inputfd[2], devnull;
  char **argv;
  char statusfd_str[10];
  int argc, user_args_counter, standard_args_counter, i, j;
  GPG_OBJECT *gpg = NULL;

#ifdef TEST_PASSPHRASE
  int passfd[2];
  char passfd_str[10];
  gchar passphrase[] = "test";
#endif

  /* FIXME: Make user_args const and copy/release the argument array */

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
      return NULL;
    }

  /* Open output pipe.
   */
  if (pipe (outputfd) == -1)
    {
      callback (GPAPA_ACTION_ERROR, "could not open output pipe", calldata);
      return NULL;
    }

  /* Open status pipe.
   */
  if (pipe (statusfd) == -1)
    {
      callback (GPAPA_ACTION_ERROR, "could not open status pipe", calldata);
      close (outputfd[0]);
      close (outputfd[1]);
      return NULL;
    }
  sprintf (statusfd_str, "%d", statusfd[1]);

#ifdef TEST_PASSPHRASE
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
	  return NULL;
	}
      sprintf (passfd_str, "%d", passfd[0]);
      write (passfd[1], passphrase, strlen (passphrase));
      write (passfd[1], "\n", 1);
    }
#endif

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

#ifdef TEST_PASSPHRASE
  	  if (passphrase)
  	    {
  	      close (passfd[0]);
  	      close (passfd[1]); 
  	    } 
#endif
	  return NULL;
	}
      write (inputfd[1], commands, strlen (commands));
      close (inputfd[1]);
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

#ifdef TEST_PASSPHRASE
    if (passphrase)
      argc += 2;
#endif

    if (commands) 
	argc += 2;
  argv = (char **) xmalloc ((argc + 1) * sizeof (char *));
  i = 0;
  argv[i++] = xstrdup (gpapa_private_get_gpg_program () );
  for (j = 0; j < standard_args_counter; j++)
    argv[i++] = standard_args[j];

#ifdef TEST_PASSPHRASE
    if (passphrase)
      {
        argv[i++] = "--passphrase-fd";
        argv[i++] = passfd_str;
      }
#endif

    if (commands)
    { 
      argv[i++] = "--command-fd";
      argv[i++] = "0";
    }
  for (j = 0; j < user_args_counter; j++)
    argv[i++] = user_args[j];
  argv[i] = NULL;

  pid = fork ();
  if (pid == -1)
    {
      callback (GPAPA_ACTION_ERROR, "fork() failed", calldata);
      /* fixme: add cleanups */
      return NULL;
    }

  if ( !pid ) /* child */
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
      close (devnull);
      execv (argv[0], argv);
      /* Reached only if execution failed (we are not the child)*/
      callback (GPAPA_ACTION_ERROR, "GnuPG execution failed", calldata);
      /* fixme: need some cleanup here */
      return NULL;
    }

  /* everything is fine */
  gpg = xcalloc (1, sizeof *gpg );
  gpg->private.pid = pid;
  gpg->private.output_fd = outputfd[0];
  gpg->private.status_fd = statusfd[0];
  gpg->running = 1;
  return gpg;
}

/*
 * select on gpgs pipes and check whether gpg is still running
 * If NO_HANG is true, the function will block but may get interrupted
 * by interrupts in which case it will simply return as if with NO_HANG not
 * set.
 */

static int
wait_gnupg ( GPG_OBJECT *gpg, int no_hang )
{
  int rc = 0;
  int status;
  pid_t retpid;
  fd_set readfds;
  int nfds;
  struct timeval timeout = { 0, 0 };
  
  gpg->output.avail = 0;
  gpg->status.avail = 0;
  if ( !gpg->running )
    return rc;  

 restart:
  /* see whether gpg is still running */
  retpid = waitpid ( gpg->private.pid, &status, WNOHANG );
  if ( retpid == gpg->private.pid )
    {
      gpg->running = 0;     
      if ( WIFSIGNALED (status) )
        {
          gpg->exit_status = 4;
          gpg->exit_signal = WTERMSIG (status);
        }
      else if ( WIFEXITED (status) )
        {
          gpg->exit_status = WEXITSTATUS (status);
        }
      else /* oops */
        {
          rc = -1;
        }
    }

  if ( !no_hang && gpg->running )
    {
      /* gpg may have stopped meanwhile, therefore we can't relay on the
       * running flag hang in select. So we simply use a timeout to
       * workaround this race and check again later
       * FIXME: Use SIGCHLD */
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
    }
  

  /* first have a look at the pipes */
  FD_ZERO (&readfds);
  FD_SET (gpg->private.output_fd, &readfds);
  FD_SET (gpg->private.status_fd, &readfds);
  nfds = MAX ( gpg->private.output_fd, gpg->private.status_fd ) + 1;
  nfds = select ( nfds, &readfds, NULL, NULL, &timeout );
   if ( nfds > 0 )
    {
      if ( FD_ISSET ( gpg->private.output_fd, &readfds ) )
        gpg->output.avail = 1;
      if ( FD_ISSET ( gpg->private.status_fd, &readfds ) )
        gpg->status.avail = 1;
    }
  else if ( nfds && errno != EINTR )
    { 
      fprintf (stderr, "wait_gpg: select failed: %s\n", strerror (errno) );
      rc = -1;
    }
  else if ( !nfds && !no_hang && gpg->running )
    goto restart;


  return rc;
}

static int
channel_read ( GPG_OBJECT *gpg, GPG_CHANNEL channel,
               void *buffer, size_t bufsize )
{
  int fd=-1, avail=0, nread;
  int dummy = 1;
  int *eofflag = &dummy;

  switch ( channel )
    {
    case STATUS_CHANNEL:
      fd = gpg->private.status_fd;
      avail = gpg->status.avail;
      eofflag = &gpg->status.eof;
      break;
    case OUTPUT_CHANNEL:
      fd = gpg->private.output_fd;
      avail = gpg->output.avail;
      eofflag = &gpg->status.eof;
      break;
    }

  if ( !avail || *eofflag )
    return 0;

  do 
    nread = read ( fd, buffer, bufsize );
  while ( nread == -1 && errno == EINTR );

  if ( nread == -1 )
    fprintf ( stderr, "gnupg_read on channel %d failed: %s\n",
              channel, strerror ( errno ) );

  if ( !nread )
      *eofflag = 1;

  return nread;
}

void
gpapa_gnupg_close ( GPG_OBJECT *gpg )
{
  if ( !gpg )
    return;

  if ( gpg->running )
    {  /* still running? Must send a killer */
      kill ( gpg->private.pid, SIGTERM);
      sleep (2);
      if ( !waitpid (gpg->private.pid, NULL, WNOHANG) )
        {  /* pay the murderer better and then forget about it */
          kill (gpg->private.pid, SIGKILL);
        }
    }

  close ( gpg->private.output_fd );
  close ( gpg->private.status_fd );

  free (gpg);
}


#endif /*__MINGW32__*/


GPG_OBJECT *
gpapa_gnupg_create ( int foo )
{
  return NULL;
}



/*
 * Handle the status output of GnuPG.  This function does read entire lines
 * and passes them as C strings to the callback function (we can use C Strings
 * because the status output is always UTF-8 encoded).  Of course we have to
 * buffer the lines to cope with long lines e.g. with a large user ID.
 * Note: We can optimize this to only cope with status line code we know about
 * and skipp all other stuff without buffering (i.e. without extending the
 * buffer).
 */
static void
handle_status (GPG_OBJECT *gpg,
               GpapaLineCallbackFunc line_cb, void *line_opaque,
               GpapaCallbackFunc callback, gpointer calldata)
{
  char *p;
  int nread;
  size_t bufsize = gpg->status.bufsize; 
  char *buffer = gpg->status.buffer;
  size_t readpos = gpg->status.readpos; 

  if (!gpg->status.avail)
    return;

  /* Hmmm: should we handle EOFs or error here?  */
  if (!buffer)
    {
      bufsize = 1024;
      buffer = xmalloc (bufsize);
      readpos = 0;
    }
  else if (bufsize - readpos < 256) /* make more room for the read */
    {
      bufsize += 1024;
      buffer = xrealloc (buffer, bufsize);
    }

  nread = channel_read (gpg, STATUS_CHANNEL,
                        buffer + readpos, bufsize - readpos);
  while (nread > 0)
    {
      /* We require that the last line is terminated by a LF.  */
      for (p = buffer + readpos; nread; nread--, p++)
        {
          if ( *p == '\n' )
            {
              *p = 0;
              fprintf (stderr, "gpg status: `%s'\n", buffer);
              if (!strncmp (buffer, "[GNUPG:] ", 9 ))
                {
                  char *line = buffer + 9;
                  /* Note that the callback is allowed to modify the line.  */
                  line_cb ( line, line_opaque, 1 );
                  /* Do some status check of our own here.  */
                  status_check (line, callback, calldata, "NODATA",
                                "no data found");
                  status_check (line, callback, calldata,
                                "BADARMOR",
                                "ASCII armor is corrupted");
                  status_check (line, callback, calldata,
                                "MISSING_PASSPHRASE", "missing passphrase");
                  status_check (line, callback, calldata,
                                "BAD_PASSPHRASE", "bad passphrase");
                  status_check (line, callback, calldata,
                                "DECRYPTION_FAILED", "decryption failed");
                  status_check (line, callback, calldata,
                                "NO_PUBKEY", "public key not available");
                  status_check (line, callback, calldata,
                                "NO_SECKEY", "secret key not available");
                }
              /* To reuse the buffer for the next line we have to shift
                 the remaining data to the buffer start and restart the loop
                 Hmmm: We can optimize this function by lookink forward in
                 the buffer to see whether a second complete line is available
                 and in this case avoid the memmove for this line.  */
              nread--; p++;
              if (nread)
                memmove (buffer, p, nread);
              readpos = 0;
              break; /* the inner loop */
            }
          else
            readpos++;
        }
    } 
  /* Update the gpg object.  */
  gpg->status.bufsize = bufsize;
  gpg->status.buffer = buffer;
  gpg->status.readpos = readpos;
  gpg->status.avail = 0;  /* make sure that a wait_gnupg must be called */
}

/*
 * nearly the same as handle_status. this one is used for gpg's keylinsting
 * and other line oriented output.
 */

static void
handle_output_line (GPG_OBJECT *gpg,
                    GpapaLineCallbackFunc line_cb, void *line_opaque)
{
  char *p;
  int nread;
  size_t bufsize = gpg->output.bufsize; 
  char *buffer = gpg->output.buffer;
  size_t readpos = gpg->output.readpos; 

  if (!gpg->output.avail)
    return;

  /* Hmmm: should we handle EOFs or error here?  */
  if (!buffer)
    {
      bufsize = 1024;
      buffer = xmalloc (bufsize);
      readpos = 0;
    }
  else if (bufsize - readpos < 256)  /* Make more room for the read.  */
    {
      bufsize += 1024;
      buffer = xrealloc (buffer, bufsize);
    }

  nread = channel_read (gpg, OUTPUT_CHANNEL,
                        buffer+readpos, bufsize - readpos);
  while (nread > 0)
    {
      /* We require that the last line is terminated by a LF.  */
      for (p = buffer + readpos; nread; nread--, p++)
        {
          if (*p == '\n')
            {
              *p = 0;
              fprintf (stderr, "gpg output: `%s'\n", buffer);
              line_cb (buffer, line_opaque, 0);
              /* To reuse the buffer for the next line we have to
                 shift the remaining data to the buffer start and
                 restart the loop.  */
              nread--; p++;
              if (nread)
                memmove (buffer, p, nread);
              readpos = 0;
              break; /* the inner loop */
            }
          else
            readpos++;
        }
    } 
  /* Update the gpg object.  */
  gpg->output.bufsize = bufsize;
  gpg->output.buffer = buffer;
  gpg->output.readpos = readpos;
  gpg->output.avail = 0;
}

/* user_args must be a NULL-terminated array of argument strings.  */
void
gpapa_call_gnupg (char **user_args, gboolean do_wait, gchar *commands,
		  char *passphrase, GpapaLineCallbackFunc line_cb,
		  gpointer line_opaque, GpapaCallbackFunc callback,
		  gpointer calldata)
{
  GPG_OBJECT *gpg;
  
  if (!line_cb)
    line_cb = dummy_linecallback;

  gpg = spawn_gnupg (user_args, commands, callback, calldata);
  if (!gpg)
    {
      callback (GPAPA_ACTION_ERROR, "spawn_gnupg failed", calldata);
      return;
    }
  /* We ignore do_wait and simply wait for now.  */
  do 
    {
      if (wait_gnupg (gpg, 0))
        {
          callback (GPAPA_ACTION_ERROR, "wait_gnupg failed", calldata);
          gpapa_gnupg_close (gpg);
          return;
        }
      handle_status (gpg, line_cb, line_opaque, callback, calldata);
      handle_output_line (gpg, line_cb, line_opaque);
    }
  while (gpg->running);
  if (gpg->exit_status)
    {
      char msg[80];
      
      if (gpg->exit_signal)
        sprintf (msg, "gpg execution terminated by uncaught signal %d",
                 gpg->exit_signal);
      else
        sprintf (msg, "gpg execution terminated with error code %d",
                 gpg->exit_status);
      callback (GPAPA_ACTION_ABORTED, msg, calldata);
    }
  else
    callback (GPAPA_ACTION_FINISHED, "GnuPG execution terminated normally",
              calldata);
}
