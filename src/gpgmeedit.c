/* gpgmeedit.c - The GNU Privacy Assistant's edit interactor.
 *      Copyright (C) 2002 Miguel Coca.
 *	Copyright (C) 2008, 2009, 2012, 2017 g10 Code GmbH.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <unistd.h>

#include "gpgmeedit.h"
#include "passwddlg.h"

/* The edit callback for all the edit operations is edit_fnc.  Each
 * operation is modelled as a sequential machine (a Moore machine, to
 * be precise).  Therefore, for each operation you must write two
 * functions.
 *
 * One of them is "action" or output function, that chooses the right value
 * for *result (the next command issued in the edit command line) based on
 * the current state. The other is the "transit" function, which chooses
 * the next state based on the current state and the input (status code and
 * args).
 *
 * See the comments below for details.
 */


#define CMP_STATUS(a) !strcmp (status, G_STRINGIFY (a))
typedef const char *status_type_t ;



/* Prototype of the action function. Returns the error if there is one */
typedef gpg_error_t (*edit_action_t) (int state, void *opaque,
                                      char **result);
/* Prototype of the transit function. Returns the next state. If and error
 * is found changes *err. If there is no error it should NOT touch it */
typedef int (*edit_transit_t) (int current_state, status_type_t status,
                               const char *args, void *opaque,
                               gpg_error_t *err);


/* States for the edit expire command.  */
enum
  {
    EXPIRE_START,
    EXPIRE_COMMAND,
    EXPIRE_DATE,
    EXPIRE_QUIT,
    EXPIRE_SAVE,
    EXPIRE_ERROR
  };


/* States for the edit trust command.  */
enum
  {
    TRUST_START,
    TRUST_COMMAND,
    TRUST_VALUE,
    TRUST_REALLY_ULTIMATE,
    TRUST_QUIT,
    TRUST_SAVE,
    TRUST_ERROR
  };


/* States for the sign command.  */
enum
  {
    SIGN_START,
    SIGN_COMMAND,
    SIGN_UIDS,
    SIGN_SET_EXPIRE,
    SIGN_SET_CHECK_LEVEL,
    SIGN_CONFIRM,
    SIGN_QUIT,
    SIGN_SAVE,
    SIGN_ERROR
  };

/* Parameter for the sign command.  */
struct sign_parms_s
{
  gchar *check_level;
  gboolean local;
};


/* States for the passwd command.  */
enum
  {
    PASSWD_START,
    PASSWD_COMMAND,
    PASSWD_ENTERNEW,
    PASSWD_QUIT,
    PASSWD_SAVE,
    PASSWD_ERROR
  };

/* Parameter for the passwd command.  */
struct passwd_parms_s
{
  gpgme_passphrase_cb_t func;
  void *opaque;
};


/* States for card related commands.  */
enum
  {
    CARD_START,
    CARD_COMMAND,
    CARD_ADMIN_COMMAND,
    CARD_QUIT,
    CARD_QUERY_LOGIN,
    CARD_GENERATE_BACKUP,
    CARD_GENERATE_REPLACE_KEYS,
    CARD_GENERATE_VALIDITY,
    CARD_GENERATE_NAME,
    CARD_GENERATE_EMAIL,
    CARD_GENERATE_COMMENT,
    CARD_GENERATE_DONE,
    CARD_ERROR,
    CARD_DEFAULT
  };



/* Helper to print information about an unexpected state.  An erro
   code is returned. */
static gpg_error_t
_unexpected_state (int line, int state)
{
  g_debug ("gpgmeedit.c:%d: unexpected state %d in " PACKAGE_STRING,
           line, state);
  return gpg_error (GPG_ERR_BUG);
}
#define unexpected_state(a)  _unexpected_state (__LINE__, (a))



/* Data to be passed to the edit callback. Must be filled by the caller of
 * gpgme_op_edit()  */
struct edit_parms_s
{
  /* Current state */
  int state;

  /* Last error: The return code of gpgme_op_edit() is the return value of
   * the last invocation of the callback. But returning an error from the
   * callback does not abort the edit operation, so we must remember any
   * error. In other words, errors from action() or transit() are sticky.
   */
  gpg_error_t err;

  /* The action function */
  edit_action_t action;

  /* The transit function */
  edit_transit_t transit;

  /* This optional function is called with a string describing the
     error conveyed by the GPGME_STATUS_ERROR message.  The function
     takes ownership of the malloced string.  */
  void (*save_error) (void *opaque, char *errdesc);

  /* The output data object */
  gpgme_data_t out;

  /* Signal attachment id */
  gulong signal_id;

  /* Data to be passed to the previous functions */
  void *opaque;

  /* The passwd dialog needs to see GPGME_STATUS_NEED_PASSPHRASE_SYM.
     To make thinks easier it is only passed to the FSM if this flag
     has been set. */
  int need_status_passphrase_sym;
};


