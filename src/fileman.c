/* fileman.c  -  The GNU Privacy Assistant
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

/*
 *	The file encryption/decryption/sign window
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
#include "gtktools.h"
#include "gpawidgets.h"
#include "siglist.h"
#include "filemenu.h"
#include "optionsmenu.h"
#include "helpmenu.h"
#include "icons.h"
#include "fileman.h"
#include "filesigndlg.h"
#include "encryptdlg.h"
#include "passphrasedlg.h"

struct _GPAFileManager {
  GtkWidget *window;
  GtkCList *clist_files;
};
typedef struct _GPAFileManager GPAFileManager;

/*
 *	File manager methods
 */

static void
fileman_destroy (gpointer param)
{
  GPAFileManager * fileman = param;

  free (fileman);
}

/*
 * Count the sigs associated with a GpapaFile
 */
struct _CountSigsParam {
  gint valid_sigs;
  gint invalid_sigs;
  GtkWidget * window;
};
typedef struct _CountSigsParam CountSigsParam;

static void
count_sigs (gpointer data, gpointer user_data)
{
  GpapaSignature *signature = data;
  CountSigsParam *count = user_data;

  switch (gpapa_signature_get_validity (signature, gpa_callback,
					count->window))
    {
    case GPAPA_SIG_VALID:
      count->valid_sigs++;
      break;
    case GPAPA_SIG_INVALID:
      count->invalid_sigs++;
      break;
    default:
      break;
    } /* switch */
} /* count_sigs */

/*
 * Functions to manage GpapaFiles attached to CList rows
 */

struct _GPAFileInfo {
  GpapaFile *file;
  GtkWidget *window;
};
typedef struct _GPAFileInfo GPAFileInfo;

static void
free_file_info (gpointer param)
{
  GPAFileInfo *info = param;

  gpapa_file_release (info->file, gpa_callback, info->window);
  free (info);
}

static void
attach_file (GPAFileManager *fileman, GtkCList *clist, gint row,
	     GpapaFile *file)
{
  GPAFileInfo *info = xmalloc (sizeof (GPAFileInfo));
  info->file = file;
  info->window = fileman->window;

  gtk_clist_set_row_data_full (clist, row, info, free_file_info);
}

static GpapaFile *
get_file (GtkCList * clist, gint row)
{
  GPAFileInfo *info = gtk_clist_get_row_data (clist, row);
  return info->file;
}


/* return the currently selected files as a new list of GPAFileInfo
 * structs. The list has to be freed by the caller, but the file info
 * instances are still managed by the CList
 */
static GList *
get_selected_files (GtkCList *clist)
{
  GList * files = NULL;
  GList * selection = clist->selection;
  gint row;

  while (selection)
    {
      row = GPOINTER_TO_INT (selection->data);
      files = g_list_prepend (files, get_file (clist, row));
      selection = g_list_next (selection);
    }

  return files;
}
					       
  

/* Add file filename to the clist. Return the index of the new row */
static gint
add_file (GPAFileManager * fileman, gchar * filename)
{
  gchar *entries[5];
  GList *signatures;
  gchar str_num_sigs[50];
  gchar str_valid_sigs[50];
  gchar str_invalid_sigs[50];
  GpapaFile *file;
  CountSigsParam count_param;
  gint row;

  file = gpapa_file_new (filename, gpa_callback, fileman->window);
  signatures = gpapa_file_get_signatures (file, gpa_callback, fileman->window);
  entries[0] = gpapa_file_get_name (file, gpa_callback, fileman->window);
  for (row = 0; row < fileman->clist_files->rows; row++)
    {
      if (!strcmp (entries[0],
		   gpapa_file_get_identifier (get_file (fileman->clist_files,
							row),
					      gpa_callback,
					      fileman->window)))
	{
	  gpa_window_error (_("The file is already open."), fileman->window);
	  return -1;
	} /* if */
    } /* for */
  entries[1]
    = gpa_file_status_string (gpapa_file_get_status (file, gpa_callback,
						     fileman->window));
  sprintf (str_num_sigs, "%d", g_list_length (signatures));
  entries[2] = str_num_sigs;

  count_param.valid_sigs = 0;
  count_param.invalid_sigs = 0;
  count_param.window = fileman->window;
  g_list_foreach (signatures, count_sigs, &count_param);

  sprintf (str_valid_sigs, "%d", count_param.valid_sigs);
  entries[3] = str_valid_sigs;
  if (count_param.invalid_sigs)
    {
      sprintf (str_invalid_sigs, "%d", count_param.invalid_sigs);
      entries[4] = str_invalid_sigs;
    } /* if */
  else
    entries[4] = "";

  row = gtk_clist_append (fileman->clist_files, entries);
  attach_file (fileman, fileman->clist_files, row, file);

  return row;
} /* add_file */


