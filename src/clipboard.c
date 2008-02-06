/* clipboard.c  -  The GNU Privacy Assistant
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
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "gpa.h"   
#include "gpapastrings.h"

#include "gtktools.h"
#include "gpawidgets.h"
#include "siglist.h"
#include "helpmenu.h"
#include "icons.h"
#include "clipboard.h"

#include "gpafiledecryptop.h"
#include "gpafileencryptop.h"
#include "gpafilesignop.h"
#include "gpafileverifyop.h"


/* Support for Gtk 2.8.  */

#if ! GTK_CHECK_VERSION (2, 10, 0)
#define gtk_text_buffer_get_has_selection(textbuf) \
  gtk_text_buffer_get_selection_bounds (textbuf, NULL, NULL);
#define gdk_atom_intern_static_string(str) gdk_atom_intern (str, FALSE)
#define MY_GTK_TEXT_BUFFER_NO_HAS_SELECTION
#endif


/* FIXME:  Move to a global file.  */
#ifndef DIM
#define DIM(array) (sizeof (array) / sizeof (*array))
#endif



/* Object and class definition.  */
struct _GpaClipboard
{
  GtkWindow parent;

  GtkWidget *text_view;
  GtkTextBuffer *text_buffer;

  /* List of sensitive widgets. See below */
  GList *selection_sensitive_widgets;
  GList *paste_sensitive_widgets;
  gboolean paste_p;
};

struct _GpaClipboardClass 
{
  GtkWindowClass parent_class;
};



/* There is only one instance of the clipboard class.  Use a global
   variable to keep track of it.  */
static GpaClipboard *instance;

/* We also need to save the parent class.  */
static GObjectClass *parent_class;


/* Local prototypes */
static GObject *gpa_clipboard_constructor 
                         (GType type,
                          guint n_construct_properties,
                          GObjectConstructParam *construct_properties);


/*
 * GtkWidget boilerplate.
 */
static void
gpa_clipboard_finalize (GObject *object)
{  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_clipboard_init (GpaClipboard *clipboard)
{
  clipboard->selection_sensitive_widgets = NULL;
  clipboard->paste_sensitive_widgets = NULL;
}

static void
gpa_clipboard_class_init (GpaClipboardClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->constructor = gpa_clipboard_constructor;
  object_class->finalize = gpa_clipboard_finalize;
}

GType
gpa_clipboard_get_type (void)
{
  static GType clipboard_type = 0;
  
  if (!clipboard_type)
    {
      static const GTypeInfo clipboard_info =
	{
	  sizeof (GpaClipboardClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gpa_clipboard_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (GpaClipboard),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) gpa_clipboard_init,
	};
      
      clipboard_type = g_type_register_static (GTK_TYPE_WINDOW,
					     "GpaClipboard",
					     &clipboard_info, 0);
    }
  
  return clipboard_type;
}



/* Definition of the sensitivity function type.  */
typedef gboolean (*sensitivity_func_t)(gpointer);


/* Add widget to the list of sensitive widgets of editor.  */
static void
add_selection_sensitive_widget (GpaClipboard *clipboard,
                                GtkWidget *widget,
                                sensitivity_func_t callback)
{
  gtk_object_set_data (GTK_OBJECT (widget), "gpa_sensitivity", callback);
  clipboard->selection_sensitive_widgets
    = g_list_append (clipboard->selection_sensitive_widgets, widget);
}


/* Return true if a selection is active.  */
static gboolean
has_selection (gpointer param)
{
  GpaClipboard *clipboard = param;

  return gtk_text_buffer_get_has_selection
    (GTK_TEXT_BUFFER (clipboard->text_buffer));
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
   of sensitive widgets and pass CLIPBOARD through as the user data
   parameter.  */
static void
update_selection_sensitive_widgets (GpaClipboard *clipboard)
{
  g_list_foreach (clipboard->selection_sensitive_widgets,
                  update_selection_sensitive_widget,
                  (gpointer) clipboard);
}


/* Add widget to the list of sensitive widgets of editor.  */
static void
add_paste_sensitive_widget (GpaClipboard *clipboard, GtkWidget *widget)
{
  clipboard->paste_sensitive_widgets
    = g_list_append (clipboard->paste_sensitive_widgets, widget);
}


static void
update_paste_sensitive_widget (gpointer data, gpointer param)
{
  GpaClipboard *clipboard = param;

  gtk_widget_set_sensitive (GTK_WIDGET (data), clipboard->paste_p);
}


static void
update_paste_sensitive_widgets (GtkClipboard *clip,
				GtkSelectionData *selection_data,
				GpaClipboard *clipboard)
{
  clipboard->paste_p = gtk_selection_data_targets_include_text (selection_data);

  g_list_foreach (clipboard->paste_sensitive_widgets,
                  update_paste_sensitive_widget, (gpointer) clipboard);
}


static void
set_paste_sensitivity (GpaClipboard *clipboard, GtkClipboard *clip)
{
  GdkDisplay *display;

  display = gtk_clipboard_get_display (clip);
  
  if (gdk_display_supports_selection_notification (display))
    gtk_clipboard_request_contents
      (clip, gdk_atom_intern_static_string ("TARGETS"),
       (GtkClipboardReceivedFunc) update_paste_sensitive_widgets,
       clipboard);
}


static void
clipboard_owner_change_cb (GtkClipboard *clip, GdkEventOwnerChange *event,
			   GpaClipboard *clipboard)
{
  set_paste_sensitivity (clipboard, clip);
}


/* Add a file created by an operation to the list */
static void
file_created_cb (GpaFileOperation *op, gpa_file_item_t item, gpointer data)
{
  GpaClipboard *clipboard = data;
  gboolean suc;
  const gchar *end;
  
  suc = g_utf8_validate (item->direct_out, item->direct_out_len, &end);
  if (! suc)
    {
      gchar *str;
      str = g_strdup_printf ("Error in operation result:\n"
			     "No valid UTF-8 at position %i.",
			     ((int) (end - item->direct_out)));
      gpa_window_error (str, GTK_WIDGET (clipboard));
      g_free (str);
      return;
    }

  
  gtk_text_buffer_set_text (clipboard->text_buffer,
			    item->direct_out, item->direct_out_len);
}


/* Do whatever is required with a file operation, to ensure proper clean up */
static void
register_operation (GpaClipboard *clipboard, GpaFileOperation *op)
{
  g_signal_connect (G_OBJECT (op), "created_file",
		    G_CALLBACK (file_created_cb), clipboard);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), NULL);
}



