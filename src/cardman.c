/* cardman.c  -  The GNU Privacy Assistant: card manager.
   Copyright (C) 2008 g10 Code GmbH

   This file is part of GPA.

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

/* The card manager window.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "gpa.h"   

#include "gtktools.h"
#include "gpawidgets.h"
#include "siglist.h"
#include "helpmenu.h"
#include "icons.h"
#include "cardman.h"
#include "convert.h"

#include "gpacardreloadop.h"
#include "gpagenkeycardop.h"


/* Object and class definition.  */
struct _GpaCardManager
{
  GtkWindow parent;

  GtkWidget *window;

  GtkUIManager *ui_manager;

  GtkWidget *card_widget;     /* The container of all the info widgets.  */
  GtkWidget *big_label;

  GtkWidget *general_frame;
  GtkWidget *personal_frame;
  GtkWidget *keys_frame;
  GtkWidget *pin_frame;

  GtkWidget *entryType;
  GtkWidget *entrySerialno;
  GtkWidget *entryVersion;
  GtkWidget *entryManufacturer;
  GtkWidget *entryLogin;
  GtkWidget *entryLanguage;
  GtkWidget *entryPubkeyUrl;
  GtkWidget *entryFirstName;
  GtkWidget *entryLastName;
  GtkWidget *entrySex;
  GtkWidget *entryKeySig;
  GtkWidget *entryKeyEnc;
  GtkWidget *entryKeyAuth;
  GtkWidget *entryPINRetryCounter;
  GtkWidget *entryAdminPINRetryCounter;
  GtkWidget *entrySigForcePIN;

  /* Labels in the status bar.  */
  GtkWidget *status_label;
  GtkWidget *status_text;

  int have_card;             /* True, if a supported card is in the reader.  */
  const char *cardtype;      /* String with the card type.  */
  int is_openpgp;            /* True if the card is an OpenPGP card.  */

  gpa_filewatch_id_t watch;  /* For watching the reader status file.  */
  int in_card_reload;        /* Sentinel for card_reload.  */
};

struct _GpaCardManagerClass 
{
  GtkWindowClass parent_class;
};

/* There is only one instance of the card manager class.  Use a global
   variable to keep track of it.  */
static GpaCardManager *instance;

/* We also need to save the parent class. */
static GObjectClass *parent_class;

#if 0
/* FIXME: I guess we should use a more intelligent handling for widget
   sensitivity similar to that in fileman.c. -mo */
/* Definition of the sensitivity function type.  */
typedef gboolean (*sensitivity_func_t)(gpointer);
#endif


/* Local prototypes */



/************************************************************ 
 *******************   Implementation   *********************
 ************************************************************/

/* Status bar handling.  */

static GtkWidget *
statusbar_new (GpaCardManager *cardman)
{
  GtkWidget *align;
  GtkWidget *hbox;
  GtkWidget *label;

  hbox = gtk_hbox_new (FALSE, 0);
 
  label = gtk_label_new (_("Status: "));
  cardman->status_label = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("");
  cardman->status_text = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
  
  align = gtk_alignment_new (0, 1, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), hbox);

  return align;
}


static void
statusbar_update (GpaCardManager *cardman, const char *text)
{
  gtk_label_set_text (GTK_LABEL (cardman->status_text), text);
}


static void
update_title (GpaCardManager *cardman)
{
  const char *title = _("GNU Privacy Assistant - Card Manager");

  if (cardman->have_card)
    gtk_window_set_title (GTK_WINDOW (cardman), title);
  else
    {
      char *tmp;
      
      tmp = g_strdup_printf ("%s (%s)", title, _("no card"));
      gtk_window_set_title (GTK_WINDOW (cardman), tmp);
      xfree (tmp);
    }
}

/* Clears the info contained in the card widget. */
static void
clear_card_data (GpaCardManager *cardman)
{
  gtk_entry_set_text (GTK_ENTRY (cardman->entryType), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entrySerialno), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryVersion), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryManufacturer), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryLogin), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryLanguage), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryPubkeyUrl), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryFirstName), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryLastName), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entrySex), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryKeySig), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryKeyEnc), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryKeyAuth), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryPINRetryCounter), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entryAdminPINRetryCounter), "");
  gtk_entry_set_text (GTK_ENTRY (cardman->entrySigForcePIN), "");
}

