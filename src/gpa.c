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

#include "argparse.h"

#include "gpapastrings.h"
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gtktools.h"
#include "optionsmenu.h"
#include "helpmenu.h"
#include "keyring.h"
#include "fileman.h"
#include "keyserver.h"
#include "keytable.h"

#ifdef __MINGW32__
#include "w32reg.h"
#include "hidewnd.h"
#endif

/* icons for toolbars */
#include "icons.h"

enum cmd_and_opt_values {
  aNull = 0,
  oQuiet	  = 'q',
  oVerbose	  = 'v',

  oNoVerbose = 500,
  oOptions,
  oDebug,
  oDebugAll,
  oNoGreeting,
  oNoOptions,
  oHomedir,
  oGPGBinary,
  oAdvancedUI,
  oBackupGenerated,

  oKeyserver,
  oDefaultKey,

  aTest
};

static ARGPARSE_OPTS opts[] = {

    { 301, NULL, 0, N_("@Options:\n ") },

    { oVerbose, "verbose",   0, N_("verbose") },
    { oQuiet,	"quiet",     0, N_("be somewhat more quiet") },
    { oOptions, "options"  , 2, N_("read options from file")},
    { oDebug,	"debug"     ,4|16, N_("set debugging flags")},
    { oDebugAll, "debug-all" ,0, N_("enable full debugging")},
    { oGPGBinary, "gpg-program", 2 , "@" },
    { oAdvancedUI, "advanced-ui", 0,N_("set the user interface to advanced mode")},
    { oBackupGenerated, "backup-generated", 0,N_("omit \"no backup yet\" warning at startup")},

    { oKeyserver, "keyserver", 2, "@" },
    { oDefaultKey, "default-key", 2, "@" },
    {0}
};

/* Directory where the gpa executable resides */
static char *gpa_homedir = NULL;
static char *gpa_configname = NULL;
static char *keyservers_configname = NULL;
GPAOptions gpa_options;
GtkWidget *global_windowMain = NULL;
GList *global_defaultRecipients = NULL;

/* GPGME context used in all the program */
GpgmeCtx ctx;
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
  gchar *dirs[] = {gpa_options.homedir, GPA_DATADIR, NULL};
#elif G_OS_WIN32
  gchar *dirs[] = {gpa_homedir, GPA_DATADIR, gpa_options.homedir, NULL};
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

static const char *
my_strusage(int level)
{
  const char *p;
  switch(level)
    {
    case 11:
      p = "gpa";
      break;
    case 13:
      p = VERSION;
      break;
      /*case 17: p = PRINTABLE_OS_NAME; break;*/
    case 19:
      p = _("Please report bugs to <" PACKAGE_BUGREPORT ">.\n");
      break;
    case 1:
    case 40:
      p = _("Usage: gpa [options] (-h for help)");
      break;
    case 41:
      p = _("Syntax: gpa [options]\n"
	    "Graphical frontend to GnuPG\n");
      break;

    default:
      p = NULL;
    }
  return p;
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
  bindtextdomain (PACKAGE, GPA_LOCALEDIR);
  textdomain (PACKAGE);
#endif
#endif
}

static gboolean simplified_ui = TRUE;

gboolean
gpa_simplified_ui (void)
{
  return simplified_ui;
}

void
gpa_set_simplified_ui (gboolean value)
{
  simplified_ui = value;
}

static gboolean backup_generated = FALSE;

gboolean
gpa_backup_generated (void)
{
  return backup_generated;
}

void
gpa_set_backup_generated (gboolean value)
{
  backup_generated = value;
}

void
gpa_remember_backup_generated (void)
{
  if (gpa_configname && !gpa_backup_generated ())
    {
      FILE *config = fopen (gpa_configname, "a");
      if (config)
        {
	  fprintf (config, "backup-generated\n");
	  fclose (config);
        }
    }
  gpa_set_backup_generated (TRUE);
}

/*
 * Manage the default key
 */

/* Key id of the default key as malloced memory. NULL means there is not
 * default key.
 */
static gchar *default_key = NULL;

static guint default_key_changed_signal_id = 0;

#ifndef __NEW_GTK__ /* @@@@@@ */
/* create a new signal type for toplevel windows */
static void
gpa_default_key_changed_marshal (GtkObject *object,
				 GtkSignalFunc func,
				 gpointer func_data,
				 GtkArg *args)
{
  ((GPADefaultKeyChanged)func)(func_data);
}
#endif

static void
gpa_create_default_key_signal (void)
{
#ifndef __NEW_GTK__ /* @@@@@@ */
  guint id;

  gpointer klass = gtk_type_class (gtk_window_get_type ());

  id = gtk_object_class_user_signal_new (klass, "gpa_default_key_changed",
					 0, gpa_default_key_changed_marshal,
					 GTK_TYPE_NONE, 0);
  default_key_changed_signal_id = id;
#endif
}

