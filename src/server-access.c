/* server-access.c - The GNU Privacy Assistant keyserver access.
   Copyright (C) 2002 Miguel Coca.
   Copyright (C) 2005 g10 Code GmbH.

   This file is part of GPA

   GPA is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GPA; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA  */

/* Note: This is the old code using the keyserver helpers
         directly.  This code is not used if GnuPG 2.1 is used.  */


#include <config.h>

#include "gpa.h"
#include <glib.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

/* For unlink() */
#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#else
#include <windows.h>
#include <io.h>
#endif

#include "gtktools.h"
#include "server-access.h"

/* WARNING: Keep this up to date with gnupg's include/keyserver.h */
#define KEYSERVER_OK               0 /* not an error */
#define KEYSERVER_INTERNAL_ERROR   1 /* gpgkeys_ internal error */
#define KEYSERVER_NOT_SUPPORTED    2 /* operation not supported */
#define KEYSERVER_VERSION_ERROR    3 /* VERSION mismatch */
#define KEYSERVER_GENERAL_ERROR    4 /* keyserver internal error */
#define KEYSERVER_NO_MEMORY        5 /* out of memory */
#define KEYSERVER_KEY_NOT_FOUND    6 /* key not found */
#define KEYSERVER_KEY_EXISTS       7 /* key already exists */
#define KEYSERVER_KEY_INCOMPLETE   8 /* key incomplete (EOF) */
#define KEYSERVER_UNREACHABLE      9 /* unable to contact keyserver */

#define KEYSERVER_SCHEME_NOT_FOUND 127

#define COMMAND_TEMP_NAME "gpa-com-XXXXXX"
#define OUTPUT_TEMP_NAME "gpa-out-XXXXXX"

/* Internal API */

/* FIXME: THIS SHOULDN'T BE HERE
 * The strsep function is not portable, yet parse_keyserver_uri needs it and
 * I'm too lazy to rewrite it using GLib or ANSI functions, so we copy an
 * implementation here. If there is no other way around it, this kind or
 * portability function should be moved to a new file, maybe even in a
 * separate directory. I'm putting it here to have a solution for the 0.6.1
 * release, and should be revised at some point after that.
 */

#ifndef HAVE_STRSEP
/* code taken from glibc-2.2.1/sysdeps/generic/strsep.c */
char *
strsep (char **stringp, const char *delim)
{
  char *begin, *end;

  begin = *stringp;
  if (begin == NULL)
    return NULL;

  /* A frequent case is when the delimiter string contains only one
     character.  Here we don't need to call the expensive `strpbrk'
     function and instead work using `strchr'.  */
  if (delim[0] == '\0' || delim[1] == '\0')
    {
      char ch = delim[0];

      if (ch == '\0')
        end = NULL;
      else
        {
          if (*begin == ch)
            end = begin;
          else if (*begin == '\0')
            end = NULL;
          else
            end = strchr (begin + 1, ch);
        }
    }
  else
    /* Find the end of the token.  */
    end = strpbrk (begin, delim);

  if (end)
    {
      /* Terminate the token and set *STRINGP past NUL character.  */
      *end++ = '\0';
      *stringp = end;
    }
  else
    /* No more delimiters; this is the last token.  */
    *stringp = NULL;

  return begin;
}
#endif /*HAVE_STRSEP*/

/* Code adapted from GnuPG (file g10/keyserver.c) */
static gboolean
parse_keyserver_uri (char *uri, char **scheme, char **host,
		     char **port, char **opaque)
{
  int assume_hkp=0;

  assert(uri!=NULL);

  *host=NULL;
  *port=NULL;
  *opaque=NULL;

  /* Get the scheme */

  *scheme=strsep(&uri,":");
  if(uri==NULL)
    {
      /* Assume HKP if there is no scheme */
      assume_hkp=1;
      uri=*scheme;
      *scheme="hkp";
    }

  if(assume_hkp || (uri[0]=='/' && uri[1]=='/'))
    {
      /* Two slashes means network path. */

      /* Skip over the "//", if any */
      if(!assume_hkp)
	uri+=2;

      /* Get the host */
      *host=strsep(&uri,":/");
      if(*host[0]=='\0')
	return FALSE;

      if(uri==NULL || uri[0]=='\0')
	*port=NULL;
      else
	{
	  char *ch;

	  /* Get the port */
	  *port=strsep(&uri,"/");

	  /* Ports are digits only */
	  ch=*port;
	  while(*ch!='\0')
	    {
	      if(!isdigit(*ch))
		return FALSE;

	      ch++;
	    }
	}

      /* (any path part of the URI is discarded for now as no keyserver
	 uses it yet) */
    }
  else if(uri[0]!='/')
    {
      /* No slash means opaque.  Just record the opaque blob and get
	 out. */
      *opaque=uri;
      return TRUE;
    }
  else
    {
      /* One slash means absolute path.  We don't need to support that
	 yet. */
      return FALSE;
    }

  if(*scheme[0]=='\0')
    return FALSE;

  return TRUE;
}

