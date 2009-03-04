/* cm-netkey.c  -  Widget to show information about a Netkey card.
 * Copyright (C) 2009 g10 Code GmbH
 *
 * This file is part of GPA.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gpa.h"   
#include "gtktools.h"
#include "convert.h"

#include "cm-object.h"
#include "cm-netkey.h"




/* Identifiers for the entry fields.  */
enum
  {
    ENTRY_SERIALNO,
    ENTRY_NKS_VERSION,

    ENTRY_PIN_RETRYCOUNTER,
    ENTRY_PUK_RETRYCOUNTER,

    ENTRY_LAST
  }; 


/* A structure for PIN information.  */
struct pininfo_s
{
  int valid;            /* Valid information.  */
  int nullpin;          /* The NullPIN is activ.  */
  int blocked;          /* The PIN is blocked.  */
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

  GtkWidget *entries[ENTRY_LAST];

  GtkWidget *change_pin_btn; /* The button to change the PIN.  */
  GtkWidget *change_puk_btn; /* The button to change the PUK.  */

  /* Information about the PINs. */
  struct pininfo_s pininfo[3];
};

/* The parent class.  */
static GObjectClass *parent_class;



/* Local prototypes */
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


struct scd_getattr_parm
{
  GpaCMNetkey *card;  /* The object.  */
  const char *name;     /* Name of expected attribute.  */
  int entry_id;        /* The identifier for the entry.  */
  void (*updfnc) (GpaCMNetkey *card, int entry_id, const char *string);
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
          if (parm->updfnc)
            parm->updfnc (parm->card, entry_id, args);
          else if (GTK_IS_LABEL (parm->card->entries[entry_id]))
            gtk_label_set_text 
              (GTK_LABEL (parm->card->entries[entry_id]), args);
          else
            gtk_entry_set_text 
              (GTK_ENTRY (parm->card->entries[entry_id]), args);
        }
    }

  return 0;
}     


/* Data callback used by check_nullpin. */
static gpg_error_t
check_nullpin_data_cb (void *opaque, const void *data_arg, size_t datalen)
{
  int *pinstat = opaque;
  const unsigned char *data = data_arg;

  if (datalen >= 2)
    {
      unsigned int sw = ((data[datalen-2] << 8) | data[datalen-1]);

      if (sw == 0x6985)
        *pinstat = -2;  /* NullPIN is activ.  */
      else if (sw == 0x6983)
        *pinstat = -3; /* PIN is blocked.  */
      else if ((sw & 0xfff0) == 0x63C0)
        *pinstat = (sw & 0x000f); /* PIN has N tries left.  */
      else
        ; /* Probably no such PIN. */
    }
  return 0;
}     


/* Check whether the NullPIN is still active.  */
static void
check_nullpin (GpaCMNetkey *card)
{
  gpg_error_t err;
  gpgme_ctx_t gpgagent;
  int idx;

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);

  for (idx=0; idx < DIM (card->pininfo); idx++)
    memset (&card->pininfo[idx], 0, sizeof card->pininfo[0]);

  /* A TCOS card responds to a verify with empty data (i.e. without
     the Lc byte) with the status of the PIN.  The PIN is given as
     usual as P2. */
  for (idx=0; idx < DIM (card->pininfo) && idx < 2; idx++)
    {
      int pinstat = -1;
      char command[100];
      
      snprintf (command, sizeof command, "SCD APDU 00:20:00:%02x", idx);

      err = gpgme_op_assuan_transact (gpgagent, command,
                                      check_nullpin_data_cb, &pinstat,
                                      NULL, NULL,
                                      NULL, NULL);
      if (!err)
        err = gpgme_op_assuan_result (gpgagent)->err;
      if (err)
        g_debug ("assuan empty verify command failed: %s <%s>\n", 
                 gpg_strerror (err), gpg_strsource (err));
      else
        {
          card->pininfo[idx].valid = 1;
          if (pinstat == -2)
            card->pininfo[idx].nullpin = 1;
          else if (pinstat == -3)
            card->pininfo[idx].blocked = 1;
          else if (pinstat >= 0)
            card->pininfo[idx].tries_left = pinstat;
          else
            card->pininfo[idx].valid = 0;
        }
    }
}


/* Put the PIN information into the field with ENTRY_ID.  If BUTTON is
   not NULL its sensitivity is set as well. */
static void
update_entry_retry_counter (GpaCMNetkey *card, int entry_id, 
                            struct pininfo_s *info, int pin0_isnull,
                            GtkWidget *button)
{
  char numbuf[50];
  const char *string;
  
  if (!info->valid)
    string = _("unknown");
  else if (info->nullpin)
    string = "NullPIN";
  else if (info->blocked)
    string = _("blocked");
  else
    {
      snprintf (numbuf, sizeof numbuf, "%d", info->tries_left);
      string = numbuf;
    }
  gtk_label_set_text (GTK_LABEL (card->entries[entry_id]), string);
  if (button)
    gtk_widget_set_sensitive (button, (info->valid && !pin0_isnull
                                       && !info->nullpin && !info->blocked));
}


