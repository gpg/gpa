/* gpa.c - The GNU Privacy Assistant main file.
   Copyright (C) 2000-2002 G-N-U GmbH.
   Copyright (C) 2005, 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <gpgme.h>
#include <getopt.h>

#include <glib/gstdio.h>

#include "get-path.h"
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
#include "confdialog.h"

#ifdef __MINGW32__
#include "hidewnd.h"
#endif

/* icons for toolbars */
#include "icons.h"

/* Global variables */
gchar *gnupg_homedir;
int cms_hack;


/* Return a malloced string with the locale dir.  Return NULL on
   error. */
static char *
get_locale_dir (void)
{
#ifdef G_OS_WIN32
  char name[530];
  char *p;

  if ( !GetModuleFileNameA (0, name, sizeof (name)-30) )
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


/* Manage the two main windows and the settings dialog.  */

static GtkWidget *keyringeditor = NULL;
static GtkWidget *settings_dialog = NULL;
static GtkWidget *backend_config_dialog = NULL;

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


void
gpa_open_backend_config_dialog (void)
{
  if (!backend_config_dialog)
    {
      backend_config_dialog = gpa_backend_config_dialog_new ();
      gtk_signal_connect (GTK_OBJECT (backend_config_dialog), "destroy",
			  GTK_SIGNAL_FUNC (close_main_window),
                          &backend_config_dialog);
      gtk_widget_show_all (backend_config_dialog);
    }
  gdk_window_raise (backend_config_dialog->window);
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


typedef struct
{
  gboolean start_keyring_editor;
  gboolean start_file_manager;
  gboolean start_only_server;
  gchar *options_filename;
} GpaCommandLineArgs;


struct option longopts[] =
{
  { "version", no_argument, NULL, 'v' },
  { "help", no_argument, NULL, 'h' },
  { "keyring", no_argument, NULL, 'k' },
  { "keymanager", no_argument, NULL, 'k' },
  { "server", no_argument, NULL, 's' },
  { "files", no_argument, NULL, 'f' },
  { "options", required_argument, NULL, 'o' },
  { "cms", no_argument, NULL, 'x' },
  { NULL, 0, NULL, 0 }
};


struct
{
  char short_opt;
  char *long_opt;
  char *description;
} option_desc[] = 
{
  { 'h', "help", N_("display this help and exit") },
  { 'v', "version", N_("output version information and exit") },
  { 'k', "keyring", N_("open keyring editor (default)") },
  { 'f', "files", N_("open filemanager") },
  { 's', "server", N_("start only the UI server") },
  { 'o', "options", N_("read options from file") },
  { 'x', "cms", "enable CMS hack" },
  { 0, NULL, NULL }
};


/* The copyright notice.  */
const char *copyright = 
"Copyright (C) 2000-2002 Miguel Coca, G-N-U GmbH, Intevation GmbH.\n"
"Copyright (C) 2008 g10 Code GmbH.\n"
"This program comes with ABSOLUTELY NO WARRANTY.\n"
"This is free software, and you are welcome to redistribute it\n"
"under certain conditions.  See the file COPYING for details.\n";

static void
print_copyright (void)
{
  printf ("%s %s\n%s", PACKAGE, VERSION, copyright);
}


/* Print usage information.  */
static void
usage (void)
{
  int i;
  
  print_copyright ();

  printf ("%s\n", _("Syntax: gpa [OPTION...] [FILE...]\n"
                    "Graphical frontend to GnuPG\n"));
  printf ("%s:\n\n", _("Options"));

  for (i = 0; option_desc[i].long_opt; i++)
    {
      printf (" -%c, --%s\t\t%s\n", option_desc[i].short_opt,
              option_desc[i].long_opt, _(option_desc[i].description));
    }

  printf ("\n%s", _("Please report bugs to <" PACKAGE_BUGREPORT ">.\n"));
}


/* Parse the command line.  */
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
        case 's':
          args->start_only_server = TRUE;
          break;
        case 'x':
          cms_hack = 1;
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


static void
dummy_log_func (const gchar *log_domain, GLogLevelFlags log_level,
                const gchar *message, gpointer user_data)
{
  /* Nothing to be done.  */
}


int
main (int argc, char *argv[])
{
  char *configname = NULL;
  char *keyservers_configname = NULL;
  GpaCommandLineArgs args = { FALSE, FALSE, FALSE, NULL };
  int i;
  GError *err = NULL;

#ifdef __MINGW32__
  hide_gpa_console_window();
#endif

  parse_command_line (argc, argv, &args);


  /* Disable logging to prevent MS Windows NT from opening a
     console.  */
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

  gtk_init (&argc, &argv);
  i18n_init ();
  /* Default icon for all windows */
  gtk_window_set_default_icon_from_file (GPA_DATADIR "/gpa.png", &err);
  if (err)
    g_error_free (err);

  gnupg_homedir = default_homedir ();

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

  /* Locate GPA's configuration file.  */
  if (! args.options_filename)
    {
      /* This is on the cheesy side.  GnuPG can not create a key if
	 its home directory is missing.  We give it a bit of a jump
	 start here.  */
      if (! g_file_test (gnupg_homedir, G_FILE_TEST_IS_DIR))
	g_mkdir (gnupg_homedir, 0700);
      configname = g_build_filename (gnupg_homedir, "gpa.conf", NULL);
    }
  else
    {
      configname = args.options_filename;
    }
  gpa_options_set_file (gpa_options_get_instance (), configname);
  g_free (configname);

  /* Locate the list of keyservers.  */
  keyservers_configname = g_build_filename (gnupg_homedir, "keyservers", NULL);

  /* Read the list of available keyservers.  */
  keyserver_read_list (keyservers_configname);

#if 0
  /* Now, make sure there are reasonable defaults for the default key
    and keyserver.  */
  gpa_options_update_default_key (gpa_options_get_instance ());
  if (!gpa_options_get_default_keyserver (gpa_options_get_instance ()))
    {
      GList *keyservers = keyserver_get_as_glist ();
      gpa_options_set_default_keyserver (gpa_options_get_instance (),
					 keyservers->data);
    }
#endif

#ifndef G_OS_WIN32
  /* Internationalisation with gtk+-2.0 wants UTF-8 instead of the
     character set determined by `locale'.  */
  putenv ("OUTPUT_CHARSET=utf8");
#endif

  /* Fire up the server.  */
  gpa_start_server ();

  if (! args.start_only_server)
    {
      /* Don't open the keyring editor if any files are given on the
         command line */
      if (args.start_keyring_editor && (optind >= argc))
	gpa_open_keyring_editor ();
  
      if (args.start_file_manager || (optind < argc))
	gpa_open_filemanager ();

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