/* Actions as called by the menu items.  */


/* Handle menu item "File/Clear".  */
static void
file_clear (gpointer param)
{
  GpaClipboard *clipboard = param;

  gtk_text_buffer_set_text (clipboard->text_buffer, "", -1);
}


/* Handle menu item "File/Open".  */
static void
file_open (gpointer param)
{
  GpaClipboard *clipboard = param;
  gchar *filename;
  struct stat buf;
  int res;
  gboolean suc;
  gchar *contents;
  gsize length;
  GError *err = NULL;
  const gchar *end;

  filename = gpa_get_load_file_name (GTK_WIDGET (clipboard),
                                     _("Open File"), NULL);
  if (! filename)
    return;

  res = g_stat (filename, &buf);
  if (res < 0)
    {
      gchar *str;
      str = g_strdup_printf ("Error determining size of file %s:\n%s",
			     filename, strerror (errno));
      gpa_window_error (str, GTK_WIDGET (clipboard));
      g_free (str);
      g_free (filename);
      return;
   }

#define MAX_CLIPBOARD_SIZE (2*1024*1024)

  if (buf.st_size > MAX_CLIPBOARD_SIZE)
    {
      GtkWidget *window;
      GtkWidget *hbox;
      GtkWidget *labelMessage;
      GtkWidget *pixmap;
      gint result;
      gchar *str;

      window = gtk_dialog_new_with_buttons
	(_("GPA Message"), GTK_WINDOW (clipboard), GTK_DIALOG_MODAL,
	 GTK_STOCK_OPEN, GTK_RESPONSE_OK,
	 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

      gtk_container_set_border_width (GTK_CONTAINER (window), 5);
      gtk_dialog_set_default_response (GTK_DIALOG (window),
				       GTK_RESPONSE_CANCEL);

      hbox = gtk_hbox_new (FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
      gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (window)->vbox), hbox);
      pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
					 GTK_ICON_SIZE_DIALOG);
      gtk_box_pack_start (GTK_BOX (hbox), pixmap, TRUE, FALSE, 10);

      str = g_strdup_printf (_("The file %s is %li%s large.  Do you really "
			       " want to open it?"), filename, 
			       buf.st_size / 1024 / 1024, "MB");
      labelMessage = gtk_label_new (str);
      g_free (str);
      gtk_label_set_line_wrap (GTK_LABEL (labelMessage), TRUE);
      gtk_box_pack_start (GTK_BOX (hbox), labelMessage, TRUE, FALSE, 10);
      
      gtk_widget_show_all (window);
      result = gtk_dialog_run (GTK_DIALOG (window));
      gtk_widget_destroy (window);

      if (result != GTK_RESPONSE_OK)
	{
	  g_free (filename);
	  return;
	}
    }
  
  suc = g_file_get_contents (filename, &contents, &length, &err);
  if (! suc)
    {
      gchar *str;
      str = g_strdup_printf ("Error loading content of file %s:\n%s",
			     filename, err->message);
      gpa_window_error (str, GTK_WIDGET (clipboard));
      g_free (str);
      g_error_free (err);
      g_free (filename);
      return;
    }

  suc = g_utf8_validate (contents, length, &end);
  if (! suc)
    {
      gchar *str;
      str = g_strdup_printf ("Error opening file %s:\n"
			     "No valid UTF-8 at position %i.",
			     filename, ((int) (end - contents)));
      gpa_window_error (str, GTK_WIDGET (clipboard));
      g_free (str);
      g_free (contents);
      g_free (filename);
      return;
    }
  
  gtk_text_buffer_set_text (clipboard->text_buffer, contents, length);
  g_free (contents);
}


