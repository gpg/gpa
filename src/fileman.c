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

#include "gpafiledecryptop.h"
#include "gpafileencryptop.h"
#include "gpafilesignop.h"
#include "gpafileverifyop.h"

static GpaFileManager *instance = NULL;

typedef enum
{
  FILE_NAME_COLUMN,
  FILE_N_COLUMNS
} FileColumn;


/*
 * GtkWidget boilerplate.
 */

static GObject*
gpa_file_manager_constructor (GType type,
			      guint n_construct_properties,
			      GObjectConstructParam *construct_properties);

static GObjectClass *parent_class = NULL;

static void
gpa_file_manager_finalize (GObject *object)
{  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_file_manager_init (GpaFileManager *fileman)
{
  fileman->selection_sensitive_widgets = NULL;
}

static void
gpa_file_manager_class_init (GpaFileManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->constructor = gpa_file_manager_constructor;
  object_class->finalize = gpa_file_manager_finalize;
}

GType
gpa_file_manager_get_type (void)
{
  static GType fileman_type = 0;
  
  if (!fileman_type)
    {
      static const GTypeInfo fileman_info =
	{
	  sizeof (GpaFileManagerClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gpa_file_manager_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (GpaFileManager),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) gpa_file_manager_init,
	};
      
      fileman_type = g_type_register_static (GTK_TYPE_WINDOW,
					     "GpaFileManager",
					     &fileman_info, 0);
    }
  
  return fileman_type;
}

/*
 *	File manager methods
 */
 

/* Return the currently selected files as a new list of filenames
 * structs. The list and the texts must be freed by the caller.
 */
static GList *
get_selected_files (GtkWidget *list)
{
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
  GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  GList *selection = gtk_tree_selection_get_selected_rows (select, &model);
  GList *files = NULL;

  while (selection)
    {
      gchar *filename;
      GtkTreeIter iter;

      gtk_tree_model_get_iter (model, &iter, (GtkTreePath*) selection->data);
      gtk_tree_model_get (model, &iter, FILE_NAME_COLUMN, &filename, -1);

      files = g_list_append (files, filename);
      selection = g_list_next (selection);
    }

  /* Free the selection */
  g_list_foreach (selection, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (selection);

  return files;
}


/* Add file filename to the list and select it */
static gboolean
add_file (GpaFileManager *fileman, const gchar *filename)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (fileman->list_files)));
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeSelection *select = gtk_tree_view_get_selection 
    (GTK_TREE_VIEW (fileman->list_files));

  /* Check for repeats */
  path = gtk_tree_path_new_first ();
  gtk_tree_model_get_iter (GTK_TREE_MODEL (store),
			   &iter, path);
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter))
    {
      gchar *tmp;
      gboolean exists;

      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, FILE_NAME_COLUMN,
			  &tmp, -1);
      exists = g_str_equal (filename, tmp);
      g_free (tmp);
      if (exists)
	  return FALSE;
    }
  gtk_tree_path_free (path);

  gtk_list_store_append (store, &iter);
  /* FIXME: Add the file status when/if gpgme supports it */
  gtk_list_store_set (store, &iter,
                      FILE_NAME_COLUMN,
		      g_filename_to_utf8 (filename, -1, NULL, NULL, NULL),
		      -1);

  /* Select the row */
  gtk_tree_selection_select_iter (select, &iter);

  return TRUE;
}

/* Add a file created by an operation to the list */
static void
file_created_cb (GpaFileOperation *op, const gchar *filename, gpointer data)
{
  GpaFileManager *fileman = data;
  
  add_file (fileman, filename);  
}

/* Do whatever is required with a file operation, to ensure proper clean up */
static void
register_operation (GpaFileManager *fileman, GpaFileOperation *op)
{
  g_signal_connect (G_OBJECT (op), "created_file",
		    G_CALLBACK (file_created_cb), fileman);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), NULL);
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
  GpaFileManager * fileman = param;
  gchar * filename;

  filename = gpa_get_load_file_name (GTK_WIDGET (fileman), _("Open File"), NULL);
  if (filename)
    {

      if (!add_file (fileman, filename))
          gpa_window_error (_("The file is already open."),
			    GTK_WIDGET (fileman));
      g_free (filename);
    }
}

static void
close_all_files (gpointer param)
{
  GpaFileManager *fileman = param;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (fileman->list_files)));

  gtk_list_store_clear (store);
}

/*
 *  Verify Signed Files
 */


static void
verify_files (gpointer param)
{
  GpaFileManager *fileman = param;
  GList * files;
  GpaFileVerifyOperation *op;

  files = get_selected_files (fileman->list_files);
  if (!files)
    return;

  op = gpa_file_verify_operation_new (GTK_WIDGET (fileman), files);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}


