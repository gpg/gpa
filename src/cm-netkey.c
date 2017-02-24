/* cm-netkey.c  -  Widget to show information about a Netkey card.
 * Copyright (C) 2009 g10 Code GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute and/or modify this part
 * of GPA under the terms of either
 *
 *   - the GNU Lesser General Public License as published by the Free
 *     Software Foundation; either version 3 of the License, or (at
 *     your option) any later version.
 *
 * or
 *
 *   - the GNU General Public License as published by the Free
 *     Software Foundation; either version 2 of the License, or (at
 *     your option) any later version.
 *
 * or both in parallel, as here.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gpa.h"
#include "gtktools.h"
#include "convert.h"
#include "gpa-key-details.h"

#include "cm-object.h"
#include "cm-netkey.h"




/* Identifiers for the entry fields.  */
enum
  {
    ENTRY_SERIALNO,
    ENTRY_NKS_VERSION,

    ENTRY_PIN_RETRYCOUNTER,
    ENTRY_PUK_RETRYCOUNTER,
    ENTRY_SIGG_PIN_RETRYCOUNTER,
    ENTRY_SIGG_PUK_RETRYCOUNTER,

    ENTRY_LAST
  };


/* A structure for PIN information.  */
struct pininfo_s
{
  int valid;            /* Valid information.  */
  int nullpin;          /* The NullPIN is activ.  */
  int blocked;          /* The PIN is blocked.  */
  int nopin;            /* No such PIN.  */
  int tries_left;       /* How many verification tries are left.  */
};



/* Object's class definition.  */
struct _GpaCMNetkeyClass
{
  GpaCMObjectClass parent_class;
};


/* Object definition.  */
struct _GpaCMNetkey
{
  GpaCMObject  parent_instance;

  GtkWidget *warning_frame;  /* The frame used to display warnings etc.  */

  GtkWidget *keys_frame;     /* The frame containing the keys.  */

  GtkWidget *entries[ENTRY_LAST];

  GtkWidget *change_pin_btn;      /* The button to change the PIN for NKS.  */
  GtkWidget *change_puk_btn;      /* The button to change the PUK for NKS.  */
  GtkWidget *change_sigg_pin_btn; /* The button to change the PIN for SigG. */
  GtkWidget *change_sigg_puk_btn; /* The button to change the PUK for SigG. */

  /* Information about the PINs. */
  struct pininfo_s pininfo[4];


  int  reloading;   /* Sentinel to avoid recursive reloads.  */

};

/* The parent class.  */
static GObjectClass *parent_class;



/* Local prototypes */
static void learn_keys_clicked_cb (GtkButton *widget, void *user_data);
static void gpa_cm_netkey_finalize (GObject *object);



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/

/* Clears the info contained in the card widget. */
static void
clear_card_data (GpaCMNetkey *card)
{
  int idx;

  for (idx=0; idx < ENTRY_LAST; idx++)
    if (GTK_IS_LABEL (card->entries[idx]))
      gtk_label_set_text (GTK_LABEL (card->entries[idx]), "");
    else
      gtk_entry_set_text (GTK_ENTRY (card->entries[idx]), "");
}


/* Put the PIN information into the field with ENTRY_ID.  If BUTTON is
   not NULL its sensitivity is set as well. */
static void
update_entry_retry_counter (GpaCMNetkey *card, int entry_id,
                            struct pininfo_s *info, int any_isnull,
                            int is_puk, GtkWidget *button)
{
  char numbuf[50];
  const char *string;

  if (!info->valid)
    string = _("unknown");
  else if (info->nullpin)
    string = "NullPIN";
  else if (info->blocked)
    string = _("blocked");
  else if (info->nopin)
    string = "n/a";
  else
    {
      snprintf (numbuf, sizeof numbuf, "%d", info->tries_left);
      string = numbuf;
    }
  gtk_label_set_text (GTK_LABEL (card->entries[entry_id]), string);
  if (button)
    {
      gtk_button_set_label (GTK_BUTTON (button),
                            (info->valid && !info->nullpin
                             && (info->blocked || !info->tries_left))
                            ? (is_puk? _("Reset PUK") : _("Reset PIN"))
                            : (is_puk? _("Change PUK"): _("Change PIN")));
      gtk_widget_set_sensitive (button,
                                (info->valid && !any_isnull
                                 && !info->nullpin && !info->blocked
                                 && !info->nopin));
    }
}