/* Parse the args of an ERROR status line and return its error
   code.  */
static gpg_error_t
parse_status_error (const char *args)
{
  size_t n = strcspn (args, " \t");
  return (gpg_error_t)strtoul (args+n, NULL, 10);
}


/* The interact/edit callback proper.  */
static gpg_error_t
edit_fnc (void *opaque, status_type_t status,
	  const char *args, int fd)
{
  struct edit_parms_s *parms = opaque;
  char *result = NULL;

  /* Whitelist all status code we know about.  */
  {
    static const char *whitelist[] = {
      "ALREADY_SIGNED",
      "ERROR",
      "GET_BOOL",
      "GET_LINE",
      "KEY_CREATED",
      "NEED_PASSPHRASE_SYM",
      "SC_OP_FAILURE",
      NULL
    };
    int i;

    /* Whitelist all status code we know about.  */
    for (i=0; whitelist[i]; i++)
      if (!strcmp (status, whitelist[i]))
        break; /* Known.  */
    if (!whitelist[i])
      {
        /* We don't know this keyword thus do not need to handle it.  */
        return parms->err;
      }
  }

  if (!parms->need_status_passphrase_sym
      && CMP_STATUS (NEED_PASSPHRASE_SYM))
    {
      return parms->err;
    }
  else if (CMP_STATUS (SC_OP_FAILURE))
    {
      if (args && !parms->err)
        {
          gpg_err_code_t ec;

          switch (strtoul (args, NULL, 10))
            {
            case 1: ec = GPG_ERR_CANCELED; break;
            case 2: ec = GPG_ERR_BAD_PIN; break;
            default: ec = GPG_ERR_CARD; break;
            }
          if (ec)
            parms->err = gpg_error (ec);
        }
      return parms->err;
    }

  /* Call the save_error function.  */
  if (CMP_STATUS (ERROR) && args && parms->save_error)
    {
      int n = strcspn (args, " \t");
      char *buf = g_strdup_printf ("%.*s: %s", n, args,
                                   gpg_strerror (strtoul (args+n, NULL, 10)));
      parms->save_error (parms->opaque, buf);
    }

  if (debug_edit_fsm)
    g_debug ("edit_fnc: state=%d input=%s (%s)"
             , parms->state, status, args);

  /* Choose the next state based on the current one and the input */
  parms->state = parms->transit (parms->state, status, args, parms->opaque,
				 &parms->err);
  if (!parms->err)
    {
      gpg_error_t err;

      /* Choose the action based on the state */
      err = parms->action (parms->state, parms->opaque, &result);

      if (debug_edit_fsm)
        g_debug ("edit_fnc: newstate=%d err=%s result=%s",
                 parms->state, gpg_strerror (err), result);

      if (err)
        parms->err = err;

      /* Send the command, if any was provided */
      if (result)
	{
          if (*result)
            gpgme_io_write (fd, result, strlen (result));
	  gpgme_io_write (fd, "\n", 1);
	}
    }
  else if (parms->err == gpg_error (GPG_ERR_EAGAIN))
    {
      parms->err = 0;
      if (debug_edit_fsm)
        g_debug ("edit_fnc: newstate=%d again, default response", parms->state);
      /* Send an empty line as default response.  */
      gpgme_io_write (fd, "\n", 1);
    }
  else
    {
      if (debug_edit_fsm)
        g_debug ("edit_fnc: newstate=%d err=%s transit failed",
                 parms->state, gpg_strerror (parms->err));
    }
  return parms->err;
}



/* Change expiry time: action.  */
static gpg_error_t
edit_expire_fnc_action (int state, void *opaque, char **result)
{
  char *date = opaque;

  switch (state)
    {
      /* Start the operation */
    case EXPIRE_COMMAND:
      *result = "expire";
      break;
      /* Send the new expire date */
    case EXPIRE_DATE:
      *result = date;
      break;
      /* End the operation */
    case EXPIRE_QUIT:
      *result = "quit";
      break;
      /* Save */
    case EXPIRE_SAVE:
      *result = "Y";
      break;
      /* Special state: an error ocurred. Do nothing until we can quit */
    case EXPIRE_ERROR:
      break;
      /* Can't happen */
    default:
      return unexpected_state (state);
    }
  return 0;
}