static void
update_info_visibility (GpaCardManager *cardman)
{
  if (cardman->have_card)
    {
      char *tmp = g_strdup_printf (_("%s card detected."), cardman->cardtype);
      
      statusbar_update (cardman, tmp);
      xfree (tmp);
      gtk_widget_hide_all (cardman->big_label);
      if (gtk_widget_get_no_show_all (cardman->card_widget) == TRUE)
	gtk_widget_set_no_show_all (cardman->card_widget, FALSE);
      gtk_widget_show_all (cardman->card_widget);
      if (!cardman->is_openpgp)
        {
          gtk_widget_hide_all (cardman->personal_frame);
          gtk_widget_hide_all (cardman->keys_frame);
        }
    }
  else
    {
      clear_card_data (cardman);
      statusbar_update (cardman, _("Checking for card..."));
      gtk_widget_hide_all (cardman->card_widget);
      gtk_widget_show_all (cardman->big_label);
    }
}


#ifdef HAVE_GPGME_OP_ASSUAN_TRANSACT  
static gpg_error_t
opass_status_cb (void *opaque, const char *status, const char *args)
{
  GpaCardManager *cardman = opaque;

  /* Just for experiments, we also print some info from the Geldkarte. */
  if (!strcmp (status, "SERIALNO"))
    gtk_entry_set_text (GTK_ENTRY (cardman->entrySerialno), args);
  else if (!strcmp (status, "X-BANKINFO"))
    gtk_entry_set_text (GTK_ENTRY (cardman->entryManufacturer), args);
  else if (!strcmp (status, "X-BALANCE"))
    gtk_entry_set_text (GTK_ENTRY (cardman->entryVersion), args);

  return 0;
}     
#endif /*HAVE_GPGME_OP_ASSUAN_TRANSACT*/


static void
get_serial_direct (GpaCardManager *cardman, int is_geldkarte)
{
#ifdef HAVE_GPGME_OP_ASSUAN_TRANSACT  
  gpg_error_t err;
  gpgme_ctx_t ctx;

  ctx = gpa_gpgme_new ();
  err = gpgme_set_protocol (ctx, GPGME_PROTOCOL_ASSUAN);
  if (err)
    goto leave;
      
  err = gpgme_op_assuan_transact (ctx, "SCD SERIALNO",
                                  NULL, NULL, NULL, NULL,
                                  opass_status_cb, cardman);
  if (!err)
    err = gpgme_op_assuan_result (ctx);

  if (is_geldkarte)
    err = gpgme_op_assuan_transact (ctx, "SCD LEARN --force",
                                    NULL, NULL, NULL, NULL,
                                    opass_status_cb, cardman);
  if (!err)
    err = gpgme_op_assuan_result (ctx);


 leave:
  gpgme_release (ctx);
  if (err)
    g_debug ("gpgme_op_assuan_transact failed: %s <%s>",
             gpg_strerror (err), gpg_strsource (err));
#endif /*HAVE_GPGME_OP_ASSUAN_TRANSACT*/
}




/* This is the callback used by the GpaCardReloadOp object. It's
   called for each data item during a reload operation and updates the
   according widgets. */