/* Use the assuan machinery to load the bulk of the OpenPGP card data.  */
static void
reload_data (GpaCMNetkey *card)
{
  static struct {
    const char *name;
    int entry_id;
    void (*updfnc) (GpaCMNetkey *card, int entry_id,  const char *string);
  } attrtbl[] = {
    { "SERIALNO",    ENTRY_SERIALNO },
    { "NKS-VERSION", ENTRY_NKS_VERSION },
    { NULL }
  };
  int attridx;
  gpg_error_t err;
  char command[100];
  struct scd_getattr_parm parm;
  gpgme_ctx_t gpgagent;
  int pin0_isnull;

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);

  check_nullpin (card);

  parm.card = card;
  for (attridx=0; attrtbl[attridx].name; attridx++)
    {
      parm.name     = attrtbl[attridx].name;
      parm.entry_id = attrtbl[attridx].entry_id;
      parm.updfnc   = attrtbl[attridx].updfnc;
      snprintf (command, sizeof command, "SCD GETATTR %s", parm.name);
      err = gpgme_op_assuan_transact (gpgagent,
                                      command,
                                      NULL, NULL,
                                      NULL, NULL,
                                      scd_getattr_cb, &parm);
      if (!err)
        err = gpgme_op_assuan_result (gpgagent)->err;

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

  pin0_isnull = (card->pininfo[0].valid && card->pininfo[0].nullpin);
  if (pin0_isnull)
    gtk_widget_show_all (card->warning_frame);
  else
    gtk_widget_hide_all (card->warning_frame);

  update_entry_retry_counter (card, ENTRY_PIN_RETRYCOUNTER, &card->pininfo[0],
                              pin0_isnull, card->change_pin_btn);
  update_entry_retry_counter (card, ENTRY_PUK_RETRYCOUNTER, &card->pininfo[1],
                              pin0_isnull, card->change_puk_btn);
}


static void
change_nullpin (GpaCMNetkey *card)
{
  GtkMessageDialog *dialog;
  gpgme_ctx_t gpgagent;
  gpg_error_t err;

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);
  
  /* FIXME:  How do we figure out our GtkWindow?  */
  dialog = GTK_MESSAGE_DIALOG (gtk_message_dialog_new 
                               (NULL /*GTK_WINDOW (card)*/,
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_MESSAGE_INFO, 
                                GTK_BUTTONS_OK_CANCEL, NULL));
  gtk_message_dialog_set_markup 
    (dialog, _("<b>Setting the Initial PIN</b>\n\n"
               "You selected to set the initial PIN of your card.  "
               "The PIN is currently set to the NullPIN, setting an "
               "initial PIN is thus <b>required but can't be reverted</b>.\n\n"
               "Please check the documentation of your card to learn "
               "for what the NullPIN is good.\n\n"
               "If you proceeed you will be asked to enter a new PIN "
               "and later to repeat that PIN.  Make sure that you "
               "will remember that PIN - it will not be possible to "
               "recover the PIN if it has been entered wrongly more "
               "than 2 times." ));
                                 
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK )
    {
      err = gpgme_op_assuan_transact (gpgagent,
                                      "SCD PASSWD --nullpin 0",
                                      NULL, NULL,
                                      NULL, NULL,
                                      NULL, NULL);
      if (!err)
        err = gpgme_op_assuan_result (gpgagent)->err;
      if (err && gpg_err_code (err) != GPG_ERR_CANCELED)
        {
          char *message = g_strdup_printf 
            (_("Error changing the NullPIN.\n"
               "(%s <%s>)"), gpg_strerror (err), gpg_strsource (err));
          gpa_window_error (message, NULL);
          xfree (message);
        }
    }
  gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
set_initial_pin_clicked_cb (GtkButton *widget, void *user_data)
{
  GpaCMNetkey *card = user_data;

  change_nullpin (card);
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
    (_("<b>The NullPIN is still active on this card</b>.\n"
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


  /* PIN frame.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>PIN</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  table = gtk_table_new (2, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);
  rowidx = 0;

  card->entries[ENTRY_PIN_RETRYCOUNTER] = gtk_label_new (NULL);
  button = gtk_button_new_with_label (_("Change PIN"));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
  add_table_row (table, &rowidx, 
                 _("PIN retry counter:"), 
                 card->entries[ENTRY_PIN_RETRYCOUNTER], button, 1);
  card->change_pin_btn = button; 

  card->entries[ENTRY_PUK_RETRYCOUNTER] = gtk_label_new (NULL);
  button = gtk_button_new_with_label (_("Change PUK"));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_HALF);
  add_table_row (table, &rowidx, 
                 _("PUK retry counter:"), 
                 card->entries[ENTRY_PUK_RETRYCOUNTER], button, 1);
  card->change_puk_btn = button; 

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
