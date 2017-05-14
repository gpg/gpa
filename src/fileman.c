/* fileman.c  -  The GNU Privacy Assistant
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2007, 2008 g10 Code GmbH

   This file is part of GPA.

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

/* The file encryption/decryption/sign window.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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


#if ! GTK_CHECK_VERSION (2, 10, 0)
#define GTK_STOCK_SELECT_ALL "gtk-select-all"
#endif


/* Object and class definition.  */
struct _GpaFileManager
{
  GtkWindow parent;

  GtkWidget *window;
  GtkWidget *list_files;
  GList *selection_sensitive_actions;
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

#define DND_TARGET_URI_LIST 1


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
  fileman->selection_sensitive_actions = NULL;
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
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  GList *selection = gtk_tree_selection_get_selected_rows (sel, &model);
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
add_file (GpaFileManager *fileman, const gchar *filename)
{
  GtkListStore *store;
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeSelection *sel;
  gchar *filename_utf8;

  /* The tree contains filenames in the UTF-8 encoding.  */
  filename_utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);

  /* Try to convert from the current locale as fallback. This is important
     for windows where g_filename_to_utf8 does not take locale into account
     because the filedialogs already convert to utf8. */
  if (!filename_utf8)
    {
      filename_utf8 = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);
    }

  /* Last fallback is guranteed to never be NULL so in doubt we can still fail
     later showing a filename that can't be found to the user etc.*/
  if (!filename_utf8)
    {
      filename_utf8 = g_filename_display_name (filename);
    }

  store = GTK_LIST_STORE (gtk_tree_view_get_model
                          (GTK_TREE_VIEW (fileman->list_files)));

  /* Check for duplicates. */
  path = gtk_tree_path_new_first ();
  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path))
    do
      {
	gchar *tmp;
	gboolean exists;

	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, FILE_NAME_COLUMN,
			    &tmp, -1);
	exists = g_str_equal (filename_utf8, tmp);
	g_free (tmp);
	if (exists)
	  {
	    g_free (filename_utf8);
	    gtk_tree_path_free (path);
	    return FALSE; /* This file is already in our list.  */
	  }
      }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
  gtk_tree_path_free (path);

  /* Append it to our list.  */
  gtk_list_store_append (store, &iter);

  /* FIXME: Add the file status when/if gpgme supports it */
  gtk_list_store_set (store, &iter, FILE_NAME_COLUMN, filename_utf8, -1);

  /* Select the row */
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (fileman->list_files));
  gtk_tree_selection_select_iter (sel, &iter);

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



/* Management of the selection sensitive actions.  */

/* Return true if a selection is active.  */
static gboolean
has_selection (gpointer param)
{
  GpaFileManager *fileman = param;
  GtkTreeSelection *sel;

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (fileman->list_files));
  return (gtk_tree_selection_count_selected_rows (sel) > 0);
}

/* Add WIDGET to the list of sensitive actions of FILEMAN.  */
static void
add_selection_sensitive_action (GpaFileManager *fileman,
				 GtkAction *action,
				 sensitivity_func_t callback)
{
  g_object_set_data (G_OBJECT (action), "gpa_sensitivity", callback);
  fileman->selection_sensitive_actions
    = g_list_append (fileman->selection_sensitive_actions, action);
}

/* Update the sensitivity of the widget DATA and pass PARAM through to
   the sensitivity callback.  Usable as an iterator function in
   g_list_foreach.  */
static void
update_selection_sensitive_action (gpointer data, gpointer param)
{
  sensitivity_func_t func;

  func = g_object_get_data (G_OBJECT (data), "gpa_sensitivity");
  gtk_action_set_sensitive (GTK_ACTION (data), func (param));
}


/* Call update_selection_sensitive_action for all widgets in the list
   of sensitive actions and pass FILEMAN through as the user data
   parameter.  */
static void
update_selection_sensitive_actions (GpaFileManager *fileman)
{
  g_list_foreach (fileman->selection_sensitive_actions,
                  update_selection_sensitive_action,
                  (gpointer) fileman);
}



/* Actions as called by the menu items.  */