static void
card_reload_cb (void *opaque,
                const char *identifier, int idx, const void *value)
{
  GpaCardManager *cardman = opaque;
  const char *string = value;

  if (!strcmp (identifier, "AID"))
    {
      if (idx == 0)
        {
          cardman->have_card = !!*string;
          cardman->cardtype = "Unknown";
          cardman->is_openpgp = 0;
        }
      else if (idx == 1)
        {
          const char *cardtype;

          if (!strcmp (string, "openpgp-card"))
            {
              cardman->cardtype = cardtype = "OpenPGP";
              cardman->is_openpgp = 1;
            }
          else if (!strcmp (string, "netkey-card"))
            cardman->cardtype = cardtype = "NetKey";
          else if (!strcmp (string, "dinsig-card"))
            cardman->cardtype = cardtype = "DINSIG";
          else if (!strcmp (string, "pkcs15-card"))
            cardman->cardtype = cardtype = "PKCS#15";
          else if (!strcmp (string, "geldkarte-card"))
            cardman->cardtype = cardtype = "Geldkarte";
          else
            cardtype = string;

          gtk_entry_set_text (GTK_ENTRY (cardman->entryType), cardtype);
          if (!cardman->is_openpgp)
            {
              gtk_entry_set_text (GTK_ENTRY (cardman->entrySerialno), "");
              gtk_entry_set_text (GTK_ENTRY (cardman->entryVersion), "");
              gtk_entry_set_text (GTK_ENTRY (cardman->entryManufacturer), "");
              /* Try to get the serial number directly from the card.  */
              get_serial_direct (cardman, !strcmp (string, "geldkarte-card"));
            }

          update_info_visibility (cardman);
          update_title (cardman);
        }
    }
  else if (strcmp (identifier, "serial") == 0 && idx == 0)
    {
      while (*string == '0' && string[1])
        string++;
      gtk_entry_set_text (GTK_ENTRY (cardman->entrySerialno), string);
    }
  else if (strcmp (identifier, "login") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryLogin), string);
  else if (strcmp (identifier, "name") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryFirstName), string);
  else if (strcmp (identifier, "name") == 0 && idx == 1)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryLastName), string);
  else if (strcmp (identifier, "sex") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entrySex),
			gpa_sex_char_to_string (string[0]));
  else if (strcmp (identifier, "lang") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryLanguage), string);
  else if (strcmp (identifier, "url") == 0 && idx == 0)
    {
      char *tmp = decode_c_string (string);
      gtk_entry_set_text (GTK_ENTRY (cardman->entryPubkeyUrl), tmp);
      xfree (tmp);
    }
  else if (strcmp (identifier, "vendor") == 0 && idx == 1)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryManufacturer), string);
  else if (strcmp (identifier, "version") == 0 && idx == 0)
    {
      char buffer[6];
      char *p;

      if (strlen (string) == 4)
        {
          /* Reformat the version number to be better human readable.  */
          p = buffer;
          if (string[0] != '0')
            *p++ = string[0];
          *p++ = string[1];
          *p++ = '.';
          if (string[2] != '0')
            *p++ = string[2];
          *p++ = string[3];
	  *p++ = '\0';
          string = buffer;
        }
      gtk_entry_set_text (GTK_ENTRY (cardman->entryVersion), string);
    }
  else if (strcmp (identifier, "fpr") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryKeySig), string);
  else if (strcmp (identifier, "fpr") == 0 && idx == 1)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryKeyEnc), string);
  else if (strcmp (identifier, "fpr") == 0 && idx == 2)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryKeyAuth), string);
  else if (strcmp (identifier, "pinretry") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryPINRetryCounter), string);
  else if (strcmp (identifier, "pinretry") == 0 && idx == 2)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryAdminPINRetryCounter), string);
  else if (strcmp (identifier, "forcepin") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entrySigForcePIN),
			(*string == '1' ? _("Yes") : _("No")));
}


/* This function is called to trigger a card-reload.  */
static void
card_reload (GpaCardManager *cardman)
{
  GpaCardReloadOperation *op;

  if (!cardman->in_card_reload)
    {
      cardman->in_card_reload++;
      op = gpa_card_reload_operation_new (cardman->window, 
                                          card_reload_cb, cardman);
      g_signal_connect (G_OBJECT (op), "completed",
                        G_CALLBACK (g_object_unref), cardman);

      /* Fixme: We should decrement the sentinel only after finishing
         the operation, so that we don't run them over and over if the
         user clicks too often.  Note however that the primary reason
         for this sentinel is to avoid concurrent reloads triggered by
         the user and by the file watcher.  */
      cardman->in_card_reload--;
    }
}


/* This function is called when the user triggers a card-reload. */
static void
card_reload_action (GtkAction *action, gpointer param)
{
  GpaCardManager *cardman = param;

  card_reload (cardman);
}


#if 0
/* This function is called when the user triggers a card-reload. */
static void
card_edit (GtkAction *action, gpointer param)
{
  GpaCardManager *cardman = param;
  gpg_error_t err;

  fprintf (stderr, "CARD_EDIT\n");
  //err = gpa_gpgme_card_edit_modify_start (CTX, "123");
}
#endif

/* Returns TRUE if the card widget contained in CARDMAN contains key
   fingerprints, FALSE otherwise. */
static gboolean
card_contains_keys (GpaCardManager *cardman)
{
  const char *key_sig;
  const char *key_enc;
  const char *key_auth;

  key_sig = gtk_entry_get_text (GTK_ENTRY (cardman->entryKeySig));
  key_enc = gtk_entry_get_text (GTK_ENTRY (cardman->entryKeyEnc));
  key_auth = gtk_entry_get_text (GTK_ENTRY (cardman->entryKeyAuth));

  if ((key_sig && strcmp (key_sig, "") != 0)
      || (key_enc && strcmp (key_enc, "") != 0)
      || (key_auth && strcmp (key_auth, "") != 0))
    return TRUE;

  return FALSE;
}

