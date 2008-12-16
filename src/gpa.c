/* gpa.c - The GNU Privacy Assistant main file.
   Copyright (C) 2000-2002 G-N-U GmbH.
   Copyright (C) 2005, 2008 g10 Code GmbH.

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
#include <signal.h>

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include <gpgme.h>

#include "get-path.h"
#include "gpa.h"
#include "keyring.h"
#include "fileman.h"
#include "clipboard.h"
#include "cardman.h"
#include "keyserver.h"
#include "settingsdlg.h"
#include "confdialog.h"
#include "icons.h"

#ifdef __MINGW32__
#include "hidewnd.h"
#endif


/* Global variables. */

/* The home directory of GnuPG.  */
gchar *gnupg_homedir;

/* True if CMS hack mode is enabled.  */
gboolean cms_hack;


/* Local variables.  */
typedef struct
{
  gboolean start_keyring_editor;
  gboolean start_file_manager;
  gboolean start_card_manager;
  gboolean start_clipboard;
  gboolean start_settings;
  gboolean start_only_server;
  gchar *options_filename;
} gpa_args_t;

static gpa_args_t args;


/* The copyright notice.  */
static const char *copyright = 
"Copyright (C) 2000-2002 Miguel Coca, G-N-U GmbH, Intevation GmbH.\n"
"Copyright (C) 2008 g10 Code GmbH.\n"
"This program comes with ABSOLUTELY NO WARRANTY.\n"
"This is free software, and you are welcome to redistribute it\n"
"under certain conditions.  See the file COPYING for details.\n";


static GtkWidget *keyringeditor = NULL;
static GtkWidget *backend_config_dialog = NULL;


static void print_version (void);


/* All command line options of the main application.  */
static GOptionEntry option_entries[] =
  {
    { "version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
      (gpointer) &print_version,
      N_("Output version information and exit"), NULL, },
    { "keyring", 'k', 0, G_OPTION_ARG_NONE, &args.start_keyring_editor,
      N_("Open keyring editor (default)"), NULL },
    { "files", 'f', 0, G_OPTION_ARG_NONE, &args.start_file_manager,
      N_("Open file manager"), NULL },
    { "card", 'C', 0, G_OPTION_ARG_NONE, &args.start_card_manager,
      N_("Open the card manager"), NULL },
    { "clipboard", 'c', 0, G_OPTION_ARG_NONE, &args.start_clipboard,
      N_("Open clipboard"), NULL },
    { "settings", 's', 0, G_OPTION_ARG_NONE, &args.start_settings,
      N_("Open the settings dialog"), NULL },
    { "daemon", 'd', 0, G_OPTION_ARG_NONE, &args.start_only_server,
      N_("Enable the UI server (implies --cms)"), NULL },
    { "options", 'o', 0, G_OPTION_ARG_FILENAME, &args.options_filename,
      N_("Read options from file"), "FILE" },
    { "cms", 'x', 0, G_OPTION_ARG_NONE, &cms_hack,
      "Enable CMS/X.509 support", NULL },
    { NULL }
  };





/* Return a malloced string with the locale dir.  Return NULL on
   error. */
static char *
get_locale_dir (void)
{
#ifdef G_OS_WIN32
  char name[530];
  char *p;

  if (! GetModuleFileNameA (0, name, sizeof (name) - 30))
    return NULL;
  p = strrchr (name, '\\');
  if (!p)
    {
      *name = '\\';
      p = name;
    }
  p++;
  strcpy (p, "share\\locale");
  return strdup (name);
#else
  return strdup (GPA_LOCALEDIR);
#endif
}


static void
i18n_init (void)
{
#ifdef ENABLE_NLS
  char *tmp;

  gtk_set_locale ();
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  tmp = get_locale_dir ();
  if (tmp)
    {
      bindtextdomain (PACKAGE, tmp);
      free (tmp);
    }
  textdomain (PACKAGE);
#endif
}


/* Manage the main windows and the settings dialog.  */
static void
quit_if_no_window (void)
{
  if (!keyringeditor 
      && !args.start_only_server
      && !gpa_file_manager_is_open ()
      && !gpa_clipboard_is_open ()
      && !gpa_card_manager_is_open ())
    gpa_stop_server ();
}


static void
close_main_window (GtkWidget *widget, gpointer param)
{
  GtkWidget ** window = param;

  *window = NULL;
  quit_if_no_window ();
}


