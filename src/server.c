/* server.c -  The UI server part of GPA.
   Copyright (C) 2007, 2008 g10 Code GmbH

   This file is part of GPA

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#ifndef HAVE_W32_SYSTEM
# include <sys/socket.h>
# include <sys/un.h>
#endif /*HAVE_W32_SYSTEM*/

#include <gpgme.h>
#include <glib.h>
#include <assuan.h>

#include "gpa.h"
#include "i18n.h"
#include "gpastreamencryptop.h"
#include "gpastreamsignop.h"
#include "gpastreamdecryptop.h"
#include "gpastreamverifyop.h"


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

  /* Flag indicating that the client died while a continuation was
     still registyered.  */
  int client_died;

  /* This is a helper to detect that the unfinished erroe code actually
     comes from our command handler.  */
  int is_unfinished;

  /* An GPAOperation object.  */
  GpaOperation *gpa_op;

  /* File descriptors used by the gpgme callbacks.  */
  int input_fd;
  int output_fd;

  /* File descriptor set with the MESSAGE command.  */
  int message_fd;

  /* Channels used with the gpgme callbacks.  */
  GIOChannel *input_channel;
  GIOChannel *output_channel;
  GIOChannel *message_channel;

  /* List of collected recipients.  */
  GSList *recipients;

  /* Array of keys already prepared for RECIPIENTS.  */
  gpgme_key_t *recipient_keys;

  /* The protocol as selected by the user.  */
  gpgme_protocol_t selected_protocol;

  /* The current sender address (malloced) and a flag telleing whether
     the sender ist just informational. */
  gchar *sender;
  int sender_just_info;

  /* Session information:  A session number and a malloced title or NULL.  */
  unsigned int session_number;
  char *session_title;
};


/* The number of active connections.  */
static int connection_counter;

/* A flag requesting a shutdown.  */
static gboolean shutdown_pending;


/* The nonce used by the server connection.  This nonce is required
   uner Windows to emulate Unix Domain Sockets.  This is managed by
   libassuan but we need to store the nonce in the application.  Under
   Unix this is just a stub.  */
static assuan_sock_nonce_t socket_nonce;


/* Forward declarations.  */
static void run_server_continuation (assuan_context_t ctx, gpg_error_t err);





static int
not_finished (conn_ctrl_t ctrl)
{
  ctrl->is_unfinished = 1;
  return gpg_error (GPG_ERR_UNFINISHED);
}


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

/* Check whether LINE contains the option NAME.  */
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

/* Helper to be used as a GFunc for free. */
static void
free_func (void *p, void *dummy)
{
  (void)dummy;
  g_free (p);
}


static ssize_t
my_gpgme_read_cb (void *opaque, void *buffer, size_t size)
{
  conn_ctrl_t ctrl = opaque;
  GIOStatus  status;
  size_t nread;
  int retval;

/*   g_debug ("my_gpgme_read_cb: requesting %d bytes\n", (int)size); */
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
      retval = -1;
    }
/*   g_debug ("my_gpgme_read_cb: got status=%x, %d bytes, retval=%d\n",  */
/*            status, (int)size, retval); */

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


static ssize_t
my_gpgme_message_read_cb (void *opaque, void *buffer, size_t size)
{
  conn_ctrl_t ctrl = opaque;
  GIOStatus  status;
  size_t nread;
  int retval;

  status = g_io_channel_read_chars (ctrl->message_channel, buffer, size,
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
      retval = -1;
    }

  return retval;
}

static struct gpgme_data_cbs my_gpgme_message_cbs =
  { 
    my_gpgme_message_read_cb,
    NULL,
    NULL,
    NULL
  };


static ssize_t
my_devnull_write_cb (void *opaque, const void *buffer, size_t size)
{
  return size;
}


static struct gpgme_data_cbs my_devnull_data_cbs =
  { 
    NULL,
    my_devnull_write_cb,
    NULL,
    NULL
  };


