/* gpa.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000 Free Software Foundation, Inc.
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

#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <gpapa.h>


#include "argparse.h"
#include "stringhelp.h"

#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gtktools.h"
#include "filemenu.h"
#include "keysmenu.h"
#include "optionsmenu.h"
#include "helpmenu.h"

/* icons for toolbars */
#include "icons.xpm"

gchar *writtenSigValidity[3] = {
  N_("unknown"),
  N_("!INVALID!"),
  N_("valid")
};

enum cmd_and_opt_values { aNull = 0,
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


    oTooltipKludge,
aTest };


static ARGPARSE_OPTS opts[] = {

    { 301, NULL, 0, N_("@Options:\n ") },

    { oVerbose, "verbose",   0, N_("verbose") },
    { oQuiet,	"quiet",     0, N_("be somewhat more quiet") },
    { oOptions, "options"  , 2, N_("read options from file")},
    { oDebug,	"debug"     ,4|16, N_("set debugging flags")},
    { oDebugAll, "debug-all" ,0, N_("enable full debugging")},
    { oGPGBinary, "gpg-program", 2 , "@" },
    { oTooltipKludge, "tooltip-kludge", 0, "enable WM workaround"},
{0} };


static GtkWidget *global_clistFile = NULL;
GtkWidget *global_windowMain;
GtkWidget *global_popupMenu;
GtkWidget *global_windowTip;
GList *global_tempWindows;
GtkWidget *global_textTip;
gboolean global_noTips = FALSE;
GpapaAction global_lastCallbackResult;
gchar *global_keyserver;
GList *global_defaultRecipients = NULL;
gchar *global_homeDirectory;
gchar *global_defaultKey;

GtkWidget *gpa_windowMain_new (char *title);

static const char *
my_strusage( int level )
{
    const char *p;
    switch( level ) {
      case 11: p = "gpa";
	break;
      case 13: p = VERSION; break;
      /*case 17: p = PRINTABLE_OS_NAME; break;*/
      case 19: p =
	    _("Please report bugs to <gpa-bugs@gnu.org>.\n");
	break;
      case 1:
      case 40:	p =
	    _("Usage: gpa [options] (-h for help)");
	break;
      case 41:	p =
	    _("Syntax: gpa [options]\n"
	      "Graphical frontend to GnuPG\n");
	break;

      default:	p = NULL;
    }
    return p;
}



static void
i18n_init (void)
{
  #ifdef USE_SIMPLE_GETTEXT
    set_gettext_file( PACKAGE );
  #else
  #ifdef ENABLE_NLS
    #ifdef HAVE_LC_MESSAGES
       setlocale( LC_TIME, "" );
       setlocale( LC_MESSAGES, "" );
    #else
       setlocale( LC_ALL, "" );
    #endif
    bindtextdomain( PACKAGE, GPA_LOCALEDIR );
    textdomain( PACKAGE );
  #endif
  #endif
}

