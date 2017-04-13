/* gpa.c - The GNU Privacy Assistant main file.
   Copyright (C) 2000-2002 G-N-U GmbH.
   Copyright (C) 2005, 2008, 2012, 2014, 2015 g10 Code GmbH.

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
#include "keymanager.h"
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

/* True if the ticker used for card operations should not be started.  */
gboolean disable_ticker;

/* True if the gpgme edit FSM shall output debug messages.  */
gboolean debug_edit_fsm;

/* True if verbose messages are requested.  */
gboolean verbose;

/* Local variables.  */
typedef struct
{
  gboolean start_key_manager;
  gboolean start_file_manager;
  gboolean start_card_manager;
  gboolean start_clipboard;
  gboolean start_settings;
  gboolean start_only_server;
  gboolean stop_running_server;
  gboolean disable_x509;
  gboolean no_remote;
  gboolean enable_logging;
  gchar *options_filename;
} gpa_args_t;

static char *dummy_arg;

static gpa_args_t args;


/* The copyright notice.  */
static const char *copyright =
"Copyright (C) 2000-2002 Miguel Coca, G-N-U GmbH, Intevation GmbH.\n"
"Copyright (C) 2005-2016 g10 Code GmbH.\n"
"This program comes with ABSOLUTELY NO WARRANTY.\n"
"This is free software, and you are welcome to redistribute it\n"
"under certain conditions.  See the file COPYING for details.\n";


static GtkWidget *backend_config_dialog = NULL;


static void print_version (void);


/* All command line options of the main application.  */
static GOptionEntry option_entries[] =
  {
    { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
      (gpointer) &print_version,
      N_("Output version information and exit"), NULL, },
    { "keyring", 'k', 0, G_OPTION_ARG_NONE, &args.start_key_manager,
      N_("Open key manager (default)"), NULL },
    { "files", 'f', 0, G_OPTION_ARG_NONE, &args.start_file_manager,
      N_("Open file manager"), NULL },
#ifdef ENABLE_CARD_MANAGER
    { "card", 'C', 0, G_OPTION_ARG_NONE, &args.start_card_manager,
      N_("Open the card manager"), NULL },
#endif
    { "clipboard", 'c', 0, G_OPTION_ARG_NONE, &args.start_clipboard,
      N_("Open clipboard"), NULL },
    { "settings", 's', 0, G_OPTION_ARG_NONE, &args.start_settings,
      N_("Open the settings dialog"), NULL },
    { "daemon", 'd', 0, G_OPTION_ARG_NONE, &args.start_only_server,
      N_("Only start the UI server"), NULL },
    { "disable-x509", 0, 0, G_OPTION_ARG_NONE, &args.disable_x509,
      N_("Disable support for X.509"), NULL },
    { "options", 'o', 0, G_OPTION_ARG_FILENAME, &args.options_filename,
      N_("Read options from file"), "FILE" },
    { "no-remote", 0, 0, G_OPTION_ARG_NONE, &args.no_remote,
      N_("Do not connect to a running instance"), NULL },
    { "stop-server", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,
      &args.stop_running_server, NULL, NULL },
    /* Note:  the cms option will eventually be removed.  */
    { "cms", 'x', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,
      &cms_hack, NULL, NULL },
    { "verbose", 'v',      G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,
      &verbose,  NULL, NULL },
    { "disable-ticker", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,
      &disable_ticker, NULL, NULL },
    { "debug-edit-fsm", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,
      &debug_edit_fsm, NULL, NULL },
    { "enable-logging", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,
      &args.enable_logging, NULL, NULL },
    { "gpg-binary", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_FILENAME,
      &dummy_arg, NULL, NULL },
    { "gpgsm-binary", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_FILENAME,
      &dummy_arg, NULL, NULL },
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

  /* If installed under bin assume that share is on the same
     hierarchy as the bin subdirectory. Otherwise assume that
     share is a subdirectory of the gpa.exe location. */
  p = strstr (name, "\\bin\\");
  if (!p)
    {
      p = strrchr (name, '\\');
    }
  if (!p)
    {
      *name = '\\';
      p = name;
    }
  p++;
  strcpy (p, "share\\locale");
  return strdup (name);
#else
  return strdup (LOCALEDIR);
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
  if (!args.start_only_server
      && !gpa_key_manager_is_open ()
      && !gpa_file_manager_is_open ()
      && !gpa_clipboard_is_open ()
#ifdef ENABLE_CARD_MANAGER
      && !gpa_card_manager_is_open ()
#endif /*ENABLE_CARD_MANAGER*/
      )
    gpa_stop_server ();
}