/* Return TRUE if the boolean option named OPTION belonging to the
   GnuPG component COMPONENT is activated, FALSE otherwise.  Return
   FALSE in case of an error. */
static gboolean
check_conf_boolean (const char *component, const char *option)
{
  gpgme_ctx_t ctx;
  gpgme_error_t err;
  gpgme_conf_comp_t conf;
  gpgme_conf_opt_t opt;
  gboolean ret = FALSE;

  ctx = NULL;
  conf = NULL;

  /* Load configuration through GPGME. */

  err = gpgme_new (&ctx);
  if (err)
    goto out;
  err = gpgme_op_conf_load (ctx, &conf);
  if (err)
    goto out;

  /* Search for the component. */
  while (conf)
    {
      if (strcmp (conf->name, component) == 0)
	break;
      conf = conf->next;
    }

  if (!conf)
    goto out;

  /* Search for the option. */
  opt = conf->options;
  while (opt)
    {
      if (strcmp (opt->name, option) == 0)
	break;
      opt = opt->next;
    }

  if (!opt)
    goto out;

  /* Check value.  */
  if (opt->value && opt->value->value.int32)
    ret = TRUE;

 out:

  /* Release. */
  if (conf)
    gpgme_conf_release (conf);
  if (ctx)
    gpgme_release (ctx);

  return ret;
}

/* This function is called to triggers a key-generation.  */
static void
card_genkey (GpaCardManager *cardman)
{
  GpaGenKeyCardOperation *op;

  if (check_conf_boolean ("scdaemon", "deny-admin") == TRUE)
    {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (GTK_WINDOW (cardman->window),
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_ERROR,
				       GTK_BUTTONS_OK,
				       "Admin commands not allowed. Key generation disabled.");
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      return;
    }

  if (card_contains_keys (cardman))
    {
      GtkWidget *dialog;
      gint dialog_response;

      dialog = gtk_message_dialog_new (GTK_WINDOW (cardman->window),
				       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_WARNING,
				       GTK_BUTTONS_OK_CANCEL,
				       "Keys are already stored on the card. "
				       "Really replace existing keys?");

      dialog_response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      switch (dialog_response)
	{
	case GTK_RESPONSE_OK:
         break;

	default:
	  return;
	}
    }

  op = gpa_gen_key_card_operation_new (cardman->window);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), cardman);
}


/* This function is called when the user triggers a key-generation.  */
static void
card_genkey_action (GtkAction *action, gpointer param)
{
  GpaCardManager *cardman = param;

  card_genkey (cardman);
}

typedef enum
  {
    READER_STATUS_UNKNOWN,
    READER_STATUS_NOCARD,
    READER_STATUS_PRESENT,
    READER_STATUS_USABLE
  } reader_status_t;

static reader_status_t
reader_status_from_file (const char *filename)
{
  reader_status_t status = READER_STATUS_UNKNOWN;
  char status_word[16];
  FILE *fp;

  fp = fopen (filename, "r");
  if (fp)
    {
      if (fgets (status_word, sizeof (status_word), fp))
	{
	  /* Remove trailing newline from STATUS_WORD.  */
	  char *nl = strchr (status_word, '\n');
	  if (nl)
	    *nl = '\0';

	  /* Store enum value belonging to STATUS_WORD in STATUS. */
	  if (strcmp (status_word, "NOCARD") == 0)
	    status = READER_STATUS_NOCARD;
	  else if (strcmp (status_word, "PRESENT") == 0)
	    status = READER_STATUS_PRESENT;
	  else if (strcmp (status_word, "USABLE") == 0)
	    status = READER_STATUS_USABLE;
	}
      /* else: read error or EOF. */

      fclose (fp);
    }

  return status;
}


static void
watcher_cb (void *opaque, const char *filename, const char *reason)
{
  GpaCardManager *cardman = opaque;

  if (cardman && strchr (reason, 'w') )
    {
      reader_status_t reader_status;

      reader_status = reader_status_from_file (filename);
      if (reader_status == READER_STATUS_PRESENT)
	statusbar_update (cardman, _("Reloading card data..."));
      card_reload (cardman);
    }
}


/* Construct the card manager menu and toolbar widgets and return
   them. */
