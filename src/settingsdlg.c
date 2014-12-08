/* settingsdlg.c - The GNU Privacy Assistant
   Copyright (C) 2002, Miguel Coca
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */


/* Violation of GNOME standards: Cancel does not revert a previous
   apply.  Except for the UI mode, we do not auto-apply or syntax
   check after focus change.  The rationale for this is that:

     * gpgconf operations are expensive.

     * Some fields depend on each other and without a proper framework
       for doing plausibility checks it is hard to get it right.

*/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Required for the IP address check.  */
#ifdef HAVE_W32_SYSTEM
# include <windows.h>
#else /*!HAVE_W32_SYSTEM*/
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif /*!HAVE_W32_SYSTEM*/


#include "gpa.h"
#include "gpakeyselector.h"
#include "keyserver.h"
#include "gtktools.h"
#include "confdialog.h"
#include "gpg-stuff.h"

#include "settingsdlg.h"

/* Object and class definition.  */
struct _SettingsDlg
{
  GtkDialog parent;

  gboolean modified;  /* True is there are unsaved changes.  */
  gboolean gnupg21;   /* True if gnupg 2.1.0 or later is in use.  */

  /* Data for the user interface frame.  */
  struct {
    GtkWidget *frame;
    GtkToggleButton *advanced_mode;
    GtkToggleButton *show_advanced_options;
  } ui;

  /* Data for the default key frame.  */
  struct {
    GtkWidget *frame;
    GpaKeySelector *list;

    gpgme_key_t key;  /* The selected key or NULL if none selected.  */
  } default_key;

  /* Data for the keyserver frame.  */
  struct {
    GtkWidget *frame;
    GtkComboBox *combo;
    char *url;         /* Malloced URL or NULL if none selected. */
  } keyserver;

  /* Data for the auto-key-locate frame.  */
  struct {
    int enabled;  /* True if the AKL feature is enabled.  */

    GtkWidget *frame;
    GtkComboBox *methods;
    GtkWidget *addr_hbox;
    GtkEntry  *addr_entry;

    int require_addr; /* A valid addr is required.  */

    int method_idx;   /* Selected method's index into the akl_table. */
    char *ip_addr;    /* Malloced server address or NULL.  */
  } akl;

};

struct _SettingsDlgClass
{
  GtkDialogClass parent_class;

};

static GObjectClass *parent_class;

/* We only want one instance of the settings dialog at a time.  Thus
   we use this variable to keep track of a created instance.  */
static SettingsDlg *the_settings_dialog;



/* Identifiers for our properties. */
enum
  {
    PROP_0
  };



/* A table with common and useful --auto-key-locate combinations. The
   first entry "local" is a made-up which indicates that no
   --auto-key-locate is active.  "Custom" ist used for all settings
   which can't be mapped to this table and is also the last entry in
   the table indicating the end of the table. */
static struct {
  const char *list;
  const char *text;
} akl_table[] = {
  { "l",   N_("Local")},
#ifdef ENABLE_KEYSERVER_SUPPORT
  { "lk",  N_("Local, Keyserver")},
#endif
  { "lp",  N_("Local, PKA")},
#ifdef ENABLE_KEYSERVER_SUPPORT
  { "lpk", N_("Local, PKA, Keyserver")},
  { "lkp", N_("Local, Keyserver, PKA")},
#endif
  { "lD",  N_("Local, kDNS")},
#ifdef ENABLE_KEYSERVER_SUPPORT
  { "lDk", N_("Local, kDNS, Keyserver")},
#endif
  { "np",  N_("PKA")},
  { "nD",  N_("kDNS")},
  { NULL,  N_("Custom")},
};



static void update_modified (SettingsDlg *dialog, int is_modified);




/*
   User interface section.
 */

