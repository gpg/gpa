/* gpa.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <gpapa.h>


#include "argparse.h"
#include "stringhelp.h"

#include "gpapastrings.h"
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gtktools.h"
#include "filemenu.h"
#include "keysmenu.h"
#include "optionsmenu.h"
#include "helpmenu.h"
#include "keyring.h"
#include "fileman.h"
#include "keyserver.h"

#ifdef __MINGW32__
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

  oKeyserver,

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
    { oAdvancedUI, "advanced-ui", 0,N_("use a advanced user interface")},

    { oKeyserver, "keyserver", 2, "@" },
    {0}
};


GPAOptions gpa_options;
static GtkWidget *global_clistFile = NULL;
GtkWidget *global_windowMain = NULL;
GpapaAction global_lastCallbackResult;
GList *global_defaultRecipients = NULL;


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
      p = _("Please report bugs to <gpa-bugs@gnu.org>.\n");
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

/*
 * Manage the default key
 */

/* Key id of the default key as malloced memory. NULL means there is not
 * default key.
 */
static gchar *default_key = NULL;

static guint default_key_changed_signal_id = 0;

/* create a new signal type for toplevel windows */
static void
gpa_default_key_changed_marshal (GtkObject *object,
				 GtkSignalFunc func,
				 gpointer func_data,
				 GtkArg *args)
{
  ((GPADefaultKeyChanged)func)(func_data);
}

static void
gpa_create_default_key_signal (void)
{
  guint id;

  gpointer klass = gtk_type_class (gtk_window_get_type ());

  id = gtk_object_class_user_signal_new (klass, "gpa_default_key_changed",
					 0, gpa_default_key_changed_marshal,
					 GTK_TYPE_NONE, 0);
  default_key_changed_signal_id = id;
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
gpa_set_default_key (gchar * key)
{
  gboolean emit;

  if (default_key && key)
    emit = strcmp (default_key, key) != 0;
  else
    emit = default_key != key;

  free (default_key);
  default_key = key;

  if (emit)
    gpa_emit_default_key_changed ();
}


/* Return the default key gpg would use, or at least a first
 * approximation. Currently this means the secret key with index 0. If
 * there's no secret key at all, return NULL
 */
static gchar *
gpa_determine_default_key (void)
{
  GpapaSecretKey *key;
  gchar * id;

  if (gpapa_get_secret_key_count (gpa_callback, NULL))
    {
      key = gpapa_get_secret_key_by_index (0, gpa_callback, NULL);
      id = xstrdup_or_null (gpapa_key_get_identifier (GPAPA_KEY (key),
                                                      gpa_callback, NULL));
    }
  else
    id = NULL;

  return id;
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

static GtkWidget * keyringeditor = NULL;
static GtkWidget * filemanager = NULL;


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


/*
 *   Main function
 */
int
main (int argc, char **argv)
{
  ARGPARSE_ARGS pargs;
  int orig_argc;
  char **orig_argv;
  FILE *configfp = NULL;
  char *configname = NULL;
  unsigned configlineno;
  int parse_debug = 0;
  int default_config =1;
  int greeting = 0;
  int nogreeting = 0;
  const char *gpg_program = GPG_PROGRAM;
  gchar * gtkrc;
  const char *keyserver = NULL;

#ifdef __MINGW32__
  hide_gpa_console_window();
#endif

  set_strusage (my_strusage);
  /*log_set_name ("gpa"); not yet implemented in logging.c */
  srand (time (NULL)); /* the about dialog uses rand() */
  gtk_init (&argc, &argv);
  i18n_init ();

  gpa_create_default_key_signal ();

  /* read the gpa gtkrc */
  gtkrc = make_filename (GPA_DATADIR, "gtkrc", NULL);
  if (gtkrc)
    {
      gtk_rc_parse (gtkrc);
      free (gtkrc);
    }

  #if defined(__MINGW32__) || defined(__CYGWIN__)
    gpa_options.homedir = read_w32_registry_string( NULL, "Software\\GNU\\GnuPG", "HomeDir" );
    gpg_program = read_w32_registry_string( NULL, "Software\\GNU\\GnuPG", "gpgProgram" );
  #else
     gpa_options.homedir = getenv ("GNUPGHOME");
  #endif

  if (!gpa_options.homedir || !*gpa_options.homedir)
    {
#ifdef HAVE_DRIVE_LETTERS
      gpa_options.homedir = "c:/gnupg";
#else
      gpa_options.homedir = "~/.gnupg";
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
    if ( strchr (gpa_opt.homedir,'\\') ) {
        char *d, *buf = m_alloc (strlen (opt.homedir)+1);
        const char *s = gpa_opt.homedir;
        for (d=buf,s=gpa_opt.homedir; *s; s++)
            *d++ = *s == '\\'? '/': *s;
        *d = 0;
        gpa_opt.homedir = buf;
    }
  #endif

  if (default_config)
    configname = make_filename (gpa_options.homedir, "gpa.conf", NULL);


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
		log_info (_("NOTE: no default option file `%s'\n"),
			  configname);
	    }
	  else {
	    log_error (_("option file `%s': %s\n"),
		       configname, strerror(errno));
	    exit(2);
	  }
	  free (configname);
	  configname = NULL;
	}
      if (parse_debug && configname)
	log_info (_("reading options from `%s'\n"), configname);
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
	      free (configname);
	      configname = xstrdup (pargs.r.ret_str);
	      goto next_pass;
	    }
	  break;
	case oNoGreeting: nogreeting = 1; break;
	case oNoVerbose: gpa_options.verbose = 0; break;
	case oNoOptions: break; /* no-options */
	case oHomedir: gpa_options.homedir = pargs.r.ret_str; break;
	case oGPGBinary: gpg_program = pargs.r.ret_str;  break;
	case oAdvancedUI: gpa_set_simplified_ui (FALSE); break;
        case oKeyserver: keyserver = pargs.r.ret_str; break;

	default : pargs.err = configfp? 1:2; break;
	}
    }
  if (configfp)
    {
      fclose(configfp);
      configfp = NULL;
      free(configname);
      configname = NULL;
      goto next_pass;
    }
  free (configname);
  configname = NULL;
  if (log_get_errorcount (0))
    exit(2);
  if (nogreeting)
    greeting = 0;

  if (greeting)
    {
      fprintf(stderr, "%s %s; %s\n",
	      strusage (11), strusage (13), strusage (14));
      fprintf(stderr, "%s\n", strusage (15));
    }