int
main (int argc, char **argv )
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

    set_strusage( my_strusage );
    /*log_set_name ("gpa"); notyet implemented in logging.c */
    i18n_init ();
    gtk_init (&argc, &argv);

    opt.homedir = getenv("GNUPGHOME");
    if( !opt.homedir || !*opt.homedir ) {
      #ifdef HAVE_DRIVE_LETTERS
	opt.homedir = "c:/gnupg";
      #else
	opt.homedir = "~/.gnupg";
      #endif
    }

    /* check whether we have a config file on the commandline */
    orig_argc = argc;
    orig_argv = argv;
    pargs.argc = &argc;
    pargs.argv = &argv;
    pargs.flags= 1|(1<<6);  /* do not remove the args, ignore version */
    while( arg_parse( &pargs, opts) ) {
	if( pargs.r_opt == oDebug || pargs.r_opt == oDebugAll )
	    parse_debug++;
	else if( pargs.r_opt == oOptions ) {
	    /* yes there is one, so we do not try the default one, but
	     * read the option file when it is encountered at the commandline
	     */
	    default_config = 0;
	}
	else if( pargs.r_opt == oNoOptions )
	    default_config = 0; /* --no-options */
	else if( pargs.r_opt == oHomedir )
	    opt.homedir = pargs.r.ret_str;
    }

    if( default_config )
	configname = make_filename(opt.homedir, "gpa.conf", NULL );


    argc = orig_argc;
    argv = orig_argv;
    pargs.argc = &argc;
    pargs.argv = &argv;
    pargs.flags=  1;  /* do not remove the args */
  next_pass:
    if( configname ) {
	configlineno = 0;
	configfp = fopen( configname, "r" );
	if( !configfp ) {
	    if( default_config ) {
		if( parse_debug )
		    log_info(_("NOTE: no default option file `%s'\n"),
							    configname );
	    }
	    else {
		log_error(_("option file `%s': %s\n"),
				    configname, strerror(errno) );
		exit(2);
	    }
	    free(configname); configname = NULL;
	}
	if( parse_debug && configname )
	    log_info(_("reading options from `%s'\n"), configname );
	default_config = 0;
    }

    while( optfile_parse( configfp, configname, &configlineno,
						&pargs, opts) ) {
	switch( pargs.r_opt ) {
	  case oQuiet: opt.quiet = 1; break;
	  case oVerbose: opt.verbose++; break;

	  case oDebug: opt.debug |= pargs.r.ret_ulong; break;
	  case oDebugAll: opt.debug = ~0; break;

	  case oOptions:
	    /* config files may not be nested (silently ignore them) */
	    if( !configfp ) {
		free(configname);
		configname = xstrdup(pargs.r.ret_str);
		goto next_pass;
	    }
	    break;
	  case oNoGreeting: nogreeting = 1; break;
	  case oNoVerbose: opt.verbose = 0; break;
	  case oNoOptions: break; /* no-options */
	  case oHomedir: opt.homedir = pargs.r.ret_str; break;
	  case oGPGBinary: gpg_program = pargs.r.ret_str;  break;


	  case oTooltipKludge: opt.tooltip_kludge = 1; break;

	  default : pargs.err = configfp? 1:2; break;
	}
    }
    if( configfp ) {
	fclose( configfp );
	configfp = NULL;
	free(configname); configname = NULL;
	goto next_pass;
    }
    free( configname ); configname = NULL;
    if( log_get_errorcount(0) )
	exit(2);
    if( nogreeting )
	greeting = 0;

    if( greeting ) {
	fprintf(stderr, "%s %s; %s\n",
			strusage(11), strusage(13), strusage(14) );
	fprintf(stderr, "%s\n", strusage(15) );
    }
  #ifdef IS_DEVELOPMENT_VERSION
    log_info("NOTE: this is a development version!\n");
  #endif


    /* fixme: read from options and add at least one default */
    opt.keyserver_names = xcalloc (3, sizeof *opt.keyserver_names );
    opt.keyserver_names[0] = "blackhole.pca.dfn.de";
    opt.keyserver_names[1] = "horowitz.surfnet.nl";

    gpapa_init ( gpg_program );

    global_windowMain = gpa_windowMain_new (_("GNU Privacy Assistant"));
    gtk_signal_connect (GTK_OBJECT (global_windowMain), "delete_event",
			GTK_SIGNAL_FUNC (file_quit), NULL);
    gtk_widget_show_all (global_windowMain);
    gtk_main ();
    return 0;
}

gchar *
getStringForSigValidity (GpapaSigValidity sigValidity)
{
  return writtenSigValidity[sigValidity];
}				/* getStringForSigValidity */

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
gpa_switch_tips (void)
{
  if (global_noTips == TRUE)
    global_noTips = FALSE;
  else
    global_noTips = TRUE;
}				/* gpa_switch_tips */