/* Update what parts of the dialog are shown.  */
void
update_show_advanced_options (SettingsDlg *dialog)
{
  GpaOptions *options = gpa_options_get_instance ();

  g_return_if_fail (IS_SETTINGS_DLG (dialog));

  if (gpa_options_get_show_advanced_options (options))
    {
#ifdef ENABLE_KEYSERVER_SUPPORT
      if (!dialog->gnupg21)
        gtk_widget_show_all (dialog->keyserver.frame);
#endif /*ENABLE_KEYSERVER_SUPPORT*/
      if (dialog->akl.enabled)
        gtk_widget_show_all (dialog->akl.frame);
    }
  else
    {
#ifdef ENABLE_KEYSERVER_SUPPORT
      if (!dialog->gnupg21)
        gtk_widget_hide_all (dialog->keyserver.frame);
#endif /*ENABLE_KEYSERVER_SUPPORT*/
      if (dialog->akl.enabled)
        gtk_widget_hide_all (dialog->akl.frame);
    }
}


static void
advanced_mode_toggled (GtkToggleButton *button, gpointer user_data)
{
  gpa_options_set_simplified_ui (gpa_options_get_instance (),
                                 !gtk_toggle_button_get_active (button));
}

static void
show_advanced_options_toggled (GtkToggleButton *button, gpointer user_data)
{
  SettingsDlg *dialog = user_data;

  gpa_options_set_show_advanced_options
    (gpa_options_get_instance (), gtk_toggle_button_get_active (button));
  update_show_advanced_options (dialog);
}


static GtkWidget *
user_interface_mode_frame (SettingsDlg *dialog)
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *frame_vbox;
  GtkWidget *button;

  /* Build UI.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>User interface</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

  frame_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), frame_vbox);

  button = gtk_check_button_new_with_mnemonic (_("Use _advanced mode"));
  gtk_container_add (GTK_CONTAINER (frame_vbox), button);
  dialog->ui.advanced_mode = GTK_TOGGLE_BUTTON (button);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (advanced_mode_toggled), dialog);

  button = gtk_check_button_new_with_mnemonic (_("Show advanced _options"));
  gtk_container_add (GTK_CONTAINER (frame_vbox), button);
  dialog->ui.show_advanced_options = GTK_TOGGLE_BUTTON (button);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (show_advanced_options_toggled), dialog);


  return frame;
}




/*
   Default key section.
 */
static void
key_selected_cb (SettingsDlg *dialog)
{
  GList *selected;

  selected = gpa_key_selector_get_selected_keys (dialog->default_key.list);
  gpgme_key_unref (dialog->default_key.key);
  dialog->default_key.key = selected? selected->data : NULL;
  if (dialog->default_key.key)
    gpgme_key_ref (dialog->default_key.key);

  g_list_free (selected);
  update_modified (dialog, 1);
}

/* Returns NULL is the default key is valid.  On error the erroneous
   widget is returned.  */
static GtkWidget *
check_default_key (SettingsDlg *dialog)
{
  /* Make sure we have all required data.  */
  key_selected_cb (dialog);

  /* We require that a default key has been selected.  If not, we ask
     the user whether to continue. */
  if (!dialog->default_key.key)
    {
      GtkWidget *msg;
      int resp;

      msg = gtk_message_dialog_new (GTK_WINDOW(dialog),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_NONE,
                                    "%s",
            _("No default key has been selected.  This may lead to problems "
              "when signing or encrypting.  For example you might later not "
              "be able to read a mail written by you and encrypted to "
              "someone else.\n\n"
              "Please consider creating your own key and select it then."));
      gtk_dialog_add_buttons (GTK_DIALOG (msg),
                              _("Continue without a default key"),
                              GTK_RESPONSE_YES,
                              _("Let me select a default key"),
                              GTK_RESPONSE_NO,
                              NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (msg), GTK_RESPONSE_NO);
      resp = gtk_dialog_run (GTK_DIALOG (msg));
      gtk_widget_destroy (msg);
      if (resp != GTK_RESPONSE_YES)
        return GTK_WIDGET (dialog->default_key.list);
    }

  return NULL;
}


