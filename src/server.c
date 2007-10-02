/* server.c -  The UI server part of GPA.
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

#include <gpgme.h>
#include <glib.h>
#include <assuan.h>

#include "gpa.h"
#include "i18n.h"
#include "gpastreamencryptop.h"



#define set_error(e,t) assuan_set_error (ctx, gpg_error (e), (t))


/* The object used to keep track of the a connection's state.  */
struct conn_ctrl_s;
typedef struct conn_ctrl_s *conn_ctrl_t;
struct conn_ctrl_s
{
  /* True if we are currently processing a command.  */
  int in_command;

  /* NULL or continuation function for a command.  */
  void (*cont_cmd) (assuan_context_t, gpg_error_t);

  /* File descriptors used by the gpgme callbacks.  */
  int input_fd;
  int output_fd;

  /* Channels used with the gpgme callbacks.  */
  GIOChannel *input_channel;
  GIOChannel *output_channel;

  /* Gpgme Data objects.  */
  gpgme_data_t input_data;
  gpgme_data_t output_data;

};



static assuan_sock_nonce_t socket_nonce;




/* Test whether LINE contains thye option NAME.  An optional argument
   of the option is ignored.  For example with NAME being "--protocol"
   this function returns true for "--protocol" as well as for
   "--protocol=foo".  The returned pointer points right behind the
   option name, which may be an equal sign, Nul or a space.  If tehre
   is no option NAME, false (i.e. NULL) is returned.
*/
static const char *
has_option_name (const char *line, const char *name)
{
  const char *s;
  int n = strlen (name);

  s = strstr (line, name);
  return (s && (s == line || spacep (s-1))
          && (!s[n] || spacep (s+n) || s[n] == '=')) ? (s+n) : NULL;
}

/* Check whether LINE coontains the option NAME.  */
static int
has_option (const char *line, const char *name)
{
  const char *s;
  int n = strlen (name);
  
  s = strstr (line, name);
  return (s && (s == line || spacep (s-1)) && (!s[n] || spacep (s+n)));
}

/* Skip over options. */
static char *
skip_options (char *line)
{
  while (spacep (line))
    line++;
  while ( *line == '-' && line[1] == '-' )
    {
      while (*line && !spacep (line))
        line++;
      while (spacep (line))
        line++;
    }
  return line;
}



static ssize_t
my_gpgme_read_cb (void *opaque, void *buffer, size_t size)
{
  conn_ctrl_t ctrl = opaque;
  GIOStatus  status;
  size_t nread;
  int retval;

  status = g_io_channel_read_chars (ctrl->input_channel, buffer, size,
                                    &nread, NULL);
  if (status == G_IO_STATUS_AGAIN
      || (status == G_IO_STATUS_NORMAL && !nread))
    {
      errno = EAGAIN;
      retval = -1;
    }
  else if (status == G_IO_STATUS_NORMAL)
    retval = (int)nread;
  else if (status == G_IO_STATUS_EOF)
    retval = 0;
  else
    {
      errno = EIO;
      retval = 1;
    }

  return retval;
}


static ssize_t
my_gpgme_write_cb (void *opaque, const void *buffer, size_t size)
{
  conn_ctrl_t ctrl = opaque;
  GIOStatus  status;
  size_t nwritten;
  int retval;

  status = g_io_channel_write_chars (ctrl->output_channel, buffer, size,
                                     &nwritten, NULL);
  if (status == G_IO_STATUS_AGAIN)
    {
      errno = EAGAIN;
      retval = -1;
    }
  else if (status == G_IO_STATUS_NORMAL)
    retval = (int)nwritten;
  else
    {
      errno = EIO;
      retval = 1;
    }

  return retval;
}


static struct gpgme_data_cbs my_gpgme_data_cbs =
  { 
    my_gpgme_read_cb,
    my_gpgme_write_cb,
    NULL,
    NULL
  };






/* Continuation for cmd_encrypt.  */
void
cont_encrypt (assuan_context_t ctx, gpg_error_t err)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);

  g_debug ("cont_encrypt called with with ERR=%s <%s>",
           gpg_strerror (err), gpg_strsource (err));

  gpgme_data_release (ctrl->input_data); ctrl->input_data = NULL;
  gpgme_data_release (ctrl->output_data); ctrl->output_data = NULL;
  if (ctrl->input_channel)
    {
      g_io_channel_shutdown (ctrl->input_channel, 0, NULL);
      ctrl->input_channel = NULL;
    }
  if (ctrl->output_channel)
    {
      g_io_channel_shutdown (ctrl->output_channel, 0, NULL);
      ctrl->output_channel = NULL;
    }
  assuan_process_done (ctx, err);
}