/* Handle menu item "File/Save As...".  */
static void
file_save_as (gpointer param)
{
  GpaClipboard *clipboard = param;
  gchar *filename;
  GError *err = NULL;
  gchar *contents;
  gssize length;
  gboolean suc;
  GtkTextIter begin;
  GtkTextIter end;

  filename = gpa_get_save_file_name (GTK_WIDGET (clipboard),
				     _("Save As..."), NULL);
  if (! filename)
    return;

  gtk_text_buffer_get_bounds (clipboard->text_buffer, &begin, &end);
  contents = gtk_text_buffer_get_text (clipboard->text_buffer, &begin, &end,
				       FALSE);
  length = strlen (contents);

  suc = g_file_set_contents (filename, contents, length, &err);
  g_free (contents);
  if (! suc)
    {
      gchar *str;
      str = g_strdup_printf ("Error saving content to file %s:\n%s",
			     filename, err->message);
      gpa_window_error (str, GTK_WIDGET (clipboard));
      g_free (str);
      g_error_free (err);
      g_free (filename);
      return;
    }
  
  g_free (filename);
}


/* Handle menu item "File/Verify".  */
static void
file_verify (gpointer param)
{
  GpaClipboard *clipboard = (GpaClipboard *) param;
  GpaFileVerifyOperation *op;
  GList *files = NULL;
  gpa_file_item_t file_item;
  GtkTextIter begin;
  GtkTextIter end;

  gtk_text_buffer_get_bounds (clipboard->text_buffer, &begin, &end);

  file_item = g_malloc0 (sizeof (*file_item));
  file_item->direct_name = g_strdup (_("Clipboard"));
  file_item->direct_in
    = gtk_text_buffer_get_slice (clipboard->text_buffer, &begin, &end, TRUE);
  /* FIXME: One would think there exists a function to get the number
     of bytes between two GtkTextIter, but no, that's too obvious.  */
  file_item->direct_in_len = strlen (file_item->direct_in);

  files = g_list_append (files, file_item);
  
  /* Start the operation.  */
  op = gpa_file_verify_operation_new (GTK_WIDGET (clipboard), files);
  
  register_operation (clipboard, GPA_FILE_OPERATION (op));
}


/* Handle menu item "File/Sign".  */
static void
file_sign (gpointer param)
{
  GpaClipboard *clipboard = (GpaClipboard *) param;
  GpaFileSignOperation *op;
  GList *files = NULL;
  gpa_file_item_t file_item;
  GtkTextIter begin;
  GtkTextIter end;

  gtk_text_buffer_get_bounds (clipboard->text_buffer, &begin, &end);

  file_item = g_malloc0 (sizeof (*file_item));
  file_item->direct_name = g_strdup (_("Clipboard"));
  file_item->direct_in
    = gtk_text_buffer_get_text (clipboard->text_buffer, &begin, &end, FALSE);
  /* FIXME: One would think there exists a function to get the number
     of bytes between two GtkTextIter, but no, that's too obvious.  */
  file_item->direct_in_len = strlen (file_item->direct_in);

  files = g_list_append (files, file_item);
  
  /* Start the operation.  */
  op = gpa_file_sign_operation_new (GTK_WIDGET (clipboard), files, TRUE);
  
  register_operation (clipboard, GPA_FILE_OPERATION (op));
}