static GtkWidget *
default_key_frame (SettingsDlg *dialog)
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *list;
  GtkWidget *scroller;

  /* Build UI.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new_with_mnemonic (_("<b>Default _key</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

  list = gpa_key_selector_new (TRUE, FALSE);
  dialog->default_key.list = GPA_KEY_SELECTOR (list);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection
			       (GTK_TREE_VIEW (list)), GTK_SELECTION_SINGLE);
  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scroller),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
				       GTK_SHADOW_IN);

  gtk_widget_set_size_request (scroller, 320, 120);
  gtk_container_set_border_width (GTK_CONTAINER (scroller), 5);
  gtk_container_add (GTK_CONTAINER (scroller), list);
  gtk_container_add (GTK_CONTAINER (frame), scroller);

  /* Connect signals.  */
  g_signal_connect_swapped (G_OBJECT (gtk_tree_view_get_selection
                                      (GTK_TREE_VIEW (list))),
                            "changed", G_CALLBACK (key_selected_cb), dialog);

  dialog->default_key.frame = frame;

  return frame;
}



/*
   Default keyserver section.
*/
#ifdef ENABLE_KEYSERVER_SUPPORT
static void
keyserver_selected_from_list_cb (SettingsDlg *dialog)
{
  char *text;

  if (dialog->gnupg21)
    return;

  text = gtk_combo_box_get_active_text (dialog->keyserver.combo);
  g_message ("got `%s'", text);
  xfree (dialog->keyserver.url);
  dialog->keyserver.url = (text && *text)? text : NULL;
  update_modified (dialog, 1);
}
#endif /*ENABLE_KEYSERVER_SUPPORT*/



/* Return NULL if the default keyserver is valid.  */
#ifdef ENABLE_KEYSERVER_SUPPORT
static GtkWidget *
check_default_keyserver (SettingsDlg *dialog)
{
  keyserver_spec_t kspec;

  if (dialog->gnupg21)
    return NULL; /* GnuPG manages the keyservers.  */

  keyserver_selected_from_list_cb (dialog);

  if (!dialog->keyserver.url)
    return NULL; /* No keyserver is just fine.  */

  kspec = parse_keyserver_uri (dialog->keyserver.url, 0, NULL, 0);
  if (!kspec)
    return GTK_WIDGET (dialog->keyserver.combo);
  if (!kspec->host
      || (kspec->scheme && !(!strcmp (kspec->scheme, "hkp")
                             || !strcmp (kspec->scheme, "http")
                             || !strcmp (kspec->scheme, "ldap")
                             || !strcmp (kspec->scheme, "finger")
                             || !strcmp (kspec->scheme, "mailto"))))
    {
      free_keyserver_spec (kspec);
      return GTK_WIDGET (dialog->keyserver.combo);
    }

  free_keyserver_spec (kspec);
  return NULL;
}
#endif /*ENABLE_KEYSERVER_SUPPORT*/



#ifdef ENABLE_KEYSERVER_SUPPORT
static void
append_to_combo (gpointer item, gpointer data)
{
  GtkWidget *combo = data;
  gchar *text = item;

  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), text);
}
#endif /*ENABLE_KEYSERVER_SUPPORT*/