static GSList *
get_load_file_name (GtkWidget *parent, const gchar *title,
		    const gchar *directory)
{
  static GtkWidget *dialog;
  GtkResponseType response;
  GSList *filenames = NULL;

  if (! dialog)
    {
      dialog = gtk_file_chooser_dialog_new
	(title, GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_OPEN,
	 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	 GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
    }
  if (directory)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), directory);
  gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (dialog));

  /* Run the dialog until there is a valid response.  */
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_OK)
    filenames = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));

  gtk_widget_hide (dialog);

  return filenames;
}


/* Handle menu item "File/Open".  */

void
open_file_one (gpointer data, gpointer user_data)
{
  GpaFileManager *fileman = user_data;
  gchar *filename = (gchar *) data;

  /* FIXME: We are ignoring errors here.  */
  add_file (fileman, filename);
  g_free (filename);
}


static void
file_open (GtkAction *action, gpointer param)
{
  GpaFileManager *fileman = param;
  GSList *filenames;

  filenames = get_load_file_name (GTK_WIDGET (fileman), _("Open File"), NULL);
  if (! filenames)
    return;

  g_slist_foreach (filenames, open_file_one, fileman);
  g_slist_free (filenames);
}


/* Handle menu item "File/Clear".  */
static void
file_clear (GtkAction *action, gpointer param)
{
  GpaFileManager *fileman = param;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (fileman->list_files)));

  gtk_list_store_clear (store);
}


/* Handle menu item "File/Verify".  */
static void
file_verify (GtkAction *action, gpointer param)
{
  GpaFileManager *fileman = param;
  GList *files;
  GpaFileVerifyOperation *op;

  files = get_selected_files (fileman->list_files);
  if (!files)
    return;

  op = gpa_file_verify_operation_new (GTK_WIDGET (fileman), files);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}


/* Handle menu item "File/Sign".  */
static void
file_sign (GtkAction *action, gpointer param)
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
file_encrypt (GtkAction *action, gpointer param)
{
  GpaFileManager *fileman = param;
  GList *files;
  GpaFileEncryptOperation *op;

  files = get_selected_files (fileman->list_files);
  if (!files)
    return;

  op = gpa_file_encrypt_operation_new (GTK_WIDGET (fileman), files, FALSE);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}


/* Handle menu item "File/Decrypt".  */
static void
file_decrypt (GtkAction *action, gpointer param)
{
  GpaFileManager *fileman = param;
  GList *files;
  GpaFileDecryptOperation *op;

  files = get_selected_files (fileman->list_files);
  if (!files)
    return;

  op = gpa_file_decrypt_verify_operation_new (GTK_WIDGET (fileman), files);

  register_operation (fileman, GPA_FILE_OPERATION (op));
}


/* Handle menu item "File/Close".  */
static void
file_close (GtkAction *action, gpointer param)
{
  GpaFileManager *fileman = param;
  gtk_widget_destroy (GTK_WIDGET (fileman));
}


/* Handle menu item "Edit/Select All".  */
static void
edit_select_all (GtkAction *action, gpointer param)
{
  GpaFileManager *fileman = param;

  gtk_tree_selection_select_all (gtk_tree_view_get_selection
				 (GTK_TREE_VIEW (fileman->list_files)));
}



/* Construct the file manager menu and toolbar widgets and return
   them. */