/* Show the keyring editor dialog.  */
void
gpa_open_keyring_editor (GtkAction *action, void *data)
{
  if (! keyringeditor)
    {
      keyringeditor = keyring_editor_new ();
      g_signal_connect (G_OBJECT (keyringeditor), "destroy",
			G_CALLBACK (close_main_window), &keyringeditor);
      gtk_widget_show_all (keyringeditor);
    }

  gtk_window_present (GTK_WINDOW (keyringeditor));
}


/* Show the clipboard dialog.  */
void
gpa_open_clipboard (GtkAction *action, void *data)
{
  /* FIXME: Shouldn't this connect only happen if the instance is
     created the first time?  Looks like a memory leak to me.  */
  g_signal_connect (G_OBJECT (gpa_clipboard_get_instance ()), "destroy",
		    G_CALLBACK (quit_if_no_window), NULL);
  gtk_widget_show_all (gpa_clipboard_get_instance ());

  gtk_window_present (GTK_WINDOW (gpa_clipboard_get_instance ()));
}


/* Show the filemanager dialog.  */
void
gpa_open_filemanager (GtkAction *action, void *data)
{
  /* FIXME: Shouldn't this connect only happen if the instance is
     created the first time?  Looks like a memory leak to me.  */
  g_signal_connect (G_OBJECT (gpa_file_manager_get_instance ()), "destroy",
		    G_CALLBACK (quit_if_no_window), NULL);
  gtk_widget_show_all (gpa_file_manager_get_instance ());

  gtk_window_present (GTK_WINDOW (gpa_file_manager_get_instance ()));
}

/* Show the card manager.  */
void
gpa_open_cardmanager (GtkAction *action, void *data)
{
  /* FIXME: Shouldn't this connect only happen if the instance is
     created the first time?  Looks like a memory leak to me.  */
  g_signal_connect (G_OBJECT (gpa_card_manager_get_instance ()), "destroy",
		    G_CALLBACK (quit_if_no_window), NULL);
  gtk_widget_show_all (gpa_card_manager_get_instance ());

  gtk_window_present (GTK_WINDOW (gpa_card_manager_get_instance ()));
}

/* Show the settings dialog.  */
void
gpa_open_settings_dialog (GtkAction *action, void *data)
{
  settings_dlg_new (data);
}


/* Show the backend configuration dialog.  */
void
gpa_open_backend_config_dialog (GtkAction *action, void *data)
{
  if (!backend_config_dialog)
    {
      backend_config_dialog = gpa_backend_config_dialog_new ();
      g_signal_connect (G_OBJECT (backend_config_dialog), "destroy",
			G_CALLBACK (close_main_window), &backend_config_dialog);
      gtk_widget_show_all (backend_config_dialog);
    }

  gtk_window_present (GTK_WINDOW (backend_config_dialog));
}


/* Command line options.  */

/* Print version information and exit.  */
static void
print_version (void)
{
  printf ("%s %s\n%s", PACKAGE, VERSION, copyright);
  exit (0);
}



#ifdef G_OS_WIN32
static void
dummy_log_func (const gchar *log_domain, GLogLevelFlags log_level,
                const gchar *message, gpointer user_data)
{
  /* Nothing to be done.  */
}
#endif /*G_OS_WIN32*/