/*
 *	Callbacks
 */

/*
 *  File/Open
 */
static void
open_file (gpointer param)
{
  GPAFileManager * fileman = param;
  gchar * filename;
  gint row;

  filename = gpa_get_load_file_name (fileman->window, _("Open File"), NULL);
  if (filename)
    {
      row = add_file (fileman, filename);
      if (row >= 0)
	gtk_clist_select_row (fileman->clist_files, row, 0);
      free (filename);
    }
}

/*
 *  File/Show Detail
 */


static void
close_file_detail (gpointer param)
{
  gtk_main_quit ();
}

static gboolean
detail_delete_event (GtkWidget *widget, GdkEvent *event, gpointer param)
{
  close_file_detail (param);
  return TRUE;
}

static void
show_file_detail (gpointer param)
{
  GPAFileManager *fileman = param;
  GpapaFile *file;
  GtkAccelGroup *accelGroup;
  gchar *contentsFilename;
  GList *signatures;
  GtkWidget *window;
  GtkWidget *vboxDetail;
  GtkWidget *tableTop;
  GtkWidget *labelFilename;
  GtkWidget *entryFilename;
  GtkWidget *labelEncrypted;
  GtkWidget *entryEncrypted;
  GtkWidget *vboxSignatures;
  GtkWidget *labelSignatures;
  GtkWidget *scrollerSignatures;
  GtkWidget *clistSignatures;
  GtkWidget *hButtonBoxDetail;
  GtkWidget *buttonClose;

  if (!fileman->clist_files->selection)
    {
      gpa_window_error (_("No file selected for detail view"),
			fileman->window);
      return;
    } /* if */

  file = get_file (fileman->clist_files,
		   GPOINTER_TO_INT (fileman->clist_files->selection->data));
  window = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (window), _("Detailed file view"));
  gtk_signal_connect_object (GTK_OBJECT (window), "delete_event",
			     GTK_SIGNAL_FUNC (detail_delete_event),
			     (gpointer)fileman);
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accelGroup);

  vboxDetail = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxDetail), 5);
  tableTop = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (tableTop), 5);
  labelFilename = gtk_label_new (_("Filename: "));
  gtk_misc_set_alignment (GTK_MISC (labelFilename), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (tableTop), labelFilename, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  entryFilename = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (entryFilename), FALSE);
  contentsFilename = gpapa_file_get_name (file, gpa_callback, fileman->window);
  gtk_entry_set_text (GTK_ENTRY (entryFilename), contentsFilename);
  gtk_widget_set_usize (entryFilename,
			gdk_string_width (entryFilename->style->font,
					  contentsFilename) +
			gdk_string_width (entryFilename->style->font,
					  "  ") +
			entryFilename->style->klass->xthickness, 0);
  gtk_table_attach (GTK_TABLE (tableTop), entryFilename, 1, 2, 0, 1, GTK_FILL,
		    GTK_FILL, 0, 0);

  labelEncrypted = gtk_label_new (_("Status:"));
  gtk_misc_set_alignment (GTK_MISC (labelEncrypted), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (tableTop), labelEncrypted, 0, 1, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  entryEncrypted = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entryEncrypted),
		      gpa_file_status_string (gpapa_file_get_status
					      (file, gpa_callback,
					       global_windowMain)));
  gtk_editable_set_editable (GTK_EDITABLE (entryEncrypted), FALSE);
  gtk_table_attach (GTK_TABLE (tableTop), entryEncrypted, 1, 2, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);

  gtk_box_pack_start (GTK_BOX (vboxDetail), tableTop, FALSE, FALSE, 0);

  vboxSignatures = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxDetail), vboxSignatures, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxSignatures), 5);

  labelSignatures = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelSignatures), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vboxSignatures), labelSignatures, FALSE, TRUE,
		      0);

  scrollerSignatures = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerSignatures, 350, 200);
  gtk_box_pack_start (GTK_BOX (vboxSignatures), scrollerSignatures, TRUE,
		      TRUE, 0);

  clistSignatures = gpa_siglist_new (fileman->window);
  signatures = gpapa_file_get_signatures (file, gpa_callback, fileman->window);
  gpa_siglist_set_signatures (clistSignatures, signatures, NULL);
  gtk_container_add (GTK_CONTAINER (scrollerSignatures), clistSignatures);
  gpa_connect_by_accelerator (GTK_LABEL (labelSignatures), clistSignatures,
			      accelGroup, _("_Signatures"));

  hButtonBoxDetail = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxDetail),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxDetail), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxDetail), 5);
  buttonClose = gpa_button_cancel_new (accelGroup, _("_Close"),
				       close_file_detail, fileman);
  gtk_container_add (GTK_CONTAINER (hButtonBoxDetail), buttonClose);
  gtk_box_pack_start (GTK_BOX (vboxDetail), hButtonBoxDetail, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vboxDetail);
  gpa_window_show_centered (window, fileman->window);

  gtk_grab_add (window);
  gtk_main ();
  gtk_grab_remove (window);
  gtk_widget_destroy (window);
} /* show_file_detail */