/* Update the CHV status fields.  This function may modify STRING.  */
static void
update_entry_chv_status (GpaCMNetkey *card, int entry_id, char *string)
{
  struct {
    int info_idx;
    int is_puk;
    int entry_id;
    GtkWidget *widget;
  } tbl[] =
    {
      {  0, 0, ENTRY_PIN_RETRYCOUNTER },
      {  1, 1, ENTRY_PUK_RETRYCOUNTER  },
      {  2, 0, ENTRY_SIGG_PIN_RETRYCOUNTER },
      {  3, 1, ENTRY_SIGG_PUK_RETRYCOUNTER },
      { -1, 0 }
    };
  int idx;
  int any_isnull = 0;

  tbl[0].widget = card->change_pin_btn;
  tbl[1].widget = card->change_puk_btn;
  tbl[2].widget = card->change_sigg_pin_btn;
  tbl[3].widget = card->change_sigg_puk_btn;

  (void)entry_id; /* Not used.  */

  for (idx=0; idx < DIM (card->pininfo); idx++)
    memset (&card->pininfo[idx], 0, sizeof card->pininfo[0]);

  while (spacep (string))
    string++;
  for (idx=0; *string && idx < DIM (card->pininfo); idx++)
    {
      int value = atoi (string);
      while (*string && !spacep (string))
        string++;
      while (spacep (string))
        string++;

      card->pininfo[idx].valid = 1;
      if (value >= 0)
        card->pininfo[idx].tries_left = value;
      else if (value == -4)
        {
          card->pininfo[idx].nullpin = 1;
          any_isnull = 1;
        }
      else if (value == -3)
        card->pininfo[idx].blocked = 1;
      else if (value == -2)
        card->pininfo[idx].nopin = 1;
      else
        card->pininfo[idx].valid = 0;
    }

  for (idx=0; tbl[idx].info_idx != -1; idx++)
    if (tbl[idx].info_idx < DIM (card->pininfo))
      update_entry_retry_counter (card, tbl[idx].entry_id,
                                  &card->pininfo[tbl[idx].info_idx],
                                  any_isnull,
                                  tbl[idx].is_puk, tbl[idx].widget);

  gtk_widget_set_no_show_all (card->warning_frame, FALSE);
  if (any_isnull)
    gtk_widget_show_all (card->warning_frame);
  else
    {
      gtk_widget_hide_all (card->warning_frame);
      gtk_widget_set_no_show_all (card->warning_frame, TRUE);
    }
}


/* Structure form comminucation between reload_more_data and
   reload_more_data_cb.  */
struct reload_more_data_parm
{
  GpaCMNetkey *card; /* self  */
  gpgme_ctx_t ctx;   /* A prepared context for relaod_more_data_cb.  */
  int any_unknown;   /* Set if at least one key is not known.  */
};


/* Helper for relaod_more_data.  This is actually an Assuan status
   callback  */