/* ENCRYPT --protocol=OPENPGP|CMS

   Encrypt the data received on INPUT to OUTPUT.
*/
static int 
cmd_encrypt (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err;
  gpgme_protocol_t protocol = GPGME_PROTOCOL_OpenPGP;
  GpaStreamEncryptOperation *op;


  if (has_option (line, "--protocol=OpenPGP"))
    ; /* This is the default.  */
  else if (has_option (line, "--protocol=CMS"))
    protocol = GPGME_PROTOCOL_CMS;
  else  if (has_option_name (line, "--protocol"))
    {
      err = set_error (GPG_ERR_ASS_PARAMETER, "invalid protocol");
      goto leave;
    }
  
  line = skip_options (line);
  if (*line)
    {
      err = set_error (GPG_ERR_ASS_SYNTAX, NULL);
      goto leave;
    }

  ctrl->input_fd = translate_sys2libc_fd (assuan_get_input_fd (ctx), 0);
  if (ctrl->input_fd == -1)
    {
      err = set_error (GPG_ERR_ASS_NO_INPUT, NULL);
      goto leave;
    }
  ctrl->output_fd = translate_sys2libc_fd (assuan_get_output_fd (ctx), 1);
  if (ctrl->output_fd == -1)
    {
      err = set_error (GPG_ERR_ASS_NO_OUTPUT, NULL);
      goto leave;
    }

#ifdef HAVE_W32_SYSTEM
  ctrl->input_channel = g_io_channel_win32_new_socket (ctrl->input_fd);
#else
  ctrl->input_channel = g_io_channel_unix_new (ctrl->input_fd);
#endif
  if (!ctrl->input_channel)
    {
      g_debug ("error creating input channel");
      err = GPG_ERR_EIO;
      goto leave;
    }
  g_io_channel_set_encoding (ctrl->input_channel, NULL, NULL);
  g_io_channel_set_buffered (ctrl->input_channel, FALSE);

#ifdef HAVE_W32_SYSTEM
  ctrl->output_channel = g_io_channel_win32_new_socket (ctrl->output_fd);
#else
  ctrl->output_channel = g_io_channel_unix_new (ctrl->output_fd);
#endif
  if (!ctrl->output_channel)
    {
      g_debug ("error creating output channel");
      err = GPG_ERR_EIO;
      goto leave;
    }
  g_io_channel_set_encoding (ctrl->output_channel, NULL, NULL);
  g_io_channel_set_buffered (ctrl->output_channel, FALSE);


  err = gpgme_data_new_from_cbs (&ctrl->input_data, &my_gpgme_data_cbs, ctrl);
  if (err)
    goto leave;
  err = gpgme_data_new_from_cbs (&ctrl->output_data, &my_gpgme_data_cbs, ctrl);
  if (err)
    goto leave;

  ctrl->cont_cmd = cont_encrypt;
  op = gpa_stream_encrypt_operation_new (NULL, ctrl->input_data, 
                                         ctrl->output_data, ctx);
  g_signal_connect (G_OBJECT (op), "completed",
                    G_CALLBACK (g_object_unref), NULL);
  return gpg_error (GPG_ERR_UNFINISHED);

 leave:
  gpgme_data_release (ctrl->input_data); ctrl->input_data = NULL;
  gpgme_data_release (ctrl->output_data); ctrl->output_data = NULL;
  if (ctrl->input_channel)
    {
      g_io_channel_shutdown (ctrl->input_channel, 0, NULL);
      ctrl->input_channel = NULL;
    }
  if (ctrl->output_channel)
    {
      g_io_channel_shutdown (ctrl->output_channel, 0, NULL);
      ctrl->output_channel = NULL;
    }
  assuan_close_input_fd (ctx);
  assuan_close_output_fd (ctx);
  return assuan_process_done (ctx, err);
}




/* GETINFO <what>

   Multipurpose function to return a variety of information.
   Supported values for WHAT are:

     version     - Return the version of the program.
     pid         - Return the process id of the server.
 */
static int
cmd_getinfo (assuan_context_t ctx, char *line)
{
  gpg_error_t err;

  if (!strcmp (line, "version"))
    {
      const char *s = PACKAGE_NAME " " PACKAGE_VERSION;
      err = assuan_send_data (ctx, s, strlen (s));
    }
  else if (!strcmp (line, "pid"))
    {
      char numbuf[50];

      snprintf (numbuf, sizeof numbuf, "%lu", (unsigned long)getpid ());
      err = assuan_send_data (ctx, numbuf, strlen (numbuf));
    }
  else
    err = set_error (GPG_ERR_ASS_PARAMETER, "unknown value for WHAT");

  return assuan_process_done (ctx, err);
}




