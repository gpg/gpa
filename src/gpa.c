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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gpapa.h>
#include "gpa.h"
#include "gtktools.h"
#include "filemenu.h"
#include "keysmenu.h"
#include "optionsmenu.h"
#include "helpmenu.h"

#include <stdio.h> /*!!!*/

static GtkWidget *global_clistFile = NULL;
GtkWidget *global_windowMain;
GtkWidget *global_popupMenu;
GtkWidget *global_windowTip;
GtkWidget *global_textTip;
gboolean global_noTips = FALSE;
GpapaAction global_lastCallbackResult;

gchar *writtenSigValidity [ 3 ] = {
  N_( "unknown" ),
  N_( "!INVALID!" ),
  N_( "valid" )
};

gchar *getStringForSigValidity ( GpapaSigValidity sigValidity ) {
  return writtenSigValidity [ sigValidity ];
} /* getStringForSigValidity */

void gpa_callback (
  GpapaAction action, gpointer actiondata, gpointer calldata
) {
  switch ( action ) {
    case GPAPA_ACTION_ERROR:
      gpa_window_error ( (gchar*) actiondata, (GtkWidget*) calldata );
      break;
    default: break;
  } /* switch */
  global_lastCallbackResult = action;
} /* gpa_callback */

GtkWidget *gpa_get_global_clist_file ( void ) {
  return ( global_clistFile );
} /* gpa_get_global_clist_file */

void gpa_switch_tips ( void ) {
  if ( global_noTips == TRUE )
    global_noTips = FALSE;
  else
    global_noTips = TRUE;
} /* gpa_switch_tips */

void gpa_windowTip_init ( void ) {
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
  global_windowTip = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( global_windowTip ), _( "GPA Tip" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( global_windowTip ), accelGroup );
  vboxTip = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxTip ), 5 );
  vboxContents = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxContents ), 5 );
  labelContents = gtk_label_new ( _( "" ) );
  labelJfdContents = gpa_widget_hjustified_new (
    labelContents, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxContents ), labelJfdContents, FALSE, FALSE, 0
  );
  hboxContents = gtk_hbox_new ( FALSE, 0 );
  textContents = gtk_text_new ( NULL, NULL );
  gtk_text_set_editable ( GTK_TEXT ( textContents ), FALSE );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelContents ), textContents, accelGroup, _( "_Tip:" )
  );
  global_textTip = textContents;
  gtk_box_pack_start (
    GTK_BOX ( hboxContents ), textContents, TRUE, TRUE, 0
  );
  vscrollbarContents = gtk_vscrollbar_new (
    GTK_TEXT ( textContents ) -> vadj
  );
  gtk_box_pack_start (
    GTK_BOX ( hboxContents ), vscrollbarContents, FALSE, FALSE, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxContents ), hboxContents, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxTip ), vboxContents, TRUE, TRUE, 0 );
  hboxTip = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxTip ), 5 );
  buttonClose = gpa_button_new ( accelGroup, _( "   _Close   " ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonClose ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_hide ), (gpointer) global_windowTip
  );
  gtk_widget_add_accelerator (
    buttonClose, "clicked", accelGroup, GDK_Escape, 0, 0
  );
  gtk_box_pack_end ( GTK_BOX ( hboxTip ), buttonClose, FALSE, FALSE, 0 );
  checkerNomore = gpa_check_button_new (
    accelGroup, _( "_No more tips, please" )
  );
  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON ( checkerNomore ), global_noTips
  );
  gtk_signal_connect (
    GTK_OBJECT ( checkerNomore ), "clicked",
    GTK_SIGNAL_FUNC ( gpa_switch_tips ), NULL
  );
  gtk_box_pack_end (
    GTK_BOX ( hboxTip ), checkerNomore, FALSE, FALSE, 10
  );
  gtk_box_pack_start ( GTK_BOX ( vboxTip ), hboxTip, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( global_windowTip ), vboxTip );
} /* gpa_windowTip_init */

