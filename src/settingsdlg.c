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
   apply.  Excvept for the UI mode, we do not auto-apply or syntax
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
  
  GtkWidget *foo;

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
    PROP_0,
    PROP_WINDOW,
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
  { "lk",  N_("Local, Keyserver")},
  { "lp",  N_("Local, PKA")},
  { "lpk", N_("Local, PKA, Keyserver")},
  { "lkp", N_("Local, Keyserver, PKA")},
  { "lD",  N_("Local, kDNS")},
  { "lDk", N_("Local, kDNS, Keyserver")},
  { "np",  N_("PKA")},
  { "nD",  N_("kDNS")},
  { NULL,  N_("Custom")},
};




/* 
   User interface section. 
 */
static void
advanced_mode_toggled (GtkToggleButton *button, gpointer user_data)
{
  gpa_options_set_simplified_ui (gpa_options_get_instance (), 
                                 !gtk_toggle_button_get_active (button));
}

static void
show_advanced_options_toggled (GtkToggleButton *button, gpointer user_data)
{
  gpa_options_set_show_advanced_options
    (gpa_options_get_instance (), gtk_toggle_button_get_active (button));
}


static GtkWidget *
user_interface_mode_frame (SettingsDlg *dialog)
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *frame_vbox;
  GtkWidget *button, *button2;

  /* Build UI.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>User interface</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

  frame_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), frame_vbox);

  button = gtk_check_button_new_with_mnemonic (_("use _advanced mode"));
  gtk_container_add (GTK_CONTAINER (frame_vbox), button);

  button2 = gtk_check_button_new_with_mnemonic (_("show advanced _options"));
  gtk_container_add (GTK_CONTAINER (frame_vbox), button2);


  /* Select default value.  */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                !gpa_options_get_simplified_ui 
                                (gpa_options_get_instance ()));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button2), 
                                !!gpa_options_get_show_advanced_options 
                                (gpa_options_get_instance ()));

  /* Connect signals.  */
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (advanced_mode_toggled), NULL);
  g_signal_connect (G_OBJECT (button2), "toggled",
                    G_CALLBACK (show_advanced_options_toggled), NULL);

  
  return frame;
}




/* Default key section.  */
static void
key_selected_cb (GtkTreeSelection *treeselection, gpointer user_data)
{
  GpaKeySelector *sel = user_data;
  GList *selected;
  
  selected = gpa_key_selector_get_selected_keys (sel);
  gpa_options_set_default_key (gpa_options_get_instance (),
			       (gpgme_key_t) selected->data);
  g_list_free (selected);
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

  list = gpa_key_selector_new (TRUE);
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
  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection
			      (GTK_TREE_VIEW (list))),
		    "changed", G_CALLBACK (key_selected_cb), list);
  return frame;
}


/* Signal handler for the "changed_show_advanced_options" signal.  */
static void
show_advanced_options_changed_for_keyserver (GpaOptions *options,
                                             gpointer param)
{
  GtkWidget *keyserver_frame = param;

  if (gpa_options_get_show_advanced_options (options))
    gtk_widget_show_all (GTK_WIDGET(keyserver_frame));
  else
    gtk_widget_hide_all (GTK_WIDGET(keyserver_frame));
}


static void
keyserver_selected_from_list_cb (GtkComboBox *combo, gpointer user_data)
{
  /* Consider the text entry activated.  */
  if (gtk_combo_box_get_active (combo) != -1)
    {
      /* FIXME: Update only on apply.  */
      char *text = gtk_combo_box_get_active_text (combo);
      if (text && *text)
        gpa_options_set_default_keyserver (gpa_options_get_instance (), text);
    }
}


static void
append_to_combo (gpointer item, gpointer data)
{
  GtkWidget *combo = data;
  gchar *text = item;

  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), text);
}


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
  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combo))),
                      gpa_options_get_default_keyserver
		      (gpa_options_get_instance ()));

  /* Connect signals.  */
  g_signal_connect (G_OBJECT (combo),
		    "changed",
                    G_CALLBACK (keyserver_selected_from_list_cb), NULL);

  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_show_advanced_options",
                    G_CALLBACK (show_advanced_options_changed_for_keyserver), 
                    frame);

  return frame;
}


/* Auto Key Locate section.  */
/* Signal handler for the "changed_show_advanced_options" signal.  */
static void
show_advanced_options_changed_for_akl (GpaOptions *options,
                                       gpointer param)
{
  GtkWidget *akl_frame = param;

  if (gpa_options_get_show_advanced_options (options))
    gtk_widget_show_all (GTK_WIDGET(akl_frame));
  else
    gtk_widget_hide_all (GTK_WIDGET(akl_frame));
}


static void
akl_method_changed_cb (GtkComboBox *combo, gpointer user_data)
{
  GtkWidget *label = user_data;
  int idx;

  idx =  gtk_combo_box_get_active (combo);
  if (idx < 0 || idx > DIM(akl_table))
    return;

  /* Enable the kdns Server entry if needed.  */
  gtk_widget_set_sensitive (label, (akl_table[idx].list
                                    && strchr (akl_table[idx].list, 'D')) );
  
}