static gpg_error_t
reload_more_data_cb (void *opaque, const char *status, const char *args)
{
  struct reload_more_data_parm *parm = opaque;
  gpgme_key_t key = NULL;
  const char *s;
  char pattern[100];
  int idx;
  gpg_error_t err;
  const char *keyid;
  int any = 0;

  g_debug ("  reload_more_data (%s=%s)", status, args);
  if (strcmp (status, "KEYPAIRINFO") )
    return 0;

  g_debug ("   start search");
  idx = 0;
  pattern[idx++] = '&';
  for (s=args; hexdigitp (s) && idx < sizeof pattern - 1; s++)
    pattern[idx++] = *s;
  pattern[idx] = 0;
  if (!spacep (s) || (s - args != 40))
    return 0;  /* Invalid formatted keygrip.  */
  while (spacep (s))
    s++;
  keyid = s;

  if (!(err=gpgme_op_keylist_start (parm->ctx, pattern, 0)))
    {
      if (!(err=gpgme_op_keylist_next (parm->ctx, &key)))
        {
          GtkWidget *vbox, *expander, *details, *hbox, *label;

          vbox = gtk_bin_get_child (GTK_BIN (parm->card->keys_frame));
          if (!vbox)
            g_debug ("Ooops, vbox missing in key frame");
          else
            {
              expander = gtk_expander_new (keyid);
              details = gpa_key_details_new ();
              gtk_container_add (GTK_CONTAINER (expander), details);
              gpa_key_details_update (details, key, 1);

              hbox = gtk_hbox_new (FALSE, 0);
              label = gtk_label_new (NULL);
              gtk_label_set_width_chars  (GTK_LABEL (label), 22);
              gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
              gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
              gtk_box_pack_start (GTK_BOX (hbox), expander, TRUE, TRUE, 0);
              gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

              any = 1;
            }
          gpgme_key_unref (key);
        }
    }
  gpgme_op_keylist_end (parm->ctx);
  if (!any)
    parm->any_unknown = 1;
  g_debug ("   ready");
  return 0;
}


/* Reload more data.  This function is called from the idle handler.  */
static void
reload_more_data (GpaCMNetkey *card)
{
  gpg_error_t err, operr;
  gpgme_ctx_t gpgagent;
  GtkWidget *vbox;
  struct reload_more_data_parm parm;

  g_debug ("start reload_more_data (count=%d)", card->reloading);
  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);
  g_return_if_fail (card->keys_frame);
  g_debug ("  gpgagent=%p", gpgagent);

  /* We remove any existing children of the keys frame and then we add
     a new vbox to be filled with new widgets by the callback.  */
  vbox = gtk_bin_get_child (GTK_BIN (card->keys_frame));
  if (vbox)
    gtk_widget_destroy (vbox);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (card->keys_frame), vbox);

  /* Create a context for key listings.  */
  parm.any_unknown = 0;
  parm.card = card;
  err = gpgme_new (&parm.ctx);
  if (err)
    {
      /* We don't want an error window because we are run from an idle
         handler and the information is not that important.  */
      g_debug ("failed to create a context: %s", gpg_strerror (err));
      return;
    }
  gpgme_set_protocol (parm.ctx, GPGME_PROTOCOL_CMS);
  /* We include ephemeral keys in the listing.  */
  gpgme_set_keylist_mode (parm.ctx, GPGME_KEYLIST_MODE_EPHEMERAL);

  g_debug ("  parm.ctx=%p", parm.ctx);

  err = gpgme_op_assuan_transact_ext (gpgagent,
                                      "SCD LEARN --keypairinfo",
                                      NULL, NULL, NULL, NULL,
                                      reload_more_data_cb, &parm, &operr);
  g_debug ("  assuan ret=%d", err);
  if (!err)
    err = operr;

  if (err)
    g_debug ("SCD LEARN failed: %s", gpg_strerror (err));

  if (parm.any_unknown)
    {
      GtkWidget *button, *align;

      align = gtk_alignment_new (0.5, 0, 0, 0);
      button = gtk_button_new_with_label (_("Learn keys"));
      gpa_add_tooltip
        (button, _("For some or all of the keys available on the card, "
                   "the GnuPG crypto engine does not yet know the "
                   "corresponding certificates.\n"
                   "\n"
                   "If you click this button, GnuPG will be asked to "
                   "\"learn\" "
                   "this card and import all certificates stored on the "
                   "card into its own certificate store.  This is not done "
                   "automatically because it may take several seconds to "
                   "read all certificates from the card.\n"
                   "\n"
                   "If you are unsure what to do, just click the button."));
      gtk_container_add (GTK_CONTAINER (align), button);
      gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 5);
      gtk_box_reorder_child (GTK_BOX (vbox), align, 0);

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (learn_keys_clicked_cb), card);
    }

  gpgme_release (parm.ctx);
  gtk_widget_show_all (card->keys_frame);
  g_debug ("end   reload_more_data (count=%d)", card->reloading);
  return;
}