/* Handle menu item "File/Encrypt".  */
static void
file_encrypt (gpointer param)
{
  GpaClipboard *clipboard = (GpaClipboard *) param;
  GpaFileEncryptOperation *op;
  GList *files = NULL;
  gpa_file_item_t file_item;
  GtkTextIter begin;
  GtkTextIter end;

  gtk_text_buffer_get_bounds (clipboard->text_buffer, &begin, &end);

  file_item = g_malloc0 (sizeof (*file_item));
  file_item->direct_name = g_strdup (_("Clipboard"));
  file_item->direct_in
    = gtk_text_buffer_get_text (clipboard->text_buffer, &begin, &end, FALSE);
  /* FIXME: One would think there exists a function to get the number
     of bytes between two GtkTextIter, but no, that's too obvious.  */
  file_item->direct_in_len = strlen (file_item->direct_in);

  files = g_list_append (files, file_item);
  
  /* Start the operation.  */
  op = gpa_file_encrypt_operation_new (GTK_WIDGET (clipboard), files, TRUE);

  register_operation (clipboard, GPA_FILE_OPERATION (op));
}


/* Handle menu item "File/Decrypt".  */
static void
file_decrypt (gpointer param)
{
  GpaClipboard *clipboard = (GpaClipboard *) param;
  GpaFileDecryptOperation *op;
  GList *files = NULL;
  gpa_file_item_t file_item;
  GtkTextIter begin;
  GtkTextIter end;

  gtk_text_buffer_get_bounds (clipboard->text_buffer, &begin, &end);

  file_item = g_malloc0 (sizeof (*file_item));
  file_item->direct_name = g_strdup (_("Clipboard"));
  file_item->direct_in
    = gtk_text_buffer_get_text (clipboard->text_buffer, &begin, &end, FALSE);
  /* FIXME: One would think there exists a function to get the number
     of bytes between two GtkTextIter, but no, that's too obvious.  */
  file_item->direct_in_len = strlen (file_item->direct_in);

  files = g_list_append (files, file_item);
  
  /* Start the operation.  */
  op = gpa_file_decrypt_operation_new (GTK_WIDGET (clipboard), files);
  
  register_operation (clipboard, GPA_FILE_OPERATION (op));
}


/* Handle menu item "File/Close".  */
static void
file_close (gpointer param)
{
  GpaClipboard *clipboard = param;
  gtk_widget_destroy (GTK_WIDGET (clipboard));
}


static void
edit_cut (gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view),
			 "cut-clipboard");
}


static void
edit_copy (gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view),
			 "copy-clipboard");
}


static void
edit_paste (gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view),
			 "paste-clipboard");
}


static void
edit_delete (gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view), "backspace");
}


/* Handle menu item "Edit/Select All".  */
static void
edit_select_all (gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view), "select-all");
}