static void
cardman_action_new (GpaCardManager *cardman, GtkWidget **menubar,
		    GtkWidget **toolbar)
{
  static const GtkActionEntry entries[] =
    {
      /* Toplevel.  */
      { "File", NULL, N_("_File"), NULL },
      { "Edit", NULL, N_("_Edit"), NULL },
      { "Card", NULL, N_("_Card"), NULL },

      /* File menu.  */
      { "FileQuit", GTK_STOCK_QUIT, NULL, NULL,
	N_("Quit the program"), G_CALLBACK (gtk_main_quit) },

      /* Card menu.  */
      { "CardReload", GTK_STOCK_REFRESH, NULL, NULL,
	N_("Reload card information"), G_CALLBACK (card_reload_action) },
#if 0
      /* FIXME: not yet implemented. */
      { "CardEdit", GTK_STOCK_EDIT, NULL, NULL,
	N_("Edit card information"), G_CALLBACK (card_edit) },
#endif
      { "CardGenkey", GTK_STOCK_NEW, "Generate new key...", NULL,
	N_("Generate new key on card"), G_CALLBACK (card_genkey_action) },
    };

  static const char *ui_description =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='File'>"
    "      <menuitem action='FileQuit'/>"
    "    </menu>"
    "    <menu action='Edit'>"
    "      <menuitem action='EditPreferences'/>"
    "      <menuitem action='EditBackendPreferences'/>"
    "    </menu>"
    "    <menu action='Card'>"
    "      <menuitem action='CardReload'/>"
#if 0
    "      <menuitem action='CardEdit'/>"
#endif
    "      <menuitem action='CardGenkey'/>"
    "    </menu>"
    "    <menu action='Windows'>"
    "      <menuitem action='WindowsKeyringEditor'/>"
    "      <menuitem action='WindowsFileManager'/>"
    "      <menuitem action='WindowsClipboard'/>"
    "      <menuitem action='WindowsCardManager'/>"
    "    </menu>"
    "    <menu action='Help'>"
#if 0
    "      <menuitem action='HelpContents'/>"
#endif
    "      <menuitem action='HelpAbout'/>"
    "    </menu>"
    "  </menubar>"
    "  <toolbar name='ToolBar'>"
    "    <toolitem action='CardReload'/>"
#if 0
    "    <toolitem action='CardEdit'/>"
#endif
    "    <separator/>"
    "    <toolitem action='EditPreferences'/>"
    "    <separator/>"
    "    <toolitem action='WindowsKeyringEditor'/>"
    "    <toolitem action='WindowsFileManager'/>"
    "    <toolitem action='WindowsClipboard'/>"
#if 0
    "    <toolitem action='HelpContents'/>"
#endif
    "  </toolbar>"
    "</ui>";

  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;
  GtkAction *action;
  GError *error;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries),
				cardman);
  gtk_action_group_add_actions (action_group, gpa_help_menu_action_entries,
				G_N_ELEMENTS (gpa_help_menu_action_entries),
				cardman);
  gtk_action_group_add_actions (action_group, gpa_windows_menu_action_entries,
				G_N_ELEMENTS (gpa_windows_menu_action_entries),
				cardman);
  gtk_action_group_add_actions
    (action_group, gpa_preferences_menu_action_entries,
     G_N_ELEMENTS (gpa_preferences_menu_action_entries), cardman);
  cardman->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (cardman->ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (cardman->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (cardman), accel_group);
  if (! gtk_ui_manager_add_ui_from_string (cardman->ui_manager, ui_description,
					   -1, &error))
    {
      g_message ("building cardman menus failed: %s", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }

  /* Fixup the icon theme labels which are too long for the toolbar.  */
  action = gtk_action_group_get_action (action_group, "WindowsKeyringEditor");
  g_object_set (action, "short_label", _("Keyring"), NULL);
  action = gtk_action_group_get_action (action_group, "WindowsFileManager");
  g_object_set (action, "short_label", _("Files"), NULL);

  *menubar = gtk_ui_manager_get_widget (cardman->ui_manager, "/MainMenu");
  *toolbar = gtk_ui_manager_get_widget (cardman->ui_manager, "/ToolBar");
  gpa_toolbar_set_homogeneous (GTK_TOOLBAR (*toolbar), FALSE);
}


/* Callback for the destroy signal.  */
static void
card_manager_closed (GtkWidget *widget, gpointer param)
{
  instance = NULL;
}


/* Helper for construct_card_widget.  */
static void
add_table_row (GtkWidget *table, int *rowidx,
               const char *labelstr, GtkWidget *widget, GtkWidget *widget2)
{
  GtkWidget *label;

  label = gtk_label_new (labelstr);
  gtk_label_set_width_chars  (GTK_LABEL (label), 22);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,	       
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0); 

  gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);
  gtk_entry_set_has_frame (GTK_ENTRY (widget), FALSE);

  gtk_table_attach (GTK_TABLE (table), widget, 1, 2,
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0);
  if (widget2)
    gtk_table_attach (GTK_TABLE (table), widget2, 2, 3,
		      *rowidx, *rowidx + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
  ++*rowidx;
}