#ifdef IS_DEVELOPMENT_VERSION
  log_info("NOTE: this is a development version!\n");
#endif

  /* read the list of available keyservers */
  keyserver_read_list ("keyservers");
  keyserver_set_current (keyserver);

  gpapa_init (gpg_program);

  /* initialize the default key to a useful default */
  gpa_update_default_key ();

  /*global_windowMain = gpa_windowMain_new (_("GNU Privacy Assistant"));*/
  /*  global_windowMain = keyring_editor_new();
  gtk_signal_connect (GTK_OBJECT (global_windowMain), "delete_event",
		      GTK_SIGNAL_FUNC (file_quit), NULL);
  gtk_widget_show_all (global_windowMain);
  */

  gpa_open_keyring_editor ();
  /* for development purposes open the file manager too: */
  /*gpa_open_filemanager ();*/
  gtk_main ();
  return 0;
}

void
gpa_callback (GpapaAction action, gpointer actiondata, gpointer calldata)
{
  switch (action)
    {
    case GPAPA_ACTION_ERROR:
      gpa_window_error ((gchar *) actiondata, (GtkWidget *) calldata);
      break;
    default:
      break;
    }				/* switch */
  global_lastCallbackResult = action;
}				/* gpa_callback */

GtkWidget *
gpa_get_global_clist_file (void)
{
  return (global_clistFile);
}				/* gpa_get_global_clist_file */


void
sigs_append (gpointer data, gpointer userData)
{
/* var */
  GpapaSignature *signature;
  gpointer *localParam;
  GtkWidget *clistSignatures;
  GtkWidget *windowPublic;
  gchar *contentsSignatures[3];
/* commands */
  signature = (GpapaSignature *) data;
  localParam = (gpointer *) userData;
  clistSignatures = (GtkWidget *) localParam[0];
  windowPublic = (GtkWidget *) localParam[1];
  contentsSignatures[0] =
    gpapa_signature_get_name (signature, gpa_callback, windowPublic);
  if (!contentsSignatures[0])
    contentsSignatures[0] = _("[Unknown user ID]");
  contentsSignatures[1] =
    gpa_sig_validity_string (gpapa_signature_get_validity
			     (signature, gpa_callback, windowPublic));
  contentsSignatures[2] =
    gpapa_signature_get_identifier (signature, gpa_callback, windowPublic);
  gtk_clist_append (GTK_CLIST (clistSignatures), contentsSignatures);
}				/* sigs_append */

gint
compareInts (gconstpointer a, gconstpointer b)
{
/* var */
  gint aInt, bInt;
/* commands */
  aInt = *(gint *) a;
  bInt = *(gint *) b;
  if (aInt > bInt)
    return 1;
  else if (aInt < bInt)
    return -1;
  else
    return 0;
}				/* compareInts */

void
gpa_selectRecipient (GtkWidget * clist, gint row, gint column,
		     GdkEventButton * event, gpointer userData)
{
/* var */
  GList **recipientsSelected;
  gint *rowData;
/* commands */
  recipientsSelected = (GList **) userData;
  rowData = (gint *) xmalloc (sizeof (gint));
  *rowData = row;
  *recipientsSelected =
    g_list_insert_sorted (*recipientsSelected, rowData, compareInts);
}				/* gpa_selectRecipient */