/* Release the recipients stored in the connection context. */
static void
release_recipients (conn_ctrl_t ctrl)
{
  if (ctrl->recipients)
    {
      g_slist_foreach (ctrl->recipients, free_func, NULL);
      g_slist_free (ctrl->recipients);
      ctrl->recipients = NULL;
    }
}

static void
release_keys (gpgme_key_t *keys)
{
  if (keys)
    {
      int idx;
      
      for (idx=0; keys[idx]; idx++)
        gpgme_key_unref (keys[idx]);
      g_free (keys);
    }
}


/* Reset already prepared keys.  */
static void
reset_prepared_keys (conn_ctrl_t ctrl)
{
  release_keys (ctrl->recipient_keys);
  ctrl->recipient_keys = NULL;
  ctrl->selected_protocol = GPGME_PROTOCOL_UNKNOWN;
}


/* Helper to parse an protocol option.  */
static gpg_error_t
parse_protocol_option (assuan_context_t ctx, char *line, int mandatory,
                       gpgme_protocol_t *r_protocol)
{
  *r_protocol = GPGME_PROTOCOL_UNKNOWN;
  if (has_option (line, "--protocol=OpenPGP"))
    *r_protocol = GPGME_PROTOCOL_OpenPGP;
  else if (has_option (line, "--protocol=CMS"))
    *r_protocol = GPGME_PROTOCOL_CMS;
  else if (has_option_name (line, "--protocol"))
    return set_error (GPG_ERR_ASS_PARAMETER, "invalid protocol");
  else if (mandatory)
    return set_error (GPG_ERR_ASS_PARAMETER, "no protocol specified");

  return 0;
}


static void 
close_message_fd (conn_ctrl_t ctrl)
{
  if (ctrl->message_fd != -1)
    {
      close (ctrl->message_fd);
      ctrl->message_fd = -1;
    }
}


static void
finish_io_streams (assuan_context_t ctx,
                   gpgme_data_t *r_input_data, gpgme_data_t *r_output_data,
                   gpgme_data_t *r_message_data)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);

  if (r_input_data)
    gpgme_data_release (*r_input_data); 
  if (r_output_data)
    gpgme_data_release (*r_output_data);
  if (r_message_data)
    gpgme_data_release (*r_message_data); 
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
  if (ctrl->message_channel)
    {
      g_io_channel_shutdown (ctrl->message_channel, 0, NULL);
      ctrl->message_channel = NULL;
    }

  close_message_fd (ctrl);
  assuan_close_input_fd (ctx);
  assuan_close_output_fd (ctx);
  ctrl->input_fd = -1;
  ctrl->output_fd = -1;
}


/* Translate the input and output file descriptors and return an error
   if they are not set.  */
static gpg_error_t
translate_io_streams (assuan_context_t ctx)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);

  ctrl->input_fd = translate_sys2libc_fd (assuan_get_input_fd (ctx), 0);
  if (ctrl->input_fd == -1)
    return set_error (GPG_ERR_ASS_NO_INPUT, NULL);
  ctrl->output_fd = translate_sys2libc_fd (assuan_get_output_fd (ctx), 1);
  if (ctrl->output_fd == -1)
    return set_error (GPG_ERR_ASS_NO_OUTPUT, NULL);
  return 0;
}


