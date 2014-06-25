/* clipboard.c  -  The GNU Privacy Assistant
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
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "gpa.h"

#include "gtktools.h"
#include "gpgmetools.h"
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
#define GTK_STOCK_SELECT_ALL "gtk-select-all"
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
  GList *selection_sensitive_actions;
  GList *paste_sensitive_actions;
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


/* GtkWidget boilerplate.  */
static void
gpa_clipboard_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_clipboard_init (GpaClipboard *clipboard)
{
  clipboard->selection_sensitive_actions = NULL;
  clipboard->paste_sensitive_actions = NULL;
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
add_selection_sensitive_action (GpaClipboard *clipboard,
                                GtkAction *action)
{
  clipboard->selection_sensitive_actions
    = g_list_append (clipboard->selection_sensitive_actions, action);
}


/* Return true if a selection is active.  */
static gboolean
has_selection (gpointer param)
{
  GpaClipboard *clipboard = param;

  return gtk_text_buffer_get_has_selection (clipboard->text_buffer);
}


/* Update the sensitivity of the widget DATA and pass PARAM through to
   the sensitivity callback. Usable as an iterator function in
   g_list_foreach. */
static void
update_selection_sensitive_action (gpointer data, gpointer param)
{
  gtk_action_set_sensitive (GTK_ACTION (data), has_selection (param));
}


/* Call update_selection_sensitive_widget for all widgets in the list
   of sensitive widgets and pass CLIPBOARD through as the user data
   parameter.  */
static void
update_selection_sensitive_actions (GpaClipboard *clipboard)
{
  g_list_foreach (clipboard->selection_sensitive_actions,
                  update_selection_sensitive_action,
                  (gpointer) clipboard);
}


/* Add ACTION to the list of sensitive actions of CLIPBOARD.  */
static void
add_paste_sensitive_action (GpaClipboard *clipboard, GtkAction *action)
{
  clipboard->paste_sensitive_actions
    = g_list_append (clipboard->paste_sensitive_actions, action);
}


static void
update_paste_sensitive_action (gpointer data, gpointer param)
{
  GpaClipboard *clipboard = param;

  gtk_action_set_sensitive (GTK_ACTION (data), clipboard->paste_p);
}


static void
update_paste_sensitive_actions (GtkClipboard *clip,
				GtkSelectionData *selection_data,
				GpaClipboard *clipboard)
{
  clipboard->paste_p = gtk_selection_data_targets_include_text (selection_data);

  g_list_foreach (clipboard->paste_sensitive_actions,
                  update_paste_sensitive_action, (gpointer) clipboard);
}


static void
set_paste_sensitivity (GpaClipboard *clipboard, GtkClipboard *clip)
{
  GdkDisplay *display;

  display = gtk_clipboard_get_display (clip);

  if (gdk_display_supports_selection_notification (display))
    gtk_clipboard_request_contents
      (clip, gdk_atom_intern_static_string ("TARGETS"),
       (GtkClipboardReceivedFunc) update_paste_sensitive_actions,
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
      gsize len;

      str = g_strdup_printf ("No valid UTF-8 encoding at position %i.\n"
                             "Assuming Latin-1 encoding instead.",
			     ((int) (end - item->direct_out)));
      gpa_window_message (str, GTK_WIDGET (clipboard));
      g_free (str);

      str = g_convert (item->direct_out, item->direct_out_len,
                       "UTF-8", "ISO-8859-1",
                       NULL, &len, NULL);
      if (str)
        {
          gtk_text_buffer_set_text (clipboard->text_buffer, str, len);
          g_free (str);
          return;
        }
      gpa_window_error ("Error converting Latin-1 to UTF-8",
                        GTK_WIDGET (clipboard));
      /* Enough warnings: Try to show even with invalid encoding.  */
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
file_clear (GtkAction *action, gpointer param)
{
  GpaClipboard *clipboard = param;

  gtk_text_buffer_set_text (clipboard->text_buffer, "", -1);
}


/* The directory last visited by load or save operations.  */
static gchar *last_directory;


static gchar *
get_load_file_name (GtkWidget *parent, const gchar *title)
{
  static GtkWidget *dialog;
  GtkResponseType response;
  gchar *filename = NULL;

  if (! dialog)
    {
      dialog = gtk_file_chooser_dialog_new
	(title, GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_OPEN,
	 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	 GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    }
  if (last_directory)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
					 last_directory);
  gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (dialog));

  /* Run the dialog until there is a valid response.  */
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_OK)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      if (filename)
	filename = g_strdup (filename);
    }

  if (last_directory)
    g_free (last_directory);
  last_directory
    = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
  if (last_directory)
    last_directory = g_strdup (last_directory);

  gtk_widget_hide (dialog);

  return filename;
}


