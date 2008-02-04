/* fileman.c  -  The GNU Privacy Assistant
 * Copyright (C) 2000, 2001 G-N-U GmbH.
 * Copyright (C) 2007, 2008 g10 Code GmbH
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
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

#include "gpa.h"   
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


/* FIXME:  Move to a gloabl file.  */
#ifndef DIM
#define DIM(array) (sizeof (array) / sizeof (*array))
#endif



/* Object and class definition.  */
struct _GpaFileManager
{
  GtkWindow parent;

  GtkWidget *window;
  GtkWidget *list_files;
  GList *selection_sensitive_widgets;
};

struct _GpaFileManagerClass 
{
  GtkWindowClass parent_class;
};



/* There is only one instance of the file manage class.  Use a global
   variable to keep track of it.  */
static GpaFileManager *instance;

/* We also need to save the parent class. */
static GObjectClass *parent_class;

/* Definition of the sensitivity function type.  */
typedef gboolean (*sensitivity_func_t)(gpointer);


/* Constants to define the file list. */
enum
{
  FILE_NAME_COLUMN,
  FILE_N_COLUMNS
};


#define DND_TARGET_URI_LIST 0

/* Drag and drop target list. */
static GtkTargetEntry dnd_target_list[] = 
  {
    { "text/uri-list", 0, DND_TARGET_URI_LIST }
  };




/* Local prototypes */
static GObject *gpa_file_manager_constructor 
                         (GType type,
                          guint n_construct_properties,
                          GObjectConstructParam *construct_properties);



/*
 * GtkWidget boilerplate.
 */
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
      gpa_file_item_t file_item;
      gchar *filename;
      GtkTreeIter iter;

      gtk_tree_model_get_iter (model, &iter, (GtkTreePath*) selection->data);
      gtk_tree_model_get (model, &iter, FILE_NAME_COLUMN, &filename, -1);

      file_item = g_malloc0 (sizeof (*file_item));
      file_item->filename_in = filename;

      files = g_list_append (files, file_item);
      selection = g_list_next (selection);
    }

  /* Free the selection */
  g_list_foreach (selection, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (selection);

  return files;
}


/* Add file FILENAME to the file list of FILEMAN and select it */
static gboolean
add_file (GpaFileManager *fileman, const char *filename)
{
  GtkListStore *store;
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeSelection *select;

  store = GTK_LIST_STORE (gtk_tree_view_get_model
                          (GTK_TREE_VIEW (fileman->list_files)));
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (fileman->list_files));

  /* Check for duplicates. */
  path = gtk_tree_path_new_first ();
  gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter))
    {
      gchar *tmp;
      gboolean exists;

      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, FILE_NAME_COLUMN,
			  &tmp, -1);
      exists = g_str_equal (filename, tmp);
      g_free (tmp);
      if (exists)
        return FALSE; /* This file is already in our list.  */
    }
  gtk_tree_path_free (path);

  /* Append it to our list.  */
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
file_created_cb (GpaFileOperation *op, gpa_file_item_t item, gpointer data)
{
  GpaFileManager *fileman = data;
  
  add_file (fileman, item->filename_out);
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
 * Management of the selection sensitive widgets;
 */

/* Return true if a selection is active.  */
static gboolean
has_selection (gpointer param)
{
  GpaFileManager *fileman = param;

  GtkTreeSelection *select = gtk_tree_view_get_selection 
    (GTK_TREE_VIEW (fileman->list_files));

  return (gtk_tree_selection_count_selected_rows (select) > 0);
}

/* Add WIDGET to the list of sensitive widgets of FILEMAN.  */
static void
add_selection_sensitive_widget (GpaFileManager *fileman,
                                GtkWidget *widget,
                                sensitivity_func_t callback)
{
  gtk_object_set_data (GTK_OBJECT (widget), "gpa_sensitivity", callback);
  fileman->selection_sensitive_widgets \
    = g_list_append(fileman->selection_sensitive_widgets, widget);
}

/* Update the sensitivity of the widget DATA and pass PARAM through to
   the sensitivity callback. Usable as an iterator function in
   g_list_foreach. */
static void
update_selection_sensitive_widget (gpointer data, gpointer param)
{
  sensitivity_func_t func;

  func = gtk_object_get_data (GTK_OBJECT (data), "gpa_sensitivity");
  gtk_widget_set_sensitive (GTK_WIDGET (data), func (param));
}


/* Call update_selection_sensitive_widget for all widgets in the list
   of sensitive widgets and pass FILEMAN through as the user data
   parameter.  */
static void
update_selection_sensitive_widgets (GpaFileManager *fileman)
{
  g_list_foreach (fileman->selection_sensitive_widgets,
                  update_selection_sensitive_widget,
                  (gpointer) fileman);
}



/* 
   Actions as called by the menu items. 

*/


/* Handle menu item "File/Open".  */
static void
open_file (gpointer param)
{
  GpaFileManager * fileman = param;
  gchar * filename;

  filename = gpa_get_load_file_name (GTK_WIDGET (fileman),
                                     _("Open File"), NULL);
  if (filename)
    {

      if (!add_file (fileman, filename))
        gpa_window_error (_("The file is already open."),
			    GTK_WIDGET (fileman));
      g_free (filename);
    }
}

/* Handle menu item "File/Clear".  */
static void
close_all_files (gpointer param)
{
  GpaFileManager *fileman = param;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (fileman->list_files)));

  gtk_list_store_clear (store);
}