static void
akl_kdnsaddr_changed_cb (GtkEntry *entry, gpointer user_data)
{
  const char *addr;
  struct in_addr binaddr;

  (void)user_data;

  if (!GTK_WIDGET_IS_SENSITIVE (entry))
    return;
  
  addr = gtk_entry_get_text (entry);
  g_message ("IP-address is '%s'", addr? addr : "[none]");
  if (!addr || !*addr || !inet_aton (addr, &binaddr) )
    {
      gpa_window_error ("You need to enter a valid IPv4 address.",
                        GTK_WIDGET (entry));
      return;
    }

  g_message ("IP-address is '%s' is valid", addr);
  /* Fixme: Contruct the new --auto-key-locate option.  */

}


static void
parse_akl (GtkWidget *combo, GtkWidget *kdns_addr, GtkWidget *label)
{
  akl_t akllist, akl;
  char *akloption = gpa_load_gpgconf_string ("gpg", "auto-key-locate");
  char list[10];
  int listidx = 0;
  int idx;
  
  if (!akloption)
    {
      /* Select the first entry ("Local").  */
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0); 
      /* Set kDNS server to empty.  */
      gtk_entry_set_text (GTK_ENTRY (kdns_addr), "");
      gtk_widget_set_sensitive (label, FALSE);
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
              gtk_entry_set_text (GTK_ENTRY (kdns_addr), k->host);
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
  fprintf (stderr, "akl list: %s\n", list);

  for (idx=0; akl_table[idx].list; idx++)
    if (!strcmp (akl_table[idx].list, list))
      break;
  /* (The last entry of tghe tale is the one used for no-match.) */
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), idx); 
  /* Enable the kdns Server entry if one is defined.  For ease of
     implementation we have already put the server address into the
     entry field even if though there might be no match.  */
  gtk_widget_set_sensitive (label, 
                            (akl_table[idx].list && strchr (list, 'D')));

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
              " Keyserver\n"
              "   - Use the default keyserver.\n"
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

  parse_akl (combo, entry, hbox);
  g_signal_connect (G_OBJECT (combo),
		    "changed", G_CALLBACK (akl_method_changed_cb), hbox);
  g_signal_connect (G_OBJECT (entry),
		    "changed", G_CALLBACK (akl_kdnsaddr_changed_cb), NULL);


  /* Connect signals.  */
  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_show_advanced_options",
                    G_CALLBACK (show_advanced_options_changed_for_akl), 
                    frame);

  
  return frame;
}

/* Save all settings, return 0 on success.  */
static int
save_settings (GtkDialog *dialog)
{


  return FALSE;
}


/* Handle the response signal.  */
static void
dialog_response (GtkDialog *dialog, gint response)
{
  switch (response)
    {
    case GTK_RESPONSE_OK:
      if (save_settings (dialog))
        return;
      break;

    case GTK_RESPONSE_APPLY:
      if (save_settings (dialog))
        return;
      /* Fixme: Reload configuration.  */
      
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
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
settings_dlg_init (SettingsDlg *dialog)
{
}


static GObject*
settings_dlg_constructor (GType type, guint n_construct_properties,
                          GObjectConstructParam *construct_properties)
{
  GObject *object;
  SettingsDlg *dialog;
  GtkWidget *frame, *keyserver_frame;

  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  dialog = SETTINGS_DLG (object);
  gtk_window_set_title (GTK_WINDOW (dialog),
			_("GNU Privacy Assistant - Settings"));
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

  /* The default keyserver section.  */
  frame = keyserver_frame = default_keyserver_frame (dialog);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
                      FALSE, FALSE, 0);

  /* The auto key locate section.  */
  frame = auto_key_locate_frame (dialog);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
                      FALSE, FALSE, 0);

  /* Connect the response signal.  */
  g_signal_connect (GTK_OBJECT (dialog), "response", 
                    G_CALLBACK (dialog_response), NULL);

  /* Show all windows and hide those we don't want. */
  gtk_widget_show_all (GTK_WIDGET(dialog));
  if (!gpa_options_get_show_advanced_options (gpa_options_get_instance ()))
    gtk_widget_hide_all (GTK_WIDGET(keyserver_frame));

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
  g_return_if_fail (GTK_IS_WINDOW (parent));

  if (!the_settings_dialog)
    {
      the_settings_dialog = g_object_new (SETTINGS_DLG_TYPE, NULL);
      g_signal_connect (the_settings_dialog,
                        "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &the_settings_dialog);

    }

  if (GTK_WINDOW (parent) 
      != gtk_window_get_transient_for (GTK_WINDOW (the_settings_dialog)))
    gtk_window_set_transient_for (GTK_WINDOW (the_settings_dialog),
                                  GTK_WINDOW (parent));
  
  gtk_window_present (GTK_WINDOW (the_settings_dialog));
}