static gpg_error_t
prepare_io_streams (assuan_context_t ctx,
                    gpgme_data_t *r_input_data, gpgme_data_t *r_output_data,
                    gpgme_data_t *r_message_data)
{
  gpg_error_t err;
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);

  if (r_input_data)
    *r_input_data = NULL;
  if (r_output_data)
    *r_output_data = NULL;
  if (r_message_data)
    *r_message_data = NULL;

  if (ctrl->input_fd != -1 && r_input_data)
    {
#ifdef HAVE_W32_SYSTEM
      ctrl->input_channel = g_io_channel_win32_new_fd (ctrl->input_fd);
#else
      ctrl->input_channel = g_io_channel_unix_new (ctrl->input_fd);
#endif
      if (!ctrl->input_channel)
        {
          g_debug ("error creating input channel");
          err = gpg_error (GPG_ERR_EIO);
          goto leave;
        }
      g_io_channel_set_encoding (ctrl->input_channel, NULL, NULL);
      g_io_channel_set_buffered (ctrl->input_channel, FALSE);
    }

  if (ctrl->output_fd != -1 && r_output_data)
    {
#ifdef HAVE_W32_SYSTEM
      ctrl->output_channel = g_io_channel_win32_new_fd (ctrl->output_fd);
#else
      ctrl->output_channel = g_io_channel_unix_new (ctrl->output_fd);
#endif
      if (!ctrl->output_channel)
        {
          g_debug ("error creating output channel");
          err = gpg_error (GPG_ERR_EIO);
          goto leave;
        }
      g_io_channel_set_encoding (ctrl->output_channel, NULL, NULL);
      g_io_channel_set_buffered (ctrl->output_channel, FALSE);
    }

  if (ctrl->message_fd != -1 && r_message_data)
    {
#ifdef HAVE_W32_SYSTEM
      ctrl->message_channel = g_io_channel_win32_new_fd (ctrl->message_fd);
#else
      ctrl->message_channel = g_io_channel_unix_new (ctrl->message_fd);
#endif
      if (!ctrl->message_channel)
        {
          g_debug ("error creating message channel");
          err = gpg_error (GPG_ERR_EIO);
          goto leave;
        }
      g_io_channel_set_encoding (ctrl->message_channel, NULL, NULL);
      g_io_channel_set_buffered (ctrl->message_channel, FALSE);
    }

  if (ctrl->input_channel)
    {
      err = gpgme_data_new_from_cbs (r_input_data, &my_gpgme_data_cbs, ctrl);
      if (err)
        goto leave;
    }
  if (ctrl->output_channel)
    {
      err = gpgme_data_new_from_cbs (r_output_data, &my_gpgme_data_cbs, ctrl);
      if (err)
        goto leave;
    }
  if (ctrl->message_channel)
    {
      err = gpgme_data_new_from_cbs (r_message_data,
				     &my_gpgme_message_cbs, ctrl);
      if (err)
        goto leave;
    }

 leave:
  if (err)
    finish_io_streams (ctx, r_input_data, r_output_data, r_message_data);
  return err;
}



/* SESSION <number> [<string>]

   The NUMBER is an arbitrary value, a server may use to associate
   simultaneous running sessions.  It is a 32 bit unsigned integer
   with 0 as a special value indicating that no session association
   shall be done.

   If STRING is given, the server may use this as the title of a
   window or, in the case of an email operation, to extract the
   sender's address. The string may contain spaces; thus no
   plus-escaping is used.

   This command may be used at any time and overrides the effect of
   the last command.  A RESET command undoes the effect of this
   command.
*/
static int
cmd_session (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  char *endp;

  line = skip_options (line);

  ctrl->session_number = strtoul (line, &endp, 10);
  for (line = endp; spacep (line); line++)
    ;
  xfree (ctrl->session_title);
  ctrl->session_title = *line? xstrdup (line) : NULL;

  return assuan_process_done (ctx, 0);
}



/*  RECIPIENT <recipient>

  Set the recipient for the encryption.  <recipient> is an RFC2822
  recipient name.  This command may or may not check the recipient for
  validity right away; if it does not (as here) all recipients are
  checked at the time of the ENCRYPT command.  All RECIPIENT commands
  are cumulative until a RESET or an successful ENCRYPT command.  */
static int
cmd_recipient (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err = 0;

  reset_prepared_keys (ctrl);

  if (*line)
    ctrl->recipients = g_slist_append (ctrl->recipients, xstrdup (line));

  return assuan_process_done (ctx, err);
}



/* MESSAGE FD=<n>

   Set the file descriptor to read a message which is used with
   detached signatures.
 */
static int 
cmd_message (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err;;
  assuan_fd_t sysfd;
  int fd;

  err = assuan_command_parse_fd (ctx, line, &sysfd);
  if (!err)
    {
      fd = translate_sys2libc_fd (sysfd, 0);
      if (fd == -1)
        err = set_error (GPG_ERR_ASS_NO_INPUT, NULL);
      else
        ctrl->message_fd = fd;
    }
  return assuan_process_done (ctx, err);
}





