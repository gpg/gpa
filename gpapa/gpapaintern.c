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

#include "gpapa.h"

/* GPGME includes.
 */
#include "gpgme_types.h"
#include "rungpg.h"
#include "gpgme.h"

/* Just a string helper function.
 */
gboolean
gpapa_line_begins_with (gchar *line, gchar *keyword)
{
  if (strncmp (line, keyword, strlen (keyword)) == 0)
    return (TRUE);
  else
    return (FALSE);
}	

/* Needed by GPGME.
 */
#ifndef HAVE_STPCPY
char *
stpcpy(char *a,const char *b)
{
    while( *b )
	*a++ = *b++;
    *a = 0;

    return (char*)a;
}
#endif

/* A dummy line_callback function to avoid "if (line_callback)"
 * in various places.
 */
static void
dummy_line_callback (gchar *a, void *opaque, GpgStatusCode status)
{
}

/* A structure to pass our parameters through GPGME.
 */
typedef struct gpapa_data_s
  {
    char *commands, *passphrase;
    GpapaLineCallbackFunc line_callback;
    gpointer line_calldata;
    GpapaCallbackFunc callback;
    gpointer calldata;
  } gpapa_data_t;

/* Output line handler to be passed to _gpgme_gpg_spawn().
 */
static void
line_handler (GpgmeCtx opaque, char *line)
{
  gpapa_data_t *g = (gpapa_data_t *) opaque;
  if (line && line[strlen (line) - 1] == '\r')
    line[strlen (line) - 1] = 0;
  g->line_callback (line, g->line_calldata, NO_STATUS);
}

/* Status handler to be passed to _gpgme_gpg_spawn().
 */
static void
status_handler (GpgmeCtx opaque, GpgStatusCode code, char *args)
{
  gpapa_data_t *g = (gpapa_data_t *) opaque;
  g->line_callback (args, g->line_calldata, code);
}

/* Command handler to be passed to _gpgme_gpg_spawn().
 */
static const char *
command_handler (void *opaque, GpgStatusCode code, const char *key)
{
  gpapa_data_t *g = (gpapa_data_t *) opaque;
  return NULL;
}

/* Call the `gpg' program, passing some standard args plus USER_ARGS
 * which must be a NULL-terminated array of argument strings.
 * If DO_WAIT is nonzero, we wait for `gpg' to terminate.
 * Otherwise, we wait either - non-blocking behaviour is not yet implemented.
 * COMMANDS is a series of commands passed to `gpg' through `--command-fd'.
 * PASSPHRASE contains a passphrase.
 * LINE_CALLBACK and is a callback the caller must provide to parse
 * the output on gpg's stdout. When called, it gets the line (char*)
 * and LINE_CALLDATA as arguments.
 * CALLBACK and CALLDATA are the usual GPAPA callback.
 */
void
gpapa_call_gnupg (char **user_args, gboolean do_wait,
                  char *commands, char *passphrase,
                  GpapaLineCallbackFunc line_callback, gpointer line_calldata,
                  GpapaCallbackFunc callback, gpointer calldata)
{
  GpgObject gpg;
  char **arg;
  int return_code;
 
  /* This data we want to have available in our callbacks to
   * _gpgme_gpg_spawn.
   */
  gpapa_data_t gpapa_data;

  /* Avoid various `if's.
   */
  if (! line_callback)
    line_callback = dummy_line_callback;

  /* Initialize gpapa_data.
   */
  gpapa_data.commands = commands;
  gpapa_data.passphrase = passphrase;
  gpapa_data.line_callback = line_callback;
  gpapa_data.line_calldata = line_calldata;
  gpapa_data.callback = callback;
  gpapa_data.calldata = calldata;

  _gpgme_gpg_new (&gpg);

  _gpgme_gpg_add_arg (gpg, "--no-options");
  _gpgme_gpg_add_arg (gpg, "--batch");
  for (arg = user_args; *arg; arg++)
    _gpgme_gpg_add_arg (gpg, *arg);

  _gpgme_gpg_set_status_handler (gpg, status_handler, &gpapa_data);
  _gpgme_gpg_set_colon_line_handler (gpg, line_handler, &gpapa_data);
  if (commands)
    _gpgme_gpg_set_command_handler (gpg, command_handler, &gpapa_data);

  return_code = _gpgme_gpg_spawn (gpg, &gpapa_data);
  if (return_code == 0)
    {
      gpgme_wait ((GpgmeCtx) &gpapa_data, 1);
      
      /* FIXME: Currently we cannot know the exit status of the
       * GnuPG execution.
       */
      callback (GPAPA_ACTION_FINISHED, "GnuPG execution terminated", calldata);
    }
  else
    callback (GPAPA_ACTION_ERROR, "GnuPG execution failed", calldata);

  _gpgme_gpg_release (gpg);
}