/* Handle menu item "File/Open".  */
static void
file_open (GtkAction *action, gpointer param)
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

  filename = get_load_file_name (GTK_WIDGET (clipboard), _("Open File"));
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

      /* TRANSLATORS: The arguments are the filename, the integer size
	 and the unit (such as KB or MB).  */
      str = g_strdup_printf (_("The file %s is %llu%s large.  Do you really "
			       " want to open it?"), filename,
                             (unsigned long long)buf.st_size / 1024 / 1024,
                             "MB");
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


/* Run the modal file selection dialog and return a new copy of the
   filename if the user pressed OK and NULL otherwise.  */
static gchar *
get_save_file_name (GtkWidget *parent, const gchar *title)
{
  static GtkWidget *dialog;
  GtkResponseType response;
  gchar *filename = NULL;

  if (! dialog)
    {
      dialog = gtk_file_chooser_dialog_new
	(title, GTK_WINDOW (parent), GTK_FILE_CHOOSER_ACTION_SAVE,
	 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	 GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      gtk_file_chooser_set_do_overwrite_confirmation
	(GTK_FILE_CHOOSER (dialog), TRUE);
    }
  if (last_directory)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
					 last_directory);
  gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (dialog));

  /* Run the dialog until there is a valid response.  */
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_OK)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      if (filename)
	filename = g_strdup (filename);
    }

  if (last_directory)
    g_free (last_directory);
  last_directory
    = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
  if (last_directory)
    last_directory = g_strdup (last_directory);

  if (last_directory)
    g_free (last_directory);
  last_directory
    = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
  if (last_directory)
    last_directory = g_strdup (last_directory);

  gtk_widget_hide (dialog);

  return filename;
}


/* Handle menu item "File/Save As...".  */
static void
file_save_as (GtkAction *action, gpointer param)
{
  GpaClipboard *clipboard = param;
  gchar *filename;
  GError *err = NULL;
  gchar *contents;
  gssize length;
  gboolean suc;
  GtkTextIter begin;
  GtkTextIter end;

  filename = get_save_file_name (GTK_WIDGET (clipboard), _("Save As..."));
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
file_verify (GtkAction *action, gpointer param)
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
file_sign (GtkAction *action, gpointer param)
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
file_encrypt (GtkAction *action, gpointer param)
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
file_decrypt (GtkAction *action, gpointer param)
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
file_close (GtkAction *action, gpointer param)
{
  GpaClipboard *clipboard = param;
  gtk_widget_destroy (GTK_WIDGET (clipboard));
}


static void
edit_cut (GtkAction *action, gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view),
			 "cut-clipboard");
}


static void
edit_copy (GtkAction *action, gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view),
			 "copy-clipboard");
}


static void
edit_paste (GtkAction *action, gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view),
			 "paste-clipboard");
}


static void
edit_delete (GtkAction *action, gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view), "backspace");
}


/* Handle menu item "Edit/Select All".  */
static void
edit_select_all (GtkAction *action, gpointer param)
{
  GpaClipboard *clipboard = param;

  g_signal_emit_by_name (GTK_TEXT_VIEW (clipboard->text_view), "select-all");
}