/* Continuation for cmd_encrypt.  */
static void
cont_encrypt (assuan_context_t ctx, gpg_error_t err)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);

  g_debug ("cont_encrypt called with with ERR=%s <%s>",
           gpg_strerror (err), gpg_strsource (err));

  finish_io_streams (ctx, NULL, NULL, NULL);
  if (!err)
    release_recipients (ctrl);
  assuan_process_done (ctx, err);
}


/* ENCRYPT --protocol=OpenPGP|CMS

   Encrypt the data received on INPUT to OUTPUT.
*/
static int 
cmd_encrypt (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err;
  gpgme_protocol_t protocol = 0;
  GpaStreamEncryptOperation *op;
  gpgme_data_t input_data = NULL;
  gpgme_data_t output_data = NULL;

  err = parse_protocol_option (ctx, line, 1, &protocol);
  if (err)
    goto leave;

  if (protocol != ctrl->selected_protocol)
    {
      if (ctrl->selected_protocol != GPGME_PROTOCOL_UNKNOWN)
        g_debug ("note: protocol does not match the one from PREP_ENCRYPT");
      reset_prepared_keys (ctrl);
    }

  line = skip_options (line);
  if (*line)
    {
      err = set_error (GPG_ERR_ASS_SYNTAX, NULL);
      goto leave;
    }

  err = translate_io_streams (ctx);
  if (err)
    goto leave;
  err = prepare_io_streams (ctx, &input_data, &output_data, NULL);
  if (err)
    goto leave;

  ctrl->cont_cmd = cont_encrypt;
  op = gpa_stream_encrypt_operation_new (NULL, input_data, output_data,
                                         ctrl->recipients, 
                                         ctrl->recipient_keys,
                                         protocol, 0);
  input_data = output_data = NULL;
  g_signal_connect_swapped (G_OBJECT (op), "completed",
			    G_CALLBACK (run_server_continuation), ctx);
  g_signal_connect (G_OBJECT (op), "completed",
                    G_CALLBACK (g_object_unref), NULL);
  g_signal_connect_swapped (G_OBJECT (op), "status",
			    G_CALLBACK (assuan_write_status), ctx);

  return not_finished (ctrl);

 leave:
  finish_io_streams (ctx, &input_data, &output_data, NULL);
  close_message_fd (ctrl);
  assuan_close_input_fd (ctx);
  assuan_close_output_fd (ctx);
  ctrl->input_fd = -1;
  ctrl->output_fd = -1;
  return assuan_process_done (ctx, err);
}



/* Continuation for cmd_prep_encrypt.  */
static void
cont_prep_encrypt (assuan_context_t ctx, gpg_error_t err)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);

  g_debug ("cont_prep_encrypt called with with ERR=%s <%s>",
           gpg_strerror (err), gpg_strsource (err));

  if (!err)
    {
      release_keys (ctrl->recipient_keys);
      ctrl->recipient_keys = gpa_stream_encrypt_operation_get_keys
        (GPA_STREAM_ENCRYPT_OPERATION (ctrl->gpa_op),
         &ctrl->selected_protocol);

      if (ctrl->recipient_keys)
        g_print ("received some keys\n");
      else
        g_print ("received no keys\n");
    }

  if (ctrl->gpa_op)
    {
      g_object_unref (ctrl->gpa_op);
      ctrl->gpa_op = NULL;
    }
  assuan_process_done (ctx, err);
}

/* PREP_ENCRYPT [--protocol=OpenPGP|CMS]

   Dummy encryption command used to check whether the given recipients
   are all valid and to tell the client the preferred protocol.  */