void gpa_windowTip_show ( gchar *text ) {
  if ( ! global_noTips )
    {
      gtk_editable_delete_text ( GTK_EDITABLE ( global_textTip ), 0, -1 );
      gtk_text_insert (
	GTK_TEXT ( global_textTip ), NULL, &global_textTip -> style -> black,
	NULL, text, -1
      );
      gtk_widget_show_all ( global_windowTip );
    } /* if */
} /* gpa_windowTip_show */

void sigs_append ( gpointer data, gpointer userData ) {
/* var */
  GpapaSignature *signature;
  gpointer *localParam;
  GtkWidget *clistSignatures;
  GtkWidget *windowPublic;
  gchar *contentsSignatures [ 3 ];
/* commands */
  signature = (GpapaSignature*) data;
  localParam = (gpointer*) userData;
  clistSignatures = (GtkWidget*) localParam [ 0 ];
  windowPublic =    (GtkWidget*) localParam [ 1 ];
  contentsSignatures [ 0 ] = gpapa_signature_get_name (
    signature, gpa_callback, windowPublic
  );
  if ( ! contentsSignatures [ 0 ] )
    contentsSignatures [ 0 ] = _( "[Unknown user ID]" );
  contentsSignatures [ 1 ] = getStringForSigValidity (
    gpapa_signature_get_validity ( signature, gpa_callback, windowPublic )
  );
  contentsSignatures [ 2 ] = gpapa_signature_get_identifier (
    signature, gpa_callback, windowPublic
  );
  gtk_clist_append ( GTK_CLIST ( clistSignatures ), contentsSignatures );
} /* sigs_append */

gint compareInts ( gconstpointer a, gconstpointer b ) {
/* var */
  gint aInt, bInt;
/* commands */
  aInt = *(gint*) a;
  bInt = *(gint*) b;
  if ( aInt > bInt )
    return 1;
  else if ( aInt < bInt )
    return -1;
  else
    return 0;
} /* compareInts */

void printRow ( gpointer data, gpointer userData ) { /*!!!*/
printf ( "%d ", *(gint*) data ); /*!!!*/
} /*!!!*/

void gpa_selectRecipient (
  GtkCList *clist, gint row, gint column, GdkEventButton *event,
  gpointer userData
) {
/* var */
  GList **recipientsSelected;
  gint *rowData;
/* commands */
  recipientsSelected = (GList**) userData;
  rowData = (gint*) xmalloc ( sizeof ( gint ) );
  *rowData = row;
  *recipientsSelected = g_list_insert_sorted (
    *recipientsSelected, rowData, compareInts
  );
g_print ( ":: " ); /*!!!*/
g_list_foreach ( *recipientsSelected, printRow, NULL ); /*!!!*/
g_print ( "\n" ); /*!!!*/
} /* gpa_selectRecipient */

void gpa_unselectRecipient (
  GtkCList *clist, gint row, gint column, GdkEventButton *event,
  gpointer userData
) {
/* var */
  GList **recipientsSelected;
  GList *indexRecipient, *indexNext;
  gpointer data;
/* commands */
  recipientsSelected = (GList**) userData;
  indexRecipient = g_list_first ( *recipientsSelected );
  while ( indexRecipient )
    {
      indexNext = g_list_next ( indexRecipient );
      if ( *(gint*) indexRecipient -> data == row )
	{
	  data = indexRecipient -> data;
	  *recipientsSelected = g_list_remove ( *recipientsSelected, data );
	  free ( data );
	} /* if */
      indexRecipient = indexNext;
    } /* while */
g_print ( ":: " ); /*!!!*/
g_list_foreach ( *recipientsSelected, printRow, NULL ); /*!!!*/
g_print ( "\n" ); /*!!!*/
} /* gpa_unselectRecipient */