/* Construct the file manager menu window and return that object. */
static void
clipboard_action_new (GpaClipboard *clipboard,
		      GtkWidget **menu, GtkWidget **toolbar)
{
  static const GtkActionEntry entries[] =
    {
      /* Toplevel.  */
      { "File", NULL, N_("_File"), NULL },
      { "Edit", NULL, N_("_Edit"), NULL },

      /* File menu.  */
      { "FileClear", GTK_STOCK_CLEAR, NULL, NULL,
	N_("Clear buffer"), G_CALLBACK (file_clear) },
      { "FileOpen", GTK_STOCK_OPEN, NULL, NULL,
	N_("Open a file"), G_CALLBACK (file_open) },
      { "FileSaveAs", GTK_STOCK_SAVE_AS, NULL, NULL,
	N_("Save to a file"), G_CALLBACK (file_save_as) },
      { "FileSign", GPA_STOCK_SIGN, NULL, NULL,
	N_("Sign buffer text"), G_CALLBACK (file_sign) },
      { "FileVerify", GPA_STOCK_VERIFY, NULL, NULL,
	N_("Check signatures of buffer text"), G_CALLBACK (file_verify) },
      { "FileEncrypt", GPA_STOCK_ENCRYPT, NULL, NULL,
	N_("Encrypt the buffer text"), G_CALLBACK (file_encrypt) },
      { "FileDecrypt", GPA_STOCK_DECRYPT, NULL, NULL,
	N_("Decrypt the buffer text"), G_CALLBACK (file_decrypt) },
      { "FileClose", GTK_STOCK_CLOSE, NULL, NULL,
	N_("Close the buffer"), G_CALLBACK (file_close) },
      { "FileQuit", GTK_STOCK_QUIT, NULL, NULL,
	N_("Quit the program"), G_CALLBACK (gtk_main_quit) },

      /* Edit menu.  */
#if 0
      /* FIXME: Not implemented yet.  */
      { "EditUndo", GTK_STOCK_UNDO, NULL, NULL,
	N_("Undo the last action"), G_CALLBACK (edit_undo) },
      { "EditRedo", GTK_STOCK_REDO, NULL, NULL,
	N_("Redo the last undone action"), G_CALLBACK (edit_redo) },
#endif
      { "EditCut", GTK_STOCK_CUT, NULL, NULL,
	N_("Cut the selection"), G_CALLBACK (edit_cut) },
      { "EditCopy", GTK_STOCK_COPY, NULL, NULL,
	N_("Copy the selection"), G_CALLBACK (edit_copy) },
      { "EditPaste", GTK_STOCK_PASTE, NULL, NULL,
	N_("Paste the clipboard"), G_CALLBACK (edit_paste) },
      { "EditDelete", GTK_STOCK_DELETE, NULL, NULL,
	N_("Delete the selected text"), G_CALLBACK (edit_delete) },
      { "EditSelectAll", GTK_STOCK_SELECT_ALL, NULL, "<control>A",
	N_("Select the entire document"), G_CALLBACK (edit_select_all) }
    };

  static const char *ui_description =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='File'>"
    "      <menuitem action='FileClear'/>"
    "      <menuitem action='FileOpen'/>"
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
#if 0
    /* Not implemented yet.  */
    "      <menuitem action='EditUndo'/>"
    "      <menuitem action='EditRedo'/>"
    "      <separator/>"
#endif
    "      <menuitem action='EditCut'/>"
    "      <menuitem action='EditCopy'/>"
    "      <menuitem action='EditPaste'/>"
    "      <menuitem action='EditDelete'/>"
    "      <separator/>"
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
    "    <toolitem action='FileClear'/>"
#if 0
    /* Disabled because the toolbar arrow mode doesn't work, and the
       toolbar takes up too much space otherwise.  */
    "    <toolitem action='FileOpen'/>"
    "    <toolitem action='FileSaveAs'/>"
#endif
    "    <separator/>"
    "    <toolitem action='EditCut'/>"
    "    <toolitem action='EditCopy'/>"
    "    <toolitem action='EditPaste'/>"
    "    <separator/>"
    "    <toolitem action='FileSign'/>"
    "    <toolitem action='FileVerify'/>"
    "    <toolitem action='FileEncrypt'/>"
    "    <toolitem action='FileDecrypt'/>"
    "    <separator/>"
    "    <toolitem action='EditPreferences'/>"
    "    <separator/>"
    "    <toolitem action='WindowsKeyringEditor'/>"
    "    <toolitem action='WindowsFileManager'/>"
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
				clipboard);
  gtk_action_group_add_actions (action_group, gpa_help_menu_action_entries,
				G_N_ELEMENTS (gpa_help_menu_action_entries),
				clipboard);
  gtk_action_group_add_actions (action_group, gpa_windows_menu_action_entries,
				G_N_ELEMENTS (gpa_windows_menu_action_entries),
				clipboard);
  gtk_action_group_add_actions
    (action_group, gpa_preferences_menu_action_entries,
     G_N_ELEMENTS (gpa_preferences_menu_action_entries), clipboard);
  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (clipboard), accel_group);
  if (! gtk_ui_manager_add_ui_from_string (ui_manager, ui_description,
					   -1, &error))
    {
      g_message ("building clipboard menus failed: %s", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }

  /* Fixup the icon theme labels which are too long for the toolbar.  */
  action = gtk_action_group_get_action (action_group, "WindowsKeyringEditor");
  g_object_set (action, "short_label", _("Keyring"), NULL);
  action = gtk_action_group_get_action (action_group, "WindowsFileManager");
  g_object_set (action, "short_label", _("Files"), NULL);
#ifdef ENABLE_CARD_MANAGER
  action = gtk_action_group_get_action (action_group, "WindowsCardManager");
  g_object_set (action, "short_label", _("Card"), NULL);
#endif

  /* Take care of sensitiveness of widgets.  */
  action = gtk_action_group_get_action (action_group, "EditCut");
  add_selection_sensitive_action (clipboard, action);
  action = gtk_action_group_get_action (action_group, "EditCopy");
  add_selection_sensitive_action (clipboard, action);
  action = gtk_action_group_get_action (action_group, "EditDelete");
  add_selection_sensitive_action (clipboard, action);

  action = gtk_action_group_get_action (action_group, "EditPaste");
  /* Initialized later.  */
  add_paste_sensitive_action (clipboard, action);

  *menu = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  *toolbar = gtk_ui_manager_get_widget (ui_manager, "/ToolBar");
  gpa_toolbar_set_homogeneous (GTK_TOOLBAR (*toolbar), FALSE);
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
			    G_CALLBACK (update_selection_sensitive_actions),
			    clipboard);