static int 
cmd_prep_encrypt (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err;
  gpgme_protocol_t protocol;
  GpaStreamEncryptOperation *op;

  err = parse_protocol_option (ctx, line, 0, &protocol);
  if (err)
    goto leave;

  line = skip_options (line);
  if (*line)
    {
      err = set_error (GPG_ERR_ASS_SYNTAX, NULL);
      goto leave;
    }

  reset_prepared_keys (ctrl);

  if (ctrl->gpa_op)
    {
      g_debug ("Oops: there is still an GPA_OP active\n");
      g_object_unref (ctrl->gpa_op);
      ctrl->gpa_op = NULL;
    }
  ctrl->cont_cmd = cont_prep_encrypt;
  op = gpa_stream_encrypt_operation_new (NULL, NULL, NULL,
                                         ctrl->recipients,
                                         ctrl->recipient_keys,
                                         protocol, 0);
  /* Store that instance for later use but also install a signal
     handler to unref it.  */
  g_object_ref (op);
  ctrl->gpa_op = GPA_OPERATION (op);
  g_signal_connect_swapped (G_OBJECT (op), "completed",
			    G_CALLBACK (run_server_continuation), ctx);
  g_signal_connect (G_OBJECT (op), "completed",
                    G_CALLBACK (g_object_unref), NULL);
  g_signal_connect_swapped (G_OBJECT (op), "status",
			    G_CALLBACK (assuan_write_status), ctx);

  return not_finished (ctrl);

 leave:
  return assuan_process_done (ctx, err);
}



/*  SENDER <email>

    EMAIL is the plain ASCII encoded address ("addr-spec" as per
    RFC-2822) enclosed in angle brackets.  The address set by this
    command is valid until a successful @code{SIGN} command or until a
    @code{RESET} command.  A second command overrides the effect of
    the first one; if EMAIL is not given the server shall use the
    default signing key.  */
static int
cmd_sender (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err = 0;

  ctrl->sender_just_info = has_option (line, "--info");
  line = skip_options (line);

  xfree (ctrl->sender);
  ctrl->sender = NULL;
  if (*line)
    ctrl->sender = xstrdup (line);

  return assuan_process_done (ctx, err);
}



/* Continuation for cmd_sign.  */
static void
cont_sign (assuan_context_t ctx, gpg_error_t err)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);

  g_debug ("cont_sign called with with ERR=%s <%s>",
           gpg_strerror (err), gpg_strsource (err));

  finish_io_streams (ctx, NULL, NULL, NULL);
  if (!err)
    {
      xfree (ctrl->sender);
      ctrl->sender = NULL;
    }
  assuan_process_done (ctx, err);
}


/* SIGN --protocol=OpenPGP|CMS [--detached]

   Sign the data received on INPUT to OUTPUT.
*/
static int 
cmd_sign (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err;
  gpgme_protocol_t protocol;
  gboolean detached;
  GpaStreamSignOperation *op;
  gpgme_data_t input_data = NULL;
  gpgme_data_t output_data = NULL;

  err = parse_protocol_option (ctx, line, 1, &protocol);
  if (err)
    goto leave;

  detached = has_option (line, "--detached");

  line = skip_options (line);
  if (*line)
    {
      err = set_error (GPG_ERR_ASS_SYNTAX, NULL);
      goto leave;
    }

  err = translate_io_streams (ctx);
  if (err)
    goto leave;
  err = prepare_io_streams (ctx, &input_data, &output_data, NULL);
  if (err)
    goto leave;

  ctrl->cont_cmd = cont_sign;
  op = gpa_stream_sign_operation_new (NULL, input_data, output_data,
                                      ctrl->sender, protocol, detached);
  input_data = output_data = NULL;
  g_signal_connect_swapped (G_OBJECT (op), "completed",
			    G_CALLBACK (run_server_continuation), ctx);
  g_signal_connect (G_OBJECT (op), "completed",
                    G_CALLBACK (g_object_unref), NULL);
  g_signal_connect_swapped (G_OBJECT (op), "status",
			    G_CALLBACK (assuan_write_status), ctx);

  return not_finished (ctrl);

 leave:
  finish_io_streams (ctx, &input_data, &output_data, NULL);
  close_message_fd (ctrl);
  assuan_close_input_fd (ctx);
  assuan_close_output_fd (ctx);
  ctrl->input_fd = -1;
  ctrl->output_fd = -1;
  return assuan_process_done (ctx, err);
}