static void
gpa_emit_default_key_changed (void)
{
  GtkWidget * window;

  window = gpa_get_keyring_editor ();
  if (window)
    {
      gtk_signal_emit (GTK_OBJECT (window), default_key_changed_signal_id);
    }
  window = gpa_get_filenamager ();
  if (window)
    {
      gtk_signal_emit (GTK_OBJECT (window), default_key_changed_signal_id);
    }
}


/* Return the default key */
gchar *
gpa_default_key (void)
{
  return default_key;
}

/* Set the default key. key is assumed to be newly malloced memory that
 * should not be free's by the caller afterwards. */
void
gpa_set_default_key (gchar *key)
{
  gboolean emit;

  if (default_key && key)
    emit = strcmp (default_key, key) != 0;
  else
    emit = default_key != key;

  g_free (default_key);
  default_key = key;

  if (gpa_configname && key)
    {
      FILE *config = fopen (gpa_configname, "a");
      if (config)
        {
	  fprintf (config, "default-key %s\n", default_key);
	  fclose (config);
        }
    }

  if (emit)
    gpa_emit_default_key_changed ();
}


/* Return the default key gpg would use, or at least a first
 * approximation. Currently this means the first secret key in the keyring.
 * If there's no secret key at all, return NULL
 */
static gchar *
gpa_determine_default_key (void)
{
  GpgmeKey key;
  GpgmeError err;
  gchar * fpr = NULL;
  err = gpgme_op_keylist_start (ctx, NULL, 1);
  if( err != GPGME_No_Error )
    gpa_gpgme_error (err);
  err = gpgme_op_keylist_next (ctx, &key);
  if( err == GPGME_No_Error )
    {
      /* Copy the ID and release the key, since it's no longer needed */
      fpr = g_strdup (gpgme_key_get_string_attr (key, GPGME_ATTR_FPR, 
                                                 NULL, 0 ));
      gpgme_key_release (key);
      err = gpgme_op_keylist_end (ctx);
      if( err != GPGME_No_Error )
        gpa_gpgme_error (err); 
    }
  else if( err == GPGME_EOF )
    /* No secret keys found */
    fpr = NULL;
  else
    gpa_gpgme_error (err);

  return fpr;
}


/* If the default key is not set yet, set it to whatever
 * gpa_determine_default_key suggests */
void
gpa_update_default_key (void)
{
  if (!default_key)
    gpa_set_default_key (gpa_determine_default_key ());
}


/*
 *  Manage the two main windows
 */

static GtkWidget *keyringeditor = NULL;
static GtkWidget *filemanager = NULL;


static void
quit_if_no_window (void)
{
  if (!keyringeditor && !filemanager)
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
  if (!filemanager)
    {
      filemanager = gpa_fileman_new();
      gtk_signal_connect (GTK_OBJECT (filemanager), "destroy",
			  GTK_SIGNAL_FUNC (close_main_window), &filemanager);
      gtk_widget_show_all (filemanager);
    }
  gdk_window_raise (filemanager->window);
}


GtkWidget *
gpa_get_keyring_editor (void)
{
  return keyringeditor;
}


GtkWidget *
gpa_get_filenamager (void)
{
  return filemanager;
}


static void
dummy_log_func (const gchar *log_domain, GLogLevelFlags log_level,
                const gchar *message, gpointer user_data)
{
  /* empty */
}

/*
 *   Main function
 */