#else
  /* Runs very often.  The changed signal is necessary for backspace
     actions.  */
  g_signal_connect_swapped (clipboard->text_buffer, "mark-set",
			    G_CALLBACK (update_selection_sensitive_actions),
			    clipboard);
  g_signal_connect_after (clipboard->text_buffer, "backspace",
			    G_CALLBACK (update_selection_sensitive_actions),
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
static GObject *
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
  gpa_window_set_title (GTK_WINDOW (clipboard), _("Clipboard"));
  gtk_window_set_default_size (GTK_WINDOW (clipboard), 640, 480);

  /* Realize the window so that we can create pixmaps without warnings
     and also access the clipboard.  */
  gtk_widget_realize (GTK_WIDGET (clipboard));

  /* Use a vbox to show the menu, toolbar and the text container.  */
  vbox = gtk_vbox_new (FALSE, 0);

  /* We need to create the text_buffer before we create the menus and
     the toolbar, because of widget sensitivity issues, which depend
     on the selection status of the text_buffer.  */
  text_frame = clipboard_text_new (clipboard);

  /* Get the menu and the toolbar.  */
  clipboard_action_new (clipboard, &menubar, &toolbar);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);

  /* Add a fancy label that tells us: This is the clipboard.  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);

  /* FIXME: Need better icon.  */
  icon = gtk_image_new_from_stock (GTK_STOCK_PASTE, GTK_ICON_SIZE_DND);
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

  gtk_box_pack_start (GTK_BOX (text_box), text_frame, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (align), text_box);

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

  /* Update the sensitivity of selection items.  */
  update_selection_sensitive_actions (clipboard);

  return object;
}



static GpaClipboard *
gpa_clipboard_new ()
{
  GpaClipboard *clipboard;

  clipboard = g_object_new (GPA_CLIPBOARD_TYPE, NULL);

  return clipboard;
}


/* API */

GtkWidget *
gpa_clipboard_get_instance (void)
{
  if (! instance)
    instance = gpa_clipboard_new ();

  return GTK_WIDGET (instance);
}

gboolean
gpa_clipboard_is_open (void)
{
  return (instance != NULL);
}
