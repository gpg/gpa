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

#include "gpa.h"
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

#include "gpapastrings.h"

#include "gtktools.h"
#include "gpawidgets.h"
#include "siglist.h"
#include "helpmenu.h"
#include "icons.h"
#include "fileman.h"
#include "filesigndlg.h"
#include "verifydlg.h"

#include "gpafiledecryptop.h"
#include "gpafileencryptop.h"
#include "gpafilesignop.h"

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

  g_free (fileman);
}

/* Return the currently selected files as a new list of filenames
 * structs. The list has to be freed by the caller, but the texts themselves
 * are still managed by the CList
 */
static GList *
get_selected_files (GtkCList *clist)
{
  GList *files = NULL;
  GList *selection = clist->selection;
  gint row;
  gchar *filename;

  while (selection)
    {
      row = GPOINTER_TO_INT (selection->data);
      gtk_clist_get_text (clist, row, 0, &filename);
      files = g_list_append (files, filename);
      selection = g_list_next (selection);
    }

  return files;
}


/* Add file filename to the clist. Return the index of the new row */
static gint
add_file (GPAFileManager *fileman, const gchar *filename)
{
  gchar *entries[1];
  gchar *tmp;
  gint row;

  for (row = 0; row < fileman->clist_files->rows; row++)
    {
      gtk_clist_get_text (fileman->clist_files, row, 0, &tmp);
      if (g_str_equal (filename, tmp))
        {
	  return -1;
	}
    }
  entries[0] = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
  /* FIXME: Add the file status when/if gpgme supports it */

  row = gtk_clist_append (fileman->clist_files, entries);

  return row;
}

/* Add a file created by an operation to the list */
static void
file_created_cb (GpaFileOperation *op, const gchar *filename, gpointer data)
{
  GPAFileManager *fileman = data;
  
  add_file (fileman, filename);  
}

/* Do whatever is required with a file operation, to ensure proper clean up */
static void
register_operation (GPAFileManager *fileman, GpaFileOperation *op)
{
  g_signal_connect (G_OBJECT (op), "created_file",
		    G_CALLBACK (file_created_cb), fileman);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (gpa_operation_destroy), NULL);
}

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
        {
          gtk_clist_select_row (fileman->clist_files, row, 0);
        }
      else
        {
          gpa_window_error (_("The file is already open."), fileman->window);
        }
      g_free (filename);
    }
}

/*
 *  Verify Signed Files
 */


static void
verify_files (gpointer param)
{
  GPAFileManager *fileman = param;
  GList * files;
  
  files = get_selected_files (fileman->clist_files);
  if (!files)
    return;

  gpa_file_verify_dialog_run (fileman->window, files);
}


/*
 * Sign Files
 */

static void
sign_files (gpointer param)
{
  GPAFileManager *fileman = param;
  GList * files;
  GpaFileSignOperation *op;

  files = get_selected_files (fileman->clist_files);
  if (!files)
    return;

  op = gpa_file_sign_operation_new (gpa_options, fileman->window, files);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}

/*
 * Encrypt Files
 */

static void
encrypt_files (gpointer param)
{
  GPAFileManager *fileman = param;
  GList * files;
  GpaFileEncryptOperation *op;

  files = get_selected_files (fileman->clist_files);
  if (!files)
    return;

  op = gpa_file_encrypt_operation_new (gpa_options, fileman->window, files);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}

/*
 * Decrypt Files
 */

static void
decrypt_files (gpointer param)
{
  GPAFileManager *fileman = param;
  GList * files;
  GpaFileDecryptOperation *op;

  files = get_selected_files (fileman->clist_files);
  if (!files)
    return;

  op = gpa_file_decrypt_operation_new (gpa_options, fileman->window, files);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}


static void
close_window (gpointer param)
{
  GPAFileManager *fileman = param;
  gtk_widget_destroy (fileman->window);
}