/* Construct the file manager menu window and return that object. */
static GtkWidget *
clipboard_menu_new (GpaClipboard *clipboard)
{
  GtkItemFactory *factory;
  GtkItemFactoryEntry file_menu[] = {
    {_("/_File"), NULL, NULL, 0, "<Branch>"},
    {_("/File/C_lear"), NULL, file_clear, 0, "<StockItem>", GTK_STOCK_CLEAR},
    {_("/File/_Open"), NULL, file_open, 0, "<StockItem>", GTK_STOCK_OPEN},
    {_("/File/Save _As"), NULL, file_save_as, 0, "<StockItem>",
     GTK_STOCK_SAVE_AS},
    {_("/File/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/File/_Sign"), NULL, file_sign, 0, NULL},
    {_("/File/_Verify"), "<control>P", file_verify, 0, NULL},
    {_("/File/_Encrypt"), NULL, file_encrypt, 0, NULL},
    {_("/File/_Decrypt"), NULL, file_decrypt, 0, NULL},
    {_("/File/sep2"), NULL, NULL, 0, "<Separator>"},
    {_("/File/_Close"), NULL, file_close, 0, "<StockItem>", GTK_STOCK_CLOSE},
    {_("/File/_Quit"), NULL, gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT},
  };
  GtkItemFactoryEntry edit_menu[] = {
    {_("/_Edit"), NULL, NULL, 0, "<Branch>"},
#if 0
    /* Not implemented yet.  */
    {_("/Edit/_Undo"), NULL, edit_undo, 0, "<StockItem>", GTK_STOCK_UNDO },
    {_("/Edit/_Redo"), NULL, edit_redo, 0, "<StockItem>", GTK_STOCK_REDO },
    {_("/Edit/sep0"), NULL, NULL, 0, "<Separator>"},
#endif
    {_("/Edit/Cut"), NULL, edit_cut, 0, "<StockItem>", GTK_STOCK_CUT },
    {_("/Edit/_Copy"), NULL, edit_copy, 0, "<StockItem>", GTK_STOCK_COPY},
    {_("/Edit/_Paste"), NULL, edit_paste, 0, "<StockItem>", GTK_STOCK_PASTE},
    {_("/Edit/_Delete"), NULL, edit_delete, 0, "<StockItem>", GTK_STOCK_DELETE},
    {_("/Edit/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/Edit/Select _All"), "<control>A", edit_select_all, 0,
     "<StockItem>", GTK_STOCK_SELECT_ALL },
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
				 file_menu, clipboard);
  gtk_item_factory_create_items (factory,
				 sizeof (edit_menu) / sizeof (edit_menu[0]),
				 edit_menu, clipboard);
  gtk_item_factory_create_items (factory,
				 sizeof(windows_menu) /sizeof(windows_menu[0]),
				 windows_menu, clipboard);

  /* Disable buttons when no file is selected.   */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Edit/Cut"));
  if (item)
    {
      gtk_widget_set_sensitive (item, has_selection (clipboard));
      add_selection_sensitive_widget (clipboard, item, has_selection);
    }

  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Edit/Copy"));
  if (item)
    {
      gtk_widget_set_sensitive (item, has_selection (clipboard));
      add_selection_sensitive_widget (clipboard, item, has_selection);
    }
  
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Edit/Delete"));
  if (item)
    {
      gtk_widget_set_sensitive (item, has_selection (clipboard));
      add_selection_sensitive_widget (clipboard, item, has_selection);
    }

  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                      _("/Edit/Paste"));
  if (item)
    /* Initialized later.  */
    add_paste_sensitive_widget (clipboard, item);

  gpa_help_menu_add_to_factory (factory, GTK_WIDGET (clipboard));
  gtk_window_add_accel_group (GTK_WINDOW (clipboard), accel_group);

  return gtk_item_factory_get_widget (factory, "<main>");
}



/* Toolbar actions.  */

static void
toolbar_file_clear (GtkWidget *widget, gpointer param)
{
  file_clear (param);
}


static void
toolbar_file_open (GtkWidget *widget, gpointer param)
{
  file_open (param);
}


static void
toolbar_file_save_as (GtkWidget *widget, gpointer param)
{
  file_save_as (param);
}


static void
toolbar_file_sign (GtkWidget *widget, gpointer param)
{
  file_sign (param);
}


static void
toolbar_file_verify (GtkWidget *widget, gpointer param)
{
  file_verify (param);
}


static void
toolbar_file_encrypt (GtkWidget *widget, gpointer param)
{
  file_encrypt (param);
}


static void
toolbar_file_decrypt (GtkWidget *widget, gpointer param)
{
  file_decrypt (param);
}


static void
toolbar_edit_cut (GtkWidget *widget, gpointer param)
{
  edit_cut (param);
}


static void
toolbar_edit_copy (GtkWidget *widget, gpointer param)
{
  edit_copy (param);
}


static void
toolbar_edit_paste (GtkWidget *widget, gpointer param)
{
  edit_paste (param);
}


static void
toolbar_edit_preferences (GtkWidget *widget, gpointer param)
{
  gpa_open_settings_dialog ();
}


/* Construct the new toolbar object and return it.  Takes the file
   manage object.  */