#ifdef ENABLE_KEYSERVER_SUPPORT
static GtkWidget *
default_keyserver_frame (SettingsDlg *dialog)
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *combo;
  GList *servers;

  /* Build UI */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new_with_mnemonic (_("<b>Default key_server</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

  combo = gtk_combo_box_entry_new_text ();
  gtk_container_set_border_width (GTK_CONTAINER (combo), 5);
  gtk_container_add (GTK_CONTAINER (frame), combo);
  /* Set current value.  */
  servers = keyserver_get_as_glist ();
  g_list_foreach (servers, append_to_combo, combo);
  g_list_free (servers);
  dialog->keyserver.combo = GTK_COMBO_BOX (combo);

  g_signal_connect_swapped (G_OBJECT (combo),
                            "changed",
                            G_CALLBACK (keyserver_selected_from_list_cb),
                            dialog);

  dialog->keyserver.frame = frame;

  return frame;
}
#endif /*ENABLE_KEYSERVER_SUPPORT*/


/*
   Auto Key Locate section.
 */

static void
akl_method_changed_cb (SettingsDlg *dialog)
{
  int idx;

  dialog->akl.require_addr = 0;
  idx = gtk_combo_box_get_active (dialog->akl.methods);
  if (idx < 0 || idx > DIM(akl_table))
    {
      dialog->akl.method_idx = -1;
      return;
    }
  dialog->akl.method_idx = idx;

  if (akl_table[idx].list && strchr (akl_table[idx].list, 'D'))
    dialog->akl.require_addr = 1;

  update_modified (dialog, 1);
}

static void
akl_addr_changed_cb (SettingsDlg *dialog)
{
  const char *addr;
  struct in_addr binaddr;

  xfree (dialog->akl.ip_addr );
  dialog->akl.ip_addr = NULL;

  if (!GTK_WIDGET_IS_SENSITIVE (dialog->akl.addr_entry))
    return;

  addr = gtk_entry_get_text (dialog->akl.addr_entry);
  if (!addr || !*addr || !inet_aton (addr, &binaddr) )
    {
      g_message ("IP-address is '%s' is NOT valid", addr? addr:"[none]");
      return;
    }

  dialog->akl.ip_addr = xstrdup (addr);
  g_message ("IP-address is '%s' is valid", addr);

  update_modified (dialog, 1);
}


/* Returns NULL is the AKL data is valid.  On error the erroneous
   widget is returned.  */
static GtkWidget *
check_akl (SettingsDlg *dialog)
{
  /* Make sure we have all required data.  */
  akl_method_changed_cb (dialog);
  akl_addr_changed_cb (dialog);

  if (dialog->akl.method_idx < 0)
    return GTK_WIDGET (dialog->akl.methods);
  if (dialog->akl.require_addr && !dialog->akl.ip_addr)
    return GTK_WIDGET (dialog->akl.addr_entry);

  return NULL;
}


static void
parse_akl (SettingsDlg *dialog)
{
  akl_t akllist, akl;
  char *akloption;
  char list[10];
  int listidx = 0;
  int idx;

  akloption = gpa_load_gpgconf_string ("gpg", "auto-key-locate");
  if (!akloption)
    {
      dialog->akl.require_addr = 0;
      dialog->akl.method_idx = 0;
      /* Select the first entry ("Local").  */
      gtk_combo_box_set_active (dialog->akl.methods, 0);
      /* Set kDNS server to empty.  */
      gtk_entry_set_text (dialog->akl.addr_entry, "");
      return;
    }

  /* Well, lets parse it.  */
  akllist = gpg_parse_auto_key_locate (akloption);
  g_free (akloption);
  for (akl = akllist; akl && listidx < sizeof list - 1; akl = akl->next)
    {
      keyserver_spec_t k = akl->spec;

      switch (akl->type )
        {
        case AKL_NODEFAULT:
          list[listidx++] = 'n';
          break;
        case AKL_LOCAL:
          list[listidx++] = 'l';
          break;
        case AKL_CERT:
          list[listidx++] = 'c';
          break;
        case AKL_PKA:
          list[listidx++] = 'p';
          break;
        case AKL_LDAP:
          list[listidx++] = 'L';
          break;
        case AKL_KEYSERVER:
          list[listidx++] = 'k';
          break;
        case AKL_SPEC:
          if ( (k = akl->spec)
               && k->scheme && !strcmp (k->scheme, "kdns")
               && !k->auth
               && k->host
               && !k->port
               && k->path && !strcmp (k->path, "/?at=_kdnscert&usevc=1")
               && !k->opaque
               && !k->options)
            {
              gtk_entry_set_text (dialog->akl.addr_entry, k->host);
              list[listidx++] = 'D';
            }
          else
            list[listidx++] = '?';
          break;

        default:
          list[listidx++] = '?';
          break;
        }
    }
  list[listidx] = 0;
  gpg_release_akl (akllist);
  g_debug ("akl list: %s", list);

  for (idx=0; akl_table[idx].list; idx++)
    if (!strcmp (akl_table[idx].list, list))
      break;
  /* (The last entry of the table is the one used for no-match.) */
  gtk_combo_box_set_active (dialog->akl.methods, idx);
  dialog->akl.method_idx = idx;
  /* Enable the kdns Server entry if one is defined.  For ease of
     implementation we have already put the server address into the
     entry field even if there might be no match.  */
  if (akl_table[idx].list && strchr (akl_table[idx].list, 'D'))
    dialog->akl.require_addr = 1;
}



static GtkWidget *
auto_key_locate_frame (SettingsDlg *dialog)
{
  const char *tooltip;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *frame_vbox;
  GtkWidget *combo;
  GtkWidget *entry;
  GtkWidget *hbox;
  int xpad, ypad;
  int idx;

  /* Build UI.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new_with_mnemonic (_("<b>Auto key _locate</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

  frame_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), frame_vbox);

  /* The method selection.  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame_vbox), hbox);
  tooltip = _("The list of methods to locate keys via an email address.\n"
              "All given methods are used in turn until a matching "
              "key is found.  The supported methods are:\n"
              " Local\n"
              "   - Use the local keyring.\n"
#ifdef ENABLE_KEYSERVER_SUPPORT
              " Keyserver\n"
              "   - Use the default keyserver.\n"
#endif /*ENABLE_KEYSERVER_SUPPORT*/
              " PKA\n"
              "   - Use the Public Key Association.\n"
              " kDNS\n"
              "   - Use kDNS with the nameserver below.\n"
              " Custom\n"
              "   - Configured in the backend dialog.\n");
  gpa_add_tooltip (hbox, tooltip);

  label = gtk_label_new (_("Method:"));
  gtk_misc_get_padding (GTK_MISC (label), &xpad, &ypad);
  xpad += 5;
  gtk_misc_set_padding (GTK_MISC (label), xpad, ypad);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  combo = gtk_combo_box_new_text ();
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);

  idx=0;
  do
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _(akl_table[idx].text));
  while (akl_table[idx++].list);

  /* The kDNS server.  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame_vbox), hbox);
  tooltip = _("The IP address of the nameserver used for the kDNS method.");
  gpa_add_tooltip (hbox, tooltip);
  label = gtk_label_new (_("kDNS Server:"));
  gtk_misc_get_padding (GTK_MISC (label), &xpad, &ypad);
  xpad += 5;
  gtk_misc_set_padding (GTK_MISC (label), xpad, ypad);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY(entry), 3+1+3+1+3+1+3);
  gtk_entry_set_width_chars (GTK_ENTRY(entry), 3+1+3+1+3+1+3);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);

  dialog->akl.addr_hbox = hbox;
  dialog->akl.addr_entry = GTK_ENTRY (entry);
  dialog->akl.methods = GTK_COMBO_BOX (combo);

  g_signal_connect_swapped (G_OBJECT (combo),
                            "changed",
                            G_CALLBACK (akl_method_changed_cb), dialog);
  g_signal_connect_swapped (G_OBJECT (entry),
                            "changed",
                            G_CALLBACK (akl_addr_changed_cb), dialog);

  dialog->akl.frame = frame;

  return frame;
}

/* Update the state of the action buttons.  */
static void
update_modified (SettingsDlg *dialog, int is_modified)
{
  int newstate = !!is_modified;

  dialog->modified = newstate;

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_APPLY, newstate);

  if (dialog->akl.enabled)
    gtk_widget_set_sensitive (dialog->akl.addr_hbox, dialog->akl.require_addr);
}