/*
 * Sign Files
 */

static void
sign_files (gpointer param)
{
  GpaFileManager *fileman = param;
  GList * files;
  GpaFileSignOperation *op;

  files = get_selected_files (fileman->list_files);
  if (!files)
    return;

  op = gpa_file_sign_operation_new (GTK_WIDGET (fileman), files);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}

/*
 * Encrypt Files
 */

static void
encrypt_files (gpointer param)
{
  GpaFileManager *fileman = param;
  GList * files;
  GpaFileEncryptOperation *op;

  files = get_selected_files (fileman->list_files);
  if (!files)
    return;

  op = gpa_file_encrypt_operation_new (GTK_WIDGET (fileman), files);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}

/*
 * Decrypt Files
 */

static void
decrypt_files (gpointer param)
{
  GpaFileManager *fileman = param;
  GList * files;
  GpaFileDecryptOperation *op;

  files = get_selected_files (fileman->list_files);
  if (!files)
    return;

  op = gpa_file_decrypt_operation_new (GTK_WIDGET (fileman), files);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}

static void
close_window (gpointer param)
{
  GpaFileManager *fileman = param;
  gtk_widget_destroy (GTK_WIDGET (fileman));
}

static void
fileman_select_all (gpointer param)
{
  GpaFileManager *fileman = param;
  
  gtk_tree_selection_select_all (gtk_tree_view_get_selection
				 (GTK_TREE_VIEW (fileman->list_files)));
}

/*
 * Toolbar actions.
 */

static void
toolbar_file_open (GtkWidget *widget, gpointer param)
{
  open_file (param);
}

static void
toolbar_close_all (GtkWidget *widget, gpointer param)
{
  close_all_files (param);
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

/*
 * Management of the selection sensitive widgets;
 */

typedef gboolean (*SensitivityFunc)(gpointer);

static
gboolean has_selection (gpointer param)
{
  GpaFileManager *fileman = param;

  GtkTreeSelection *select = gtk_tree_view_get_selection 
    (GTK_TREE_VIEW (fileman->list_files));

  return (gtk_tree_selection_count_selected_rows (select) > 0);
}

/* Add widget to the list of sensitive widgets of editor
 */

static void
add_selection_sensitive_widget (GpaFileManager *fileman,
                                GtkWidget *widget,
                                SensitivityFunc callback)
{
  gtk_object_set_data (GTK_OBJECT (widget), "gpa_sensitivity", callback);
  fileman->selection_sensitive_widgets \
    = g_list_append(fileman->selection_sensitive_widgets, widget);
}

/* Update the sensitivity of the widget data and pass param through to
 * the sensitivity callback. Usable as iterator function in
 * g_list_foreach */
static void
update_selection_sensitive_widget (gpointer data, gpointer param)
{
  SensitivityFunc func;

  func = gtk_object_get_data (GTK_OBJECT (data), "gpa_sensitivity");
  gtk_widget_set_sensitive (GTK_WIDGET (data), func (param));
}


/* Call update_selection_sensitive_widget for all widgets in the list of
 * sensitive widgets and pass editor through as the user data parameter
 */
static void
update_selection_sensitive_widgets (GpaFileManager *fileman)
{
  g_list_foreach (fileman->selection_sensitive_widgets,
                  update_selection_sensitive_widget,
                  (gpointer) fileman);
}


/*
 *	Construct the file manager window
 */

static GtkWidget *
fileman_menu_new (GpaFileManager *fileman)
{
  GtkItemFactory *factory;
  GtkItemFactoryEntry file_menu[] = {
    {_("/_File"), NULL, NULL, 0, "<Branch>"},
    {_("/File/_Open"), NULL, open_file, 0, "<StockItem>", GTK_STOCK_OPEN},
    {_("/File/C_lear"), NULL, close_all_files, 0, "<StockItem>", GTK_STOCK_CLEAR},
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
  GtkWidget *item;

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

  /* Disable buttons when no file is selected */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/File/Sign"));
  if (item)
    {
      add_selection_sensitive_widget (fileman, item, has_selection);
    }

  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/File/Verify"));
  if (item)
    {
      add_selection_sensitive_widget (fileman, item, has_selection);
    }

  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/File/Encrypt"));
  if (item)
    {
      add_selection_sensitive_widget (fileman, item, has_selection);
    }

  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/File/Decrypt"));
  if (item)
    {
      add_selection_sensitive_widget (fileman, item, has_selection);
    }


  gpa_help_menu_add_to_factory (factory, GTK_WIDGET (fileman));
  gtk_window_add_accel_group (GTK_WINDOW (fileman), accel_group);
  return gtk_item_factory_get_widget (factory, "<main>");
}

static GtkWidget *
fileman_toolbar_new (GpaFileManager *fileman)
{
  GtkWidget *toolbar, *icon, *item;

  toolbar = gtk_toolbar_new ();
  
  /* Open */
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_OPEN,
                            _("Open a file"), _("open file"),
                            GTK_SIGNAL_FUNC (toolbar_file_open),
                            fileman, -1);
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_CLEAR,
                            _("Close all files"), _("close files"),
                            GTK_SIGNAL_FUNC (toolbar_close_all),
                            fileman, -1);
  /* Sign */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "sign")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Sign"),
				      _("Sign the selected file"), _("sign file"),
				      icon,
				      GTK_SIGNAL_FUNC (toolbar_file_sign),
				      fileman);
      add_selection_sensitive_widget (fileman, item, has_selection);
    }
  /* Verify */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "verify")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Verify"),
				      _("Check signatures of selected file"), 
				      _("verify file"), icon,
				      GTK_SIGNAL_FUNC (toolbar_file_verify), 
				      fileman);
      add_selection_sensitive_widget (fileman, item, has_selection);
    }
  /* Encrypt */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "encrypt")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Encrypt"),
				      _("Encrypt the selected file"),
				      _("encrypt file"),
				      icon, GTK_SIGNAL_FUNC (toolbar_file_encrypt),
				      fileman);
      add_selection_sensitive_widget (fileman, item, has_selection);
    }
  /* Decrypt */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "decrypt")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Decrypt"),
				      _("Decrypt the selected file"),
				      _("decrypt file"),
				      icon, GTK_SIGNAL_FUNC (toolbar_file_decrypt),
				      fileman);
      add_selection_sensitive_widget (fileman, item, has_selection);
    }