void gpa_removeRecipients ( gpointer param ) {
/* var */
  gpointer *localParam;
  GList **recipientsSelected;
  GtkWidget *clistRecipients;
  GtkWidget *windowEncrypt;
  GList *indexRecipient;
/* commands */
  localParam = (gpointer*) param;
  recipientsSelected = (GList**) localParam [ 0 ];
  clistRecipients = (GtkWidget*) localParam [ 1 ];
  windowEncrypt =   (GtkWidget*) localParam [ 2 ];
  if ( ! *recipientsSelected )
    {
      gpa_window_error (
	_( "No files selected to remove from recipients list" ), windowEncrypt
      );
      return;
    } /* if */
  indexRecipient = g_list_last ( *recipientsSelected );
  while ( indexRecipient )
    {
      gtk_clist_remove (
	GTK_CLIST ( clistRecipients ), *(gint*) indexRecipient -> data
      );
      free ( indexRecipient -> data );
      indexRecipient = g_list_previous ( indexRecipient );
    } /* while */
  g_list_free ( *recipientsSelected );
} /* gpa_removeRecipients */

void gpa_addRecipient ( gpointer data, gpointer userData ) {
/* var */
  gpointer *localParam;
  GpapaPublicKey *key;
  GtkWidget *clistRecipients;
  GtkWidget *windowEncrypt;
  gchar *contentsRecipients [ 2 ];
/* commands */
  localParam = (gpointer*) userData;
  clistRecipients = (GtkWidget*) localParam [ 0 ];
  windowEncrypt =   (GtkWidget*) localParam [ 1 ];
  key = gpapa_get_public_key_by_ID (
    (gchar*) data, gpa_callback, windowEncrypt
  );
  contentsRecipients [ 0 ] = gpapa_key_get_name (
    GPAPA_KEY ( key ), gpa_callback, windowEncrypt
  );
  contentsRecipients [ 1 ] = gpapa_key_get_identifier (
    GPAPA_KEY ( key ), gpa_callback, windowEncrypt
  );
  gtk_clist_append ( GTK_CLIST ( clistRecipients ), contentsRecipients );
} /* gpa_addRecipient */

void gpa_addRecipients ( gpointer param ) {
/* var */
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *clistKeys;
  GtkWidget *clistRecipients;
  GtkWidget *windowEncrypt;
  gpointer newParam [ 2 ];
/* commands */
  localParam = (gpointer*) param;
  keysSelected =       (GList**) localParam [ 0 ];
  clistKeys =       (GtkWidget*) localParam [ 1 ];
  clistRecipients = (GtkWidget*) localParam [ 2 ];
  windowEncrypt =   (GtkWidget*) localParam [ 3 ];
  if ( ! *keysSelected )
    {
      gpa_window_error (
	_( "No keys selected to add to recipients list." ), windowEncrypt
      );
      return;
    } /* if */
  newParam [ 0 ] = clistRecipients;
  newParam [ 1 ] = windowEncrypt;
  g_list_foreach (
    *keysSelected, gpa_addRecipient, (gpointer) newParam
  );
  gtk_clist_unselect_all ( GTK_CLIST ( clistKeys ) );
} /* gpa_addRecipients */

void freeRowData ( gpointer data, gpointer userData ) {
  free ( data );
} /* freeRowData */

void gpa_recipientWindow_close ( gpointer param ) {
/* var */
  gpointer *localParam;
  GList **recipientsSelected;
  GList **keysSelected;
  GtkWidget *windowEncrypt;
  static gpointer paramClose [ 2 ];
/* commands */
  localParam = (gpointer*) param;
  recipientsSelected = (GList**) localParam [ 0 ];
  keysSelected =       (GList**) localParam [ 1 ];
  windowEncrypt =   (GtkWidget*) localParam [ 2 ];
  g_list_foreach ( *recipientsSelected, freeRowData, NULL );
  g_list_free ( *recipientsSelected );
  g_list_free ( *keysSelected );
  paramClose [ 0 ] = windowEncrypt;
  paramClose [ 1 ] = NULL;
  gpa_window_destroy ( paramClose );
} /* gpa_recipientWindow_close */

gint delete_event ( GtkWidget *widget, GdkEvent *event, gpointer data ) {
  file_quit ();
  return ( FALSE );
} /* delete_event */