/* Continuation for cmd_decrypt.  */
static void
cont_decrypt (assuan_context_t ctx, gpg_error_t err)
{
  g_debug ("cont_decrypt called with with ERR=%s <%s>",
           gpg_strerror (err), gpg_strsource (err));

  finish_io_streams (ctx, NULL, NULL, NULL);
  assuan_process_done (ctx, err);
}


/* DECRYPT --protocol=OpenPGP|CMS [--no-verify]

   Decrypt a message given by the source set with the INPUT command
   and write the plaintext to the sink set with the OUTPUT command.

   If the option --no-verify is given, the server should not try to
   verify a signature, in case the input data is an OpenPGP combined
   message.  */
static int 
cmd_decrypt (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err;
  gpgme_protocol_t protocol = 0;
  int no_verify;
  GpaStreamDecryptOperation *op;
  gpgme_data_t input_data = NULL;
  gpgme_data_t output_data = NULL;

  err = parse_protocol_option (ctx, line, 1, &protocol);
  if (err)
    goto leave;

  no_verify = has_option (line, "--no-verify");

  line = skip_options (line);
  if (*line)
    {
      err = set_error (GPG_ERR_ASS_SYNTAX, NULL);
      goto leave;
    }

  err = translate_io_streams (ctx);
  if (err)
    goto leave;
  err = prepare_io_streams (ctx, &input_data, &output_data, NULL);
  if (err)
    goto leave;

  ctrl->cont_cmd = cont_decrypt;

  op = gpa_stream_decrypt_operation_new (NULL, input_data, output_data,
					 no_verify, protocol,
                                         ctrl->session_title);

  input_data = output_data = NULL;
  g_signal_connect_swapped (G_OBJECT (op), "completed",
			    G_CALLBACK (run_server_continuation), ctx);
  g_signal_connect (G_OBJECT (op), "completed",
                    G_CALLBACK (g_object_unref), NULL);
  g_signal_connect_swapped (G_OBJECT (op), "status",
			    G_CALLBACK (assuan_write_status), ctx);

  return not_finished (ctrl);

 leave:
  finish_io_streams (ctx, &input_data, &output_data, NULL);
  close_message_fd (ctrl);
  assuan_close_input_fd (ctx);
  assuan_close_output_fd (ctx);
  ctrl->input_fd = -1;
  ctrl->output_fd = -1;
  return assuan_process_done (ctx, err);
}




/* Continuation for cmd_verify.  */
static void
cont_verify (assuan_context_t ctx, gpg_error_t err)
{
  g_debug ("cont_verify called with with ERR=%s <%s>",
           gpg_strerror (err), gpg_strsource (err));

  finish_io_streams (ctx, NULL, NULL, NULL);
  assuan_process_done (ctx, err);
}


/* VERIFY --protocol=OpenPGP|CMS [--silent]

   Verify a message.  Depending on the combination of the
   sources/sinks set by the commands MESSAGE, INPUT and OUTPUT, the
   verification mode is selected like this:

   MESSAGE and INPUT
     This indicates a detached signature.  Output data is not applicable.

   INPUT 
     This indicates an opaque signature.  As no output command has
     been given, we are only required to check the signature.

   INPUT and OUTPUT
     This indicates an opaque signature.  We write the signed data to
     the file descriptor set by the OUTPUT command.  This data is even
     written if the signature can't be verified.

   With the option --silent we won't display any dialog; this is for
   example used by the client to get the content of an opaque signed
   message.  Sending the follwoing status message is mandatory before
   sending the OK response:

      SIGSTATUS <flag> <displaystring>

   Defines FLAGS are:

      none
        The message has a signature but it could not not be verified
        due to a missing key.

      green
        The signature is fully valid.

      yellow
        The signature is valid but additional information was shown
        regarding the validity of the key.

      red
        The signature is not valid. 

   The DISPLAYSTRING is a percent-and-plus-encoded string with a short
   human readable description of the status.  */
