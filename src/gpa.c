/* gpa.c  -  The GNU Privacy Assistant
 *      Copyright (C) 2000 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "gpa.h"
#include "gpa_file.h"
#include "gpa_gtktools.h"
#include "gpa_keys.h"
#include "gpa_options.h"
#include "gpa_help.h"

/*!!!*/ static char *text5 [] = { N_( "Dummy Text" ), N_( "Dummy Text" ), N_( "Dummy Text" ), N_( "Dummy Text" ), N_( "Dummy Text" ) }; /*!!!*/

void gpa_switch_tips ( void ) {
  if ( noTips == TRUE )
    noTips = FALSE;
  else
    noTips = TRUE;
} /* gpa_switch_tips */

void gpa_dialog_tip ( char *tip ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowTip;
    GtkWidget *vboxTip;
      GtkWidget *vboxContents;
        GtkWidget *labelJfdContents;
          GtkWidget *labelContents;
        GtkWidget *textContents;
      GtkWidget *hboxTip;
        GtkWidget *checkerNomore;
        GtkWidget *buttonClose;
/* commands */
  windowTip = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowTip ), _( "GPA Tip" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowTip ), accelGroup );
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
  textContents = gtk_text_new ( NULL, NULL );
  gtk_text_set_editable ( GTK_TEXT ( textContents ), FALSE );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelContents ), textContents, accelGroup, _( "_Tip:" )
  );
  gtk_box_pack_start ( GTK_BOX ( vboxContents ), textContents, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxTip ), vboxContents, TRUE, TRUE, 0 );
  hboxTip = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxTip ), 5 );
  buttonClose = gpa_buttonCancel_new (
    windowTip, accelGroup, _( "  _Close  " )
  );
  gtk_box_pack_end ( GTK_BOX ( hboxTip ), buttonClose, FALSE, FALSE, 0 );
  checkerNomore = gpa_check_button_new (
    accelGroup, _( "_No more tips, please" )
  );
  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON ( checkerNomore ), noTips
  );
  gtk_signal_connect (
    GTK_OBJECT ( checkerNomore ), "clicked",
    GTK_SIGNAL_FUNC ( gpa_switch_tips ), NULL
  );
  gtk_box_pack_end (
    GTK_BOX ( hboxTip ), checkerNomore, FALSE, FALSE, 10
  );
  gtk_box_pack_start ( GTK_BOX ( vboxTip ), hboxTip, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( windowTip ), vboxTip );
  gtk_widget_show_all ( windowTip );
} /* gpa_dialog_tip */

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
    { _( "/File/_Protect by Password" ), NULL, file_protect, 0, NULL },
    { _( "/File/_Decrypt" ), NULL, file_decrypt, 0, NULL },
    { _( "/File/Decrypt _as" ), NULL, file_decryptAs, 0, NULL },
    { _( "/File/_Verify" ), NULL, file_verify, 0, NULL },
    { _( "/File/sep2" ), NULL, NULL, 0, "<Separator>" },
    { _( "/File/_Close" ), NULL, file_close, 0, NULL },
    { _( "/File/_Quit" ), "<control>Q", file_quit, 0, NULL },
    { _( "/_Keys" ), NULL, NULL, 0, "<Branch>" },
    { _( "/Keys/Open _public Keyring" ), "<control>K", keys_openPublic, 0, NULL },
    { _( "/Keys/Open _secret Keyring" ), NULL, keys_openSecret, 0, NULL  },
    { _( "/Keys/Open _keyring" ), NULL, keys_open, 0, NULL },
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

GtkWidget *gpa_fileFrame_new ( void ) {
/* var */
  char *fileListTitle [ 5 ] = {
    N_( "File" ), N_( "Encrypted" ), N_( "Sigs total" ), N_( "Valid Sigs" ),
    N_( "Invalid Sigs" )
  };
  int i;
/* objects */
  GtkWidget *fileFrame;
    GtkWidget *fileScroller;
      GtkWidget *fileList;
/* commands */
  fileFrame = gtk_frame_new ( _( "Files in work" ) );
  fileScroller = gtk_scrolled_window_new ( NULL, NULL );
  fileList = gtk_clist_new_with_titles ( 5, fileListTitle );
  gtk_clist_set_column_width ( GTK_CLIST ( fileList ), 0, 170 );
  gtk_clist_set_column_width ( GTK_CLIST ( fileList ), 1, 100 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( fileList ), 1, GTK_JUSTIFY_CENTER
  );
  for ( i = 2; i <= 4; i++ )
    {
      gtk_clist_set_column_width ( GTK_CLIST ( fileList ), i, 100 );
      gtk_clist_set_column_justification (
        GTK_CLIST ( fileList ), i, GTK_JUSTIFY_RIGHT
      );
    } /* for */
  for ( i = 0; i <= 4; i++ )
    gtk_clist_set_column_auto_resize ( GTK_CLIST ( fileList ), i, FALSE );
gtk_clist_append ( GTK_CLIST ( fileList ), text5 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( fileList ), text5 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( fileList ), text5 ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( fileScroller ), fileList );
  gtk_container_add ( GTK_CONTAINER ( fileFrame ), fileScroller );
  return ( fileFrame );
} /* gpa_fileFrame_new */

GtkWidget *gpa_windowMain_new ( char *title ) {
/* objects */
  GtkWidget *window;
    GtkWidget *vbox;
      GtkWidget *menubar;
      GtkWidget *fileBox;
        GtkWidget *fileFrame;
/* commands */
  window = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_title ( GTK_WINDOW ( window ), title );
  gtk_widget_set_usize ( window, 640, 480 );
  vbox = gtk_vbox_new ( FALSE, 0 );
  menubar = gpa_menubar_new ( window );
  gtk_box_pack_start ( GTK_BOX ( vbox ), menubar, FALSE, TRUE, 0 );
  fileBox = gtk_hbox_new ( TRUE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( fileBox ), 5 );
  fileFrame = gpa_fileFrame_new ();
  gtk_box_pack_start ( GTK_BOX ( fileBox ), fileFrame, TRUE, TRUE, 0 );
  gtk_box_pack_end ( GTK_BOX ( vbox ), fileBox, TRUE, TRUE, 0 );
  gtk_container_add ( GTK_CONTAINER ( window ), vbox );
  gpa_fileOpenSelect_init ( _( "Open a file" ) );
  gpa_ringOpenSelect_init ( _( "Open key ring" ) );
  gpa_homeDirSelect_init ( _( "Set home directory" ) );
  gpa_loadOptionsSelect_init ( _( "Load options file" ) );
  gpa_saveOptionsSelect_init ( _( "Save options file" ) );
  return ( window );
} /* gpa_windowMain_new */

int main ( int params, char *param [] ) {
  noTips = FALSE;
  gtk_init ( &params, &param );
  windowMain = gpa_windowMain_new ( _( "GNU Privacy Assistant" ) );
  gtk_signal_connect (
    GTK_OBJECT ( windowMain ), "delete_event",
    GTK_SIGNAL_FUNC ( file_quit ), NULL
  );
  gtk_widget_show_all ( windowMain );
  gtk_main ();
  return ( 0 );
} /* main */