GtkWidget *gpa_menubar_new ( GtkWidget *window ) {
/* var */
  GtkItemFactory *itemFactory;
  GtkItemFactoryEntry menuItem [] = {
    { _( "/_File" ), NULL, NULL, 0, "<Branch>" },
    { _( "/File/_Open" ), "<control>O", file_open, 0, NULL },
    { _( "/File/S_how Detail" ), "<control>H", file_showDetail, 0, NULL },
    { _( "/File/sep1" ), NULL, NULL, 0, "<Separator>" },
    { _( "/File/_Sign" ), NULL, file_sign, 0, NULL },
    { _( "/File/_Encrypt" ), NULL, file_encrypt, 0, NULL },
    { _( "/File/E_ncrypt as" ), NULL, file_encryptAs, 0, NULL },
    { _( "/File/_Protect by Password" ), NULL, file_protect, 0, NULL },
    { _( "/File/P_rotect as" ), NULL, file_protectAs, 0, NULL },
    { _( "/File/_Decrypt" ), NULL, file_decrypt, 0, NULL },
    { _( "/File/Decrypt _as" ), NULL, file_decryptAs, 0, NULL },
    { _( "/File/_Verify" ), NULL, file_verify, 0, NULL },
    { _( "/File/sep2" ), NULL, NULL, 0, "<Separator>" },
    { _( "/File/_Close" ), NULL, file_close, 0, NULL },
    { _( "/File/_Quit" ), "<control>Q", file_quit, 0, NULL },
    { _( "/_Keys" ), NULL, NULL, 0, "<Branch>" },
    { _( "/Keys/Open _public Keyring" ), "<control>K", keys_openPublic, 0, NULL },
    { _( "/Keys/Open _secret Keyring" ), NULL, keys_openSecret, 0, NULL  },
    { _( "/Keys/sep1" ), NULL, NULL, 0, "<Separator>" },
    { _( "/Keys/_Generate Key" ), NULL, keys_generateKey, 0, NULL },
    { _( "/Keys/Generate _Revocation Certificate" ), NULL, keys_generateRevocation, 0, NULL },
    { _( "/Keys/_Import" ), NULL, keys_import, 0, NULL },
    { _( "/Keys/Import _Ownertrust" ), NULL, keys_importOwnertrust, 0, NULL },
    { _( "/Keys/_Update Trust Database" ), NULL, keys_updateTrust, 0, NULL },
    { _( "/_Options" ), NULL, NULL, 0, "<Branch>" },
    { _( "/Options/_Keyserver" ), NULL, options_keyserver, 0, NULL },
    { _( "/Options/Default _Recipients" ), NULL, options_recipients, 0, NULL },
    { _( "/Options/_Default Key" ), NULL, options_key, 0, NULL },
    { _( "/Options/_Home Directory" ), NULL, options_homedir, 0, NULL },
    { _( "/Options/sep1" ), NULL, NULL, 0, "<Separator>" },
    { _( "/Options/Online _tips" ), NULL, options_tips, 0, NULL },
    { _( "/Options/_Load Options File" ), NULL, options_load, 0, NULL },
    { _( "/Options/_Save Options File" ), NULL, options_save, 0, NULL },
    { _( "/_Help" ), NULL, NULL, 0, "<Branch>" },
    { _( "/Help/_Version" ), NULL, help_version, 0, NULL },
    { _( "/Help/_License" ), NULL, help_license, 0, NULL },
    { _( "/Help/_Warranty" ), NULL, help_warranty, 0, NULL },
    { _( "/Help/_Help" ), "F1", help_help, 0, NULL }
  };
  gint menuItems = sizeof ( menuItem ) / sizeof ( menuItem [ 0 ] );
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *menubar;
/* commands */
  accelGroup = gtk_accel_group_new ();
  itemFactory = gtk_item_factory_new (
    GTK_TYPE_MENU_BAR, "<main>", accelGroup
  );
  gtk_item_factory_create_items ( itemFactory, menuItems, menuItem, NULL );
  gtk_window_add_accel_group ( GTK_WINDOW ( window ), accelGroup );
  menubar = gtk_item_factory_get_widget ( itemFactory, "<main>" );
  return ( menubar );
} /* gpa_menubar_new */