static void
fileman_action_new (GpaFileManager *fileman, GtkWidget **menubar,
		    GtkWidget **toolbar)
{
  static const GtkActionEntry entries[] =
    {
      /* Toplevel.  */
      { "File", NULL, N_("_File"), NULL },
      { "Edit", NULL, N_("_Edit"), NULL },

      /* File menu.  */
      { "FileOpen", GTK_STOCK_OPEN, NULL, NULL,
	N_("Open a file"), G_CALLBACK (file_open) },
      { "FileClear", GTK_STOCK_CLEAR, NULL, NULL,
	N_("Close all files"), G_CALLBACK (file_clear) },
      { "FileSign", GPA_STOCK_SIGN, NULL, NULL,
	N_("Sign the selected file"), G_CALLBACK (file_sign) },
      { "FileVerify", GPA_STOCK_VERIFY, NULL, NULL,
	N_("Check signatures of selected file"), G_CALLBACK (file_verify) },
      { "FileEncrypt", GPA_STOCK_ENCRYPT, NULL, NULL,
	N_("Encrypt the selected file"), G_CALLBACK (file_encrypt) },
      { "FileDecrypt", GPA_STOCK_DECRYPT, NULL, NULL,
	N_("Decrypt the selected file"), G_CALLBACK (file_decrypt) },
      { "FileClose", GTK_STOCK_CLOSE, NULL, NULL,
	N_("Close the window"), G_CALLBACK (file_close) },
      { "FileQuit", GTK_STOCK_QUIT, NULL, NULL,
	N_("Quit the program"), G_CALLBACK (gtk_main_quit) },

      /* Edit menu.  */
      { "EditSelectAll", GTK_STOCK_SELECT_ALL, NULL, "<control>A",
	N_("Select all files"), G_CALLBACK (edit_select_all) }
    };

  static const char *ui_description =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='File'>"
    "      <menuitem action='FileOpen'/>"
    "      <menuitem action='FileClear'/>"
    "      <separator/>"
    "      <menuitem action='FileSign'/>"
    "      <menuitem action='FileVerify'/>"
    "      <menuitem action='FileEncrypt'/>"
    "      <menuitem action='FileDecrypt'/>"
    "      <separator/>"
    "      <menuitem action='FileClose'/>"
    "      <menuitem action='FileQuit'/>"
    "    </menu>"
    "    <menu action='Edit'>"
    "      <menuitem action='EditSelectAll'/>"
    "      <separator/>"
    "      <menuitem action='EditPreferences'/>"
    "      <menuitem action='EditBackendPreferences'/>"
    "    </menu>"
    "    <menu action='Windows'>"
    "      <menuitem action='WindowsKeyringEditor'/>"
    "      <menuitem action='WindowsFileManager'/>"
    "      <menuitem action='WindowsClipboard'/>"
#ifdef ENABLE_CARD_MANAGER
    "      <menuitem action='WindowsCardManager'/>"
#endif
    "    </menu>"
    "    <menu action='Help'>"
#if 0
    "      <menuitem action='HelpContents'/>"
#endif
    "      <menuitem action='HelpAbout'/>"
    "    </menu>"
    "  </menubar>"
    "  <toolbar name='ToolBar'>"
    "    <toolitem action='FileOpen'/>"
    "    <toolitem action='FileClear'/>"
    "    <separator/>"
    "    <toolitem action='FileSign'/>"
    "    <toolitem action='FileVerify'/>"
    "    <toolitem action='FileEncrypt'/>"
    "    <toolitem action='FileDecrypt'/>"
    "    <separator/>"
    "    <toolitem action='EditPreferences'/>"
    "    <separator/>"
    "    <toolitem action='WindowsKeyringEditor'/>"
    "    <toolitem action='WindowsClipboard'/>"
#ifdef ENABLE_CARD_MANAGER
    "    <toolitem action='WindowsCardManager'/>"
#endif
#if 0
    "    <toolitem action='HelpContents'/>"
#endif
    "  </toolbar>"
    "</ui>";

  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;
  GtkAction *action;
  GtkUIManager *ui_manager;
  GError *error;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, PACKAGE);
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries),
				fileman);
  gtk_action_group_add_actions (action_group, gpa_help_menu_action_entries,
				G_N_ELEMENTS (gpa_help_menu_action_entries),
				fileman);
  gtk_action_group_add_actions (action_group, gpa_windows_menu_action_entries,
				G_N_ELEMENTS (gpa_windows_menu_action_entries),
				fileman);
  gtk_action_group_add_actions
    (action_group, gpa_preferences_menu_action_entries,
     G_N_ELEMENTS (gpa_preferences_menu_action_entries), fileman);
  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (fileman), accel_group);
  if (! gtk_ui_manager_add_ui_from_string (ui_manager, ui_description,
					   -1, &error))
    {
      g_message ("building fileman menus failed: %s", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }

  /* Fixup the icon theme labels which are too long for the toolbar.  */
  action = gtk_action_group_get_action (action_group, "WindowsKeyringEditor");
  g_object_set (action, "short_label", _("Keyring"), NULL);
#ifdef ENABLE_CARD_MANAGER
  action = gtk_action_group_get_action (action_group, "WindowsCardManager");
  g_object_set (action, "short_label", _("Card"), NULL);
#endif

  /* Take care of sensitiveness of widgets.  */
  action = gtk_action_group_get_action (action_group, "FileSign");
  add_selection_sensitive_action (fileman, action, has_selection);
  action = gtk_action_group_get_action (action_group, "FileVerify");
  add_selection_sensitive_action (fileman, action, has_selection);
  action = gtk_action_group_get_action (action_group, "FileEncrypt");
  add_selection_sensitive_action (fileman, action, has_selection);
  action = gtk_action_group_get_action (action_group, "FileDecrypt");
  add_selection_sensitive_action (fileman, action, has_selection);

  *menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  *toolbar = gtk_ui_manager_get_widget (ui_manager, "/ToolBar");
  gpa_toolbar_set_homogeneous (GTK_TOOLBAR (*toolbar), FALSE);
}