/* Return a new GtkEntry filled with TEXT, without frame and editable
   flag set depending on READONLY.  */
static GtkWidget *
new_gtk_entry (const gchar *text, gboolean readonly)
{
  GtkWidget *entry;

  entry = gtk_entry_new ();
  if (text)
    gtk_entry_set_text (GTK_ENTRY (entry), text);

  if (readonly)
    gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE);
  
  gtk_entry_set_has_frame (GTK_ENTRY (entry), FALSE);

  return entry;
}

/* Action for "Change Name" button.  Display a new dialog through
   which the user can change the name (firstname + lastname) stored on
   the card. */
/* FIXME: gtl_dialog_get_content_area is a newer GTK feature.  We
   can't use it.  Thus this function is disabled.  */
#if 0
static void
modify_name_cb (GtkWidget *widget, gpointer data)
{
  GpaCardManager *cardman = data;
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *table;
  GtkWidget *entryFirstName;
  GtkWidget *entryLastName;
  gint result;
  

  dialog = gtk_dialog_new_with_buttons ("Change Name",
					GTK_WINDOW (cardman),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_OK,
					GTK_RESPONSE_ACCEPT,
					GTK_STOCK_CANCEL,
					GTK_RESPONSE_REJECT,
					NULL);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);

  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("Current Value:"), 
                    0, 1, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("New Value:"), 
                    0, 1,  2, 3, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("First Name"),
                    1,  2, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("Last Name"),
                    2,  3, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new (gtk_entry_get_text 
                                   (GTK_ENTRY (cardman->entryFirstName))),
		    1, 2, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new (gtk_entry_get_text 
                                   (GTK_ENTRY (cardman->entryLastName))),
		    2, 3, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);

  entryFirstName = new_gtk_entry (NULL, FALSE);
  entryLastName = new_gtk_entry (NULL, FALSE);

  gtk_table_attach (GTK_TABLE (table), entryFirstName, 
                    1, 2, 2, 3, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entryLastName,
                    2, 3, 2, 3, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_container_add (GTK_CONTAINER (content_area), table);
  gtk_widget_show_all (dialog);

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
      fprintf (stderr, "NOT IMPLEMENTED YET, "
               "CHANGING NAME to \"%s\", \"%s\"...\n",
	       gtk_entry_get_text (GTK_ENTRY (entryFirstName)), 
               gtk_entry_get_text (GTK_ENTRY (entryLastName)));
      break;

    default:
      break;
    }

  gtk_widget_destroy (entryFirstName);
  gtk_widget_destroy (entryLastName);
  gtk_widget_destroy (dialog);
}
#endif /* Disabled code.  */


/* This function constructs the container holding the card "form". It
   updates CARDMAN with new references to the entry widgets, etc.  */