#if 0
  /* Disabled for now. The long label causes the toolbar to grow too much.
   * See http://bugzilla.gnome.org/show_bug.cgi?id=75068
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
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "help")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Help"),
			     _("Understanding the GNU Privacy Assistant"),
			     _("help"), icon,
			     GTK_SIGNAL_FUNC (help_help), NULL);
#endif

  return toolbar;
}

static GtkWidget *
file_list_new (GpaFileManager * fileman)
{
  GtkWidget *scrollerFile;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;
  GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
  GtkWidget *list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("File"), renderer,
						     "text", 
						     FILE_NAME_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_MULTIPLE);
  g_signal_connect_swapped (select, "changed",
			    G_CALLBACK (update_selection_sensitive_widgets),
			    fileman);

  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);

  scrollerFile = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scrollerFile),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

  fileman->list_files = list;
  gtk_widget_grab_focus (list);
  gtk_container_add (GTK_CONTAINER (scrollerFile), list);

  return scrollerFile;
} /* gpa_window_file_new */


static void
file_manager_closed (GtkWidget *widget, gpointer param)
{
  instance = NULL;
}

static GObject*
gpa_file_manager_constructor (GType type,
			      guint n_construct_properties,
			      GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaFileManager *fileman;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GtkWidget *fileBox;
  GtkWidget *file_frame;
  GtkWidget *toolbar;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  fileman = GPA_FILE_MANAGER (object);
  /* Initialize */
  gtk_window_set_title (GTK_WINDOW (fileman),
			_("GNU Privacy Assistant - File Manager"));
  gtk_widget_set_usize (GTK_WIDGET (fileman), 640, 480);
  /* Realize the window so that we can create pixmaps without warnings */
  gtk_widget_realize (GTK_WIDGET (fileman));

  vbox = gtk_vbox_new (FALSE, 0);
  menubar = fileman_menu_new (fileman);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);

  /* set up the toolbar */
  toolbar = fileman_toolbar_new(fileman);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);
  
  fileBox = gtk_hbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (fileBox), 5);
  file_frame = file_list_new (fileman);
  gtk_box_pack_start (GTK_BOX (fileBox), file_frame, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), fileBox, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (fileman), vbox);

  g_signal_connect (object, "destroy", G_CALLBACK (file_manager_closed),
		    object);

  return object;
}

static GpaFileManager *
gpa_fileman_new ()
{
  GpaFileManager *fileman;

  fileman = g_object_new (GPA_FILE_MANAGER_TYPE, NULL);  
  update_selection_sensitive_widgets (fileman);

  return fileman;
}

/* API */

GtkWidget * gpa_file_manager_get_instance (void)
{
  if (!instance)
    {
      instance = gpa_fileman_new ();
    }
  return GTK_WIDGET (instance);
}

gboolean gpa_file_manager_is_open (void)
{
  return (instance != NULL);
}

void gpa_file_manager_open_file (GpaFileManager *fileman,
				 const gchar *filename)

{
  if (!add_file (fileman, filename))
    gpa_window_error (_("The file is already open."),
		      GTK_WIDGET (fileman));
}