/* Handle menu item "File/Verify".  */
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


/* Handle menu item "File/Sign".  */
static void
sign_files (gpointer param)
{
  GpaFileManager *fileman = param;
  GList * files;
  GpaFileSignOperation *op;

  files = get_selected_files (fileman->list_files);
  if (!files)
    return;

  op = gpa_file_sign_operation_new (GTK_WIDGET (fileman), files, FALSE);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}


/* Handle menu item "File/Encrypt".  */
static void
encrypt_files (gpointer param)
{
  GpaFileManager *fileman = param;
  GList * files;
  GpaFileEncryptOperation *op;

  files = get_selected_files (fileman->list_files);
  if (!files)
    return;

  op = gpa_file_encrypt_operation_new (GTK_WIDGET (fileman), files, FALSE);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}


/* Handle menu item "File/Decrypt".  */
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


/* Handle menu item "File/Close".  */
static void
close_window (gpointer param)
{
  GpaFileManager *fileman = param;
  gtk_widget_destroy (GTK_WIDGET (fileman));
}


/* Handle menu item "Edit/Select All".  */
static void
fileman_select_all (gpointer param)
{
  GpaFileManager *fileman = param;
  
  gtk_tree_selection_select_all (gtk_tree_view_get_selection
				 (GTK_TREE_VIEW (fileman->list_files)));
}



/* Construct the file manager menu window and return that object. */
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
    {_("/Edit/_Backend Preferences..."), NULL,
     gpa_open_backend_config_dialog, 0, "<StockItem>", GTK_STOCK_PREFERENCES},
  };
  GtkItemFactoryEntry windows_menu[] = {
    {_("/_Windows"), NULL, NULL, 0, "<Branch>"},
    {_("/Windows/_Keyring Editor"), NULL, gpa_open_keyring_editor, 0, NULL},
    {_("/Windows/_Filemanager"), NULL, gpa_open_filemanager, 0, NULL},
    {_("/Windows/_Clipboard"), NULL, gpa_open_clipboard, 0, NULL},
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

  /* Disable buttons when no file is selected.   */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/File/Sign"));
  if (item)
    add_selection_sensitive_widget (fileman, item, has_selection);

  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/File/Verify"));
  if (item)
    add_selection_sensitive_widget (fileman, item, has_selection);

  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/File/Encrypt"));
  if (item)
    add_selection_sensitive_widget (fileman, item, has_selection);

  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/File/Decrypt"));
  if (item)
    add_selection_sensitive_widget (fileman, item, has_selection);

  gpa_help_menu_add_to_factory (factory, GTK_WIDGET (fileman));
  gtk_window_add_accel_group (GTK_WINDOW (fileman), accel_group);

  return gtk_item_factory_get_widget (factory, "<main>");
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


static void
toolbar_preferences (GtkWidget *widget, gpointer param)
{
  gpa_open_settings_dialog ();
}


/* Construct the new toolbar object and return it.  Takes the file
   manage object.  */
static GtkWidget *
fileman_toolbar_new (GpaFileManager *fileman)
{
  GtkWidget *toolbar, *icon, *item;

  toolbar = gtk_toolbar_new ();
  
  /* Build the "Open" button.  */
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_OPEN,
                            _("Open a file"), _("open file"),
                            GTK_SIGNAL_FUNC (toolbar_file_open),
                            fileman, -1);
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_CLEAR,
                            _("Close all files"), _("close files"),
                            GTK_SIGNAL_FUNC (toolbar_close_all),
                            fileman, -1);
  /* Build the "Sign" button.  */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "sign")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Sign"),
				      _("Sign the selected file"),
                                      _("sign file"),
				      icon,
				      GTK_SIGNAL_FUNC (toolbar_file_sign),
				      fileman);
      add_selection_sensitive_widget (fileman, item, has_selection);
    }
  /* Build the "Verify" button.  */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "verify")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Verify"),
				      _("Check signatures of selected file"), 
				      _("verify file"),
                                      icon,
				      GTK_SIGNAL_FUNC (toolbar_file_verify), 
				      fileman);
      add_selection_sensitive_widget (fileman, item, has_selection);
    }
  /* Build the "Encrypt" button.  */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "encrypt")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Encrypt"),
				      _("Encrypt the selected file"),
				      _("encrypt file"),
				      icon,
                                      GTK_SIGNAL_FUNC (toolbar_file_encrypt),
				      fileman);
      add_selection_sensitive_widget (fileman, item, has_selection);
    }
  /* Build the "Decrypt" button.  */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "decrypt")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Decrypt"),
				      _("Decrypt the selected file"),
				      _("decrypt file"),
				      icon, 
                                      GTK_SIGNAL_FUNC (toolbar_file_decrypt),
				      fileman);
      add_selection_sensitive_widget (fileman, item, has_selection);
    }

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
  
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), 
                            GTK_STOCK_PREFERENCES,
                            _("Open the Preferences dialog"),
                            _("preferences"),
                            GTK_SIGNAL_FUNC (toolbar_preferences),
                            fileman, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  icon = gpa_create_icon_widget (GTK_WIDGET (fileman), "keyringeditor");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Keyring"),
                                  _("Open the Keyring Editor"),
                                  _("keyring editor"), icon,
				  GTK_SIGNAL_FUNC (gpa_open_keyring_editor),
                                  NULL);

  /* FIXME: Should be just the clipboard.  */
  icon = gtk_image_new_from_stock ("gtk-paste",
				   GTK_ICON_SIZE_SMALL_TOOLBAR);
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Clipboard"),
				  _("Open the clipboard"),
				  _("clipboard"), icon,
				  GTK_SIGNAL_FUNC (gpa_open_clipboard),
				  NULL);

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