static int 
cmd_verify (assuan_context_t ctx, char *line)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  gpg_error_t err;
  gpgme_protocol_t protocol = 0;
  int silent;
  GpaStreamVerifyOperation *op;
  gpgme_data_t input_data = NULL;
  gpgme_data_t output_data = NULL;
  gpgme_data_t message_data = NULL;
  enum { VERIFY_DETACH, VERIFY_OPAQUE, VERIFY_OPAQUE_WITH_OUTPUT } op_mode;

  err = parse_protocol_option (ctx, line, 1, &protocol);
  if (err)
    goto leave;

  silent = has_option (line, "--silent");

  line = skip_options (line);
  if (*line)
    {
      err = set_error (GPG_ERR_ASS_SYNTAX, NULL);
      goto leave;
    }

  /* Note: We can't use translate_io_streams because that returns an
     error if one is not opened but that is not an error here.  */
  ctrl->input_fd = translate_sys2libc_fd (assuan_get_input_fd (ctx), 0);
  ctrl->output_fd = translate_sys2libc_fd (assuan_get_output_fd (ctx), 1);
  if (ctrl->message_fd != -1 && ctrl->input_fd != -1 
      && ctrl->output_fd == -1)
    op_mode = VERIFY_DETACH;
  else if (ctrl->message_fd == -1 && ctrl->input_fd != -1 
           && ctrl->output_fd == -1)
    op_mode = VERIFY_OPAQUE;
  else if (ctrl->message_fd == -1 && ctrl->input_fd != -1 
           && ctrl->output_fd != -1)
    op_mode = VERIFY_OPAQUE_WITH_OUTPUT;
  else
    {
      err = set_error (GPG_ERR_CONFLICT, "invalid verify mode");
      goto leave;
    }
  
  err = prepare_io_streams (ctx, &input_data, &output_data, &message_data);
  if (! err && op_mode == VERIFY_OPAQUE)
    err = gpgme_data_new_from_cbs (&output_data, &my_devnull_data_cbs, ctrl);
  if (err)
    goto leave;

  ctrl->cont_cmd = cont_verify;

  op = gpa_stream_verify_operation_new (NULL, input_data, message_data,
					output_data, silent, protocol,
                                        ctrl->session_title);

  input_data = output_data = message_data = NULL;
  g_signal_connect_swapped (G_OBJECT (op), "completed",
			    G_CALLBACK (run_server_continuation), ctx);
  g_signal_connect (G_OBJECT (op), "completed",
                    G_CALLBACK (g_object_unref), NULL);
  g_signal_connect_swapped (G_OBJECT (op), "status",
			    G_CALLBACK (assuan_write_status), ctx);

  return not_finished (ctrl);

 leave:
  finish_io_streams (ctx, &input_data, &output_data, &message_data);
  close_message_fd (ctrl);
  assuan_close_input_fd (ctx);
  assuan_close_output_fd (ctx);
  ctrl->input_fd = -1;
  ctrl->output_fd = -1;
  return assuan_process_done (ctx, err);
}



/* START_KEYMANAGER

   Pop up the key manager window.  The client expects that the key
   manager is brought into the foregound and that this command
   immediatley returns.
*/
static int
cmd_start_keymanager (assuan_context_t ctx, char *line)
{
  gpa_open_keyring_editor (NULL, NULL);

  return assuan_process_done (ctx, 0);
}



/* START_CONFDIALOG

   Pop up the configure dialog.  The client expects that the key
   manager is brought into the foregound and that this command
   immediatley returns.
*/
static int
cmd_start_confdialog (assuan_context_t ctx, char *line)
{
  gpa_open_settings_dialog (NULL, NULL);

  return assuan_process_done (ctx, 0);
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


static void
reset_notify (assuan_context_t ctx)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);

  reset_prepared_keys (ctrl);
  release_recipients (ctrl);
  xfree (ctrl->sender);
  ctrl->sender = NULL;
  finish_io_streams (ctx, NULL, NULL, NULL);
  close_message_fd (ctrl);
  assuan_close_input_fd (ctx);
  assuan_close_output_fd (ctx);
  ctrl->input_fd = -1;
  ctrl->output_fd = -1;
  if (ctrl->gpa_op)
    {
      g_object_unref (ctrl->gpa_op);
      ctrl->gpa_op = NULL;
    }
  ctrl->session_number = 0;
  xfree (ctrl->session_title);
  ctrl->session_title = NULL;
}