/*
 * Sign Files
 */

static void
sign_files (gpointer param)
{
  GPAFileManager *fileman = param;
  GList * files;
  GList *signed_files, *cur;
  gint row;
  
  files = get_selected_files (fileman->clist_files);
  if (!files)
    return;

  signed_files = gpa_file_sign_dialog_run (fileman->window, files);
  if (signed_files)
    {
      gtk_clist_unselect_all (fileman->clist_files);
      cur = signed_files;
      while (cur)
	{
	  row = add_file (fileman, (gchar*)(cur->data));
	  if (row >= 0)
	    gtk_clist_select_row (fileman->clist_files, row, 0);
	  free (cur->data);
	  cur = g_list_next (cur);
	}
      g_list_free (signed_files);
    }
}

/*
 * Encrypt Files
 */

static void
encrypt_files (gpointer param)
{
  GPAFileManager *fileman = param;
  GList * files;
  GList *encrypted_files, *cur;
  gint row;
  
  files = get_selected_files (fileman->clist_files);
  if (!files)
    return;

  encrypted_files = gpa_file_encrypt_dialog_run (fileman->window, files);
  if (encrypted_files)
    {
      gtk_clist_unselect_all (fileman->clist_files);
      cur = encrypted_files;
      while (cur)
	{
	  row = add_file (fileman, (gchar*)(cur->data));
	  if (row >= 0)
	    gtk_clist_select_row (fileman->clist_files, row, 0);
	  free (cur->data);
	  cur = g_list_next (cur);
	}
      g_list_free (encrypted_files);
    }
}

/*
 * Decrypt Files
 */