/* Tell the assuan library about our commands.   */
static int
register_commands (assuan_context_t ctx)
{
  static struct {
    const char *name;
    int (*handler)(assuan_context_t, char *line);
  } table[] = {
    { "INPUT",   NULL },
    { "OUTPUT",   NULL },
    { "ENCRYPT", cmd_encrypt },
    { "GETINFO", cmd_getinfo },
    { NULL }
  };
  int i, rc;

  for (i=0; table[i].name; i++)
    {
      rc = assuan_register_command (ctx, table[i].name, table[i].handler);
      if (rc)
        return rc;
    } 

  return 0;
}


/* Prepare for a new connection on descriptor FD.  */
static assuan_context_t
connection_startup (int fd)
{
  gpg_error_t err;
  assuan_context_t ctx;
  conn_ctrl_t ctrl;

  /* Get an assuan context for the already accepted file descriptor
     FD.  */
  err = assuan_init_socket_server_ext (&ctx, ASSUAN_INT2FD(fd), 2);
  if (err)
    {
      g_debug ("failed to initialize the new connection: %s",
               gpg_strerror (err));
      return NULL;
    }
  err = register_commands (ctx);
  if (err)
    {
      g_debug ("failed to register commands with Assuan: %s",
               gpg_strerror (err));
      assuan_deinit_server (ctx);
      return NULL;
    }

  ctrl = g_malloc0 (sizeof *ctrl);
  assuan_set_pointer (ctx, ctrl);
  assuan_set_log_stream (ctx, stderr);

  return ctx;
}


/* Finish a connection.  This releases all resources and needs to be
   called becore the file descriptor is closed.  */
static void
connection_finish (assuan_context_t ctx)
{
  if (ctx)
    {
      conn_ctrl_t ctrl = assuan_get_pointer (ctx);

      gpgme_data_release (ctrl->input_data);
      gpgme_data_release (ctrl->output_data);
      if (ctrl->input_channel)
        {
          g_io_channel_shutdown (ctrl->input_channel, 0, NULL);
          ctrl->input_channel = NULL;
        }
      if (ctrl->output_channel)
        {
          g_io_channel_shutdown (ctrl->output_channel, 0, NULL);
          ctrl->output_channel = NULL;
        }
      assuan_deinit_server (ctx);
      g_free (ctrl);
    }
}


/* If the assuan context CTX has a registered continuation function,
   run it.  */
void
gpa_run_server_continuation (assuan_context_t ctx, gpg_error_t err)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  void (*cont_cmd) (assuan_context_t, gpg_error_t);

  if (!ctrl)
    {
      g_debug ("no context in gpa_run_server_continuation");
      return;
    }
  if (!ctrl->cont_cmd)
    {
      g_debug ("no continuation in gpa_run_server_continuation");
      return;
    }
  cont_cmd = ctrl->cont_cmd;
  ctrl->cont_cmd = NULL;
  g_debug ("calling gpa_run_server_continuation (%s)", gpg_strerror (err));
  cont_cmd (ctx, err);
  g_debug ("leaving gpa_run_server_continuation");
}


/* This function is called by the main event loop if data can be read
   from the status channel.  */
static gboolean 
receive_cb (GIOChannel *channel, GIOCondition condition, void *data)
{
  assuan_context_t ctx = data;
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err;

  assert (ctrl);
  if (condition & G_IO_IN)
    {
      g_debug ("receive_cb");
      if (ctrl->cont_cmd)
        {
          g_debug ("  input received while waiting for continuation");
          g_usleep (2000000);
        }
      else if (ctrl->in_command)
        {
          g_debug ("  input received while still processing command");
          g_usleep (2000000);
        }
      else
        {
          ctrl->in_command++;
          err = assuan_process_next (ctx);
          ctrl->in_command--;
          g_debug ("assuan_process_next returned: %s",
                   err == -1? "EOF": gpg_strerror (err));
          if (gpg_err_code (err) == GPG_ERR_EOF || err == -1)
            {
              connection_finish (ctx);
              /* FIXME: what about the socket? */
              return FALSE; /* Remove from the watch.  */
            }
          else if (gpg_err_code (err) == GPG_ERR_UNFINISHED)
            {
              if ( !ctrl->cont_cmd)
                {
                  /* It is quite possible that some other subsystem
                     returns that error code.  Tell the user about
                     this curiosity and finish the command.  */
                  g_debug ("note: Unfinished error code not emitted by us");
                  assuan_process_done (ctx, err);
                }
            }
          else 
            assuan_process_done (ctx, err);
        }
    }
  return TRUE;
}


/* This function is called by the main event loop if the listen fd is
   readable.  The function runs the accept and prepares the
   connection.  */