/* Tell the assuan library about our commands.   */
static int
register_commands (assuan_context_t ctx)
{
  static struct {
    const char *name;
    int (*handler)(assuan_context_t, char *line);
  } table[] = {
    { "SESSION",   cmd_session },
    { "RECIPIENT", cmd_recipient },
    { "INPUT",     NULL },
    { "OUTPUT",    NULL },
    { "MESSAGE",   cmd_message },
    { "ENCRYPT",   cmd_encrypt },
    { "PREP_ENCRYPT", cmd_prep_encrypt },
    { "SENDER",    cmd_sender },
    { "SIGN",      cmd_sign   },
    { "DECRYPT",   cmd_decrypt },
    { "VERIFY",    cmd_verify },
    { "START_KEYMANAGER", cmd_start_keymanager },
    { "START_CONFDIALOG", cmd_start_confdialog },
    { "GETINFO",   cmd_getinfo },
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

  /* Get an Assuan context for the already accepted file descriptor
     FD.  Allow descriptor passing.  */
  err = assuan_init_socket_server_ext (&ctx, ASSUAN_INT2FD(fd), 1|2);
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
  assuan_register_reset_notify (ctx, reset_notify);
  ctrl->message_fd = -1;

  connection_counter++;
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

      reset_notify (ctx);
      assuan_deinit_server (ctx);
      g_free (ctrl);
      connection_counter--;
      if (!connection_counter && shutdown_pending)
        gtk_main_quit ();
    }
}


/* If the assuan context CTX has a registered continuation function,
   run it.  */
static void
run_server_continuation (assuan_context_t ctx, gpg_error_t err)
{
  conn_ctrl_t ctrl = assuan_get_pointer (ctx);
  void (*cont_cmd) (assuan_context_t, gpg_error_t);

  if (!ctrl)
    {
      g_debug ("no context in gpa_run_server_continuation");
      return;
    }
  g_debug ("calling gpa_run_server_continuation (%s)", gpg_strerror (err));
  if (!ctrl->cont_cmd)
    {
      g_debug ("no continuation defined; using default");
      assuan_process_done (ctx, err);
    }
  else if (ctrl->client_died)
    {
      g_debug ("not running continuation as client has disconnected");
      connection_finish (ctx);
    }
  else
    {
      cont_cmd = ctrl->cont_cmd;
      ctrl->cont_cmd = NULL;
      cont_cmd (ctx, err);
    }
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
          if (gpg_err_code (err) == GPG_ERR_EAGAIN)
            ; /* Ignore.  */
          else if (gpg_err_code (err) == GPG_ERR_EOF || err == -1)
            {
              if (ctrl->cont_cmd)
                ctrl->client_died = 1; /* Need to delay the cleanup.  */
              else
                connection_finish (ctx);
              return FALSE; /* Remove from the watch.  */
            }
          else if (gpg_err_code (err) == GPG_ERR_UNFINISHED)
            {
              if (!ctrl->is_unfinished)
                {
                  /* It is quite possible that some other subsystem
                     returns that error code.  Tell the user about
                     this curiosity and finish the command.  */
                  g_debug ("note: Unfinished error code not emitted by us");
                  if (ctrl->cont_cmd)
                    g_debug ("OOPS: pending continuation!");
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

  assuan_set_assuan_err_source (GPG_ERR_SOURCE_DEFAULT);
  
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

/* Set a flag to shutdown the server in a friendly way.  */
void
gpa_stop_server (void)
{
  shutdown_pending = TRUE;
  if (!connection_counter)
    gtk_main_quit ();
}