/* Return the path to the helper for a certain scheme */
static gchar *
helper_path (const gchar *scheme)
{
  gchar *helper;
  gchar *path;
#ifdef G_OS_WIN32
  char name[530];

  if ( !GetModuleFileNameA (0, name, sizeof (name)-30) )
    helper = GPA_KEYSERVER_HELPERS_DIR;
  else
    {
      char *p = strrchr (name, '\\');
      if (p)
        *p = 0;
      else
        *name = 0;
      helper = name;
    }
  path = g_strdup_printf ("%s\\gpg2keys_%s.exe", helper, scheme);
  if (access (path, F_OK))
    {
      g_free (path);
      path = g_strdup_printf ("%s\\gpgkeys_%s.exe", helper, scheme);
    }
#else
  helper = g_strdup_printf ("gpg2keys_%s", scheme);
  path = g_build_filename (GPA_KEYSERVER_HELPERS_DIR, helper, NULL);
  g_free (helper);
  if (access (path, F_OK))
    {
      g_free (path);
      helper = g_strdup_printf ("gpgkeys_%s", scheme);
      path = g_build_filename (GPA_KEYSERVER_HELPERS_DIR, helper, NULL);
      g_free (helper);
    }
#endif
  return path;
}

/* Find out the plugin protocol version */
static int
protocol_version (const gchar *scheme)
{
  gchar *helper[] = {helper_path (scheme), "-V", NULL};
  gchar *output = NULL;
  gint version;

  g_spawn_sync (NULL, helper, NULL, G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL,
		&output, NULL, NULL, NULL);
  if (output && *output)
    {
      /* The version is a number, and it's value is it's ascii code minus
       * the ascii code of 0 */
      version = output[0] - '0';
    }
  else
    {
      version = 0;
    }
  g_free (output);
  g_free (helper[0]);
  return version;
}

/* Return the first error code found */
static gint
parse_helper_output (const gchar *filename)
{
  FILE *file = fopen (filename, "r");
  char line[80];
  gint error = KEYSERVER_GENERAL_ERROR;
  char keyid[17];

  /* Can't happen */
  if (!file)
    return KEYSERVER_INTERNAL_ERROR;

  while (fgets(line,sizeof(line),file) != NULL)
    if (sscanf (line,"KEY %16s FAILED %i\n", keyid, &error) == 2 )
      {
	break;
      }
  fclose (file);

  return error;
}

/* Return an error string for the code */
static const gchar *
error_string (gint error_code)
{
  switch (error_code)
    {
    case KEYSERVER_OK:
      return _("No error");
      break;
    case KEYSERVER_INTERNAL_ERROR:
      return _("Internal error");
      break;
    case KEYSERVER_NOT_SUPPORTED:
      return _("Operation not supported");
      break;
    case KEYSERVER_VERSION_ERROR:
      return _("Version mismatch");
      break;
    case KEYSERVER_GENERAL_ERROR:
      return _("Internal keyserver error");
      break;
    case KEYSERVER_NO_MEMORY:
      return _("Out of memory");
      break;
    case KEYSERVER_KEY_NOT_FOUND:
      return _("Key not found");
      break;
    case KEYSERVER_KEY_EXISTS:
      return _("Key already exists on server");
      break;
    case KEYSERVER_KEY_INCOMPLETE:
      return _("Key incomplete");
      break;
    case KEYSERVER_UNREACHABLE:
      return _("Could not contact keyserver");
      break;
    default:
      return _("Unknown Error");
    }
}

static void
write_command (FILE *file, const char *scheme,
	       const char *host, const char *port,
	       const char *opaque, const char *command)
{
  fprintf (file, "%s\n", "VERSION 1");
  fprintf (file, "SCHEME %s\n", scheme);

  if (opaque)
    {
      fprintf (file, "OPAQUE %s\n", opaque);
    }
  else
    {
      fprintf (file, "HOST %s\n", host);
      if (port)
	{
	  fprintf (file, "PORT %s\n", port);
	}
    }
  fprintf (file, "%s\n", "OPTION include-revoked");
  fprintf (file, "%s\n", "OPTION include-subkeys");
  fprintf (file, "COMMAND %s\n\n", command);
}