/* Change expiry time: transit.  */
static int
edit_expire_fnc_transit (int current_state, status_type_t status,
			 const char *args, void *opaque, gpg_error_t *err)
{
  int next_state;

  switch (current_state)
    {
    case EXPIRE_START:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          next_state = EXPIRE_COMMAND;
        }
      else
        {
          next_state = EXPIRE_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case EXPIRE_COMMAND:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keygen.valid"))
        {
          next_state = EXPIRE_DATE;
        }
      else
        {
          next_state = EXPIRE_ERROR;
          *err =  gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case EXPIRE_DATE:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          next_state = EXPIRE_QUIT;
        }
      else if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keygen.valid"))
        {
          next_state = EXPIRE_ERROR;
          *err = gpg_error (GPG_ERR_INV_TIME);
        }
      else
        {
          next_state = EXPIRE_ERROR;
          *err =  gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case EXPIRE_QUIT:
      if (CMP_STATUS (GET_BOOL) && g_str_equal (args, "keyedit.save.okay"))
        {
          next_state = EXPIRE_SAVE;
        }
      else
        {
          next_state = EXPIRE_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case EXPIRE_ERROR:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          next_state = EXPIRE_QUIT;
        }
      else
        {
          next_state = EXPIRE_ERROR;
        }
      break;
    default:
      next_state = EXPIRE_ERROR;
      *err = gpg_error (GPG_ERR_GENERAL);
    }
  return next_state;
}



/* Change the key ownertrust: action.  */
static gpg_error_t
edit_trust_fnc_action (int state, void *opaque, char **result)
{
  gchar *trust = opaque;

  switch (state)
    {
      /* Start the operation */
    case TRUST_COMMAND:
      *result = "trust";
      break;
      /* Send the new trust date */
    case TRUST_VALUE:
      *result = trust;
      break;
      /* Really set to ultimate trust */
    case TRUST_REALLY_ULTIMATE:
      *result = "Y";
      break;
      /* End the operation */
    case TRUST_QUIT:
      *result = "quit";
      break;
      /* Save */
    case TRUST_SAVE:
      *result = "Y";
      break;
      /* Special state: an error ocurred. Do nothing until we can quit */
    case TRUST_ERROR:
      break;
      /* Can't happen */
    default:
      return unexpected_state (state);
    }
  return gpg_error (GPG_ERR_NO_ERROR);
}

/* Change the key ownertrust: transit.  */
static int
edit_trust_fnc_transit (int current_state, status_type_t status,
			const char *args, void *opaque, gpg_error_t *err)
{
  int next_state;

  switch (current_state)
    {
    case TRUST_START:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          next_state = TRUST_COMMAND;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case TRUST_COMMAND:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "edit_ownertrust.value"))
        {
          next_state = TRUST_VALUE;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case TRUST_VALUE:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          next_state = TRUST_QUIT;
        }
      else if (CMP_STATUS (GET_BOOL)
               && g_str_equal (args, "edit_ownertrust.set_ultimate.okay"))
        {
          next_state = TRUST_REALLY_ULTIMATE;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case TRUST_REALLY_ULTIMATE:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          next_state = TRUST_QUIT;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case TRUST_QUIT:
      if (CMP_STATUS (GET_BOOL) && g_str_equal (args, "keyedit.save.okay"))
        {
          next_state = TRUST_SAVE;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case TRUST_ERROR:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          next_state = TRUST_QUIT;
        }
      else
        {
          next_state = TRUST_ERROR;
        }
      break;
    default:
      next_state = TRUST_ERROR;
      *err = gpg_error (GPG_ERR_GENERAL);
    }

  return next_state;
}




/* Sign a key: action.  */
static gpg_error_t
edit_sign_fnc_action (int state, void *opaque, char **result)
{
  struct sign_parms_s *parms = opaque;

  switch (state)
    {
      /* Start the operation */
    case SIGN_COMMAND:
      *result = parms->local ? "lsign" : "sign";
      break;
      /* Sign all user ID's */
    case SIGN_UIDS:
      *result = "Y";
      break;
      /* The signature expires at the same time as the key */
    case SIGN_SET_EXPIRE:
      *result = "Y";
      break;
      /* Set the check level on the key */
    case SIGN_SET_CHECK_LEVEL:
      *result = parms->check_level;
      break;
      /* Confirm the signature */
    case SIGN_CONFIRM:
      *result = "Y";
      break;
      /* End the operation */
    case SIGN_QUIT:
      *result = "quit";
      break;
      /* Save */
    case SIGN_SAVE:
      *result = "Y";
      break;
      /* Special state: an error ocurred. Do nothing until we can quit */
    case SIGN_ERROR:
      break;
      /* Can't happen */
    default:
      return unexpected_state (state);
    }
  return gpg_error (GPG_ERR_NO_ERROR);
}