void
gpa_unselectRecipient (GtkWidget * clist, gint row, gint column,
		       GdkEventButton * event, gpointer userData)
{
/* var */
  GList **recipientsSelected;
  GList *indexRecipient, *indexNext;
  gpointer data;
/* commands */
  recipientsSelected = (GList **) userData;
  indexRecipient = g_list_first (*recipientsSelected);
  while (indexRecipient)
    {
      indexNext = g_list_next (indexRecipient);
      if (*(gint *) indexRecipient->data == row)
	{
	  data = indexRecipient->data;
	  *recipientsSelected = g_list_remove (*recipientsSelected, data);
	  free (data);
	}			/* if */
      indexRecipient = indexNext;
    }				/* while */
}				/* gpa_unselectRecipient */

void
gpa_removeRecipients (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **recipientsSelected;
  GtkWidget *clistRecipients;
  GtkWidget *windowEncrypt;
  GList *indexRecipient;
/* commands */
  localParam = (gpointer *) param;
  recipientsSelected = (GList **) localParam[0];
  clistRecipients = (GtkWidget *) localParam[1];
  windowEncrypt = (GtkWidget *) localParam[2];
  if (!*recipientsSelected)
    {
      gpa_window_error (_("No keys selected to remove from recipients list"),
			windowEncrypt);
      return;
    }				/* if */
  indexRecipient = g_list_last (*recipientsSelected);
  while (indexRecipient)
    {
      gtk_clist_remove (GTK_CLIST (clistRecipients),
			*(gint *) indexRecipient->data);
      free (indexRecipient->data);
      indexRecipient = g_list_previous (indexRecipient);
    }				/* while */
  g_list_free (*recipientsSelected);
}				/* gpa_removeRecipients */

void
gpa_addRecipient (gpointer data, gpointer userData)
{
  gpointer *localParam;
  GpapaPublicKey *key;
  GtkWidget *clistRecipients;
  GtkWidget *windowEncrypt;
  gchar *contentsRecipients[2];
  gboolean do_insert = TRUE;
  gint row, num_rows;

  localParam = (gpointer *) userData;
  clistRecipients = (GtkWidget *) localParam[0];
  windowEncrypt = (GtkWidget *) localParam[1];

  /* find out, whether the key is already in the recipients list and set
   * do_insert to FALSE if that'sthe case. FIXME: This is very
   * inefficient but it's simple and the real solution would be to
   * rewrite the whole default recipients stuff completely anyway */
  num_rows = GTK_CLIST (clistRecipients)->rows;
  for (row = 0; row < num_rows; row++)
    {
      gchar * text = "";
      gtk_clist_get_text (GTK_CLIST (clistRecipients), row, 1, &text);
      if (strcmp (text, (const char*)data) == 0)
	{
	  do_insert = FALSE;
	  break;
	}
    }

  /* Insert the key in the recipients list if do_insert is true, i.e. if
   * it's not already in the list */
  if (do_insert)
    {
      key = gpapa_get_public_key_by_ID ((gchar *) data, gpa_callback,
					windowEncrypt);
      contentsRecipients[0] = gpapa_key_get_name (GPAPA_KEY (key),
						  gpa_callback, windowEncrypt);
      contentsRecipients[1] = gpapa_key_get_identifier (GPAPA_KEY (key),
							gpa_callback,
							windowEncrypt);
      gtk_clist_append (GTK_CLIST (clistRecipients), contentsRecipients);
    }
} /* gpa_addRecipient */

void
gpa_addRecipients (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *clistKeys;
  GtkWidget *clistRecipients;
  GtkWidget *windowEncrypt;
  gpointer newParam[2];
/* commands */
  localParam = (gpointer *) param;
  keysSelected = (GList **) localParam[0];
  clistKeys = (GtkWidget *) localParam[1];
  clistRecipients = (GtkWidget *) localParam[2];
  windowEncrypt = (GtkWidget *) localParam[3];
  if (!*keysSelected)
    {
      gpa_window_error (_("No keys selected to add to recipients list."),
			windowEncrypt);
      return;
    }				/* if */
  newParam[0] = clistRecipients;
  newParam[1] = windowEncrypt;
  g_list_foreach (*keysSelected, gpa_addRecipient, (gpointer) newParam);
  gtk_clist_unselect_all (GTK_CLIST (clistKeys));
}				/* gpa_addRecipients */

void
freeRowData (gpointer data, gpointer userData)
{
  free (data);
}				/* freeRowData */

void
gpa_recipientWindow_close (gpointer param)
{
/* var */
  gpointer *localParam;
  GList **recipientsSelected;
  GList **keysSelected;
  GpaWindowKeeper *keeperEncrypt;
  gpointer *paramClose;
/* commands */
  localParam = (gpointer *) param;
  recipientsSelected = (GList **) localParam[0];
  keysSelected = (GList **) localParam[1];
  keeperEncrypt = (GpaWindowKeeper *) localParam[2];
  g_list_foreach (*recipientsSelected, freeRowData, NULL);
  g_list_free (*recipientsSelected);
  g_list_free (*keysSelected);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeperEncrypt, paramClose);
  paramClose[0] = keeperEncrypt;
  paramClose[1] = NULL;
  gpa_window_destroy (paramClose);
}				/* gpa_recipientWindow_close */
