/* server_access.c - The GNU Privacy Assistant
 *      Copyright (C) 2002, Miguel Coca.
 *
 * This file is part of GPA
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

#include "gpa.h"
#include <glib.h>
#include <assert.h>
#include <ctype.h>

/* For unlink() */
#ifdef G_OS_UNIX
#include <unistd.h>
#else
#include <io.h>
#endif

#include "gtktools.h"
#include "server_access.h"

#define COMMAND_TEMP_NAME "gpa-com-XXXXXX"
#define OUTPUT_TEMP_NAME "gpa-out-XXXXXX"

/* Internal API */

/* Code adapted from GnuPG (file g10/keyserver.c) */
gboolean 
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

static void
write_command (FILE *file, const char *host, const char *port, 
	       const char *opaque, const char *command)
{
  fprintf (file, "%s\n", "VERSION 0");
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
			    GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, 
			    GTK_MESSAGE_INFO, GTK_BUTTONS_NONE,
			    _("Connecting to server \"%s\".\n"
			      "Please wait."), server);
  /* FIXME: The dialog is not displayed */
  gtk_widget_show_all (dialog);
  return dialog;
}

static int
invoke_helper (const gchar *scheme, gchar *command_filename, 
	       gchar **output_filename, GtkWidget *parent)
{
  gchar *helper, *helper_path;
  gchar *helper_argv[] = {NULL, "-o", NULL, NULL, NULL};
  GError *error = NULL;
  int exit_status;
  int output_fd;
  gchar *helper_stderr;
  
  /* Open the output file */
  output_fd = g_file_open_tmp (OUTPUT_TEMP_NAME, output_filename, NULL);
  /* Invoke the keyserver helper */
  helper = g_strdup_printf ("gpgkeys_%s", scheme);
  helper_path = g_build_filename (GPA_KEYSERVER_HELPERS_DIR, helper, NULL);
  helper_argv[0] = helper_path;
  helper_argv[2] = *output_filename;
  helper_argv[3] = command_filename;
  g_spawn_sync (NULL, helper_argv, NULL, 
		G_SPAWN_STDOUT_TO_DEV_NULL, NULL, NULL, NULL, 
		&helper_stderr, &exit_status, &error);
  if (error)
    {
      gpa_window_error (_("There is no plugin available for the keyserver\n"
			  "protocol you specified."), parent);
    }
  else if (exit_status)
    {
      gchar *message = g_strdup_printf (_("An error ocurred while contacting "
					  "the server:\n\n%s"), helper_stderr);
      gpa_window_error (message, parent);
    }
  close (output_fd);
  g_free (helper);
  g_free (helper_path);
  g_free (helper_stderr);

  return exit_status;
}

/* Public functions */

void server_send_keys (const gchar *server, const gchar *keyid,
		       GpgmeData data, GtkWidget *parent)
{
  gchar *keyserver = g_strdup (server);
  gchar *command_filename, *output_filename;
  int command_fd;
  FILE *command, *output;
  GtkWidget *dialog;
  gchar *scheme, *host, *port, *opaque;
  int exit_status;

  /* Display a pretty dialog */
  dialog = wait_dialog (server, parent);
  /* Parse the URI */
  if (!parse_keyserver_uri (keyserver, &scheme, &host, &port, &opaque))
    {
      gpa_window_error (_("The keyserver you specified is not valid"), dialog);
      return;
    }
  /* Create a temp command file */
  command_fd = g_file_open_tmp (COMMAND_TEMP_NAME, &command_filename, NULL);
  command = fdopen (command_fd, "w");
  /* Write the command to the file */
  write_command (command, host, port, opaque, "SEND");
  /* Write the keys to the file */
  fprintf (command, "\nKEY %s BEGIN\n", keyid);
  dump_data_to_file (data, command);
  fprintf (command, "\nKEY %s END\n", keyid);
  fclose (command);
  exit_status = invoke_helper (scheme, command_filename, &output_filename,
			       parent);
  /* Read the output */
  /* FIXME: Currently we ignore the output entirely, and just trust the exit
   * code of the child. This should be fixed once the helpers do a more
   * detailed error reporting. */
  output = fopen (output_filename, "r");
  fclose (output);
  /* Destroy the dialog */
  gtk_widget_destroy (dialog);
  g_free (keyserver);
  /* Delete temp files */
  unlink (command_filename);
  g_free (command_filename);
  unlink (output_filename);
  g_free (output_filename);
}

void server_get_key (const gchar *server, const gchar *keyid,
		     GpgmeData *data, GtkWidget *parent)
{
  gchar *keyserver = g_strdup (server);
  gchar *command_filename, *output_filename;
  int command_fd;
  FILE *command;
  GtkWidget *dialog;
  gchar *scheme, *host, *port, *opaque;
  int exit_status;
  GpgmeError err;

  /* Display a pretty dialog */
  dialog = wait_dialog (server, parent);
  /* Parse the URI */
  if (!parse_keyserver_uri (keyserver, &scheme, &host, &port, &opaque))
    {
      gpa_window_error (_("The keyserver you specified is not valid"), dialog);
      return;
    }
  /* Create a temp command file */
  command_fd = g_file_open_tmp (COMMAND_TEMP_NAME, &command_filename, NULL);
  command = fdopen (command_fd, "w");
  /* Write the command to the file */
  write_command (command, host, port, opaque, "GET");
  /* Write the keys to the file */
  fprintf (command, "0x%s\n", keyid);
  fclose (command);
  exit_status = invoke_helper (scheme, command_filename, &output_filename,
			       parent);
  /* Read the output */
  /* No error checking: the import will take care of that. */
  err = gpgme_data_new_from_file (data, output_filename, 1);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  /* Destroy the dialog */
  gtk_widget_destroy (dialog);
  g_free (keyserver);
  /* Delete temp files */
  unlink (command_filename);
  g_free (command_filename);
  unlink (output_filename);
  g_free (output_filename);
}