static GtkWidget *
wait_dialog (const gchar *server, GtkWidget *parent)
{
  GtkWidget *dialog =
    gtk_message_dialog_new (GTK_WINDOW (parent),
			    GTK_DIALOG_MODAL,
			    GTK_MESSAGE_INFO, GTK_BUTTONS_NONE,
			    _("Connecting to server \"%s\".\n"
			      "Please wait."), server);
  /* FIXME: The dialog is not displayed */
  gtk_widget_show_all (dialog);
  return dialog;
}

/* Report any errors to the user. Returns TRUE if there were errors and false
 * otherwise */
static gboolean
check_errors (int exit_status, gchar *error_message, gchar *output_filename,
              int version, GtkWidget *parent)
{
  /* Error during connection. Try to parse the output and report the
   * error.
   */
  if (version == 0)
    {
      /* With Version 0 plugins, we just can't know the error, so we
       * show the error output from the plugin to the user if the process
       * ended with error */
      if (exit_status)
        {
          gchar *message = g_strdup_printf (_("An error ocurred while "
                                              "contacting the server:\n\n%s"),
                                            error_message);
          gpa_window_error (message, parent);
          return TRUE;
        }
    }
  /* If version != 0, at least try to use version 1 error codes */
  else if (exit_status)
    {
      gint error_code = parse_helper_output (output_filename);
      /* Not really errors */
      if (error_code == KEYSERVER_OK ||
          error_code == KEYSERVER_KEY_NOT_FOUND ||
          error_code == KEYSERVER_KEY_EXISTS)
        {
          return FALSE;
        }
      else
        {
          gchar *message = g_strdup_printf (_("An error ocurred while "
                                              "contacting the server:\n\n%s"),
                                            error_string (error_code));
          gpa_window_error (message, parent);
          return TRUE;
        }
    }
  return FALSE;
}

/* This is a hack: when a SIGCHLD is received, close the dialog. We need a
 * global variable to pass the dialog to the signal handler. */
#ifdef G_OS_UNIX
GtkWidget *waiting_dlg;
static void
close_dialog (int sig)
{
  gtk_dialog_response (GTK_DIALOG (waiting_dlg), GTK_RESPONSE_CLOSE);
}
#endif /*G_OS_UNIX*/