static void
load_settings (SettingsDlg *dialog)
{
  GpaOptions *options = gpa_options_get_instance ();

  g_return_if_fail (IS_SETTINGS_DLG (dialog));

  /* UI section.  */
  gtk_toggle_button_set_active
    (dialog->ui.advanced_mode, !gpa_options_get_simplified_ui (options));
  gtk_toggle_button_set_active
    (dialog->ui.show_advanced_options,
     !!gpa_options_get_show_advanced_options (options));

  /* Default key section.  */
  /*gpa_options_get_default_key (options);*/
  /* FIXME: Need to select the one from the list. */


  /* Default keyserver section.  */
#ifdef ENABLE_KEYSERVER_SUPPORT
  if (!dialog->gnupg21)
    {
      gtk_entry_set_text (GTK_ENTRY
                          (gtk_bin_get_child
                           (GTK_BIN (dialog->keyserver.combo))),
                          gpa_options_get_default_keyserver (options));
    }
#endif /*ENABLE_KEYSERVER_SUPPORT*/

  /* AKL section. */
  if (dialog->akl.enabled)
    parse_akl (dialog);


  update_modified (dialog, 0);
}


/* Save all settings, return 0 on success.  */
static int
save_settings (SettingsDlg *dialog)
{
  GtkWidget *errwdg;

  if (!dialog->modified)
    return 0;  /* No need to save anything.  */

  if ((errwdg = check_default_key (dialog)))
    {
      gtk_widget_grab_focus (errwdg);
      return -1;
    }

#ifdef ENABLE_KEYSERVER_SUPPORT
  if (!dialog->gnupg21)
    {
      if ((errwdg = check_default_keyserver (dialog)))
        {
          gpa_window_error
            (_("The URL given for the keyserver is not valid."),
             GTK_WIDGET (dialog));
          gtk_widget_grab_focus (errwdg);
          return -1;
        }
    }
#endif /*ENABLE_KEYSERVER_SUPPORT*/

  if ( dialog->akl.enabled && (errwdg = check_akl (dialog)))
    {
      gpa_window_error
        (_("The data given for \"Auto key locate\" is not valid."),
         GTK_WIDGET (dialog));
      gtk_widget_grab_focus (errwdg);
      return -1;
    }

  gpa_options_set_default_key (gpa_options_get_instance (),
 			       dialog->default_key.key);


#ifdef ENABLE_KEYSERVER_SUPPORT
  if (!dialog->gnupg21)
    gpa_options_set_default_keyserver (gpa_options_get_instance (),
                                       dialog->keyserver.url);
#endif /*ENABLE_KEYSERVER_SUPPORT*/

  if (!dialog->akl.enabled)
    ;
  else if (dialog->akl.method_idx == -1)
    ; /* oops: none selected.  */
  else if (!akl_table[dialog->akl.method_idx].list)
    {
      /* Custom setting: Do nothing.  */
    }
  else
    {
      const char *methods = akl_table[dialog->akl.method_idx].list;
      GString *newakl;

      newakl = g_string_new ("");
      for (; *methods; methods++)
        {
          if (newakl->len)
            g_string_append_c (newakl, ',');
          switch (*methods)
            {
            case 'n':
              g_string_append (newakl, "nodefault");
              break;
            case 'l':
              g_string_append (newakl, "local");
              break;
            case 'c':
              g_string_append (newakl, "cert");
              break;
            case 'L':
              g_string_append (newakl, "ldap");
              break;
            case 'k':
              g_string_append (newakl, "keyserver");
              break;
            case 'D':
              g_string_append_printf (newakl,
                                      "kdns://%s/?at=_kdnscert&usevc=1",
                                      dialog->akl.ip_addr ?
                                      dialog->akl.ip_addr : "");
              break;
            default:
              if (newakl->len)
                g_string_truncate (newakl, newakl->len - 1);
              break;
            }
        }
      gpa_store_gpgconf_string ("gpg", "auto-key-locate", newakl->str);
      g_string_free (newakl, TRUE);
    }
  return 0;
}