/* Sign a key: transit.  */
static int
edit_sign_fnc_transit (int current_state, status_type_t status,
		       const char *args, void *opaque, gpg_error_t *err)
{
  int next_state;

  switch (current_state)
    {
    case SIGN_START:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          next_state = SIGN_COMMAND;
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case SIGN_COMMAND:
      if (CMP_STATUS (GET_BOOL) && g_str_equal (args, "keyedit.sign_all.okay"))
        {
          next_state = SIGN_UIDS;
        }
      else if (CMP_STATUS (GET_BOOL) && g_str_equal (args, "sign_uid.okay"))
        {
          next_state = SIGN_CONFIRM;
        }
      else if (CMP_STATUS (GET_LINE) && g_str_equal (args, "sign_uid.expire"))
        {
          next_state = SIGN_SET_EXPIRE;
          break;
        }
      else if (CMP_STATUS (GET_LINE) && g_str_equal (args, "sign_uid.class"))
        {
          next_state = SIGN_SET_CHECK_LEVEL;
        }
      else if (CMP_STATUS (ALREADY_SIGNED))
        {
          /* The key has already been signed with this key */
          next_state = SIGN_ERROR;
          *err =  gpg_error (GPG_ERR_CONFLICT);
        }
      else if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_UNUSABLE_PUBKEY);
        }
      else if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          next_state = current_state;
          *err = gpg_error (GPG_ERR_EAGAIN);
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case SIGN_UIDS:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "sign_uid.expire"))
        {
          next_state = SIGN_SET_EXPIRE;
          break;
        }
      else if (CMP_STATUS (GET_LINE) && g_str_equal (args, "sign_uid.class"))
        {
          next_state = SIGN_SET_CHECK_LEVEL;
        }
      else if (CMP_STATUS (GET_BOOL) && g_str_equal (args, "sign_uid.okay"))
        {
          next_state = SIGN_CONFIRM;
        }
      else if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_UNUSABLE_PUBKEY);
        }
      else if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          next_state = current_state;
          *err = gpg_error (GPG_ERR_EAGAIN);
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case SIGN_SET_EXPIRE:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "sign_uid.class"))
        {
          next_state = SIGN_SET_CHECK_LEVEL;
        }
      else if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          next_state = current_state;
          *err = gpg_error (GPG_ERR_EAGAIN);
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case SIGN_SET_CHECK_LEVEL:
      if (CMP_STATUS (GET_BOOL) && g_str_equal (args, "sign_uid.okay"))
        {
          next_state = SIGN_CONFIRM;
        }
      else if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          next_state = current_state;
          *err = gpg_error (GPG_ERR_EAGAIN);
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case SIGN_CONFIRM:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          next_state = SIGN_QUIT;
        }
      else if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          next_state = current_state;
          *err = gpg_error (GPG_ERR_EAGAIN);
        }
      else if (CMP_STATUS (ERROR))
        {
          next_state = SIGN_ERROR;
          *err = parse_status_error (args);
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case SIGN_QUIT:
      if (CMP_STATUS (GET_BOOL) && g_str_equal (args, "keyedit.save.okay"))
        {
          next_state = SIGN_SAVE;
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case SIGN_ERROR:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          next_state = SIGN_QUIT;
        }
      else
        {
          next_state = SIGN_ERROR;
        }
      break;
    default:
      next_state = SIGN_ERROR;
      *err = gpg_error (GPG_ERR_GENERAL);
    }

  return next_state;
}


/* Change passphrase: action.  */
static gpg_error_t
edit_passwd_fnc_action (int state, void *opaque, char **result)
{
  switch (state)
    {
    case PASSWD_COMMAND:
      /* Start the operation */
      *result = "passwd";
      break;
    case PASSWD_ENTERNEW:
      /* No action required.  This state is only used by the
         passphrase callback.  */
      break;
    case PASSWD_QUIT:
      /* End the operation */
      *result = "quit";
      break;
    case PASSWD_SAVE:
      /* Save */
      *result = "Y";
      break;
    case PASSWD_ERROR:
      /* Special state: an error ocurred. Do nothing until we can quit */
      break;
    default:
      /* Can't happen */
      return unexpected_state (state);
    }
  return gpg_error (GPG_ERR_NO_ERROR);
}