/* 
     Drag and Drop handler
 */


/* Handler for "drag-drop".  This signal is emitted when the user
   drops the selection.  */
static gboolean
dnd_drop_handler (GtkWidget *widget, GdkDragContext *context, 
                  gint x, gint y, guint time, gpointer user_data)
{
  GdkAtom  target_type;
        
  /* If the source offers a target we request the data from her. */
  if (context->targets)
    {
      target_type = GDK_POINTER_TO_ATOM 
        (g_list_nth_data (context->targets, DND_TARGET_URI_LIST));
      gtk_drag_get_data (widget, context, target_type, time);
      
      return TRUE;
    }

  return FALSE;
}


/* Handler for "drag-data-received".  This signal is emitted when the
   data has been received from the source.  */
static void
dnd_data_received_handler (GtkWidget *widget, GdkDragContext *context,
                           gint x, gint y, GtkSelectionData *selection_data,
                           guint target_type, guint time, gpointer user_data)
{       
  GpaFileManager *fileman = user_data;
  gboolean dnd_success = FALSE;
  gboolean delete_selection_data = FALSE;
        
  /* Is that usable by us?  */
  if (selection_data && selection_data->length >= 0 )
    {
      if (context->action == GDK_ACTION_MOVE)
        delete_selection_data = TRUE;
      
      /* Check that we got a format we can use.  */
      if (target_type == DND_TARGET_URI_LIST)
        {
          char *p = selection_data->data;
          char **list;
          int i;

          list = g_uri_list_extract_uris (p);
          for (i=0; list && list[i]; i++)
            {
              char *name = g_filename_from_uri (list[i], NULL, NULL);
              if (name)
                {
                  /* Canonical line endings are required for an uri-list. */
                  if ((p = strchr (name, '\r')))
                    *p = 0;
                  add_file (fileman, name);
                  g_free (name);
                }
            }
          g_strfreev (list);
          dnd_success = TRUE;
        }
    }

  /* Finish the DnD processing.  */
  gtk_drag_finish (context, dnd_success, delete_selection_data, time);
}






/* Construct the file list object.  */
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
} 


/* Callback for the destroy signal.  */
static void
file_manager_closed (GtkWidget *widget, gpointer param)
{
  instance = NULL;
}


/* Construct a new class object of GpaFileManager.  */
static GObject*
gpa_file_manager_constructor (GType type,
			      guint n_construct_properties,
			      GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaFileManager *fileman;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GtkWidget *file_box;
  GtkWidget *file_frame;
  GtkWidget *toolbar;

  /* Invoke parent's constructor.  */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  fileman = GPA_FILE_MANAGER (object);

  /* Initialize.  */
  gtk_window_set_title (GTK_WINDOW (fileman),
			_("GNU Privacy Assistant - File Manager"));
  gtk_window_set_default_size (GTK_WINDOW (fileman), 640, 480);
  /* Realize the window so that we can create pixmaps without warnings.  */
  gtk_widget_realize (GTK_WIDGET (fileman));

  /* Use a vbox to show the menu, toolbar and the file container.  */
  vbox = gtk_vbox_new (FALSE, 0);

  /* First comes the menu.  */
  menubar = fileman_menu_new (fileman);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);

  /* Second the toolbar.  */
  toolbar = fileman_toolbar_new(fileman);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);

  /* Third a hbox with the file list.  */
  file_box = gtk_hbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (file_box), 5);
  file_frame = file_list_new (fileman);
  gtk_box_pack_start (GTK_BOX (file_box), file_frame, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), file_box, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (fileman), vbox);

  g_signal_connect (object, "destroy",
                    G_CALLBACK (file_manager_closed), object);


  /* Make the file box a DnD destination. */
  gtk_drag_dest_set (file_box,
                     (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT),
                     dnd_target_list,
                     DIM (dnd_target_list),
                     GDK_ACTION_COPY);
  g_signal_connect (file_box, "drag-drop",
                    G_CALLBACK (dnd_drop_handler), fileman);
  g_signal_connect (file_box, "drag-data-received", 
                    G_CALLBACK (dnd_data_received_handler), fileman);


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

GtkWidget *
gpa_file_manager_get_instance (void)
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
  /* FIXME: Release filename?  */
}