int
main (int argc, char **argv)
{
  ARGPARSE_ARGS pargs;
  int orig_argc;
  char **orig_argv;
  char *configname = NULL;
  FILE *configfp = NULL;
  unsigned configlineno;
  int parse_debug = 0;
  int default_config = 1;
  int greeting = 0;
  int nogreeting = 0;
  const char *gpg_program = GPG_PROGRAM;
  const char *keyserver = NULL;
  GpgmeError err;

#ifdef __MINGW32__
  hide_gpa_console_window();
#endif

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
  gpa_homedir = g_path_get_dirname (argv[0]);

  set_strusage (my_strusage);
  srand (time (NULL)); /* the about dialog uses rand() */
  gtk_init (&argc, &argv);
  i18n_init ();

  gpa_create_default_key_signal ();

#if defined(__MINGW32__) || defined(__CYGWIN__)
  gpa_options.homedir = read_w32_registry_string (NULL, "Software\\GNU\\GnuPG", "HomeDir");
  gpg_program = read_w32_registry_string (NULL, "Software\\GNU\\GnuPG", "gpgProgram");
#else
  gpa_options.homedir = getenv ("GNUPGHOME");
#endif

  if (!gpa_options.homedir || !*gpa_options.homedir)
    {
#ifdef HAVE_DRIVE_LETTERS
      gpa_options.homedir = "c:/gnupg";
#else
      gpa_options.homedir = g_build_filename (getenv("HOME"), ".gnupg", NULL);
#endif
    }

  if (!gpg_program || !*gpg_program) {
      gpg_program = GPG_PROGRAM;
  }

  /* check whether we have a config file on the commandline */
  orig_argc = argc;
  orig_argv = argv;
  pargs.argc = &argc;
  pargs.argv = &argv;
  pargs.flags= 1|(1<<6);  /* do not remove the args, ignore version */
  while (arg_parse (&pargs, opts))
    {
      if (pargs.r_opt == oDebug || pargs.r_opt == oDebugAll)
	parse_debug++;
      else if (pargs.r_opt == oOptions)
	{
	  /* yes there is one, so we do not try the default one, but
	   * read the option file when it is encountered at the commandline
	   */
	  default_config = 0;
	}
      else if (pargs.r_opt == oNoOptions)
	default_config = 0; /* --no-options */
      else if (pargs.r_opt == oHomedir)
	gpa_options.homedir = pargs.r.ret_str;
    }

  #ifdef HAVE_DOSISH_SYSTEM
    if ( strchr (gpa_options.homedir,'\\') ) {
        char *d, *buf = (char*) g_malloc (strlen (gpa_options.homedir)+1);
        const char *s = gpa_options.homedir;
        for (d=buf,s=gpa_options.homedir; *s; s++)
            *d++ = *s == '\\'? '/': *s;
        *d = 0;
        gpa_options.homedir = buf;
    }
  #endif

  /* Locate GPA's configuration file.
   */
  if (default_config)
    configname = search_config_file ("gpa.conf");

  gpa_configname = g_strdup (configname);

  /* Locate the list of keyservers.
   */
  keyservers_configname = search_config_file ("keyservers");

  argc = orig_argc;
  argv = orig_argv;
  pargs.argc = &argc;
  pargs.argv = &argv;
  pargs.flags=  1;  /* do not remove the args */

 next_pass:
  if (configname)
    {
      configlineno = 0;
      configfp = fopen (configname, "r");
      if (!configfp)
	{
	  if (default_config)
	    {
	      if (parse_debug)
                fprintf (stderr, _("NOTE: no default option file `%s'\n"),
                         configname);
	    }
	  else {
	    fprintf (stderr, _("option file `%s': %s\n"),
		       configname, strerror(errno));
	    exit(2);
	  }
	  g_free (configname);
	  configname = NULL;
	}
      if (parse_debug && configname)
	fprintf (stderr, _("reading options from `%s'\n"), configname);
      default_config = 0;
    }

  while (optfile_parse (configfp, configname, &configlineno,
			&pargs, opts))
    {
      switch(pargs.r_opt)
	{
	case oQuiet: gpa_options.quiet = 1; break;
	case oVerbose: gpa_options.verbose++; break;

	case oDebug: gpa_options.debug |= pargs.r.ret_ulong; break;
	case oDebugAll: gpa_options.debug = ~0; break;

	case oOptions:
	  /* config files may not be nested (silently ignore them) */
	  if (!configfp)
	    {
	      g_free (configname);
	      configname = g_strdup (pargs.r.ret_str);
	      goto next_pass;
	    }
	  break;
	case oNoGreeting: nogreeting = 1; break;
	case oNoVerbose: gpa_options.verbose = 0; break;
	case oNoOptions: break; /* no-options */
	case oHomedir: gpa_options.homedir = pargs.r.ret_str; break;
	case oGPGBinary: gpg_program = pargs.r.ret_str;  break;
	case oAdvancedUI: gpa_set_simplified_ui (FALSE); break;
	case oBackupGenerated: gpa_set_backup_generated (TRUE); break;
        case oKeyserver: keyserver = pargs.r.ret_str; break;
        case oDefaultKey:
	  if (default_key)
	    g_free (default_key);
	  default_key = g_strdup (pargs.r.ret_str);
	  break;

	default : pargs.err = configfp? 1:2; break;
	}
    }
  if (configfp)
    {
      fclose(configfp);
      configfp = NULL;
      g_free (configname);
      configname = NULL;
      goto next_pass;
    }
  g_free (configname);
  configname = NULL;
  if (nogreeting)
    greeting = 0;

  if (greeting)
    {
      fprintf (stderr, "%s %s; %s\n",
	       strusage (11), strusage (13), strusage (14));
      fprintf (stderr, "%s\n", strusage (15));
      fflush (stderr);
    }
#ifdef IS_DEVELOPMENT_VERSION
  fprintf (stderr, "NOTE: this is a development version!\n");
#endif

  /* Read the list of available keyservers.
   */
  keyserver_read_list (keyservers_configname);
  keyserver_set_current (keyserver);

  /* Initialize GPGME */
  gpgme_check_version (NULL);
  err = gpgme_new (&ctx);
  if (err != GPGME_No_Error)
    gpa_gpgme_error (err);
  gpgme_set_passphrase_cb (ctx, gpa_passphrase_cb, NULL);
  /* initialize the default key to a useful default */
  gpa_update_default_key ();
  keytable = gpa_keytable_new ();

#ifndef HAVE_DOSISH_SYSTEM
  /* Internationalisation with gtk+-2.0 wants UTF8 instead of the
   * character set determined by `locale'.
   */
  setenv ("OUTPUT_CHARSET", "utf8", 1);
#endif

  gpa_open_keyring_editor ();

  /* for development purposes open the file manager too: */
  /*gpa_open_filemanager ();*/
  gtk_main ();
  return 0;
}