/* Drag and Drop handler.  */


/* Handler for "drag-drop".  This signal is emitted when the user
   drops the selection.  */
static gboolean
dnd_drop_handler (GtkWidget *widget, GdkDragContext *context,
                  gint x, gint y, guint tim, gpointer user_data)
{
  GdkAtom target_type = gdk_atom_intern ("text/uri-list", FALSE);

  /* If the source offers a target we request the data from her. */
  if (context->targets && g_list_find (context->targets,
                                       GDK_ATOM_TO_POINTER (target_type)))
    {
      gtk_drag_get_data (widget, context, target_type, tim);

      return TRUE;
    }

  return FALSE;
}


/* Handler for "drag-data-received".  This signal is emitted when the
   data has been received from the source.  */
static void
dnd_data_received_handler (GtkWidget *widget, GdkDragContext *context,
                           gint x, gint y, GtkSelectionData *selection_data,
                           guint target_type, guint tim, gpointer user_data)
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
          char *p = (char *) selection_data->data;
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
  gtk_drag_finish (context, dnd_success, delete_selection_data, tim);
}






/* Construct the file list object.  */
static GtkWidget *
file_list_new (GpaFileManager * fileman)
{
  GtkWidget *scrollerFile;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *sel;
  GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
  GtkWidget *list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("File"), renderer,
						     "text",
						     FILE_NAME_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
  g_signal_connect_swapped (sel, "changed",
			    G_CALLBACK (update_selection_sensitive_actions),
			    fileman);

  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);

  scrollerFile = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scrollerFile),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollerFile),
				       GTK_SHADOW_IN);

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
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *icon;
  gchar *markup;
  GtkWidget *menubar;
  GtkWidget *file_box;
  GtkWidget *file_frame;
  GtkWidget *toolbar;
  GtkWidget *align;
  guint pt, pb, pl, pr;

  /* Invoke parent's constructor.  */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  fileman = GPA_FILE_MANAGER (object);

  /* Initialize.  */
  gpa_window_set_title (GTK_WINDOW (fileman), _("File Manager"));
  gtk_window_set_default_size (GTK_WINDOW (fileman), 640, 480);
  /* Realize the window so that we can create pixmaps without warnings.  */
  gtk_widget_realize (GTK_WIDGET (fileman));

  /* Use a vbox to show the menu, toolbar and the file container.  */
  vbox = gtk_vbox_new (FALSE, 0);

  /* Get the menu and the toolbar.  */
  fileman_action_new (fileman, &menubar, &toolbar);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);


  /* Add a fancy label that tells us: This is the file manager.  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);

  icon = gtk_image_new_from_stock ("gtk-directory", GTK_ICON_SIZE_DND);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);

  label = gtk_label_new (NULL);
  markup = g_strdup_printf ("<span font_desc=\"16\">%s</span>",
                            _("File Manager"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 10);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);


  /* Third a hbox with the file list.  */
  align = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_alignment_get_padding (GTK_ALIGNMENT (align),
			     &pt, &pb, &pl, &pr);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), pt, pb + 5,
			     pl + 5, pr + 5);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);

  file_box = gtk_hbox_new (TRUE, 0);
  file_frame = file_list_new (fileman);
  gtk_box_pack_start (GTK_BOX (file_box), file_frame, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (align), file_box);

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
  update_selection_sensitive_actions (fileman);

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