/* Idle queue callback to reload more data.  */
static gboolean
reload_more_data_idle_cb (void *user_data)
{
  GpaCMNetkey *card = user_data;

  if (card->reloading)
    {
      g_debug ("already reloading (count=%d)", card->reloading);
    return TRUE; /* Just in case we are still reloading, wait for the
                    next idle slot.  */
    }

  card->reloading++;
  reload_more_data (card);
  g_object_unref (card);
  card->reloading--;

  return FALSE;  /* Remove us from the idle queue.  */
}


struct scd_getattr_parm
{
  GpaCMNetkey *card;  /* The object.  */
  const char *name;   /* Name of expected attribute.  */
  int entry_id;       /* The identifier for the entry.  */
  void (*updfnc) (GpaCMNetkey *card, int entry_id, char *string);
};


static gpg_error_t
scd_getattr_cb (void *opaque, const char *status, const char *args)
{
  struct scd_getattr_parm *parm = opaque;
  int entry_id;

/*   g_debug ("STATUS_CB: status=`%s'  args=`%s'", status, args); */

  if (!strcmp (status, parm->name) )
    {
      entry_id = parm->entry_id;

      if (entry_id < ENTRY_LAST)
        {
          char *tmp = xstrdup (args);

          percent_unescape (tmp, 1);
          if (parm->updfnc)
            parm->updfnc (parm->card, entry_id, tmp);
          else if (GTK_IS_LABEL (parm->card->entries[entry_id]))
            gtk_label_set_text
              (GTK_LABEL (parm->card->entries[entry_id]), tmp);
          else
            gtk_entry_set_text
              (GTK_ENTRY (parm->card->entries[entry_id]), tmp);
          xfree (tmp);
        }
    }

  return 0;
}


/* Use the assuan machinery to load the bulk of the OpenPGP card data.  */
static void
reload_data (GpaCMNetkey *card)
{
  static struct {
    const char *name;
    int entry_id;
    void (*updfnc) (GpaCMNetkey *card, int entry_id, char *string);
  } attrtbl[] = {
    { "SERIALNO",    ENTRY_SERIALNO },
    { "NKS-VERSION", ENTRY_NKS_VERSION },
    { "CHV-STATUS",  ENTRY_PIN_RETRYCOUNTER, update_entry_chv_status },
    { NULL }
  };
  int attridx;
  gpg_error_t err = 0;
  gpg_error_t operr;
  char command[100];
  struct scd_getattr_parm parm;
  gpgme_ctx_t gpgagent;

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);

  card->reloading++;
  g_debug ("uped reloading counter (count=%d)", card->reloading);

  /* Show all attributes.  */
  parm.card = card;
  for (attridx=0; attrtbl[attridx].name; attridx++)
    {
      parm.name     = attrtbl[attridx].name;
      parm.entry_id = attrtbl[attridx].entry_id;
      parm.updfnc   = attrtbl[attridx].updfnc;
      snprintf (command, sizeof command, "SCD GETATTR %s", parm.name);
      err = gpgme_op_assuan_transact_ext (gpgagent,
                                          command,
                                          NULL, NULL,
                                          NULL, NULL,
                                          scd_getattr_cb, &parm, &operr);
      if (!err)
        err = operr;

      if (err && attrtbl[attridx].entry_id == ENTRY_NKS_VERSION)
        {
          /* The NKS-VERSION is only supported by GnuPG > 2.0.11
             thus we ignore the error.  */
          gtk_label_set_text
            (GTK_LABEL (card->entries[attrtbl[attridx].entry_id]),
             _("unknown"));
        }
      else if (err)
        {
          if (gpg_err_code (err) == GPG_ERR_CARD_NOT_PRESENT)
            ; /* Lost the card.  */
          else
            {
              g_debug ("assuan command `%s' failed: %s <%s>\n",
                       command, gpg_strerror (err), gpg_strsource (err));
            }
          clear_card_data (card);
          break;
        }
    }
  if (!err)
    {
      g_object_ref (card);
      g_idle_add (reload_more_data_idle_cb, card);
    }
  card->reloading--;
  g_debug ("downed reloading counter (count=%d)", card->reloading);
}