/* Handle the response signal.  */
static void
dialog_response (GtkDialog *dialog, gint response)
{
  switch (response)
    {
    case GTK_RESPONSE_OK:
      if (save_settings (SETTINGS_DLG (dialog)))
        return;
      break;

    case GTK_RESPONSE_APPLY:
      if (save_settings (SETTINGS_DLG (dialog)))
        return;
      load_settings (SETTINGS_DLG (dialog));
      return; /* Do not close.  */

    default:
      break;
    }

  gtk_widget_destroy (GTK_WIDGET(dialog));
}


/************************************************************
 ******************   Object Management  ********************
 ************************************************************/


static void
settings_dlg_finalize (GObject *object)
{
  SettingsDlg *dialog = SETTINGS_DLG (object);

  gpgme_key_unref (dialog->default_key.key);
  dialog->default_key.key = NULL;

  xfree (dialog->keyserver.url);
  dialog->keyserver.url = NULL;

  xfree (dialog->akl.ip_addr);
  dialog->akl.ip_addr = NULL;


  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
settings_dlg_init (SettingsDlg *dialog)
{
  dialog->akl.method_idx = -1;
  dialog->gnupg21 = is_gpg_version_at_least ("2.1.0");
}


static GObject*
settings_dlg_constructor (GType type, guint n_construct_properties,
                          GObjectConstructParam *construct_properties)
{
  GObject *object;
  SettingsDlg *dialog;
  GtkWidget *frame;

  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  dialog = SETTINGS_DLG (object);
  gpa_window_set_title (GTK_WINDOW (dialog), _("Settings"));
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           GTK_RESPONSE_APPLY,
                                           -1);

  /* The UI mode section.  */
  frame = user_interface_mode_frame (dialog);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
                      FALSE, FALSE, 0);

  /* The default key section.  */
  frame = default_key_frame (dialog);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame);

  /* The default keyserver section.  Note that there is no keyserver
     entry if we are using gnupg 2.1.  There we do not have the
     keyserver helpers anymore and thus the keyservers are to be
     enabled in the backend preferences. */