void gpa_popupMenu_init ( void ) {
/* var */
  GtkItemFactory *itemFactory;
  GtkItemFactoryEntry menuItem [] = {
    { _( "/Show detail" ), NULL, file_showDetail, 0, NULL },
    { _( "/sep1" ), NULL, NULL, 0, "<Separator>" },
    { _( "/Sign" ), NULL, file_sign, 0, NULL },
    { _( "/Encrypt" ), NULL, file_encrypt, 0, NULL },
    { _( "/E_ncrypt as" ), NULL, file_encryptAs, 0, NULL },
    { _( "/Protect by Password" ), NULL, file_protect, 0, NULL },
    { _( "/P_rotect as" ), NULL, file_protectAs, 0, NULL },
    { _( "/Decrypt" ), NULL, file_decrypt, 0, NULL },
    { _( "/Decrypt _as" ), NULL, file_decryptAs, 0, NULL },
    { _( "/Verify" ), NULL, file_verify, 0, NULL },
    { _( "/sep2" ), NULL, NULL, 0, "<Separator>" },
    { _( "/Close" ), NULL, file_close, 0, NULL }
  };
  gint menuItems = sizeof ( menuItem ) / sizeof ( menuItem [ 0 ] );
/* commands */
  itemFactory = gtk_item_factory_new ( GTK_TYPE_MENU, "<main>", NULL );
  gtk_item_factory_create_items ( itemFactory, menuItems, menuItem, NULL );
  global_popupMenu = gtk_item_factory_get_widget ( itemFactory, "<main>" );
} /* gpa_popupMenu_init */

void setFileSelected (
  GtkWidget *clistFile, gint row, gint column,
  GdkEventButton *event, gboolean selected
) {
/* var */
  GpapaFile *aFile;
/* commands */
  aFile = g_list_nth_data ( filesOpened, row );
  if ( selected )
    {
      if (
	! g_list_find ( filesSelected, aFile )
      )
	filesSelected = g_list_append ( filesSelected, aFile );
    } /* if */
  else
    {
      if (
	g_list_find ( filesSelected, aFile )
      )
	filesSelected = g_list_remove ( filesSelected, aFile );
    } /* else */
} /* setFileSelected */

gboolean evalKeyClistFile (
  GtkWidget *clistFile, GdkEventKey *event, gpointer userData
) {
  switch ( event -> keyval ) {
    case GDK_Delete: file_close (); break;
    case GDK_Insert: file_open (); break;
  } /* switch */
  return ( TRUE );
} /* evalKeyClistFile */

gboolean evalMouseClistFile (
  GtkWidget *clistFile, GdkEventButton *event, gpointer userData
) {
/* var */
  gint x, y;
  gint row, column;
/* commands */
  if ( event -> button == 3 )
    {
      x = event -> x;
      y = event -> y;
      if (
        gtk_clist_get_selection_info (
          GTK_CLIST ( clistFile ), x, y, &row, &column
        )
      )
        {
          if (
            ! (
              event -> state & GDK_SHIFT_MASK ||
              event -> state & GDK_CONTROL_MASK
            )
          )
            gtk_clist_unselect_all ( GTK_CLIST ( clistFile ) );
          gtk_clist_select_row ( GTK_CLIST ( clistFile ), row, column );
          gtk_menu_popup (
            GTK_MENU ( global_popupMenu ), NULL, NULL, NULL, NULL, 3, 0
          );
        } /* if */
    } /* if */
  else if ( event -> type == GDK_2BUTTON_PRESS )
    file_showDetail ();
  return ( TRUE );
} /* evalMouseClistFile */