/* A structure used to pass data to the learn_keys_gpg_status_cb.  */
struct learn_keys_gpg_status_parm
{
  GpaCMNetkey    *card;
  GtkWidget      *button;
  GtkProgressBar *pbar;
};


/* Helper for learn_keys_clicked_cb.  */
static gboolean
learn_keys_gpg_status_cb (void *opaque, char *line)
{
  struct learn_keys_gpg_status_parm *parm = opaque;

  if (!line)
    {
      /* We are finished with the command.  */
      /* Trigger a reload of the key data.  */
      g_object_ref (parm->card);
      g_idle_add (reload_more_data_idle_cb, parm->card);
      /* Cleanup.  */
      gtk_widget_destroy (parm->button);
      g_object_unref (parm->button);
      g_object_unref (parm->card);
      xfree (parm);
      return FALSE; /* (The return code does not matter here.)  */
    }

  if (!strncmp (line, "PROGRESS", 8))
    gtk_progress_bar_pulse (parm->pbar);

  return TRUE; /* Keep on running.  */
}


/* Fire up the learn command.  */
static void
learn_keys_clicked_cb (GtkButton *button, void *user_data)
{
  GpaCMNetkey *card = user_data;
  GtkWidget *widget;
  gpg_error_t err;
  struct learn_keys_gpg_status_parm *parm;

  parm = xcalloc (1, sizeof *parm);

  gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
  widget = gtk_bin_get_child (GTK_BIN (button));
  if (widget)
    gtk_widget_destroy (widget);

  widget = gtk_progress_bar_new ();
  gtk_container_add (GTK_CONTAINER (button), widget);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (widget),
                              _("Learning keys ..."));
  gtk_widget_show_all (GTK_WIDGET (button));

  g_object_ref (card);
  parm->card = card;
  g_object_ref (button);
  parm->button = GTK_WIDGET (button);
  parm->pbar = GTK_PROGRESS_BAR (widget);

  err = gpa_start_simple_gpg_command
    (learn_keys_gpg_status_cb, parm, GPGME_PROTOCOL_CMS, 1,
     "--learn-card", "-v", NULL);
  if (err)
    {
      g_debug ("error starting gpg: %s", gpg_strerror (err));
      learn_keys_gpg_status_cb (parm, NULL);
      return;
    }
}


/* Run the dialog to change the NullPIN.  */
static void
change_nullpin (GpaCMNetkey *card)
{
  gpg_error_t err, operr;
  GtkWidget *dialog;
  gpgme_ctx_t gpgagent;
  int is_sigg;
  char *string;
  int okay;

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);

  if (card->pininfo[0].valid && card->pininfo[0].nullpin)
    is_sigg = 0;
  else if (card->pininfo[2].valid && card->pininfo[2].nullpin)
    is_sigg = 1;
  else
    return; /* Oops: No NullPIN.  */

  string = g_strdup_printf
    (_("<b>Setting the Initial PIN</b> (%s)\n\n"
       "You selected to set the initial PIN of your card.  "
       "The PIN is currently set to the NullPIN.  Setting an "
       "initial PIN is <b>required but can't be reverted</b>.\n\n"
       "Please check the documentation of your card to learn "
       "for what the NullPIN is good.\n\n"
       "If you proceed you will be asked to enter a new PIN "
       "and later to repeat that PIN.  Make sure that you "
       "will remember that PIN - it will not be possible to "
       "recover the PIN if it has been entered wrongly more "
       "than %d times.\n\n%s"),
     is_sigg? "SigG" : "NKS",
     2,
     is_sigg
     ? _("You are now setting the PIN for the SigG key used to create "
         "<b>qualified signatures</b>.  You may want to set the "
         "PIN to the same value as used for the NKS keys.")
     : _("You are now setting the PIN for the NKS keys used for standard "
         "signatures, encryption and authentication."));

  /* FIXME:  How do we figure out our GtkWindow?  */
  dialog = gtk_message_dialog_new_with_markup (NULL /*GTK_WINDOW (card)*/,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK_CANCEL,
                                               NULL);
  gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), string);
  g_free (string);

  okay = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);
  if (okay)
    {
      err = gpgme_op_assuan_transact_ext (gpgagent,
                                          is_sigg
                                          ? "SCD PASSWD --nullpin PW1.CH.SIG"
                                          : "SCD PASSWD --nullpin PW1.CH",
                                          NULL, NULL,
                                          NULL, NULL,
                                          NULL, NULL, &operr);
      if (!err)
        err = operr;

      if (gpg_err_code (err) == GPG_ERR_CANCELED)
        okay = 0; /* No need to reload the data.  */
      else if (err)
        {
          char *message = g_strdup_printf
            (_("Error changing the NullPIN.\n"
               "(%s <%s>)"), gpg_strerror (err), gpg_strsource (err));
          gpa_window_error (message, NULL);
          xfree (message);
        }
    }
  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (okay)
    reload_data (card);
}