static void
decrypt_files (gpointer param)
{
  GPAFileManager *fileman = param;
  GList * files;
  GpapaFile *file;
  gchar *passphrase;
  gchar *filename, *newname, *extension;
  GList *cur;
  gint row;
  
  files = get_selected_files (fileman->clist_files);
  if (!files)
    return;

  passphrase = gpa_passphrase_run_dialog (fileman->window, NULL);
  if (passphrase)
    {
      cur = files;
      while (cur)
	{
	  file = cur->data;
	  filename = gpapa_file_get_identifier (file, gpa_callback,
						fileman->window);
	  extension = filename + strlen (filename) - 4;
	  if (strcmp (extension, ".gpg") == 0
	      || strcmp (extension, ".asc") == 0)
	    {
	      newname = xstrdup (filename);
	      newname[strlen (filename) - 4] = 0;
	    } /* if */
	  else
	    newname = xstrcat2 (filename, ".txt");
	  global_lastCallbackResult = GPAPA_ACTION_NONE;
	  gpapa_file_decrypt (file, newname, passphrase, gpa_callback,
			      fileman->window);
	  if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
	    {
	      row = add_file (fileman, newname);
	      if (row >= 0)
		gtk_clist_select_row (fileman->clist_files, row, 0);
	    }
	  free (newname);
	  cur = g_list_next (cur);
	}
    }
  free (passphrase);
  g_list_free (files);
}


static void
close_window (gpointer param)
{
  GPAFileManager *fileman = param;
  gtk_widget_destroy (fileman->window);
}


/*
 *	Construct the file manager window
 */


static GtkWidget *
fileman_menu_new (GtkWidget * window, GPAFileManager *fileman)
{
  GtkItemFactory *factory;
  GtkItemFactoryEntry file_menu[] = {
    {_("/_File"), NULL, NULL, 0, "<Branch>"},
    {_("/File/_Open"), "<control>O", open_file, 0, NULL},
    {_("/File/S_how Detail"), "<control>H", show_file_detail, 0, NULL},
    {_("/File/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/File/_Sign"), NULL, sign_files, 0, NULL},
    {_("/File/_Encrypt"), NULL, encrypt_files, 0, NULL},
    /*    {_("/File/E_ncrypt as"), NULL, file_encryptAs, 0, NULL},
    {_("/File/_Protect by Password"), NULL, file_protect, 0, NULL},
    {_("/File/P_rotect as"), NULL, file_protectAs, 0, NULL},
    */
    {_("/File/_Decrypt"), NULL, decrypt_files, 0, NULL},
    /*
    {_("/File/Decrypt _as"), NULL, file_decryptAs, 0, NULL},
    */
    {_("/File/sep2"), NULL, NULL, 0, "<Separator>"},
    {_("/File/_Close"), NULL, close_window, 0, NULL},
    {_("/File/_Quit"), "<control>Q", file_quit, 0, NULL},
  };
  GtkItemFactoryEntry windows_menu[] = {
    {_("/_Windows"), NULL, NULL, 0, "<Branch>"},
    {_("/Windows/_Filemanager"), NULL, gpa_open_filemanager, 0, NULL},
    {_("/Windows/_Keyring Editor"), NULL, gpa_open_keyring_editor, 0, NULL},
  };
  GtkAccelGroup *accel_group;

  accel_group = gtk_accel_group_new ();
  factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
  gtk_item_factory_create_items (factory,
				 sizeof (file_menu) / sizeof (file_menu[0]),
				 file_menu, fileman);
  gpa_options_menu_add_to_factory (factory, window);
  gtk_item_factory_create_items (factory,
				 sizeof(windows_menu) /sizeof(windows_menu[0]),
				 windows_menu, fileman);
  gpa_help_menu_add_to_factory (factory, window);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  return gtk_item_factory_get_widget (factory, "<main>");
}