static GtkWidget *
clipboard_toolbar_new (GpaClipboard *clipboard)
{
  GtkWidget *toolbar, *icon, *item;

  toolbar = gtk_toolbar_new ();

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_CLEAR,
                            _("Clear buffer"), _("clear buffer"),
                            GTK_SIGNAL_FUNC (toolbar_file_clear),
                            clipboard, -1);

  /* Disabled because the toolbar arrow mode doesn't work, and the
     toolbar takes up too much space otherwise.  */
#if 0
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_OPEN,
                            _("Open a file"), _("open file"),
                            GTK_SIGNAL_FUNC (toolbar_file_open),
                            clipboard, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_SAVE_AS,
                            _("Save to a file"), _("save file as"),
                            GTK_SIGNAL_FUNC (toolbar_file_save_as),
                            clipboard, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
#endif

  item = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_CUT,
				   _("Cut the selection"),
				   _("cut the selection"),
				   GTK_SIGNAL_FUNC (toolbar_edit_cut),
				   clipboard, -1);
  add_selection_sensitive_widget (clipboard, item, has_selection);

  item = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_COPY,
				   _("Copy the selection"),
				   _("copy the selection"),
				   GTK_SIGNAL_FUNC (toolbar_edit_copy),
				   clipboard, -1);
  add_selection_sensitive_widget (clipboard, item, has_selection);

  item = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_PASTE,
				   _("Paste the clipboard"),
				   _("paste the clipboard"),
				   GTK_SIGNAL_FUNC (toolbar_edit_paste),
				   clipboard, -1);
  add_paste_sensitive_widget (clipboard, item);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  /* Build the "Sign" button.  */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (clipboard), "sign")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Sign"),
				      _("Sign the selected file"),
                                      _("sign file"),
				      icon,
				      GTK_SIGNAL_FUNC (toolbar_file_sign),
				      clipboard);
    }
  /* Build the "Verify" button.  */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (clipboard), "verify")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Verify"),
				      _("Check signatures of selected file"), 
				      _("verify file"),
                                      icon,
				      GTK_SIGNAL_FUNC (toolbar_file_verify), 
				      clipboard);
    }
  /* Build the "Encrypt" button.  */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (clipboard), "encrypt")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Encrypt"),
				      _("Encrypt the selected file"),
				      _("encrypt file"),
				      icon,
                                      GTK_SIGNAL_FUNC (toolbar_file_encrypt),
				      clipboard);
    }
  /* Build the "Decrypt" button.  */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (clipboard), "decrypt")))
    {
      item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Decrypt"),
				      _("Decrypt the selected file"),
				      _("decrypt file"),
				      icon, 
                                      GTK_SIGNAL_FUNC (toolbar_file_decrypt),
				      clipboard);
    }

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
  
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), 
                            GTK_STOCK_PREFERENCES,
                            _("Open the preferences dialog"),
                            _("preferences"),
                            GTK_SIGNAL_FUNC (toolbar_edit_preferences),
                            clipboard, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  icon = gpa_create_icon_widget (GTK_WIDGET (clipboard), "keyringeditor");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Keyring"),
                                  _("Open the keyring editor"),
                                  _("keyring editor"), icon,
				  GTK_SIGNAL_FUNC (gpa_open_keyring_editor),
                                  NULL);

  icon = gtk_image_new_from_stock ("gtk-directory",
				   GTK_ICON_SIZE_SMALL_TOOLBAR);
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Files"),
				  _("Open the file manager"),
				  _("file manager"), icon,
				  GTK_SIGNAL_FUNC (gpa_open_filemanager),
				  NULL);

#if 0  /* FIXME: Help is not available yet. :-( */
  /* Help */
  if ((icon = gpa_create_icon_widget (GTK_WIDGET (clipboard), "help")))
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Help"),
			     _("Understanding the GNU Privacy Assistant"),
			     _("help"), icon,
			     GTK_SIGNAL_FUNC (help_help), NULL);
#endif

  gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), TRUE);

  return toolbar;
}


/* Callback for the destroy signal.  */
static void
clipboard_closed (GtkWidget *widget, gpointer param)
{
  instance = NULL;
}


/* Construct the clipboard text.  */
static GtkWidget *
clipboard_text_new (GpaClipboard *clipboard)
{
  GtkWidget *scroller;

  clipboard->text_view = gtk_text_view_new ();
  gtk_widget_grab_focus (clipboard->text_view);

  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (clipboard->text_view), 4);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (clipboard->text_view), 4);

  clipboard->text_buffer
    = gtk_text_view_get_buffer (GTK_TEXT_VIEW (clipboard->text_view));