static GtkWidget *
construct_card_widget (GpaCardManager *cardman)
{
  GtkWidget *scrolled;
  GtkWidget *container;
  GtkWidget *label;
  GtkWidget *general_frame;
  GtkWidget *general_table;
  GtkWidget *personal_frame;
  GtkWidget *personal_table;
  GtkWidget *keys_frame;
  GtkWidget *keys_table;
  GtkWidget *pin_frame;
  GtkWidget *pin_table;
  int rowidx;

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  container = gtk_vbox_new (FALSE, 0);

  /* Create frames and tables. */

  general_frame = cardman->general_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (general_frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>General</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (general_frame), label);

  personal_frame = cardman->personal_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (personal_frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>Personal</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (personal_frame), label);

  keys_frame = cardman->keys_frame = gtk_expander_new (NULL);
  label = gtk_label_new (_("<b>Keys</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_expander_set_label_widget (GTK_EXPANDER (keys_frame), label);

  pin_frame = cardman->pin_frame = gtk_expander_new (NULL);
  label = gtk_label_new (_("<b>PIN</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_expander_set_label_widget (GTK_EXPANDER (pin_frame), label);

  general_table = gtk_table_new (4, 3, FALSE);
  personal_table = gtk_table_new (6, 3, FALSE);
  keys_table = gtk_table_new (3, 3, FALSE);
  pin_table = gtk_table_new (3, 3, FALSE);

  gtk_container_set_border_width (GTK_CONTAINER (general_table), 10);
  gtk_container_set_border_width (GTK_CONTAINER (personal_table), 10);
  gtk_container_set_border_width (GTK_CONTAINER (keys_table), 10);
  gtk_container_set_border_width (GTK_CONTAINER (pin_table), 10);
  
  /* General frame.  */
  rowidx = 0;

  cardman->entryType = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (cardman->entryType), 24);
  add_table_row (general_table, &rowidx,
                 "Card Type: ", cardman->entryType, NULL);

  cardman->entrySerialno = gtk_entry_new ();
  add_table_row (general_table, &rowidx, 
                 "Serial Number: ", cardman->entrySerialno, NULL);

  cardman->entryVersion = gtk_entry_new ();
  add_table_row (general_table, &rowidx,
                 "Card Version: ", cardman->entryVersion, NULL);

  cardman->entryManufacturer = gtk_entry_new ();
  add_table_row (general_table, &rowidx,
                 "Manufacturer: ", cardman->entryManufacturer, NULL);

  gtk_container_add (GTK_CONTAINER (general_frame), general_table);

  /* Personal frame.  */
  rowidx = 0;

  cardman->entryFirstName = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (cardman->entryFirstName), 48);
  add_table_row (personal_table, &rowidx, 
                 "First Name:", cardman->entryFirstName, NULL);

  cardman->entryLastName = gtk_entry_new ();
  {
    GtkWidget *modify_name_button;

    modify_name_button = gtk_button_new_with_label ("Change");

    /* Disabled because we need to change modify_name_cb first.  */
/*     g_signal_connect (G_OBJECT (modify_name_button), "clicked", */
/* 		      G_CALLBACK (modify_name_cb), cardman); */
    add_table_row (personal_table, &rowidx,
		   "Last Name:", cardman->entryLastName, modify_name_button);
  }

  cardman->entrySex = gtk_entry_new ();
  add_table_row (personal_table, &rowidx,
                 "Sex:", cardman->entrySex, NULL);

  cardman->entryLanguage = gtk_entry_new ();
  add_table_row (personal_table, &rowidx,
                 "Language: ", cardman->entryLanguage, NULL);

  cardman->entryLogin = gtk_entry_new ();
  add_table_row (personal_table, &rowidx,
                 "Login Data: ", cardman->entryLogin, NULL);

  cardman->entryPubkeyUrl = gtk_entry_new ();
  add_table_row (personal_table, &rowidx,
                 "Public key URL: ", cardman->entryPubkeyUrl, NULL);

  gtk_container_add (GTK_CONTAINER (personal_frame), personal_table);

  /* Keys frame.  */
  rowidx = 0;

  cardman->entryKeySig = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (cardman->entryKeySig), 48);
  add_table_row (keys_table, &rowidx, 
                 "Signature Key: ", cardman->entryKeySig, NULL);

  cardman->entryKeyEnc = gtk_entry_new ();
  add_table_row (keys_table, &rowidx,
                 "Encryption Key: ", cardman->entryKeyEnc, NULL);

  cardman->entryKeyAuth = gtk_entry_new ();
  add_table_row (keys_table, &rowidx,
                 "Authentication Key: ", cardman->entryKeyAuth, NULL);

  gtk_container_add (GTK_CONTAINER (keys_frame), keys_table);


  /* PIN frame.  */
  rowidx = 0;

  cardman->entryPINRetryCounter = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (cardman->entryPINRetryCounter), 24);
  add_table_row (pin_table, &rowidx, 
                 "PIN Retry Counter: ", cardman->entryPINRetryCounter, NULL);

  cardman->entryAdminPINRetryCounter = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (cardman->entryAdminPINRetryCounter), 24);
  add_table_row (pin_table, &rowidx, 
                 "Admin PIN Retry Counter: ", cardman->entryAdminPINRetryCounter, NULL);

  cardman->entrySigForcePIN = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (cardman->entrySigForcePIN), 24);
  add_table_row (pin_table, &rowidx, 
                 "Force Signature PIN: ", cardman->entrySigForcePIN, NULL);

  gtk_container_add (GTK_CONTAINER (pin_frame), pin_table);

  /* Put all frames together.  */
  gtk_box_pack_start (GTK_BOX (container), general_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (container), personal_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (container), keys_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (container), pin_frame, FALSE, TRUE, 0);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled), container);
  
  return scrolled;
}



