/* gpa.c - The GNU Privacy Assistant
 * Copyright (C) 2000-2002 G-N-U GmbH.
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GPA; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#include <gpgme.h>
#include <getopt.h>

#include "gpapastrings.h"
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gtktools.h"
#include "helpmenu.h"
#include "keyring.h"
#include "fileman.h"
#include "keyserver.h"
#include "keytable.h"
#include "settingsdlg.h"

#ifdef __MINGW32__
#include "w32reg.h"
#include "hidewnd.h"
#endif

/* icons for toolbars */
#include "icons.h"

/* Global variables */
gchar *gpa_exec_dir;
gchar *gnupg_homedir;
/* The global table of keys */
GPAKeyTable *keytable;

/* Search for a configuration file.
 * It uses a different search order for Windows and Unix.
 *
 * On Unix:
 *  1. in the GnuPG home directory
 *  2. in GPA_DATADIR
 * If the file is not found, return a filename that will create it
 * in the GnuPG home directory.
 *
 * On Windows:
 *  1. in the directory where `gpa.exe' resides,
 *  2. in GPA_DATADIR
 *  3. in the GnuPG home directory.
 * If the file is not found, return a filename that will create it
 * in the directory where `gpa.exe' resides.
 */
static gchar *
search_config_file (const gchar *filename)
{
  gchar *candidate = NULL;
  gint i;
  /* The search order for each OS, a NULL terminated array */
#ifdef G_OS_UNIX
  gchar *dirs[] = {gnupg_homedir, GPA_DATADIR, NULL};
#elif G_OS_WIN32
  gchar *dirs[] = {gpa_exec_dir, GPA_DATADIR, gnupg_homedir, NULL};
#endif
  for( i = 0; dirs[i]; i++ )
    {
      candidate = g_build_filename (dirs[i], filename, NULL);
      if (g_file_test (candidate, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
        return candidate;
      else
        g_free (candidate);
    }
  /* If no file exists, return the first option to be created */
  return g_build_filename (dirs[0], filename, NULL);
}

static void
i18n_init (void)
{
#ifdef USE_SIMPLE_GETTEXT
  gtk_set_locale ();
  set_gettext_file (PACKAGE);
#else
#ifdef ENABLE_NLS
  gtk_set_locale ();
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  bindtextdomain (PACKAGE, GPA_LOCALEDIR);
  textdomain (PACKAGE);
#endif
#endif
}


/*
 *  Manage the two main windows and the settings dialog.
 */

static GtkWidget *keyringeditor = NULL;
static GtkWidget *settings_dialog = NULL;

static void
quit_if_no_window (void)
{
  if (!keyringeditor && !gpa_file_manager_is_open ())
    {
      gtk_main_quit ();
    }
}


static void
close_main_window (GtkWidget *widget, gpointer param)
{
  GtkWidget ** window = param;

  *window = NULL;
  quit_if_no_window ();
}


void
gpa_open_keyring_editor (void)
{
  if (!keyringeditor)
    {
      keyringeditor = keyring_editor_new();
      gtk_signal_connect (GTK_OBJECT (keyringeditor), "destroy",
			  GTK_SIGNAL_FUNC (close_main_window), &keyringeditor);
      gtk_widget_show_all (keyringeditor);
    }
  gdk_window_raise (keyringeditor->window);
}


void
gpa_open_filemanager (void)
{
  gtk_signal_connect (GTK_OBJECT (gpa_file_manager_get_instance ()), "destroy",
		      GTK_SIGNAL_FUNC (quit_if_no_window), NULL);
  gtk_widget_show_all (gpa_file_manager_get_instance ());
  gdk_window_raise (gpa_file_manager_get_instance ()->window);
}

void
gpa_open_settings_dialog (void)
{
  if (!settings_dialog)
    {
      settings_dialog = gpa_settings_dialog_new ();
      gtk_signal_connect (GTK_OBJECT (settings_dialog), "destroy",
			  GTK_SIGNAL_FUNC (close_main_window),
                          &settings_dialog);
      gtk_widget_show_all (settings_dialog);
    }
  gdk_window_raise (settings_dialog->window);
}

GtkWidget *
gpa_get_keyring_editor (void)
{
  return keyringeditor;
}

GtkWidget *
gpa_get_settings_dialog (void)
{
  return settings_dialog;
}

static void
dummy_log_func (const gchar *log_domain, GLogLevelFlags log_level,
                const gchar *message, gpointer user_data)
{
  /* empty */
}

typedef struct
{
  gboolean start_keyring_editor;
  gboolean start_file_manager;
  gchar *options_filename;
} GpaCommandLineArgs;

struct option longopts[] =
{
  { "version", no_argument, NULL, 'v' },
  { "help", no_argument, NULL, 'h' },
  { "keyring", no_argument, NULL, 'k' },
  { "files", no_argument, NULL, 'f' },
  { "options", required_argument, NULL, 'o' },
  { NULL, 0, NULL, 0 }
};

struct
{
  char short_opt;
  char *long_opt;
  char *description;
} option_desc[] = 
{
  {'h', "help", N_("display this help and exit")},
  {'v', "version", N_("output version information and exit")},
  {'k', "keyring", N_("open keyring editor (default)")},
  {'f', "files", N_("open filemanager")},
  {'o', "options", N_("read options from file")},
  {0, NULL, NULL}
};

/* The copyright notice */
const char *copyright = 
"Copyright (C) 2000-2002 Miguel Coca, G-N-U GmbH, Intevation GmbH.\n"
"This program comes with ABSOLUTELY NO WARRANTY.\n"
"This is free software, and you are welcome to redistribute it\n"
"under certain conditions. See the file COPYING for details.\n";

static void
print_copyright (void)
{
  printf ("%s %s\n%s", PACKAGE, VERSION, copyright);
}

/* Help option */
static void
usage (void)
{
  int i;
  
  print_copyright ();
  printf ("%s\n", _("Syntax: gpa [options]\n"
                    "Graphical frontend to GnuPG\n"));
  printf ("%s:\n\n", _("Options"));
  for (i = 0; option_desc[i].long_opt; i++)
    {
      printf (" -%c, --%s\t\t%s\n", option_desc[i].short_opt,
              option_desc[i].long_opt, _(option_desc[i].description));
    }
  printf ("\n%s", _("Please report bugs to <" PACKAGE_BUGREPORT ">.\n"));
}

/* Parse the command line. */
static void
parse_command_line (int argc, char **argv, GpaCommandLineArgs *args)
{
  int optc;

  opterr = 1;
  
  while ((optc = getopt_long (argc, argv, "hvo:kf", longopts, (int *) 0))
         != EOF)
    {
      switch (optc)
        {
        case 'h':
          usage ();
          exit (EXIT_SUCCESS);
        case 'v':
          print_copyright ();
          exit (EXIT_SUCCESS);
          break;
        case 'o':
          args->options_filename = g_strdup (optarg);
          break;
        case 'k':
          args->start_keyring_editor = TRUE;
          break;
        case 'f':
          args->start_file_manager = TRUE;
          break;
        default:
          exit (EXIT_FAILURE);
        }
    }

  /* Start the keyring editor by default. */
  if (!args->start_keyring_editor && !args->start_file_manager)
    {
      args->start_keyring_editor = TRUE;
    }
}

/*
 *   Main function
 */
int
main (int argc, char **argv)
{
  char *configname = NULL, *keyservers_configname = NULL;
  GpaCommandLineArgs args = {FALSE, FALSE, NULL};
  int i;

#ifdef __MINGW32__
  hide_gpa_console_window();
#endif

  parse_command_line (argc, argv, &args);

  /* Disable logging to prevent MS Windows NT from opening a console.
   */
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

  /* Find out where our executable lives */
  gpa_exec_dir = g_path_get_dirname (argv[0]);

  srand (time (NULL)); /* the about dialog uses rand() */
  gtk_init (&argc, &argv);
  i18n_init ();

  /* Try to find the GnuPG homedir (~/.gnupg) dinamically */
#ifdef G_OS_WIN32
  gnupg_homedir = read_w32_registry_string (NULL, "Software\\GNU\\GnuPG", "HomeDir");
#else
  gnupg_homedir = getenv ("GNUPGHOME");
#endif

  /* If the previous guess failed, resot to the default value */
  if (!gnupg_homedir || !*gnupg_homedir)
    {
#ifdef G_OS_WIN32
      gnupg_homedir = "c:/gnupg";
#else
      gnupg_homedir = g_build_filename (getenv("HOME"), ".gnupg", NULL);
#endif
    }

#ifdef IS_DEVELOPMENT_VERSION
  fprintf (stderr, "NOTE: this is a development version!\n");
#endif

  /* Initialize GPGME */
  gpgme_check_version (NULL);

  /* Locate GPA's configuration file.
   */
  if (!args.options_filename)
    {
      configname = search_config_file ("gpa.conf");
    }
  else
    {
      configname = args.options_filename;
    }
  gpa_options_set_file (gpa_options_get_instance (),
			configname);
  g_free (configname);

  /* Locate the list of keyservers.
   */
  keyservers_configname = search_config_file ("keyservers");
  /* Read the list of available keyservers.
   */
  keyserver_read_list (keyservers_configname);

  /* Now, make sure there are reasonable defaults for the default key and
   * keyserver */
  gpa_options_update_default_key (gpa_options_get_instance ());
  if (!gpa_options_get_default_keyserver (gpa_options_get_instance ()))
    {
      GList *keyservers = keyserver_get_as_glist ();
      gpa_options_set_default_keyserver (gpa_options_get_instance (),
					 keyservers->data);
    }

  /* Load the list of keys */
  keytable = gpa_keytable_new ();

#ifndef G_OS_WIN32
  /* Internationalisation with gtk+-2.0 wants UTF8 instead of the
   * character set determined by `locale'.
   */
  putenv ("OUTPUT_CHARSET=utf8");
#endif

  /* Don't open the keyring editor if any files are given on the command
   * line */
  if (args.start_keyring_editor && (optind >= argc))
    {
      gpa_open_keyring_editor ();
    }
  
  if (args.start_file_manager || (optind < argc))
    {
      gpa_open_filemanager ();
    }

  /* If there are any command line arguments that are not options, try
   * to open them as files in the filemanager */
  for (i = optind; i < argc; i++)
    {
      gpa_file_manager_open_file (GPA_FILE_MANAGER
				  (gpa_file_manager_get_instance ()),
				  argv[i]);
    }

  gtk_main ();
  return 0;
}