void
gpa_windowTip_init (void)
{
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *vboxTip;
  GtkWidget *vboxContents;
  GtkWidget *labelJfdContents;
  GtkWidget *labelContents;
  GtkWidget *hboxContents;
  GtkWidget *textContents;
  GtkWidget *vscrollbarContents;
  GtkWidget *hboxTip;
  GtkWidget *checkerNomore;
  GtkWidget *buttonClose;
/* commands */
  global_windowTip = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (global_windowTip), _("GPA Tip"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (global_windowTip), accelGroup);
  vboxTip = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxTip), 5);
  vboxContents = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxContents), 5);
  labelContents = gtk_label_new (_(""));
  labelJfdContents =
    gpa_widget_hjustified_new (labelContents, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxContents), labelJfdContents, FALSE, FALSE,
		      0);
  hboxContents = gtk_hbox_new (FALSE, 0);
  textContents = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (textContents), FALSE);
  gpa_connect_by_accelerator (GTK_LABEL (labelContents), textContents,
			      accelGroup, _("_Tip:"));
  global_textTip = textContents;
  gtk_box_pack_start (GTK_BOX (hboxContents), textContents, TRUE, TRUE, 0);
  vscrollbarContents = gtk_vscrollbar_new (GTK_TEXT (textContents)->vadj);
  gtk_box_pack_start (GTK_BOX (hboxContents), vscrollbarContents, FALSE,
		      FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxContents), hboxContents, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxTip), vboxContents, TRUE, TRUE, 0);
  hboxTip = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxTip), 5);
  buttonClose = gpa_button_new (accelGroup, _("   _Close   "));
  gtk_signal_connect_object (GTK_OBJECT (buttonClose), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_hide),
			     (gpointer) global_windowTip);
  gtk_widget_add_accelerator (buttonClose, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  gtk_box_pack_end (GTK_BOX (hboxTip), buttonClose, FALSE, FALSE, 0);
  checkerNomore =
    gpa_check_button_new (accelGroup, _("_No more tips, please"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkerNomore),
				global_noTips);
  gtk_signal_connect (GTK_OBJECT (checkerNomore), "clicked",
		      GTK_SIGNAL_FUNC (gpa_switch_tips), NULL);
  gtk_box_pack_end (GTK_BOX (hboxTip), checkerNomore, FALSE, FALSE, 10);
  gtk_box_pack_start (GTK_BOX (vboxTip), hboxTip, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (global_windowTip), vboxTip);
}				/* gpa_windowTip_init */

void
gpa_windowTip_show (gchar * text)
{
  if (!global_noTips)
    {
      gtk_editable_delete_text (GTK_EDITABLE (global_textTip), 0, -1);
      gtk_text_insert (GTK_TEXT (global_textTip), NULL,
		       &global_textTip->style->black, NULL, text, -1);
      gtk_widget_show_all (global_windowTip);
    }				/* if */
}				/* gpa_windowTip_show */

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
    getStringForSigValidity (gpapa_signature_get_validity
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
      gpa_window_error (_("No files selected to remove from recipients list"),
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
/* var */
  gpointer *localParam;
  GpapaPublicKey *key;
  GtkWidget *clistRecipients;
  GtkWidget *windowEncrypt;
  gchar *contentsRecipients[2];
/* commands */
  localParam = (gpointer *) userData;
  clistRecipients = (GtkWidget *) localParam[0];
  windowEncrypt = (GtkWidget *) localParam[1];
  key = gpapa_get_public_key_by_ID (
				    (gchar *) data, gpa_callback,
				    windowEncrypt);
  contentsRecipients[0] =
    gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, windowEncrypt);
  contentsRecipients[1] =
    gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback, windowEncrypt);
  gtk_clist_append (GTK_CLIST (clistRecipients), contentsRecipients);
}				/* gpa_addRecipient */

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

gint
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  file_quit ();
  return (FALSE);
}				/* delete_event */