int
main (int argc, char *argv[])
{
  GError *err = NULL;
  GOptionContext *context;
  char *configname = NULL;
  char *keyservers_configname = NULL;
  int i;

#ifdef __MINGW32__
  hide_gpa_console_window();
#endif

  /* Set locale before option parsing for UTF-8 conversion.  */
  i18n_init ();

  /* Parse command line options.  */
  context = g_option_context_new (N_("[FILE...]"));
#if GLIB_CHECK_VERSION (2, 12, 0)
  g_option_context_set_summary (context, N_("Graphical frontend to GnuPG"));
  g_option_context_set_description (context, N_("Please report bugs to <"
						PACKAGE_BUGREPORT ">."));
  g_option_context_set_translation_domain (context, PACKAGE);
#endif 
  g_option_context_add_main_entries (context, option_entries, PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  if (! g_option_context_parse (context, &argc, &argv, &err))
    {
      g_print ("option parsing failed: %s\n", err->message);
      exit (1);
    }


  /* Disable logging to prevent MS Windows NT from opening a
     console.  */

#ifdef G_OS_WIN32
  g_log_set_handler ("Glib", G_LOG_LEVEL_CRITICAL
                             | G_LOG_LEVEL_WARNING
                             | G_LOG_LEVEL_MESSAGE
                             | G_LOG_LEVEL_INFO, dummy_log_func, NULL);

  g_log_set_handler ("Gdk", G_LOG_LEVEL_CRITICAL
                            | G_LOG_LEVEL_WARNING
                            | G_LOG_LEVEL_MESSAGE
                            | G_LOG_LEVEL_INFO, dummy_log_func, NULL);

  g_log_set_handler ("Gtk", G_LOG_LEVEL_CRITICAL
                            | G_LOG_LEVEL_WARNING
                            | G_LOG_LEVEL_MESSAGE
                            | G_LOG_LEVEL_INFO, dummy_log_func, NULL);
#endif /*G_OS_WIN32*/

  gtk_init (&argc, &argv);

  /* Default icon for all windows.  */
  gtk_window_set_default_icon_from_file (GPA_DATADIR "/gpa.png", &err);
  if (err)
    g_error_free (err);

  gpa_register_stock_items ();

#ifdef IS_DEVELOPMENT_VERSION
  fprintf (stderr, "NOTE: This is a development version!\n");
#endif

  /* Initialize GPGME.  */
  gpgme_check_version (NULL);
#ifdef USE_SIMPLE_GETTEXT
  /* FIXME */
#else
#ifdef ENABLE_NLS
  gpgme_set_locale (NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));
  gpgme_set_locale (NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
#endif
#endif

#ifndef G_OS_WIN32
  /* Internationalisation with gtk+-2.0 wants UTF-8 instead of the
     character set determined by locale.  */
  putenv ("OUTPUT_CHARSET=utf8");

  /* We don't want the SIGPIPE.  I wonder why gtk_init does not set it
     to ignore.  Nobody wants this signal.  */
  {
    struct sigaction sa;
    
    sa.sa_handler = SIG_IGN;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction (SIGPIPE, &sa, NULL);
  }
#endif

  /* Prefer gpg2 over gpg1.  */
  gpa_switch_to_gpg2 ();


  /* Handle command line options.  */

  if (args.start_only_server)
    cms_hack = 1; 

  /* Start the keyring editor by default.  */
  if (!args.start_keyring_editor 
      && !args.start_file_manager
      && !args.start_clipboard
      && !args.start_settings
      && !args.start_card_manager)
    args.start_keyring_editor = TRUE;

  /* Note: We can not use GPGME's engine info, as that returns NULL
     (default) for home_dir.  Consider improving GPGME to get it from
     there, or using gpgconf (via GPGME).  */
  gnupg_homedir = default_homedir ();
  /* FIXME: GnuPG can not create a key if its home directory is
     missing.  We help it out here.  Should be fixed in GnuPG.  */
  if (! g_file_test (gnupg_homedir, G_FILE_TEST_IS_DIR))
    g_mkdir (gnupg_homedir, 0700);

  /* Locate GPA's configuration file.  */
  if (! args.options_filename)
    configname = g_build_filename (gnupg_homedir, "gpa.conf", NULL);
  else
    configname = args.options_filename;
  gpa_options_set_file (gpa_options_get_instance (), configname);
  g_free (configname);

  /* Locate the list of keyservers.  */
  keyservers_configname = g_build_filename (gnupg_homedir, "keyservers", NULL);

  /* Read the list of available keyservers.  */
  keyserver_read_list (keyservers_configname);

  gpa_options_update_default_key (gpa_options_get_instance ());
  /* Now, make sure there are reasonable defaults for the default key
    and keyserver.  */
  if (!gpa_options_get_default_keyserver (gpa_options_get_instance ()))
    {
      GList *keyservers = keyserver_get_as_glist ();
      gpa_options_set_default_keyserver (gpa_options_get_instance (),
					 keyservers->data);
    }

  if (args.start_only_server)
    {
      /* Fire up the server.  Note that the server allows to start the
         other parts too.  */
      gpa_start_server ();
    }
  else
    {
      /* Don't open the keyring editor if any files are given on the
         command line.  Ditto for the clipboard.   */
      if (args.start_keyring_editor && (optind >= argc))
	gpa_open_keyring_editor (NULL, NULL);

      if (args.start_clipboard && (optind >= argc))
	gpa_open_clipboard (NULL, NULL);
  
      if (args.start_file_manager || (optind < argc))
	gpa_open_filemanager (NULL, NULL);

      if (args.start_card_manager)
	gpa_open_cardmanager (NULL, NULL);

      if (args.start_settings)
	gpa_open_settings_dialog (NULL, NULL);

      /* If there are any command line arguments that are not options,
	 try to open them as files in the filemanager */
      for (i = optind; i < argc; i++)
	gpa_file_manager_open_file (GPA_FILE_MANAGER
				    (gpa_file_manager_get_instance ()),
				    argv[i]);
    }

  gtk_main ();

  return 0;
}