static void
change_or_reset_pin (GpaCMNetkey *card, int info_idx)
{
  gpg_error_t err, operr;
  GtkWidget *dialog;
  gpgme_ctx_t gpgagent;
  int reset_mode;
  const char *string;
  int is_puk;
  int okay;
  const char *pwidstr;

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);
  g_return_if_fail (info_idx < DIM (card->pininfo));

  if (!card->pininfo[info_idx].valid
      || card->pininfo[info_idx].nopin
      || card->pininfo[info_idx].nullpin)
    {
      g_debug ("oops: NullPIN or not valid");
      return;
    }
  reset_mode = (card->pininfo[info_idx].blocked
                || !card->pininfo[info_idx].tries_left);
  is_puk = !!(info_idx & 1);
  g_debug ("%s PIN at idx %d", reset_mode?"Resetting":"Changing", info_idx);
  switch (info_idx)
    {
    case 0: pwidstr = "PW1.CH"; break;
    case 1: pwidstr = "PW2.CH"; break;
    case 2: pwidstr = "PW1.CH.SIG"; break;
    case 3: pwidstr = "PW2.CH.SIG"; break;
    default: return;  /* Ooops.  */
    }

  string = (!reset_mode
            ? _("<b>Changing a PIN or PUK</b>\n"
                "\n"
                "If you proceed you will be asked to enter "
                "the current value and then to enter a new "
                "value and repeat that value at another prompt.\n"
                "\n"
                "Entering a wrong value for the current value "
                "decrements the retry counter.  If the retry counters "
                "of the PIN and the corresponding PUK are both down "
                "to zero, the keys controlled by the PIN are not anymore "
                "usable and there is no way to unblock them!")
            : is_puk
            ? _("<b>Resetting a PUK</b>\n"
                "\n"
                "Although <i>PUK</i> stands for <i>PIN Unblocking Code</i> "
                "the TCOS operating system of the NetKey card implements it "
                "as an alternative PIN and thus it is possible to use the "
                "PIN to unblock the PUK.\n"
                "\n"
                "If the PUK is blocked (the retry counter is down to zero), "
                "you may unblock it by using the non-blocked PIN.  The retry "
                "counter is then set back to its initial value.\n"
                "\n"
                "If you proceed you will be asked to enter the current "
                "value of the PIN and then to enter a new value for the "
                "blocked PUK and repeat that new value at another prompt.")
            : _("<b>Resetting a PIN</b>\n"
                "\n"
                "If the PIN is blocked (the retry counter is down to zero), "
                "you may unblock it by using the non-blocked PUK.  The retry "
                "counter is then set back to its initial value.\n"
                "\n"
                "If you proceed you will be asked to enter the current "
                "value of the PUK and then to enter a new value for the "
                "blocked PIN and repeat that new value at another prompt.")
            );

  /* FIXME:  How do we figure out our GtkWindow?  */
  dialog = gtk_message_dialog_new_with_markup (NULL /*GTK_WINDOW (card)*/,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK_CANCEL,
                                               NULL);
  gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), string);
  okay = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);
  if (okay)
    {
      char command[100];

      snprintf (command, sizeof command, "SCD PASSWD%s %s",
                reset_mode? " --reset":"", pwidstr);
      err = gpgme_op_assuan_transact_ext (gpgagent, command,
                                          NULL, NULL, NULL, NULL, NULL, NULL,
                                          &operr);
      if (!err)
        err = operr;

      if (gpg_err_code (err) == GPG_ERR_CANCELED)
        okay = 0; /* No need to reload the data.  */
      else if (err)
        {
          char *message = g_strdup_printf
            (_("Error changing or resetting the PIN/PUK.\n"
               "(%s <%s>)"), gpg_strerror (err), gpg_strsource (err));
          gpa_window_error (message, NULL);
          xfree (message);
        }
    }
  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (okay)
    reload_data (card);
}