static gboolean 
accept_connection_cb (GIOChannel *listen_channel, 
                      GIOCondition condition, void *data)
{
  gpg_error_t err;
  int listen_fd, fd;
  struct sockaddr_un paddr;
  socklen_t plen = sizeof paddr;
  assuan_context_t ctx;
  GIOChannel *channel;
  unsigned int source_id;

  g_debug ("new connection request");
#ifdef HAVE_W32_SYSTEM
  listen_fd = g_io_channel_win32_get_fd (listen_channel);
#else
  listen_fd = g_io_channel_unix_get_fd (listen_channel);
#endif
  fd = accept (listen_fd, (struct sockaddr *)&paddr, &plen);
  if (fd == -1)
    {
      g_debug ("error accepting connection: %s", strerror (errno));
      goto leave;
    }
  if (assuan_sock_check_nonce (ASSUAN_INT2FD(fd), &socket_nonce))
    {
      g_debug ("new connection at fd %d refused", fd); 
      goto leave;
    }

  g_debug ("new connection at fd %d", fd);
  ctx = connection_startup (fd);
  if (!ctx)
    goto leave;

#ifdef HAVE_W32_SYSTEM
  channel = g_io_channel_win32_new_socket (fd);
#else
  channel = g_io_channel_unix_new (fd);
#endif
  if (!channel)
    {
      g_debug ("error creating a channel for fd %d\n", fd);
      goto leave;
    }
  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_buffered (channel, FALSE);

  source_id = g_io_add_watch (channel, G_IO_IN, receive_cb, ctx);
  if (!source_id)
    {
      g_debug ("error creating watch for fd %d", fd);
      g_io_channel_shutdown (channel, 0, NULL);
      goto leave;
    }
  err = assuan_accept (ctx);
  if (err)
    {
      g_debug ("assuan accept failed: %s", gpg_strerror (err));
      g_io_channel_shutdown (channel, 0, NULL);
      goto leave;
    }
  g_debug ("connection at fd %d ready", fd);
  fd = -1;

 leave:
  if (fd != -1)
    assuan_sock_close (ASSUAN_INT2FD (fd));
  return TRUE; /* Keep the listen_fd in the event loop.  */
}



/* Startup the server.  */
void
gpa_start_server (void)
{
  char *socket_name;
  int rc;
  assuan_fd_t fd;
  struct sockaddr_un serv_addr;
  socklen_t serv_addr_len = sizeof serv_addr;
  GIOChannel *channel;
  unsigned int source_id;
  
  socket_name = g_build_filename (gnupg_homedir, "S.uiserver", NULL);
  if (strlen (socket_name)+1 >= sizeof serv_addr.sun_path ) 
    {
      g_debug ("name of socket too long\n");
      g_free (socket_name);
      return;
    }
  g_debug ("using server socket `%s'", socket_name);
    
  fd = assuan_sock_new (AF_UNIX, SOCK_STREAM, 0);
  if (fd == ASSUAN_INVALID_FD)
    {
      g_debug ("can't create socket: %s\n", strerror(errno));
      g_free (socket_name);
      return;
    }

  memset (&serv_addr, 0, sizeof serv_addr);
  serv_addr.sun_family = AF_UNIX;
  strcpy (serv_addr.sun_path, socket_name);
  serv_addr_len = (offsetof (struct sockaddr_un, sun_path)
                   + strlen(serv_addr.sun_path) + 1);

  rc = assuan_sock_bind (fd, (struct sockaddr*) &serv_addr, serv_addr_len);
  if (rc == -1 && errno == EADDRINUSE)
    {
      remove (socket_name);
      rc = assuan_sock_bind (fd, (struct sockaddr*) &serv_addr, serv_addr_len);
    }
  if (rc != -1 && (rc=assuan_sock_get_nonce ((struct sockaddr*) &serv_addr,
                                             serv_addr_len, &socket_nonce)))
    g_debug ("error getting nonce for the socket");
  if (rc == -1)
    {
      g_debug ("error binding socket to `%s': %s\n",
               serv_addr.sun_path, strerror (errno) );
      assuan_sock_close (fd);
      g_free (socket_name);
      return;
    }
  g_free (socket_name);
  socket_name = NULL;

  if (listen (ASSUAN_FD2INT (fd), 5) == -1)
    {
      g_debug ("listen() failed: %s\n", strerror (errno));
      assuan_sock_close (fd);
      return;
    }

#ifdef HAVE_W32_SYSTEM
  channel = g_io_channel_win32_new_socket (ASSUAN_FD2INT(fd));
#else
  channel = g_io_channel_unix_new (fd);
#endif
  if (!channel)
    {
      g_debug ("error creating a new listening channel\n");
      assuan_sock_close (fd);
      return;
    }
  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_buffered (channel, FALSE);

  source_id = g_io_add_watch (channel, G_IO_IN, accept_connection_cb, NULL);
  if (!source_id)
    {
      g_debug ("error creating watch for listening channel\n");
      g_io_channel_shutdown (channel, 0, NULL);
      assuan_sock_close (fd);
      return;
    }

}