#ifndef MY_GTK_TEXT_BUFFER_NO_HAS_SELECTION
  /* A change in selection status causes a property change, which we
     can listen in on.  */
  g_signal_connect_swapped (clipboard->text_buffer, "notify::has-selection",
			    G_CALLBACK (update_selection_sensitive_widgets),
			    clipboard);
#else
  /* Runs very often.  The changed signal is necessary for backspace
     actions.  */
  g_signal_connect_swapped (clipboard->text_buffer, "mark-set",
			    G_CALLBACK (update_selection_sensitive_widgets),
			    clipboard);
  g_signal_connect_after (clipboard->text_buffer, "backspace",
			    G_CALLBACK (update_selection_sensitive_widgets2),
			    clipboard);
#endif

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scroller),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
				       GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (scroller), clipboard->text_view);

  return scroller;
} 


/* Construct a new class object of GpaClipboard.  */
static GObject*
gpa_clipboard_constructor (GType type,
			   guint n_construct_properties,
			   GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaClipboard *clipboard;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *icon;
  GtkWidget *label;
  gchar *markup;
  GtkWidget *menubar;
  GtkWidget *text_box;
  GtkWidget *text_frame;
  GtkWidget *toolbar;
  GtkWidget *align;
  guint pt, pb, pl, pr;

  /* Invoke parent's constructor.  */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  clipboard = GPA_CLIPBOARD (object);

  /* Initialize.  */
  gtk_window_set_title (GTK_WINDOW (clipboard),
			_("GNU Privacy Assistant - Clipboard"));
  gtk_window_set_default_size (GTK_WINDOW (clipboard), 640, 480);

  /* Realize the window so that we can create pixmaps without warnings
     and also access the clipboard.  */
  gtk_widget_realize (GTK_WIDGET (clipboard));

  /* Use a vbox to show the menu, toolbar and the text container.  */
  vbox = gtk_vbox_new (FALSE, 0);

  /* First comes the menu.  */
  menubar = clipboard_menu_new (clipboard);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);

  /* Second the toolbar.  */
  toolbar = clipboard_toolbar_new (clipboard);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);


  /* Add a fancy label that tells us: This is the clipboard.  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);
  
  /* FIXME: Need better icon.  */
  icon = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_DND);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);

  label = gtk_label_new (NULL);
  markup = g_strdup_printf ("<span font_desc=\"16\">%s</span>",
                            _("Clipboard"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 10);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);


  /* Third a text entry.  */
  text_box = gtk_hbox_new (TRUE, 0);
  align = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_alignment_get_padding (GTK_ALIGNMENT (align),
			     &pt, &pb, &pl, &pr);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), pt, pb + 5,
			     pl + 5, pr + 5);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);

  text_frame = clipboard_text_new (clipboard);
  gtk_box_pack_start (GTK_BOX (text_box), text_frame, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (align), text_box);
  gtk_box_pack_end (GTK_BOX (vbox), align, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (clipboard), vbox);

  g_signal_connect (object, "destroy",
                    G_CALLBACK (clipboard_closed), object);

  /* Update the sensitivity of paste items.  */
  {
    GtkClipboard *clip;
    
    /* Do this once for all paste sensitive items.  Note that the
       window is realized already.  */
    clip = gtk_widget_get_clipboard (GTK_WIDGET (clipboard),
				     GDK_SELECTION_CLIPBOARD);
    g_signal_connect (clip, "owner_change",
		      G_CALLBACK (clipboard_owner_change_cb), clipboard);
    set_paste_sensitivity (clipboard, clip);
  }

  return object;
}



static GpaClipboard *
gpa_clipboard_new ()
{
  GpaClipboard *clipboard;

  clipboard = g_object_new (GPA_CLIPBOARD_TYPE, NULL);  
  // FIXME  update_selection_sensitive_widgets (clipboard);

  return clipboard;
}


/* API */

GtkWidget *
gpa_clipboard_get_instance (void)
{
  if (!instance)
    {
      instance = gpa_clipboard_new ();
    }
  return GTK_WIDGET (instance);
}

gboolean gpa_clipboard_is_open (void)
{
  return (instance != NULL);
}
