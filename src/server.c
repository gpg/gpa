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

#ifdef HAVE_W32_SYSTEM
# include "w32-afunix.h"
#else
# include <sys/socket.h>
# include <sys/un.h>
#endif

#include "gpa.h"
#include "i18n.h"
#include "gpafileencryptop.h"


#ifdef HAVE_W32_SYSTEM
#define myclosesock(a)  _w32_close ((a))
#else
#define myclosesock(a)  close ((a))
#endif

#define set_error(e,t) assuan_set_error (ctx, gpg_error (e), (t))


/* The object used to keep track of the a connection's state.  */
struct conn_ctrl_s;
typedef struct conn_ctrl_s *conn_ctrl_t;
struct conn_ctrl_s
{
  /* NULL or continuation function or a command.  */
  void (*cont_cmd) (assuan_context_t, gpg_error_t);
};




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





/* Continuation for cmd_encrypt.  */
void
cont_encrypt (assuan_context_t ctx, gpg_error_t err)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);

  g_debug ("cont_encrypt called with with ERR=%s <%s>",
           gpg_strerror (err), gpg_strsource (err));

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
  int inp_fd, out_fd;

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

/*   inp_fd = translate_sys2libc_fd (assuan_get_input_fd (ctx), 0); */
/*   if (inp_fd == -1) */
/*     { */
/*       err = set_error (GPG_ERR_ASS_NO_INPUT, NULL); */
/*       goto leave; */
/*     } */
/*   out_fd = translate_sys2libc_fd (assuan_get_output_fd (ctx), 1); */
/*   if (out_fd == -1) */
/*     { */
/*       err = set_error (GPG_ERR_ASS_NO_OUTPUT, NULL); */
/*       goto leave; */
/*     } */

  ctrl->cont_cmd = cont_encrypt;
  {
    GList *files = g_list_append (NULL, g_strdup ("test.txt"));
    GpaFileEncryptOperation *op;

    op = gpa_file_encrypt_operation_new_for_server (files, ctx);
    g_signal_connect (G_OBJECT (op), "completed",
                      G_CALLBACK (g_object_unref), NULL);
  }
  return gpg_error (GPG_ERR_UNFINISHED);

 leave:
  assuan_close_input_fd (ctx);
  assuan_close_output_fd (ctx);
  return assuan_process_done (ctx, err);
}




/* GETINFO <what>

   Multipurpose function to return a variety of information.
   Supported values for WHAT are:

     version     - Return the version of the program.
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
    { "ENCRYPT",        cmd_encrypt },
    { "GETINFO",        cmd_getinfo },
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
  err = assuan_init_socket_server_ext (&ctx, fd, 2);
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
      else
        {
          err = assuan_process_next (ctx);
          g_debug ("assuan_process_next returned: %s",
                   err == -1? "EOF": gpg_strerror (err));
          if (gpg_err_code (err) == GPG_ERR_EOF || err == -1)
            {
              connection_finish (ctx);
              /* FIXME: what about the socket? */
              return FALSE; /* Remove from the watch.  */
            }
          else if (gpg_err_code (err) == GPG_ERR_UNFINISHED
                   && !ctrl->cont_cmd)
            {
              /* It is quite possible that some other subsystem
                 retruns that erro code.  Note the user about this
                 curiosity.  */
              g_debug ("note: Unfinished error code not emitted by us");
            }
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
    myclosesock (fd);
  return TRUE; /* Keep the listen_fd in the event loop.  */
}



/* Startup the server.  */
void
gpa_start_server (void)
{
  char *socket_name;
  int rc;
  int fd;
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
    
#ifdef HAVE_W32_SYSTEM
  fd = _w32_sock_new (AF_UNIX, SOCK_STREAM, 0);
#else
  fd = socket (AF_UNIX, SOCK_STREAM, 0);
#endif
  if (fd == -1)
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

#ifdef HAVE_W32_SYSTEM
  rc = _w32_sock_bind (fd, (struct sockaddr*) &serv_addr, serv_addr_len);
  if (rc == -1 && errno == WSAEADDRINUSE)
    {
      remove (socket_name);
      rc = _w32_sock_bind (fd, (struct sockaddr*) &serv_addr, serv_addr_len);
    }
#else
  rc = bind (fd, (struct sockaddr*)&serv_addr, serv_addr_len);
  if (rc == -1 && errno == EADDRINUSE)
    {
      remove (socket_name);
      rc = bind (fd, (struct sockaddr*)&serv_addr, serv_addr_len);
    }
#endif
  if (rc == -1)
    {
      g_debug ("error binding socket to `%s': %s\n",
               serv_addr.sun_path, strerror (errno) );
      myclosesock (fd);
      g_free (socket_name);
      return;
    }
  g_free (socket_name);
  socket_name = NULL;

  if (listen (fd, 5) == -1)
    {
      g_debug ("listen() failed: %s\n", strerror (errno));
      myclosesock (fd);
      return;
    }

#ifdef HAVE_W32_SYSTEM
  channel = g_io_channel_win32_new_socket (fd);
#else
  channel = g_io_channel_unix_new (fd);
#endif
  if (!channel)
    {
      g_debug ("error creating a new listening channel\n");
      myclosesock (fd);
      return;
    }
  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_buffered (channel, FALSE);

  source_id = g_io_add_watch (channel, G_IO_IN, accept_connection_cb, NULL);
  if (!source_id)
    {
      g_debug ("error creating watch for listening channel\n");
      g_io_channel_shutdown (channel, 0, NULL);
      myclosesock (fd);
      return;
    }

}