/* Change passphrase: transit.  */
static int
edit_passwd_fnc_transit (int current_state, status_type_t status,
			 const char *args, void *opaque, gpg_error_t *err)
{
  int next_state;

  switch (current_state)
    {
    case PASSWD_START:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          next_state = PASSWD_COMMAND;
        }
      else
        {
          next_state = PASSWD_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case PASSWD_COMMAND:
    case PASSWD_ENTERNEW:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          next_state = PASSWD_QUIT;
        }
      else if (CMP_STATUS (NEED_PASSPHRASE_SYM))
        {
          next_state = PASSWD_ENTERNEW;
        }
      else
        {
          next_state = PASSWD_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case PASSWD_QUIT:
      if (CMP_STATUS (GET_BOOL) && g_str_equal (args, "keyedit.save.okay"))
        {
          next_state = PASSWD_SAVE;
        }
      else
        {
          next_state = PASSWD_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case PASSWD_ERROR:
      if (CMP_STATUS (GET_LINE) && g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          next_state = PASSWD_QUIT;
        }
      else
        {
          next_state = PASSWD_ERROR;
        }
      break;
    default:
      next_state = PASSWD_ERROR;
      *err = gpg_error (GPG_ERR_GENERAL);
    }
  return next_state;
}


/* Release the edit parameters needed for setting owner trust. The
   prototype is that of a GpaContext's "done" signal handler.  */
static void
gpa_gpgme_edit_trust_parms_release (GpaContext *ctx, gpg_error_t err,
				   struct edit_parms_s* parms)
{
  gpgme_data_release (parms->out);
  if (parms->signal_id != 0)
    {
      /* Don't run this signal handler again if the context is reused */
      g_signal_handler_disconnect (ctx, parms->signal_id);
    }
  g_free (parms->opaque);
  g_free (parms);
}


/* Generate the edit parameters needed for setting owner trust.  */
static struct edit_parms_s*
gpa_gpgme_edit_trust_parms_new (GpaContext *ctx, const char *trust_string,
				gpgme_data_t out)
{
  struct edit_parms_s *edit_parms = g_malloc0 (sizeof (struct edit_parms_s));

  edit_parms->state = TRUST_START;
  edit_parms->action = edit_trust_fnc_action;
  edit_parms->transit = edit_trust_fnc_transit;
  edit_parms->out = out;
  edit_parms->opaque = g_strdup (trust_string);

  /* Make sure the cleanup is run when the edit completes */
  edit_parms->signal_id =
    g_signal_connect (G_OBJECT (ctx), "done",
		      G_CALLBACK (gpa_gpgme_edit_trust_parms_release),
		      edit_parms);

  return edit_parms;
}


/* Change the ownertrust of a key.  */
gpg_error_t
gpa_gpgme_edit_trust_start (GpaContext *ctx, gpgme_key_t key,
                            gpgme_validity_t ownertrust)
{
  const gchar *trust_strings[] = {"1", "1", "2", "3", "4", "5"};
  struct edit_parms_s *parms = NULL;
  gpg_error_t err;
  gpgme_data_t out = NULL;

  err = gpgme_data_new (&out);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }
  parms = gpa_gpgme_edit_trust_parms_new (ctx, trust_strings[ownertrust], out);
  err = gpgme_op_interact_start (ctx->ctx, key, 0, edit_fnc, parms, out);
  return err;
}


/* Release the edit parameters needed for changing the expiry date. The
   prototype is that of a GpaContext's "done" signal handler.  */
static void
gpa_gpgme_edit_expire_parms_release (GpaContext *ctx, gpg_error_t err,
				     struct edit_parms_s* parms)
{
  gpgme_data_release (parms->out);
  if (parms->signal_id != 0)
    {
      /* Don't run this signal handler again if the context is reused */
      g_signal_handler_disconnect (ctx, parms->signal_id);
    }
  g_free (parms->opaque);
  g_free (parms);
}


/* Generate the edit parameters needed for changing the expiry date.  */
static struct edit_parms_s*
gpa_gpgme_edit_expire_parms_new (GpaContext *ctx, GDate *date,
				 gpgme_data_t out)
{
  struct edit_parms_s *edit_parms = g_malloc0 (sizeof (struct edit_parms_s));
  int buf_len = 12;
  gchar *buf = g_malloc (buf_len);

  edit_parms->state = EXPIRE_START;
  edit_parms->action = edit_expire_fnc_action;
  edit_parms->transit = edit_expire_fnc_transit;
  edit_parms->out = out;
  edit_parms->opaque = buf;

  /* The new expiration date */
  if (date)
    {
      g_date_strftime (buf, buf_len, "%Y-%m-%d", date);
    }
  else
    {
      strncpy (buf, "0", buf_len);
    }

  /* Make sure the cleanup is run when the edit completes */
  edit_parms->signal_id =
    g_signal_connect (G_OBJECT (ctx), "done",
		      G_CALLBACK (gpa_gpgme_edit_expire_parms_release),
		      edit_parms);

  return edit_parms;
}


/* Change the expire date of a key.  */
gpg_error_t
gpa_gpgme_edit_expire_start (GpaContext *ctx, gpgme_key_t key, GDate *date)
{
  struct edit_parms_s *parms;
  gpg_error_t err;
  gpgme_data_t out = NULL;

  err = gpgme_data_new (&out);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }

  parms = gpa_gpgme_edit_expire_parms_new (ctx, date, out);
  err = gpgme_op_interact_start (ctx->ctx, key, 0, edit_fnc, parms, out);
  return err;
}


/* Release the edit parameters needed for signing. The prototype is that of
   a GpaContext's "done" signal handler.  */
static void
gpa_gpgme_edit_sign_parms_release (GpaContext *ctx, gpg_error_t err,
				   struct edit_parms_s* parms)
{
  gpgme_data_release (parms->out);
  if (parms->signal_id != 0)
    {
      g_signal_handler_disconnect (ctx, parms->signal_id);
    }
  g_free (parms->opaque);
  g_free (parms);
}