GtkWidget *
gpa_menubar_new (GtkWidget * window)
{
  GtkItemFactory *itemFactory;
  GtkItemFactoryEntry menuItem[] = {
    {_("/_File"), NULL, NULL, 0, "<Branch>"},
    {_("/File/_Open"), "<control>O", file_open, 0, NULL},
    {_("/File/S_how Detail"), "<control>H", file_showDetail, 0, NULL},
    {_("/File/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/File/_Sign"), NULL, file_sign, 0, NULL},
    {_("/File/_Encrypt"), NULL, file_encrypt, 0, NULL},
    {_("/File/E_ncrypt as"), NULL, file_encryptAs, 0, NULL},
    {_("/File/_Protect by Password"), NULL, file_protect, 0, NULL},
    {_("/File/P_rotect as"), NULL, file_protectAs, 0, NULL},
    {_("/File/_Decrypt"), NULL, file_decrypt, 0, NULL},
    {_("/File/Decrypt _as"), NULL, file_decryptAs, 0, NULL},
    {_("/File/sep2"), NULL, NULL, 0, "<Separator>"},
    {_("/File/_Close"), NULL, file_close, 0, NULL},
    {_("/File/_Quit"), "<control>Q", file_quit, 0, NULL},

    {_("/_Keys"), NULL, NULL, 0, "<Branch>"},
    {_("/Keys/Open _public Keyring"), "<control>K", keys_openPublic, 0, NULL},
    {_("/Keys/Open _secret Keyring"), NULL, keys_openSecret, 0, NULL},
    {_("/Keys/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/Keys/_Generate Key"), NULL, keys_generateKey, 0, NULL},

    {_("/Keys/Generate _Revocation Certificate"), NULL,
					 keys_generateRevocation, 0, NULL},
    {_("/Keys/_Import Keys"), NULL, keys_import, 0, NULL},
    {_("/Keys/Import _Ownertrust"), NULL, keys_importOwnertrust, 0, NULL},
    {_("/Keys/_Update Trust Database"), NULL, keys_updateTrust, 0, NULL},

    {_("/_Options"), NULL, NULL, 0, "<Branch>"},
    {_("/Options/_Keyserver"), NULL, options_keyserver, 0, NULL},
    {_("/Options/Default _Recipients"), NULL, options_recipients, 0, NULL},
    {_("/Options/_Default Key"), NULL, options_key, 0, NULL},
    {_("/Options/_Home Directory"), NULL, options_homedir, 0, NULL},
    {_("/Options/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/Options/Online _tips"), NULL, options_tips, 0, NULL},
    {_("/Options/_Load Options File"), NULL, options_load, 0, NULL},
    {_("/Options/_Save Options File"), NULL, options_save, 0, NULL},

    {_("/_Help"), NULL, NULL, 0, "<Branch>"},
    {_("/Help/_About"), NULL, help_about, 0, NULL},
    {_("/Help/_License"), NULL, help_license, 0, NULL},
    {_("/Help/_Warranty"), NULL, help_warranty, 0, NULL},
    {_("/Help/_Help"), "F1", help_help, 0, NULL}
  };
  gint menuItems = sizeof (menuItem) / sizeof (menuItem[0]);
  GtkAccelGroup *accelGroup;
  GtkWidget *menubar;

  accelGroup = gtk_accel_group_new ();
  itemFactory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accelGroup);
  gtk_item_factory_create_items (itemFactory, menuItems, menuItem, NULL);
  gtk_window_add_accel_group (GTK_WINDOW (window), accelGroup);
  menubar = gtk_item_factory_get_widget (itemFactory, "<main>");
  return menubar;
}

void
gpa_popupMenu_init (void)
{
  /* var */
  GtkItemFactory *itemFactory;
  GtkItemFactoryEntry menuItem[] = {
    {_("/Show detail"), NULL, file_showDetail, 0, NULL},
    {_("/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/Sign"), NULL, file_sign, 0, NULL},
    {_("/Encrypt"), NULL, file_encrypt, 0, NULL},
    {_("/E_ncrypt as"), NULL, file_encryptAs, 0, NULL},
    {_("/Protect by Password"), NULL, file_protect, 0, NULL},
    {_("/P_rotect as"), NULL, file_protectAs, 0, NULL},
    {_("/Decrypt"), NULL, file_decrypt, 0, NULL},
    {_("/Decrypt _as"), NULL, file_decryptAs, 0, NULL},
    {_("/sep2"), NULL, NULL, 0, "<Separator>"},
    {_("/Close"), NULL, file_close, 0, NULL}
  };
  gint menuItems = sizeof (menuItem) / sizeof (menuItem[0]);
  /* commands */
  itemFactory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", NULL);
  gtk_item_factory_create_items (itemFactory, menuItems, menuItem, NULL);
  global_popupMenu = gtk_item_factory_get_widget (itemFactory, "<main>");
} /* gpa_popupMenu_init */