#ifdef ENABLE_KEYSERVER_SUPPORT
  if (!dialog->gnupg21)
    {
      frame = default_keyserver_frame (dialog);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
                          FALSE, FALSE, 0);
    }
#endif /*ENABLE_KEYSERVER_SUPPORT*/

  /* The auto key locate section.  */
  dialog->akl.enabled = dialog->gnupg21;
  if (dialog->akl.enabled)
    {
      frame = auto_key_locate_frame (dialog);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
                          FALSE, FALSE, 0);
    }

  /* Connect the response signal.  */
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (dialog_response), NULL);


  /* Load values.  */
  load_settings (dialog);

  /* Show all windows and hide those we don't want. */
  gtk_widget_show_all (GTK_WIDGET(dialog));
  update_show_advanced_options (dialog);

  return object;
}


static void
settings_dlg_class_init (SettingsDlgClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = settings_dlg_constructor;
  object_class->finalize     = settings_dlg_finalize;

}


GType
settings_dlg_get_type (void)
{
  static GType this_type;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (SettingsDlgClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) settings_dlg_class_init,
	  NULL, /* class_finalize */
	  NULL, /* class_data */

	  sizeof (SettingsDlg),
	  0,    /* n_preallocs */
	  (GInstanceInitFunc) settings_dlg_init,
	};

      this_type = g_type_register_static (GTK_TYPE_DIALOG,
                                          "SettingsDlg",
                                          &this_info, 0);
    }

  return this_type;
}



/************************************************************
 **********************  Public API  ************************
 ************************************************************/

/* Show the preference dialog and it create it if not yet done.  */
void
settings_dlg_new (GtkWidget *parent)
{
  if (parent)
    g_return_if_fail (GTK_IS_WINDOW (parent));

  if (!the_settings_dialog)
    {
      the_settings_dialog = g_object_new (SETTINGS_DLG_TYPE, NULL);
      g_signal_connect (the_settings_dialog,
                        "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &the_settings_dialog);

    }

  if (parent
      && (GTK_WINDOW (parent)
          != gtk_window_get_transient_for (GTK_WINDOW (the_settings_dialog))))
    gtk_window_set_transient_for (GTK_WINDOW (the_settings_dialog),
                                  GTK_WINDOW (parent));

  gtk_window_present (GTK_WINDOW (the_settings_dialog));
}

/* Tell whether the settings dialog is open.  */
gboolean
settings_is_open (void)
{
  return !!the_settings_dialog;
}