/* Generate the edit parameters needed for signing.  */
static struct edit_parms_s*
gpa_gpgme_edit_sign_parms_new (GpaContext *ctx, char *check_level,
			       gboolean local, gpgme_data_t out)
{
  struct sign_parms_s *sign_parms = g_malloc0 (sizeof (struct sign_parms_s));
  struct edit_parms_s *edit_parms = g_malloc0 (sizeof (struct edit_parms_s));

  edit_parms->state = SIGN_START;
  edit_parms->action = edit_sign_fnc_action;
  edit_parms->transit = edit_sign_fnc_transit;
  edit_parms->out = out;
  edit_parms->opaque = sign_parms;
  sign_parms->check_level = check_level;
  sign_parms->local = local;

  /* Make sure the cleanup is run when the edit completes */
  edit_parms->signal_id =
    g_signal_connect (G_OBJECT (ctx), "done",
		      G_CALLBACK (gpa_gpgme_edit_sign_parms_release),
		      edit_parms);

  return edit_parms;
}


/* Sign this key with the given private key.  */
gpg_error_t
gpa_gpgme_edit_sign_start (GpaContext *ctx, gpgme_key_t key,
                           gpgme_key_t secret_key, gboolean local)
{
  struct edit_parms_s *parms = NULL;
  gpg_error_t err = gpg_error (GPG_ERR_NO_ERROR);
  gpgme_data_t out;

  err = gpgme_data_new (&out);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }
  gpgme_signers_clear (ctx->ctx);
  err = gpgme_signers_add (ctx->ctx, secret_key);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }
  parms = gpa_gpgme_edit_sign_parms_new (ctx, "0", local, out);
  err = gpgme_op_interact_start (ctx->ctx, key, 0, edit_fnc, parms, out);
  return err;
}


/*
 * Change the passphrase of the key.
 */

/* Special passphrase callback for use within the passwd command */
gpg_error_t
passwd_passphrase_cb (void *hook, const char *uid_hint,
                      const char *passphrase_info,
                      int prev_was_bad, int fd)
{
  struct edit_parms_s *parms = hook;

  if (parms->state == PASSWD_ENTERNEW)
    {
      /* Gpg is asking for the new passphrase.  Run the dialog to
         enter and re-enter the new passphrase.  */
      return gpa_change_passphrase_dialog_run (hook, uid_hint,
					       passphrase_info,
					       prev_was_bad, fd);
    }
  else
    {
      /* Gpg is asking for the passphrase to unprotect the key.  */
      return gpa_passphrase_cb (hook, uid_hint, passphrase_info,
				prev_was_bad, fd);
    }
}


/* Release the edit parameters needed for changing the passphrase. The
 * prototype is that of a GpaContext's "done" signal handler.
 */
static void
gpa_gpgme_edit_passwd_parms_release (GpaContext *ctx, gpg_error_t err,
				     struct edit_parms_s* parms)
{
  struct passwd_parms_s *passwd_parms = parms->opaque;

  gpgme_data_release (parms->out);

  /* Make sure the normal passphrase callback is used */
  if (!cms_hack)
    gpgme_set_passphrase_cb (ctx->ctx,
                             passwd_parms->func, passwd_parms->opaque);

  if (parms->signal_id != 0)
    {
      /* Don't run this signal handler again if the context is reused */
      g_signal_handler_disconnect (ctx, parms->signal_id);
    }

  g_free (parms);
  g_free (parms->opaque);
}

/* Generate the edit parameters needed for changing the passphrase.
 */
static struct edit_parms_s*
gpa_gpgme_edit_passwd_parms_new (GpaContext *ctx, gpgme_data_t out)
{
  struct edit_parms_s *edit_parms = g_malloc0 (sizeof (struct edit_parms_s));
  struct passwd_parms_s *passwd_parms = g_malloc0 (sizeof
                                                   (struct passwd_parms_s));

  edit_parms->state = PASSWD_START;
  edit_parms->action = edit_passwd_fnc_action;
  edit_parms->transit = edit_passwd_fnc_transit;
  edit_parms->out = out;
  edit_parms->opaque = passwd_parms;
  gpgme_get_passphrase_cb (ctx->ctx, &passwd_parms->func,
			   &passwd_parms->opaque);

  /* Make sure the cleanup is run when the edit completes */
  edit_parms->signal_id =
    g_signal_connect (G_OBJECT (ctx), "done",
		      G_CALLBACK (gpa_gpgme_edit_passwd_parms_release),
		      edit_parms);

  return edit_parms;
}

gpg_error_t
gpa_gpgme_edit_passwd_start (GpaContext *ctx, gpgme_key_t key)
{
  struct edit_parms_s *parms;
  gpg_error_t err;
  gpgme_data_t out;

  err = gpgme_data_new (&out);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }

  parms = gpa_gpgme_edit_passwd_parms_new (ctx, out);

  /* Use our own passphrase callback: The data are the actual edit
     parms.  */
  if (!cms_hack)
    gpgme_set_passphrase_cb (ctx->ctx, passwd_passphrase_cb, parms);

  err = gpgme_op_interact_start (ctx->ctx, key, 0, edit_fnc, parms, out);
  return err;
}