static void
set_initial_pin_clicked_cb (GtkButton *widget, void *user_data)
{
  GpaCMNetkey *card = user_data;

  change_nullpin (card);
}


static void
change_pin_clicked_cb (void *widget, void *user_data)
{
  GpaCMNetkey *card = user_data;

  if (widget == card->change_pin_btn)
    change_or_reset_pin (card, 0);
  else if (widget == card->change_puk_btn)
    change_or_reset_pin (card, 1);
  else if (widget == card->change_sigg_pin_btn)
    change_or_reset_pin (card, 2);
  else if (widget == card->change_sigg_puk_btn)
    change_or_reset_pin (card, 3);
}


/* Helper for construct_data_widget.  Returns the label widget. */
static GtkLabel *
add_table_row (GtkWidget *table, int *rowidx,
               const char *labelstr, GtkWidget *widget, GtkWidget *widget2,
               int readonly)
{
  GtkWidget *label;
  int is_label = GTK_IS_LABEL (widget);

  label = gtk_label_new (labelstr);
  gtk_label_set_width_chars  (GTK_LABEL (label), 22);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0);

  if (is_label)
    gtk_misc_set_alignment (GTK_MISC (widget), 0, 0.5);

  if (readonly)
    {
      if (!is_label && GTK_IS_ENTRY (widget))
        {
          gtk_entry_set_has_frame (GTK_ENTRY (widget), FALSE);
          gtk_entry_set_editable (GTK_ENTRY (widget), FALSE);
        }
    }
  else
    {
      if (is_label)
        gtk_label_set_selectable (GTK_LABEL (widget), TRUE);
    }

  gtk_table_attach (GTK_TABLE (table), widget, 1, 2,
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0);
  if (widget2)
    gtk_table_attach (GTK_TABLE (table), widget2, 2, 3,
		      *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 5, 0);
  ++*rowidx;

  return GTK_LABEL (label);
}


/* This function constructs the container holding all widgets making
   up this data widget.  It is called during instance creation.  */