static GtkWidget *
gpa_window_file_new (GPAFileManager * fileman)
{
  char *clistFileTitle[5] = {
    _("File"), _("Status"), _("Sigs total"), _("Valid Sigs"), _("Invalid Sigs")
  };
  int i;

  GtkWidget *windowFile;
  GtkWidget *scrollerFile;
  GtkWidget *clistFile;

  windowFile = gtk_frame_new (_("Files in work"));
  scrollerFile = gtk_scrolled_window_new (NULL, NULL);
  clistFile = gtk_clist_new_with_titles (5, clistFileTitle);
  fileman->clist_files = GTK_CLIST (clistFile);
  gtk_clist_set_column_width (GTK_CLIST (clistFile), 0, 170);
  gtk_clist_set_column_width (GTK_CLIST (clistFile), 1, 100);
  gtk_clist_set_column_justification (GTK_CLIST (clistFile), 1,
				      GTK_JUSTIFY_CENTER);
  for (i = 2; i <= 4; i++)
    {
      gtk_clist_set_column_width (GTK_CLIST (clistFile), i, 100);
      gtk_clist_set_column_justification (GTK_CLIST (clistFile), i,
					  GTK_JUSTIFY_RIGHT);
    } /* for */
  for (i = 0; i <= 4; i++)
    {
      gtk_clist_set_column_auto_resize (GTK_CLIST (clistFile), i, FALSE);
      gtk_clist_column_title_passive (GTK_CLIST (clistFile), i);
    } /* for */
  gtk_clist_set_selection_mode (GTK_CLIST (clistFile),
				GTK_SELECTION_EXTENDED);
  gtk_widget_grab_focus (clistFile);
  gtk_container_add (GTK_CONTAINER (scrollerFile), clistFile);
  gtk_container_add (GTK_CONTAINER (windowFile), scrollerFile);

  return windowFile;
} /* gpa_window_file_new */


static void
toolbar_file_open (GtkWidget *widget, gpointer param)
{
  open_file (param);
}

static void
toolbar_file_sign (GtkWidget *widget, gpointer param)
{
  sign_files (param);
}

static void
toolbar_file_encrypt (GtkWidget *widget, gpointer param)
{
  encrypt_files (param);
}

static void
toolbar_file_decrypt (GtkWidget *widget, gpointer param)
{
  decrypt_files (param);
}



static GtkWidget *
gpa_fileman_toolbar_new (GtkWidget * window, GPAFileManager *fileman)
{
  GtkWidget *toolbar, *icon;

  toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  
  /* Open */
  if ((icon = gpa_create_icon_widget (window, "openfile")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Open"),
                            _("Open a file"), _("open file"), icon,
                            GTK_SIGNAL_FUNC (toolbar_file_open), fileman);
  /* Sign */
  if ((icon = gpa_create_icon_widget (window, "sign")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Sign"),
			     _("Sign the selected file"), _("sign file"), icon,
			     GTK_SIGNAL_FUNC (toolbar_file_sign), fileman);
  /* Encrypt */
  if ((icon = gpa_create_icon_widget (window, "encrypt")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Encrypt"),
			     _("Encrypt the selected file"), _("encrypt file"),
			     icon, GTK_SIGNAL_FUNC (toolbar_file_encrypt),
			     fileman);
  /* Decrypt */
  if ((icon = gpa_create_icon_widget (window, "decrypt")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _(" Decrypt "),
			     _("Decrypt the selected file"), _("decrypt file"),
			     icon, GTK_SIGNAL_FUNC (toolbar_file_decrypt),
			     fileman);
  /* Help */
  if ((icon = gpa_create_icon_widget (window, "help")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Help"),
			     _("Understanding the GNU Privacy Assistant"),
			     _("help"), icon,
			     GTK_SIGNAL_FUNC (help_help), NULL);

  return toolbar;
} 

GtkWidget *
gpa_fileman_new ()
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GtkWidget *fileBox;
  GtkWidget *windowFile;
  GtkWidget *toolbar;
  GPAFileManager * fileman;

  fileman = xmalloc (sizeof(GPAFileManager));

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window),
			_("GNU Privacy Assistant - File Manager"));
  gtk_widget_set_usize (window, 640, 480);
  gtk_object_set_data_full (GTK_OBJECT (window), "user_data", fileman,
			    fileman_destroy);
  fileman->window = window;
  /* Realize the window so that we can create pixmaps without warnings */
  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  menubar = fileman_menu_new (window, fileman);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);

  /* set up the toolbar */
  toolbar = gpa_fileman_toolbar_new(window, fileman);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);
  
  fileBox = gtk_hbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (fileBox), 5);
  windowFile = gpa_window_file_new (fileman);
  gtk_box_pack_start (GTK_BOX (fileBox), windowFile, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), fileBox, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  /*  gpa_popupMenu_init ();
*/

  return window;
} /* gpa_fileman_new */