/*
 * OpenPGP card key generation.  Although we do all other card
 * operation directly via gpg-agent/scdaemon, we need to use an edit
 * interactor for the key generation because that involves OpenPGP key
 * management as well.
 *
 * Note that on-disk key generation is done using a parameter file and
 * thus there is no edit function for that (cf. gpgmetools.c).
 */

struct genkey_parms_s
{
  int  post_default_state;
  char expiration_day[11];	/* "YYYY-MM-DD" or "0". */
  char *name;
  char *email;
  char *comment;
  gboolean backup;
  char **error_desc_adr;
};


static gpg_error_t
card_edit_genkey_fnc_action (int state, void *opaque, char **result)
{
  struct genkey_parms_s *parms = opaque;

  switch (state)
    {
    case CARD_DEFAULT:
      /* Return an empty line to indicate that the default is to be used.  */
      *result = "";
      break;

    case CARD_COMMAND:
      *result = "admin";
      break;

    case CARD_ADMIN_COMMAND:
      *result = "generate";
      break;

    case CARD_GENERATE_BACKUP:
      *result = parms->backup? "Y":"N";
      break;

    case CARD_GENERATE_REPLACE_KEYS:
      {
        GtkWidget *dialog;

        dialog = gtk_message_dialog_new
          (NULL, GTK_DIALOG_MODAL,
           GTK_MESSAGE_WARNING,  GTK_BUTTONS_OK_CANCEL,
           _("Keys are already stored on the card.\n"
             "Really replace existing keys?"));

        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK )
          *result = "Y";
        else
          *result = "N";
        gtk_widget_destroy (dialog);
      }
      break;

    case CARD_GENERATE_VALIDITY:
      *result = parms->expiration_day;
      break;

    case CARD_GENERATE_NAME:
      *result = parms->name;
      break;

    case CARD_GENERATE_EMAIL:
      *result = parms->email;
      break;

    case CARD_GENERATE_COMMENT:
      *result = parms->comment;
      break;

    case CARD_GENERATE_DONE:
      *result = NULL;
      break;

    case CARD_QUIT:
      *result = "quit";
      break;

    default:
      return unexpected_state (state);
    }

  return 0;
}


static int
card_edit_genkey_fnc_transit (int current_state, status_type_t status,
                              const char *args, void *opaque, gpg_error_t *err)
{
  struct genkey_parms_s *genkey_parms = opaque;
  int next_state;

  if (current_state == CARD_DEFAULT)
    current_state = genkey_parms->post_default_state;

  switch (current_state)
    {
    case CARD_START:
      if (CMP_STATUS (GET_LINE) && !strcmp (args, "cardedit.prompt"))
	next_state = CARD_COMMAND;
      else
        goto bad_state;
      break;

    case CARD_COMMAND:
      if (CMP_STATUS (GET_LINE) && !strcmp (args, "cardedit.prompt"))
        next_state = CARD_ADMIN_COMMAND;
      else
        goto bad_state;
      break;

    case CARD_ADMIN_COMMAND:
      if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          if (!strcmp (args, "cardedit.genkeys.backup_enc"))
            next_state = CARD_GENERATE_BACKUP;
          else if (!strcmp (args, "cardedit.prompt"))
            goto unexpected_prompt;
          else
            {
              genkey_parms->post_default_state = current_state;
              next_state = CARD_DEFAULT;
            }
        }
      else
        goto bad_state;
      break;

    case CARD_GENERATE_BACKUP:
      if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          if (!strcmp (args, "cardedit.genkeys.replace_keys"))
            next_state = CARD_GENERATE_REPLACE_KEYS;
          else if (!strcmp (args, "keygen.valid"))
            next_state = CARD_GENERATE_VALIDITY;
          else if (!strcmp (args, "cardedit.prompt"))
            goto unexpected_prompt;
          else
            {
              genkey_parms->post_default_state = current_state;
              next_state = CARD_DEFAULT;
            }
        }
      else
        goto bad_state;
      break;

    case CARD_GENERATE_REPLACE_KEYS:
      if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          if (!strcmp (args, "keygen.valid"))
            next_state = CARD_GENERATE_VALIDITY;
          else if (!strcmp (args, "cardedit.prompt"))
            goto unexpected_prompt;
          else
            {
              genkey_parms->post_default_state = current_state;
              next_state = CARD_DEFAULT;
            }
        }
      else
        goto bad_state;
      break;

    case CARD_GENERATE_VALIDITY:
      if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          if (!strcmp (args, "keygen.name"))
            next_state = CARD_GENERATE_NAME;
          else if (!strcmp (args, "cardedit.prompt"))
            goto unexpected_prompt;
          else
            {
              genkey_parms->post_default_state = current_state;
              next_state = CARD_DEFAULT;
            }
        }
      else
        goto bad_state;
      break;

    case CARD_GENERATE_NAME:
      if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          if (!strcmp (args, "keygen.email"))
            next_state = CARD_GENERATE_EMAIL;
          else if (!strcmp (args, "cardedit.prompt")
                   || !strcmp (args, "keygen.name"))
            goto unexpected_prompt;
          else
            {
              genkey_parms->post_default_state = current_state;
              next_state = CARD_DEFAULT;
            }
        }
      else
        goto bad_state;
      break;

    case CARD_GENERATE_EMAIL:
      if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          if (!strcmp (args, "keygen.comment"))
            next_state = CARD_GENERATE_COMMENT;
          else if (!strcmp (args, "cardedit.prompt")
                   || !strcmp (args, "keygen.email"))
            goto unexpected_prompt;
          else
            {
              genkey_parms->post_default_state = current_state;
              next_state = CARD_DEFAULT;
            }
        }
      else
        goto bad_state;
      break;

    case CARD_GENERATE_COMMENT:
      if (CMP_STATUS (KEY_CREATED))
	next_state = CARD_GENERATE_DONE;
      else if (!strcmp (args, "keygen.comment"))
        goto unexpected_prompt;
      else
        goto bad_state;
      break;

    case CARD_GENERATE_DONE:
      if (CMP_STATUS (GET_LINE) || CMP_STATUS (GET_BOOL))
        {
          if (!strcmp (args, "cardedit.prompt"))
            next_state = CARD_QUIT;
          else
            {
              genkey_parms->post_default_state = current_state;
              next_state = CARD_DEFAULT;
            }
        }
      else
        goto bad_state;
      break;

    bad_state:
    unexpected_prompt:
    default:
      next_state = CARD_ERROR;
      *err = gpg_error (GPG_ERR_GENERAL);
      break;
    }

  return next_state;
}