/************************************************************ 
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_card_manager_finalize (GObject *object)
{  
  /* FIXME: Remove the watch object and all other resources.  */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_card_manager_init (GpaCardManager *cardman)
{
  char *fname;

  fname = g_build_filename (gnupg_homedir, "reader_0.status", NULL);
  cardman->watch = gpa_add_filewatch (fname, "w", watcher_cb, cardman);
  xfree (fname);
}

/* Construct a new class object of GpaCardManager.  */
static GObject*
gpa_card_manager_constructor (GType type,
			      guint n_construct_properties,
			      GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaCardManager *cardman;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *icon;
  gchar *markup;
  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkWidget *statusbar;
  GtkWidget *box;

  /* Invoke parent's constructor.  */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  cardman = GPA_CARD_MANAGER (object);

  cardman->have_card = 0;
  cardman->cardtype = "Unknown";
  cardman->is_openpgp = 0;

  cardman->entryLogin = NULL;

  cardman->entrySerialno = NULL;
  cardman->entryVersion = NULL;
  cardman->entryManufacturer = NULL;
  cardman->entryLogin = NULL;
  cardman->entryLanguage = NULL;
  cardman->entryPubkeyUrl = NULL;
  cardman->entryFirstName = NULL;
  cardman->entryLastName = NULL;
  cardman->entryKeySig = NULL;
  cardman->entryKeyEnc = NULL;
  cardman->entryKeyAuth = NULL;
  cardman->entryPINRetryCounter = NULL;
  cardman->entryAdminPINRetryCounter = NULL;
  cardman->entrySigForcePIN = NULL;


  /* Initialize.  */
  update_title (cardman);
  gtk_window_set_default_size (GTK_WINDOW (cardman), 680, 480);
  /* Realize the window so that we can create pixmaps without warnings.  */
  gtk_widget_realize (GTK_WIDGET (cardman));

  /* Use a vbox to show the menu, toolbar and the file container.  */
  vbox = gtk_vbox_new (FALSE, 0);

  /* Get the menu and the toolbar.  */
  cardman_action_new (cardman, &menubar, &toolbar);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);


  /* Add a fancy label that tells us: This is the card manager.  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);
  
  icon = gtk_image_new_from_stock (GPA_STOCK_CARDMAN, GTK_ICON_SIZE_DND);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);

  label = gtk_label_new (NULL);
  markup = g_strdup_printf ("<span font_desc=\"16\">%s</span>",
                            _("Card Manager"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 10);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  cardman->big_label = gtk_label_new (_("No smartcard found."));
  gtk_box_pack_start (GTK_BOX (vbox), cardman->big_label, TRUE, TRUE, 0);

  box = gtk_vbox_new (FALSE, 0);

  cardman->card_widget = construct_card_widget (cardman);
  gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), cardman->card_widget, TRUE, TRUE, 0);

  /* We set the no-show-all flag here.  This makes sure that the
     card_widget is not shown in the time window between start of the
     card manager and the first call to card_reload (through the
     filewatcher callback).  The flag gets disabled again in
     update_info_visibility.  */
  gtk_widget_set_no_show_all (cardman->card_widget, TRUE);

  statusbar = statusbar_new (cardman);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (cardman), vbox);

  g_signal_connect (object, "destroy",
                    G_CALLBACK (card_manager_closed), object);

  return object;
}


static void
gpa_card_manager_class_init (GpaCardManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->constructor = gpa_card_manager_constructor;
  object_class->finalize = gpa_card_manager_finalize;
}


GType
gpa_card_manager_get_type (void)
{
  static GType this_type = 0;
  
  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaCardManagerClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gpa_card_manager_class_init,
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	  sizeof (GpaCardManager),
	  0,    /* n_preallocs */
	  (GInstanceInitFunc) gpa_card_manager_init,
	};
      
      this_type = g_type_register_static (GTK_TYPE_WINDOW,
                                          "GpaCardManager",
                                          &this_info, 0);
    }
  
  return this_type;
}


/************************************************************ 
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_card_manager_get_instance (void)
{
  if (!instance)
    {
      instance = g_object_new (GPA_CARD_MANAGER_TYPE, NULL);  
      card_reload (instance);
    }
  return GTK_WIDGET (instance);
}


gboolean 
gpa_card_manager_is_open (void)
{
  return (instance != NULL);
}