static void
close_main_window (GtkWidget *widget, gpointer param)
{
  GtkWidget ** window = param;

  *window = NULL;
  quit_if_no_window ();
}


/* Show the key manager dialog.  */
void
gpa_open_key_manager (GtkAction *action, void *data)
{
  GtkWidget *widget;
  gboolean created;

  widget = gpa_key_manager_get_instance (&created);
  if (created)
    g_signal_connect (G_OBJECT (widget), "destroy",
                      G_CALLBACK (quit_if_no_window), NULL);

  gtk_widget_show_all (widget);
  gtk_window_present (GTK_WINDOW (widget));
}


/* Show the clipboard dialog.  */
void
gpa_open_clipboard (GtkAction *action, void *data)
{
  /* FIXME: Shouldn't this connect only happen if the instance is
     created the first time?  Looks like a memory leak to me.  Right:
     although the closure is ref counted an internal data object will
     get allocated.  */
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
#ifdef ENABLE_CARD_MANAGER
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
#endif /*ENABLE_CARD_MANAGER*/

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



static void
dummy_log_func (const gchar *log_domain, GLogLevelFlags log_level,
                const gchar *message, gpointer user_data)
{
  /* Nothing to be done.  */
}



/* Helper for main.  */
static gpg_error_t
open_requested_window (int argc, char **argv, int use_server)
{
  gpg_error_t err = 0;
  int did_server = 0;
  int i;

  /* Don't open the key manager if any files are given on the command
     line.  Ditto for the clipboard.  */
  if (args.start_key_manager && (optind >= argc))
    {
      if (use_server)
        {
          did_server = 1;
          err = gpa_send_to_server ("START_KEYMANAGER");
        }
      else
        gpa_open_key_manager (NULL, NULL);
    }

  if (args.start_clipboard && (optind >= argc))
    {
      if (use_server)
        {
          did_server = 1;
          err = gpa_send_to_server ("START_CLIPBOARD");
        }
      else
        gpa_open_clipboard (NULL, NULL);
    }

  if (args.start_file_manager || (optind < argc))
    {
      /* Do not use the server if there are file args - see below.  */
      if (use_server && !(optind < argc))
        {
          did_server = 1;
          err = gpa_send_to_server ("START_FILEMANAGER");
        }
      else
        gpa_open_filemanager (NULL, NULL);
    }

#ifdef ENABLE_CARD_MANAGER
  if (args.start_card_manager)
    {
      if (use_server)
        {
          did_server = 1;
          err = gpa_send_to_server ("START_CARDMANAGER");
        }
      else
        gpa_open_cardmanager (NULL, NULL);
    }
#endif /*ENABLE_CARD_MANAGER*/

  if (args.start_settings)
    {
      if (use_server)
        {
          did_server = 1;
          err = gpa_send_to_server ("START_CONFDIALOG");
        }
      else
        gpa_open_settings_dialog (NULL, NULL);
    }

  /* If there are any command line arguments that are not options, try
     to open them as files in the filemanager.  However if we are to
     connect to a server this can't be done because the running
     instance may already have files in the file manager and thus we
     better do not add new files.  Instead we start a new instance. */
  if (use_server)
    {
      if (!did_server)
        err = -1; /* Create a new instance.  */
    }
  else
    {
      for (i = optind; i < argc; i++)
        gpa_file_manager_open_file (GPA_FILE_MANAGER
                                    (gpa_file_manager_get_instance ()),
                                    argv[i]);
    }

  return err;
}


int
main (int argc, char *argv[])
{
  GError *err = NULL;
  GOptionContext *context;
  char *configname = NULL;
  char *keyservers_configname = NULL;

  /* Under W32 logging is disabled by default to prevent MS Windows NT
     from opening a console.  */
#ifndef G_OS_WIN32
  args.enable_logging = 1;
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

  if (!args.enable_logging)
    {
#ifdef __MINGW32__
      hide_gpa_console_window();
#endif
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
    }

  gtk_init (&argc, &argv);
#ifdef G_OS_WIN32
  gtk_settings_set_string_property(gtk_settings_get_default(),
                                   "gtk-theme-name",
                                   "MS-Windows",
                                   "XProperty");
#endif

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

  /* Start the agent if needed.  We need to do this because the card
     manager uses direct assuan commands to the agent and thus expects
     that the agent has been startet. */
  gpa_start_agent ();

  gnupg_homedir = default_homedir ();

  /* GnuPG can not create a key if its home directory is missing.  We
     help it out here.  Should be fixed in GnuPG.  */
  if (! g_file_test (gnupg_homedir, G_FILE_TEST_IS_DIR))
    g_mkdir (gnupg_homedir, 0700);

  /* Locate GPA's configuration file.  */
  if (! args.options_filename)
    configname = g_build_filename (gnupg_homedir, "gpa.conf", NULL);
  else
    configname = args.options_filename;
  gpa_options_set_file (gpa_options_get_instance (), configname);
  g_free (configname);

  if (args.stop_running_server)
    {
      /* If a UI server with the same name is already running, stop
         it.  If not, do nothing.  */
      if (gpa_check_server () == 2)
        gpa_send_to_server ("KILL_UISERVER");
      return 0;
    }

  /* Handle command line options.  */
  cms_hack = !args.disable_x509;

  /* Start the default component.  */
  if (!args.start_key_manager
      && !args.start_file_manager
      && !args.start_clipboard
      && !args.start_settings
      && !args.start_card_manager
      )
    {
      /* The default action is to start the clipboard.  However, if we
         have not yet created a key we remind the user by starting
         with the key manager dialog.  */
      if (key_manager_maybe_firsttime ())
        args.start_key_manager = TRUE;
      else
        args.start_clipboard = TRUE;
    }


  /* Check whether we need to start a server or to simply open a
     window in an already running server.  */
  switch (gpa_check_server ())
    {
    case 0: /* No running server on the expected socket.  Start one.  */
      gpa_start_server ();
      break;
    case 1: /* An old instance or a differen UI server is already running.
               Do not start a server.  */
      break;
    case 2: /* An instance is already running - open the appropriate
               window and terminate.  */
      if (args.no_remote)
        break;
      if (!open_requested_window (argc, argv, 1))
        return 0; /* ready */
      /* Start a new instance on error.  */
      break;
    }

  /* Locate the list of keyservers.  */
  keyservers_configname = g_build_filename (gnupg_homedir, "keyservers", NULL);

  /* Read the list of available keyservers.  */
  keyserver_read_list (keyservers_configname);

  gpa_options_update_default_key (gpa_options_get_instance ());
  /* Now, make sure there are reasonable defaults for the default key
    and keyserver.  */
  if (!is_gpg_version_at_least ("2.1.0")
      && !gpa_options_get_default_keyserver (gpa_options_get_instance ()))
    {
      GList *keyservers = keyserver_get_as_glist ();
      gpa_options_set_default_keyserver (gpa_options_get_instance (),
					 keyservers->data);
    }

  /* Initialize the file watch facility.  */
  gpa_init_filewatch ();

  /* Startup whatever has been requested by the user.  */
  if (!args.start_only_server)
    open_requested_window (argc, argv, 0);

  gtk_main ();

  return 0;
}