static void
fileman_select_all (gpointer param)
{
  GPAFileManager *fileman = param;
  
  gtk_clist_select_all (GTK_CLIST(fileman->clist_files));
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
    {_("/File/_Open"), NULL, open_file, 0, "<StockItem>", GTK_STOCK_OPEN},
    {_("/File/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/File/_Sign"), NULL, sign_files, 0, NULL},
    {_("/File/_Verify"), "<control>P", verify_files, 0, NULL},
    {_("/File/_Encrypt"), NULL, encrypt_files, 0, NULL},
    {_("/File/_Decrypt"), NULL, decrypt_files, 0, NULL},
    {_("/File/sep2"), NULL, NULL, 0, "<Separator>"},
    {_("/File/_Close"), NULL, close_window, 0, "<StockItem>", GTK_STOCK_CLOSE},
    {_("/File/_Quit"), NULL, gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT},
  };
  GtkItemFactoryEntry edit_menu[] = {
    {_("/_Edit"), NULL, NULL, 0, "<Branch>"},
    {_("/Edit/Select _All"), "<control>A", fileman_select_all, 0, NULL},
    {_("/Edit/sep2"), NULL, NULL, 0, "<Separator>"},
    {_("/Edit/Pr_eferences..."), NULL, gpa_open_settings_dialog, 0,
     "<StockItem>", GTK_STOCK_PREFERENCES},
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
  gtk_item_factory_create_items (factory,
				 sizeof (edit_menu) / sizeof (edit_menu[0]),
				 edit_menu, fileman);
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
  char *clistFileTitle[] = {
    _("File")
  };
  int i;

  GtkWidget *windowFile;
  GtkWidget *scrollerFile;
  GtkWidget *clistFile;

  windowFile = gtk_frame_new (_("Files in work"));
  scrollerFile = gtk_scrolled_window_new (NULL, NULL);
  clistFile = gtk_clist_new_with_titles (1, clistFileTitle);
  fileman->clist_files = GTK_CLIST (clistFile);
  gtk_clist_set_column_justification (GTK_CLIST (clistFile), 1,
				      GTK_JUSTIFY_CENTER);
  for (i = 0; i <= 1; i++)
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
toolbar_file_verify (GtkWidget *widget, gpointer param)
{
  verify_files (param);
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

#if 0
static void
toolbar_preferences (GtkWidget *widget, gpointer param)
{
  gpa_open_settings_dialog ();
}
#endif

static GtkWidget *
gpa_fileman_toolbar_new (GtkWidget * window, GPAFileManager *fileman)
{
  GtkWidget *toolbar, *icon;

#ifdef __NEW_GTK__
  toolbar = gtk_toolbar_new ();
#else
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
#endif
  
  /* Open */
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_OPEN,
                            _("Open a file"), _("open file"),
                            GTK_SIGNAL_FUNC (toolbar_file_open),
                            fileman, -1);
  /* Sign */
  if ((icon = gpa_create_icon_widget (window, "sign")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Sign"),
			     _("Sign the selected file"), _("sign file"), icon,
			     GTK_SIGNAL_FUNC (toolbar_file_sign), fileman);
  /* Verify */
  if ((icon = gpa_create_icon_widget (window, "verify")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Verify"),
			     _("Check signatures of selected file"), _("verify file"), icon,
			     GTK_SIGNAL_FUNC (toolbar_file_verify), fileman);
  /* Encrypt */
  if ((icon = gpa_create_icon_widget (window, "encrypt")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Encrypt"),
			     _("Encrypt the selected file"), _("encrypt file"),
			     icon, GTK_SIGNAL_FUNC (toolbar_file_encrypt),
			     fileman);
  /* Decrypt */
  if ((icon = gpa_create_icon_widget (window, "decrypt")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Decrypt"),
			     _("Decrypt the selected file"), _("decrypt file"),
			     icon, GTK_SIGNAL_FUNC (toolbar_file_decrypt),
			     fileman);

#if 0
  /* Disabled for now. The long label causes the toolbar to grow too much.
   * See http://bugzilla.gnome.org/show_bug.cgi?id=75086
   */
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
  
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), 
                            GTK_STOCK_PREFERENCES,
                            _("Open the Preferences dialog"),
                            _("preferences"),
                            GTK_SIGNAL_FUNC (toolbar_preferences),
                            fileman, -1);
#endif

#if 0  /* FIXME: Help is not available yet. :-( */
  /* Help */
  if ((icon = gpa_create_icon_widget (window, "help")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Help"),
			     _("Understanding the GNU Privacy Assistant"),
			     _("help"), icon,
			     GTK_SIGNAL_FUNC (help_help), NULL);
#endif

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

  fileman = (GPAFileManager*) g_malloc (sizeof(GPAFileManager));

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

