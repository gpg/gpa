/* gpapaintern.c - The GNU Privacy Assistant Pipe Access - internal routines
 * Copyright (C) 2000, 2001 G-N-U GmbH, http://www.g-n-u.de
 *
 * This file is part of GPAPA.
 *
 * GPAPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPAPA is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GPAPA; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

/* A dummy line_callback function to avoid "if (line_callback)"
 * in various places.
 */
static void
dummy_line_callback (gchar *a, void *opaque, GpgStatusCode status)
{
}

/* Counter for STATUS_NODATA messages.
 */
static int nodata_counter = 0;

/* Used by various line_callback functions: report a GPG error status.
 * STATUS_NODATA is reported only once per gpapa_call_gnupg().
 */
void
gpapa_report_error_status (GpgStatusCode status,
                           GpapaCallbackFunc callback, gpointer calldata)
{
  char *errstr;
  switch (status)
    {
      case STATUS_ABORT:
        errstr = "gpg execution aborted";
        break;
      case STATUS_BADARMOR:
        errstr = "invalid file: bad armor";
        break;
      case STATUS_BADMDC:
        errstr = "invalid file: bad MDC";
        break;
      case STATUS_BAD_PASSPHRASE:
        errstr = "bad passphrase";
        break;
      case STATUS_DECRYPTION_FAILED:
        errstr = "decryption failed";
        break;
      case STATUS_DELETE_PROBLEM:
        errstr = "delete problem";
        break;
      case STATUS_ERRMDC:
        errstr = "error in MDC";
        break;
      case STATUS_ERRSIG:
        errstr = "error in signature";
        break;
      case STATUS_FILE_ERROR:
        errstr = "file error";
        break;
      case STATUS_MISSING_PASSPHRASE:
        errstr = "missing passphrase";
        break;
      case STATUS_NODATA:
        if (nodata_counter == 0)
          errstr = "no valid OpenPGP data found";
        else
          errstr = NULL;
        nodata_counter++;
        break;
      default:
        errstr = NULL;
        break;
    }
  if (errstr)
    callback (GPAPA_ACTION_ERROR, errstr, calldata);
}

/* A structure to pass our parameters through GPGME.
 */
typedef struct gpapa_data_s
  {
    const gchar *commands, *passphrase;
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
#ifdef DEBUG
  fprintf (stderr, "line handler called with line = \"%s\"\n",
           line);
#endif
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
#ifdef DEBUG
  fprintf (stderr, "status handler called with code = %d, args = \"%s\"\n",
           code, args);
#endif
  g->line_callback (args, g->line_calldata, code);
}

#ifdef USE_COMMAND_HANDLER

/* Command handler to be passed to _gpgme_gpg_spawn().
 */
static const char *
command_handler (void *opaque, GpgStatusCode code, const char *key)
{
  gpapa_data_t *g = (gpapa_data_t *) opaque;
#ifdef DEBUG
  fprintf (stderr, "command handler called with code = %d, key = \"%s\"\n",
           code, key);
#endif
  if (g != NULL
      && code == STATUS_GET_HIDDEN
      && strcmp (key, "passphrase.enter") == 0)
    {
      fprintf (stderr, "passphrase = %s\n", g->passphrase);
      return g->passphrase;
    }
  else
    return NULL;
}

#endif /* USE_COMMAND_HANDLER */

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
gpapa_call_gnupg (const gchar **user_args, gboolean do_wait,
                  const gchar *commands, const gchar *data,
		  const gchar *passphrase,
                  GpapaLineCallbackFunc line_callback, gpointer line_calldata,
                  GpapaCallbackFunc callback, gpointer calldata)
{
  GpgObject gpg;
  const gchar **arg;
  int return_code = 0;

  /* This data we want to have available in our callbacks to
   * _gpgme_gpg_spawn.
   */
  gpapa_data_t gpapa_data;

  /* Reset the counter of STATUS_NODATA reports.
   * !!! This is not thread-safe.
   */
  nodata_counter = 0;
 
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
  _gpgme_gpg_add_arg (gpg, "--always-trust");
  _gpgme_gpg_add_arg (gpg, "--utf8-strings");

  _gpgme_gpg_set_status_handler (gpg, status_handler, &gpapa_data);
  _gpgme_gpg_set_colon_line_handler (gpg, line_handler, &gpapa_data);

#ifdef USE_COMMAND_HANDLER
  if (commands || passphrase)
    {
      return_code = _gpgme_gpg_set_command_handler (gpg, command_handler, &gpapa_data);
#ifdef DEBUG
      fprintf (stderr, "installing command handler: return_code = %d\n", return_code);
#endif
    }
#else /* not USE_COMMAND_HANDLER */
  if (commands)
    {
      GpgmeData tmp;
#ifdef DEBUG
      fprintf (stderr, "commands:\n%s(end of commands)\n", commands);
#endif
      return_code = gpgme_data_new_from_mem (&tmp, commands,
                                             strlen (commands), 0);
      if (return_code == 0)
        {
          _gpgme_data_set_mode (tmp, GPGME_DATA_MODE_OUT);
          _gpgme_gpg_add_arg (gpg, "--command-fd");
          _gpgme_gpg_add_arg (gpg, "0");
          return_code = _gpgme_gpg_add_data (gpg, tmp, 0);
        }
    }
  else
    _gpgme_gpg_add_arg (gpg, "--batch");
  if (data && return_code == 0)
    {
      GpgmeData tmp;
      return_code = gpgme_data_new_from_mem (&tmp, data, strlen (data), 0);
      if (return_code == 0)
        {
          _gpgme_data_set_mode (tmp, GPGME_DATA_MODE_OUT);
          return_code = _gpgme_gpg_add_data (gpg, tmp, 0);
        }
#ifdef DEBUG
      fprintf (stderr, "installing data:\n%s\nreturn_code = %d\n", data, return_code);
#endif
    }
  if (passphrase && return_code == 0)
    {
      GpgmeData tmp;
      return_code = gpgme_data_new_from_mem (&tmp, passphrase,
                                             strlen (passphrase), 0);
      if (return_code == 0)
        {
          _gpgme_data_set_mode (tmp, GPGME_DATA_MODE_OUT);
          _gpgme_gpg_add_arg (gpg, "--passphrase-fd");
          return_code = _gpgme_gpg_add_data (gpg, tmp, -2);
        }
#ifdef DEBUG
      fprintf (stderr, "installing passphrase data: return_code = %d\n", return_code);
#endif
    }
#endif /* not USE_COMMAND_HANDLER */

  if (return_code != 0)
    callback (GPAPA_ACTION_ERROR,
              "GnuPG execution failed: internal error while setting command handler",
              calldata);
  else
    {
      for (arg = user_args; *arg; arg++)
        _gpgme_gpg_add_arg (gpg, *arg);

      return_code = _gpgme_gpg_spawn (gpg, &gpapa_data);
      if (return_code != 0)
        callback (GPAPA_ACTION_ERROR,
                  "GnuPG execution failed: could not spawn external program",
                  calldata);
      else
        {
          gpgme_wait ((GpgmeCtx) &gpapa_data, 1);
          
          /* FIXME: Currently we cannot know the exit status of the
           * GnuPG execution.
           */
          callback (GPAPA_ACTION_FINISHED, "GnuPG execution terminated", calldata);
        }
    }

  _gpgme_gpg_release (gpg);
}
