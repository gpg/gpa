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
#include <sys/wait.h>
#include <fcntl.h>

static gboolean status_check (
  gchar *buffer,
  GpapaCallbackFunc callback, gpointer calldata,
  gchar *keyword, gchar *message
) {
  gchar *checkval = xstrcat2 ( "[GNUPG:] ", keyword );
  gboolean result = FALSE;
  if ( strncmp ( buffer, checkval, strlen ( checkval ) ) == 0 )
    {
      result = TRUE;
      callback ( GPAPA_ACTION_ERROR, message, calldata );
    }
  free ( checkval );
  return ( result );
} /* status_check */

/* user_args must be a NULL-terminated array of argument strings.
 */
void gpapa_call_gnupg (
  char **user_args, gboolean do_wait, gchar *passphrase,
  GpapaLineCallbackFunc linecallback, gpointer linedata,
  GpapaCallbackFunc callback, gpointer calldata
) {
  int pid;
  int outputfd [ 2 ], statusfd [ 2 ], passfd [ 2 ], devnull;
  char **argv;
  char statusfd_str [ 10 ], passfd_str [ 10 ];
  int argc, user_args_counter, standard_args_counter, i, j;

  char *standard_args [ ] = {
    "--status-fd",
    statusfd_str,
    "--no-options",
    "--batch",
    NULL
  };

  /* Open /dev/null for redirecting stderr.
   */
  devnull = open ( "/dev/null", O_RDWR );
  if ( devnull == -1 )
    {
      callback ( GPAPA_ACTION_ERROR, "could not open `/dev/null'", calldata );
      return;
    }

  /* Open output pipe.
   */
  if ( pipe ( outputfd ) == -1 )
    {
      callback ( GPAPA_ACTION_ERROR, "could not open output pipe", calldata );
      return;
    }

  /* Open status pipe.
   */
  if ( pipe ( statusfd ) == -1 )
    {
      callback ( GPAPA_ACTION_ERROR, "could not open status pipe", calldata );
      return;
    }
  sprintf ( statusfd_str, "%d", statusfd [ 1 ] );

  if ( passphrase )
    {
      /* Open passphrase pipe.
       */
      if ( pipe ( passfd ) == -1 )
	{
	  callback ( GPAPA_ACTION_ERROR, "could not open passphrase pipe", calldata );
	  return;
	}
      sprintf ( passfd_str, "%d", passfd [ 0 ] );
      write ( passfd [ 1 ], passphrase, strlen ( passphrase ) );
      write ( passfd [ 1 ], "\n", 1 );
    }

  /* Construct the command line.
   */
  user_args_counter = 0;
  while ( user_args [ user_args_counter ] != NULL )
    user_args_counter++;
  standard_args_counter = 0;
  while ( standard_args [ standard_args_counter ] != NULL )
    standard_args_counter++;
  argc = 1 + standard_args_counter + user_args_counter;
  if ( passphrase )
    argc += 2;
  argv = (char **) xmalloc ( ( argc + 1 ) * sizeof ( char * ) );
  i = 0;
  argv [ i++ ] = "/usr/local/bin/gpg";
  for ( j = 0; j < standard_args_counter; j++ )
    argv [ i++ ] = standard_args [ j ];
  if ( passphrase )
    {
      argv [ i++ ] = "--passphrase-fd";
      argv [ i++ ] = passfd_str;
    }
  for ( j = 0; j < user_args_counter; j++ )
    argv [ i++ ] = user_args [ j ];
  argv [ i ] = NULL;

#ifdef DEBUG
  for ( j = 0; j < i; j++ )
    fprintf ( stderr, "%s ", argv [ j ] );
  fprintf ( stderr, "\n" );
#endif

  pid = fork ( );
  if ( pid == -1 )  /* error */
    callback ( GPAPA_ACTION_ERROR, "fork() failed", calldata );
  else if ( pid == 0 )	/* child */
    {
      close ( outputfd [ 0 ] );
      close ( statusfd [ 0 ] );
      dup2 ( outputfd [ 1 ], 1 );
      close ( outputfd [ 1 ] );
      dup2 ( devnull, 2 );
      close ( devnull );
      execv ( argv [ 0 ], argv );

      /* Reached only if execution failed.
       */
      callback ( GPAPA_ACTION_ERROR, "GnuPG execution failed", calldata );
    }
  else	/* parent */
    {
      if ( do_wait )
	{
	  int status, retpid, dataavail;
	  size_t bufsize = MIN ( 8192, SSIZE_MAX );
	  size_t pendingsize = 0;
	  char *buffer = (char *) xmalloc ( bufsize );
	  char *pendingbuffer = NULL;
	  fd_set readfds;
	  int maxfd = MAX ( outputfd [ 0 ], statusfd [ 0 ] );
	  struct timeval timeout = { 0, 0 };
	  gboolean missing_passphrase = FALSE;

	  /* Check for data in the pipe and whether the GnuPG
	   * process is still running.
	   */
	  retpid = waitpid ( pid, &status, WNOHANG );
	  FD_ZERO ( &readfds );
	  FD_SET ( outputfd [ 0 ], &readfds );
	  FD_SET ( statusfd [ 0 ], &readfds );
	  dataavail = select ( maxfd + 1, &readfds, NULL, NULL, &timeout );

	  while ( retpid == 0 || dataavail != 0 )
	    {
	      int p, q;

	      /* Handle output data.
	       */
	      if ( FD_ISSET ( outputfd [ 0 ], &readfds ) )
		p = read ( outputfd [ 0 ], buffer, bufsize );
	      else
		p = 0;
	      while ( p > 0 )
		{
		  q = 0;
		  while ( q < p && buffer [ q ] != '\n' )
		    q++;
		  if ( q < p )
		    {
		      if ( pendingsize > 0 )
			{
			  int newpendingsize = pendingsize + q + 1;
			  pendingbuffer = (char *) xrealloc ( pendingbuffer, newpendingsize );
			  memcpy ( pendingbuffer + pendingsize, buffer, q );
			  pendingbuffer [ newpendingsize - 1 ] = 0;
			  linecallback ( pendingbuffer, linedata );
			  free ( pendingbuffer );
			  pendingbuffer = NULL;
			  pendingsize = 0;
			}
		      else
			{
			  buffer [ q ] = 0;
			  linecallback ( buffer, linedata );
			}
		      memmove ( buffer, buffer + q + 1, p - q - 1 );
		      p -= q + 1;
		    }
		  else
		    {
		      int newpendingsize = pendingsize + p;
		      pendingbuffer = (char *) xrealloc ( pendingbuffer, newpendingsize );
		      memcpy ( pendingbuffer + pendingsize, buffer, p );
		      pendingsize = newpendingsize;
		      p = 0;
		    }
		}

	      /* Handle status data.
	       */
	      if ( FD_ISSET ( statusfd [ 0 ], &readfds ) )
		{
		  p = read ( statusfd [ 0 ], buffer, bufsize );
		  buffer [ MIN ( bufsize - 1, p ) ] = 0;
		  if ( p )
		    {
#ifdef DEBUG
		      fprintf ( stderr, "status data: %s\n", buffer );
#endif
		      status_check ( buffer, callback, calldata, "NODATA",
				     "no data found" );
		      status_check ( buffer, callback, calldata, "BADARMOR",
				     "ASCII armor is corrupted" );
		      if ( status_check ( buffer, callback, calldata, "MISSING_PASSPHRASE",
					  "missing passphrase" ) )
			missing_passphrase = TRUE;
		      if ( ! missing_passphrase )
			status_check ( buffer, callback, calldata, "BAD_PASSPHRASE",
				       "bad passphrase" );
		      status_check ( buffer, callback, calldata, "DECRYPTION_FAILED",
				     "symmetrical decryption failed" );
		      status_check ( buffer, callback, calldata, "DECRYPTION_FAILED",
				     "symmetrical decryption failed" );
		      status_check ( buffer, callback, calldata, "NO_PUBKEY",
				     "public key not available" );
		      status_check ( buffer, callback, calldata, "NO_SECKEY",
				     "secret key not available" );
		    }
		}

	      /* Check for more data in the pipes and whether the GnuPG
	       * process is still running.
	       */
	      if ( retpid == 0 )
		retpid = waitpid ( pid, &status, WNOHANG );
	      FD_ZERO ( &readfds );
	      FD_SET ( outputfd [ 0 ], &readfds );
	      FD_SET ( statusfd [ 0 ], &readfds );
	      dataavail = select ( maxfd + 1, &readfds, NULL, NULL, &timeout );
	    }

	  /* Handle remaining output data.
	   */
	  if ( pendingsize > 0 )
	    {
	      int newpendingsize = pendingsize + 1;
	      pendingbuffer = (char *) xrealloc ( pendingbuffer, newpendingsize );
	      pendingbuffer [ newpendingsize - 1 ] = 0;
	      linecallback ( pendingbuffer, linedata );
	      free ( pendingbuffer );
	      pendingbuffer = NULL;
	      pendingsize = 0;
	    }
	  if ( ! WIFEXITED ( status ) )
	    callback ( GPAPA_ACTION_ERROR, "GnuPG execution terminated abnormally",
		       calldata );
	  free ( buffer );
	}
      else
	{
	  /* @@ To be implemented:
	   *
	   * Install linecallback() in a way that gpapa_idle()
	   * will call it for each line coming from this pipe.
	   */
	}
      close ( outputfd [ 0 ] );
      close ( statusfd [ 0 ] );
      if ( passphrase )
	close ( passfd [ 1 ] );
      close ( outputfd [ 1 ] );  /* @@ Shouldn't this be done earlier? */
      close ( statusfd [ 1 ] );
      if ( passphrase )
	close ( passfd [ 0 ] );
      close ( devnull );
    }
  free ( argv );
} /* gpapa_call_gnupg */