/* Run the helper, and update the dialog if possible. Returns FALSE on error */
static gboolean
do_spawn (const gchar *scheme, const gchar *command_filename,
          const gchar *output_filename, gchar **error_output,
          gint *exit_status, GtkWidget *dialog)
{
  gchar *helper_argv[] = {NULL, "-o", NULL, NULL, NULL};
  GError *error = NULL;
#ifdef G_OS_UNIX
  pid_t pid;
  gint standard_error;
  gsize length;
  GIOChannel *channel;
#endif

  /* Make sure output parameters get a sane value */
  *error_output = NULL;
  *exit_status = 127;
  /* Build the command line */
  helper_argv[0] = helper_path (scheme);
  helper_argv[2] = (gchar*) output_filename;
  helper_argv[3] = (gchar*) command_filename;

  /* Invoke the keyserver helper */
#ifdef G_OS_UNIX
  /* On Unix, run the helper asyncronously, so that we can update the dialog */
  g_spawn_async_with_pipes (NULL, helper_argv, NULL,
                            G_SPAWN_STDOUT_TO_DEV_NULL|
                            G_SPAWN_DO_NOT_REAP_CHILD,
                            NULL, NULL, &pid, NULL, NULL, &standard_error,
                            &error);
#else
  /* On Windows, use syncronous spawn */
  g_spawn_sync (NULL, helper_argv, NULL,
		G_SPAWN_STDOUT_TO_DEV_NULL, NULL, NULL,
		NULL, error_output, exit_status, &error);
#endif

  /* Free the helper's filename */
  g_free (helper_argv[0]);

  if (error)
    {
      /* An error ocurred in the fork/exec: we assume that there is no plugin.
       */
      gpa_window_error (_("There is no plugin available for the keyserver\n"
                          "protocol you specified."), dialog);
      return FALSE;
    }

#ifdef G_OS_UNIX
  /* FIXME: Could we do this properly, without a global variable? */
  /* We run the dialog until we get a SIGCHLD, meaning the helper has
   * finnished, then we wait for it. */
  waiting_dlg = dialog;
  signal (SIGCHLD, close_dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  signal (SIGCHLD, SIG_DFL);
  wait (exit_status);
  /* Read stderr, since we need any error message */
  channel = g_io_channel_unix_new (standard_error);
  g_io_channel_read_to_end (channel, error_output, &length, NULL);
  g_io_channel_unref (channel);
  /* Make sure the pipe is closed */
  close (standard_error);
#endif

  return TRUE;
}

static gboolean
invoke_helper (const gchar *server, const gchar *scheme,
               gchar *command_filename, gchar **output_filename,
               GtkWidget *parent)
{
  int exit_status;
  int output_fd;
  gchar *error_output;
  GtkWidget *dialog = NULL;
  gboolean result = TRUE;

  /* Display a pretty dialog */
  dialog = wait_dialog (server, parent);
  /* Open the output file */
  output_fd = g_file_open_tmp (OUTPUT_TEMP_NAME, output_filename, NULL);
  /* Run the helper */
  result = do_spawn (scheme, command_filename, *output_filename, &error_output,
                     &exit_status, dialog);
  /* If the exec went well, check for errors in the output */
  if (result)
    {
      result = !check_errors (exit_status, error_output, *output_filename,
                              protocol_version (scheme), dialog);
    }
  close (output_fd);
  g_free (error_output);
  /* Destroy the dialog */
  gtk_widget_destroy (dialog);

  return result;
}

/* Public functions */

gboolean
server_send_keys (const gchar *server, const gchar *keyid,
                  gpgme_data_t data, GtkWidget *parent)
{
  gchar *keyserver = g_strdup (server);
  gchar *command_filename, *output_filename;
  int command_fd;
  FILE *command;
  gchar *scheme, *host, *port, *opaque;
  gboolean success;

  /* Parse the URI */
  if (!parse_keyserver_uri (keyserver, &scheme, &host, &port, &opaque))
    {
      gpa_window_error (_("The keyserver you specified is not valid"), parent);
      return FALSE;
    }
  /* Create a temp command file */
  command_fd = g_file_open_tmp (COMMAND_TEMP_NAME, &command_filename, NULL);
  command = fdopen (command_fd, "w");
  /* Write the command to the file */
  write_command (command, scheme, host, port, opaque, "SEND");
  /* Write the keys to the file */
  fprintf (command, "\nKEY %s BEGIN\n", keyid);
  dump_data_to_file (data, command);
  fprintf (command, "\nKEY %s END\n", keyid);
  fclose (command);
  success = invoke_helper (server, scheme, command_filename,
                           &output_filename, parent);
  g_free (keyserver);
  /* Delete temp files */
  unlink (command_filename);
  g_free (command_filename);
  unlink (output_filename);
  g_free (output_filename);

  return success;
}

gboolean
server_get_key (const gchar *server, const gchar *keyid,
                gpgme_data_t *data, GtkWidget *parent)
{
  gchar *keyserver = g_strdup (server);
  gchar *command_filename, *output_filename;
  int command_fd;
  FILE *command;
  gchar *scheme, *host, *port, *opaque;
  int success;
  gpg_error_t err;

  /* Parse the URI */
  if (!parse_keyserver_uri (keyserver, &scheme, &host, &port, &opaque))
    {
      /* Create an empty gpgme_data_t, so that we always return a valid one */
      err = gpgme_data_new (data);
      if (err)
        gpa_gpgme_error (err);
      gpa_window_error (_("The keyserver you specified is not valid"), parent);
      return FALSE;
    }
  /* Create a temp command file */
  command_fd = g_file_open_tmp (COMMAND_TEMP_NAME, &command_filename, NULL);
  command = fdopen (command_fd, "w");
  /* Write the command to the file */
  write_command (command, scheme, host, port, opaque, "GET");
  /* Write the keys to the file */
  fprintf (command, "0x%s\n", keyid);
  fclose (command);
  success = invoke_helper (server, scheme, command_filename,
                           &output_filename, parent);
  /* Read the output */
  /* No error checking: the import will take care of that. */
  err = gpa_gpgme_data_new_from_file (data, output_filename, parent);
  if (err)
    gpa_gpgme_error (err);
  g_free (keyserver);
  /* Delete temp files */
  unlink (command_filename);
  g_free (command_filename);
  unlink (output_filename);
  g_free (output_filename);

  return success;
}