void
setFileSelected (GtkWidget * clistFile, gint row, gint column,
		 GdkEventButton * event, gboolean selected)
{
  /* var */
  GpapaFile *aFile;
  /* commands */
  aFile = g_list_nth_data (filesOpened, row);
  if (selected)
    {
      if (!g_list_find (filesSelected, aFile))
	filesSelected = g_list_append (filesSelected, aFile);
    }
  else
    {
      if (g_list_find (filesSelected, aFile))
	filesSelected = g_list_remove (filesSelected, aFile);
    }
} /* setFileSelected */

gboolean
evalKeyClistFile (GtkWidget * clistFile, GdkEventKey * event,
		  gpointer userData)
{
  switch (event->keyval)
    {
    case GDK_Delete:
      file_close ();
      break;
    case GDK_Insert:
      file_open ();
      break;
    }
  return (TRUE);
} /* evalKeyClistFile */

gboolean
evalMouseClistFile (GtkWidget * clistFile, GdkEventButton * event,
		    gpointer userData)
{
  /* var */
  gint x, y;
  gint row, column;
  /* commands */
  if (event->button == 3)
    {
      x = event->x;
      y = event->y;
      if (gtk_clist_get_selection_info
	  (GTK_CLIST (clistFile), x, y, &row, &column))
	{
	  if (!(event->state & GDK_SHIFT_MASK ||
		event->state & GDK_CONTROL_MASK))
	    gtk_clist_unselect_all (GTK_CLIST (clistFile));
	  gtk_clist_select_row (GTK_CLIST (clistFile), row, column);
	  gtk_menu_popup (GTK_MENU (global_popupMenu), NULL, NULL, NULL, NULL,
			  3, 0);
	}			/* if */
    }				/* if */
  else if (event->type == GDK_2BUTTON_PRESS)
    file_showDetail ();
  return (TRUE);
}				/* evalMouseClistFile */

GtkWidget *
gpa_windowFile_new (void)
{
  /* var */
  char *clistFileTitle[5] = {
    N_("File"), N_("Status"), N_("Sigs total"), N_("Valid Sigs"),
    N_("Invalid Sigs")
  };
  int i;
  /* objects */
  GtkWidget *windowFile;
  GtkWidget *scrollerFile;
  GtkWidget *clistFile;
  /* commands */
  windowFile = gtk_frame_new (_("Files in work"));
  scrollerFile = gtk_scrolled_window_new (NULL, NULL);
  clistFile = gtk_clist_new_with_titles (5, clistFileTitle);
  gtk_clist_set_column_width (GTK_CLIST (clistFile), 0, 170);
  gtk_clist_set_column_width (GTK_CLIST (clistFile), 1, 100);
  gtk_clist_set_column_justification (GTK_CLIST (clistFile), 1,
				      GTK_JUSTIFY_CENTER);
  for (i = 2; i <= 4; i++)
    {
      gtk_clist_set_column_width (GTK_CLIST (clistFile), i, 100);
      gtk_clist_set_column_justification (GTK_CLIST (clistFile), i,
					  GTK_JUSTIFY_RIGHT);
    }				/* for */
  for (i = 0; i <= 4; i++)
    {
      gtk_clist_set_column_auto_resize (GTK_CLIST (clistFile), i, FALSE);
      gtk_clist_column_title_passive (GTK_CLIST (clistFile), i);
    }				/* for */
  gtk_clist_set_selection_mode (GTK_CLIST (clistFile),
				GTK_SELECTION_EXTENDED);
  gtk_widget_grab_focus (clistFile);
  gtk_signal_connect (GTK_OBJECT (clistFile), "select-row",
		      GTK_SIGNAL_FUNC (setFileSelected), (gpointer) TRUE);
  gtk_signal_connect (GTK_OBJECT (clistFile), "unselect-row",
		      GTK_SIGNAL_FUNC (setFileSelected), (gpointer) FALSE);
  gtk_signal_connect (GTK_OBJECT (clistFile), "key-press-event",
		      GTK_SIGNAL_FUNC (evalKeyClistFile), NULL);
  gtk_signal_connect (GTK_OBJECT (clistFile), "button-press-event",
		      GTK_SIGNAL_FUNC (evalMouseClistFile), NULL);
  global_clistFile = clistFile;
  gtk_container_add (GTK_CONTAINER (scrollerFile), clistFile);
  gtk_container_add (GTK_CONTAINER (windowFile), scrollerFile);
  return (windowFile);
} /* gpa_windowFile_new */