GtkWidget *gpa_windowFile_new ( void ) {
/* var */
  char *clistFileTitle [ 5 ] = {
    N_( "File" ), N_( "Status" ), N_( "Sigs total" ), N_( "Valid Sigs" ),
    N_( "Invalid Sigs" )
  };
  int i;
/* objects */
  GtkWidget *windowFile;
    GtkWidget *scrollerFile;
      GtkWidget *clistFile;
/* commands */
  windowFile = gtk_frame_new ( _( "Files in work" ) );
  scrollerFile = gtk_scrolled_window_new ( NULL, NULL );
  clistFile = gtk_clist_new_with_titles ( 5, clistFileTitle );
  gtk_clist_set_column_width ( GTK_CLIST ( clistFile ), 0, 170 );
  gtk_clist_set_column_width ( GTK_CLIST ( clistFile ), 1, 100 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistFile ), 1, GTK_JUSTIFY_CENTER
  );
  for ( i = 2; i <= 4; i++ )
    {
      gtk_clist_set_column_width ( GTK_CLIST ( clistFile ), i, 100 );
      gtk_clist_set_column_justification (
	GTK_CLIST ( clistFile ), i, GTK_JUSTIFY_RIGHT
      );
    } /* for */
  for ( i = 0; i <= 4; i++ )
    gtk_clist_set_column_auto_resize ( GTK_CLIST ( clistFile ), i, FALSE );
  gtk_clist_set_selection_mode (
    GTK_CLIST ( clistFile ), GTK_SELECTION_EXTENDED
  );
  gtk_widget_grab_focus ( clistFile );
  gtk_signal_connect (
    GTK_OBJECT ( clistFile ), "select-row",
    GTK_SIGNAL_FUNC ( setFileSelected ), (gpointer) TRUE
  );
  gtk_signal_connect (
    GTK_OBJECT ( clistFile ), "unselect-row",
    GTK_SIGNAL_FUNC ( setFileSelected ), (gpointer) FALSE
  );
  gtk_signal_connect (
    GTK_OBJECT ( clistFile ), "key-press-event",
    GTK_SIGNAL_FUNC ( evalKeyClistFile ), NULL
  );
  gtk_signal_connect (
    GTK_OBJECT ( clistFile ), "button-press-event",
    GTK_SIGNAL_FUNC ( evalMouseClistFile ), NULL
  );
  global_clistFile = clistFile;
  gtk_container_add ( GTK_CONTAINER ( scrollerFile ), clistFile );
  gtk_container_add ( GTK_CONTAINER ( windowFile ), scrollerFile );
  return ( windowFile );
} /* gpa_windowFile_new */

GtkWidget *gpa_windowMain_new ( char *title ) {
/* objects */
  GtkWidget *window;
    GtkWidget *vbox;
      GtkWidget *menubar;
      GtkWidget *fileBox;
	GtkWidget *windowFile;
/* commands */
  window = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_title ( GTK_WINDOW ( window ), title );
  gtk_widget_set_usize ( window, 640, 480 );
  vbox = gtk_vbox_new ( FALSE, 0 );
  menubar = gpa_menubar_new ( window );
  gtk_box_pack_start ( GTK_BOX ( vbox ), menubar, FALSE, TRUE, 0 );
  fileBox = gtk_hbox_new ( TRUE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( fileBox ), 5 );
  windowFile = gpa_windowFile_new ();
  gtk_box_pack_start ( GTK_BOX ( fileBox ), windowFile, TRUE, TRUE, 0 );
  gtk_box_pack_end ( GTK_BOX ( vbox ), fileBox, TRUE, TRUE, 0 );
  gtk_container_add ( GTK_CONTAINER ( window ), vbox );
  gpa_popupMenu_init ();
  gpa_windowTip_init ();
  gpa_fileOpenSelect_init ( _( "Open a file" ) );
  gpa_homeDirSelect_init ( _( "Set home directory" ) );
  gpa_loadOptionsSelect_init ( _( "Load options file" ) );
  gpa_saveOptionsSelect_init ( _( "Save options file" ) );
  return ( window );
} /* gpa_windowMain_new */

int main ( int params, char *param [] ) {
  gtk_init ( &params, &param );
  global_windowMain = gpa_windowMain_new ( _( "GNU Privacy Assistant" ) );
  gtk_signal_connect (
    GTK_OBJECT ( global_windowMain ), "delete_event",
    GTK_SIGNAL_FUNC ( file_quit ), NULL
  );
  gtk_widget_show_all ( global_windowMain );
  gtk_main ();
  return ( 0 );
} /* main */