static void
construct_data_widget (GpaCMNetkey *card)
{
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *vbox, *hbox;
  GtkWidget *label;
  GtkWidget *button;
  int rowidx;

  /* General frame.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>General</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  table = gtk_table_new (2, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);
  rowidx = 0;

  card->entries[ENTRY_SERIALNO] = gtk_label_new (NULL);
  add_table_row (table, &rowidx, _("Serial number:"),
                 card->entries[ENTRY_SERIALNO], NULL, 0);

  card->entries[ENTRY_NKS_VERSION] = gtk_label_new (NULL);
  add_table_row (table, &rowidx, _("Card version:"),
                 card->entries[ENTRY_NKS_VERSION], NULL, 0);

  gtk_box_pack_start (GTK_BOX (card), frame, FALSE, TRUE, 0);


  /* Warning frame.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  vbox = gtk_vbox_new (FALSE, 5);
  label = gtk_label_new
    (_("<b>A NullPIN is still active on this card</b>.\n"
       "You need to set a real PIN before you can make use of the card."));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  hbox = gtk_alignment_new (0.5, 0.5, 0, 0);
  button = gtk_button_new_with_label (_("Set initial PIN"));
  gtk_container_add (GTK_CONTAINER (hbox), button);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_box_pack_start (GTK_BOX (card), frame, FALSE, TRUE, 0);
  card->warning_frame = frame;

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (set_initial_pin_clicked_cb), card);


  /* Keys frame.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>Keys</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

  /* There is only a label widget yet in the frame for now.  The real
     widgets are added while figuring out the keys of the card.  */
  label = gtk_label_new (_("scanning ..."));
  gtk_container_add (GTK_CONTAINER (frame), label);

  gtk_box_pack_start (GTK_BOX (card), frame, FALSE, TRUE, 0);
  card->keys_frame = frame;


  /* PIN frame.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>PIN</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  table = gtk_table_new (4, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);
  rowidx = 0;

  card->entries[ENTRY_PIN_RETRYCOUNTER] = gtk_label_new (NULL);
  button = gtk_button_new ();
  add_table_row (table, &rowidx, _("PIN retry counter:"),
                 card->entries[ENTRY_PIN_RETRYCOUNTER], button, 1);
  card->change_pin_btn = button;
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (change_pin_clicked_cb), card);

  card->entries[ENTRY_PUK_RETRYCOUNTER] = gtk_label_new (NULL);
  button = gtk_button_new ();
  add_table_row (table, &rowidx, _("PUK retry counter:"),
                 card->entries[ENTRY_PUK_RETRYCOUNTER], button, 1);
  card->change_puk_btn = button;
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (change_pin_clicked_cb), card);

  card->entries[ENTRY_SIGG_PIN_RETRYCOUNTER] = gtk_label_new (NULL);
  button = gtk_button_new ();
  add_table_row (table, &rowidx, _("SigG PIN retry counter:"),
                 card->entries[ENTRY_SIGG_PIN_RETRYCOUNTER], button, 1);
  card->change_sigg_pin_btn = button;
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (change_pin_clicked_cb), card);

  card->entries[ENTRY_SIGG_PUK_RETRYCOUNTER] = gtk_label_new (NULL);
  button = gtk_button_new ();
  add_table_row (table, &rowidx, _("SigG PUK retry counter:"),
                 card->entries[ENTRY_SIGG_PUK_RETRYCOUNTER], button, 1);
  card->change_sigg_puk_btn = button;
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (change_pin_clicked_cb), card);

  gtk_box_pack_start (GTK_BOX (card), frame, FALSE, TRUE, 0);
}



/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_cm_netkey_class_init (void *class_ptr, void *class_data)
{
  GpaCMNetkeyClass *klass = class_ptr;

  parent_class = g_type_class_peek_parent (klass);

  G_OBJECT_CLASS (klass)->finalize = gpa_cm_netkey_finalize;
}


static void
gpa_cm_netkey_init (GTypeInstance *instance, void *class_ptr)
{
  GpaCMNetkey *card = GPA_CM_NETKEY (instance);

  construct_data_widget (card);

}


static void
gpa_cm_netkey_finalize (GObject *object)
{
/*   GpaCMNetkey *card = GPA_CM_NETKEY (object); */

  parent_class->finalize (object);
}


/* Construct the class.  */
GType
gpa_cm_netkey_get_type (void)
{
  static GType this_type = 0;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaCMNetkeyClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_cm_netkey_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL, /* class_data */
	  sizeof (GpaCMNetkey),
	  0,    /* n_preallocs */
	  gpa_cm_netkey_init
	};

      this_type = g_type_register_static (GPA_CM_OBJECT_TYPE,
                                          "GpaCMNetkey",
                                          &this_info, 0);
    }

  return this_type;
}


/************************************************************
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_cm_netkey_new ()
{
  return GTK_WIDGET (g_object_new (GPA_CM_NETKEY_TYPE, NULL));
}


/* If WIDGET is of Type GpaCMNetkey do a data reload through the
   assuan connection.  */
void
gpa_cm_netkey_reload (GtkWidget *widget, gpgme_ctx_t gpgagent)
{
  if (GPA_IS_CM_NETKEY (widget))
    {
      GPA_CM_OBJECT (widget)->agent_ctx = gpgagent;
      if (gpgagent)
        reload_data (GPA_CM_NETKEY (widget));
    }
}