GtkWidget *
gpa_file_toolbar_new ( GtkWidget * window ) {
	GtkWidget *toolbar, *iconw;
	GtkStyle *style;
	GdkPixmap *icon;
	GdkBitmap *mask;

	toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
	style = gtk_widget_get_style(window);
	icon = gdk_pixmap_create_from_xpm_d(window->window,&mask,
		&style->bg[GTK_STATE_NORMAL],(gchar **)openfile);
	/* Open */
	iconw = gtk_pixmap_new(icon, mask);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Open"),
		_("Open a file"), _("open file"), iconw,
		GTK_SIGNAL_FUNC(file_open), NULL);
	/* Sign */
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Sign"),
		_("Sign the selected file"), _("sign file"), NULL,
		GTK_SIGNAL_FUNC(file_sign), NULL);
	/* Encrypt */
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Encrypt"),
		_("Encrypt the selected file"), _("encrypt file"), NULL,
	GTK_SIGNAL_FUNC(file_encrypt), NULL);
	/* Decrypt */
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Decrypt"),
		_("Decrypt the selected file"), _("decrypt file"), NULL,
	GTK_SIGNAL_FUNC(file_decrypt), NULL);
	/* Protect */
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Protect"),
		_("Protect the selected file as"), _("protect file"), NULL,
	GTK_SIGNAL_FUNC(file_protect), NULL);
	/* Public keyring */
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Public Keyring"),
		_("Open public keyring"), _("open public keyring"), NULL,
	GTK_SIGNAL_FUNC(keys_openPublic), NULL);
	/* Secret keyring */
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Secret Keyring"),
		_("Open secret keyring"), _("open secret keyring"), NULL,
	GTK_SIGNAL_FUNC(keys_openSecret), NULL);
	/* Help */
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Help"),
		_("Understanding the GNU Privacy Assistant"), _("help"), NULL,
	GTK_SIGNAL_FUNC(help_help), NULL);

	return toolbar;
} /* gpa_file_toolbar_new */

GtkWidget *
gpa_windowMain_new (char *title)
{
/* objects */
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GtkWidget *fileBox;
  GtkWidget *windowFile;
  GtkWidget *toolbar;
/* commands */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_widget_set_usize (window, 640, 480);
  vbox = gtk_vbox_new (FALSE, 0);
  menubar = gpa_menubar_new (window);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);

  /* set up the toolbar */
  toolbar = gpa_file_toolbar_new(window);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER (vbox), toolbar);

  fileBox = gtk_hbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (fileBox), 5);
  windowFile = gpa_windowFile_new ();
  gtk_box_pack_start (GTK_BOX (fileBox), windowFile, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), fileBox, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gpa_popupMenu_init ();
  gpa_windowTip_init ();
  gpa_fileOpenSelect_init (_("Open a file"));
  gpa_homeDirSelect_init (_("Set home directory"));
  gpa_loadOptionsSelect_init (_("Load options file"));
  gpa_saveOptionsSelect_init (_("Save options file"));
  assert( opt.keyserver_names[0] );
  global_keyserver = opt.keyserver_names[0];  /* FIXME: bad style */
  global_defaultKey = NULL;
  return (window);
} /* gpa_windowMain_new */