static void
card_edit_genkey_save_error (void *opaque, char *errdesc)
{
  struct genkey_parms_s *parms = opaque;

  if (*parms->error_desc_adr)
    xfree (errdesc); /* Already set.  */
  else
    *parms->error_desc_adr = errdesc;
}


static void
card_edit_genkey_parms_release (GpaContext *ctx, gpg_error_t err,
				struct edit_parms_s *parms)
{
  gpgme_data_release (parms->out);
  if (parms->signal_id != 0)
    {
      /* Don't run this signal handler again if the context is reused */
      g_signal_handler_disconnect (ctx, parms->signal_id);
    }
  g_free (parms->opaque);
  g_free (parms);
}


/* Generate the edit parameters needed for setting owner trust.  */
static struct edit_parms_s *
card_edit_genkey_parms_new (GpaContext *ctx,
                            gpa_keygen_para_t *parms, gpgme_data_t out)
{
  struct edit_parms_s *edit_parms;
  struct genkey_parms_s *genkey_parms;

  edit_parms = xcalloc (1, sizeof *edit_parms);
  genkey_parms = xcalloc (1, sizeof *genkey_parms);

  edit_parms->state = CARD_START;
  edit_parms->action = card_edit_genkey_fnc_action;
  edit_parms->transit = card_edit_genkey_fnc_transit;
  edit_parms->save_error = card_edit_genkey_save_error;
  edit_parms->out = out;
  edit_parms->opaque = genkey_parms;

  assert (sizeof (genkey_parms->expiration_day) > 10);
  if (g_date_valid (&parms->expire))
    snprintf (genkey_parms->expiration_day,
              sizeof genkey_parms->expiration_day,
              "%04d-%02d-%02d",
              g_date_get_year (&parms->expire),
              g_date_get_month (&parms->expire),
              g_date_get_day (&parms->expire));
  else /* Never expire.  */
    strcpy (genkey_parms->expiration_day, "0");

  genkey_parms->name = parms->name;
  genkey_parms->email = parms->email;
  genkey_parms->comment = parms->comment;
  genkey_parms->backup = parms->backup;
  genkey_parms->error_desc_adr = &parms->r_error_desc;

  /* Make sure the cleanup is run when the edit completes */
  edit_parms->signal_id =
    g_signal_connect (G_OBJECT (ctx), "done",
		      G_CALLBACK (card_edit_genkey_parms_release),
		      edit_parms);

  return edit_parms;
}


gpg_error_t
gpa_gpgme_card_edit_genkey_start (GpaContext *ctx,
                                  gpa_keygen_para_t *genkey_parms)
{
  struct edit_parms_s *edit_parms;
  gpgme_data_t out = NULL;
  gpg_error_t err;

  err = gpgme_data_new (&out);
  if (err)
    return err;

  edit_parms = card_edit_genkey_parms_new (ctx, genkey_parms, out);

  err = gpgme_op_interact_start (ctx->ctx, NULL, GPGME_INTERACT_CARD,
                                 edit_fnc, edit_parms, out);
  return err;
}
