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
#include <gtk/gtk.h>
#include "gpa_file.h"
#include "gpa_keys.h"
#include "gpa_options.h"
#include "gpa_help.h"

/*!!!*/ static char *text5 [] = { "Dummy Text", "Dummy Text", "Dummy Text", "Dummy Text", "Dummy Text" }; /*!!!*/

gint delete_event ( GtkWidget *widget, GdkEvent *event, gpointer data ) {
  file_quit ();
  return ( FALSE );
} /* delete_event */

GtkWidget *gpa_menubar_new ( GtkWidget *windowMain ) {
/* var */
  GtkItemFactory *itemFactory;
  GtkItemFactoryEntry menuItem [] = {
    { "/_File", NULL, NULL, 0, "<Branch>" },
    { "/File/_Open", "<control>O", file_open, 0, NULL },
    { "/File/S_how Detail", "<control>H", file_showDetail, 0, NULL },
    { "/File/sep1", NULL, NULL, 0, "<Separator>" },
    { "/File/_Sign", NULL, file_sign, 0, NULL },
    { "/File/_Encrypt", NULL, file_encrypt, 0, NULL },
    { "/File/_Protect by Password", NULL, file_protect, 0, NULL },
    { "/File/_Decrypt", NULL, file_decrypt, 0, NULL },
    { "/File/_Verify", NULL, file_verify, 0, NULL },
    { "/File/sep2", NULL, NULL, 0, "<Separator>" },
    { "/File/_Close", NULL, file_close, 0, NULL },
    { "/File/_Quit", "<control>Q", file_quit, 0, NULL },
    { "/_Keys", NULL, NULL, 0, "<Branch>" },
    { "/Keys/Open _public Keyring", "<control>K", keys_openPublic, 0, NULL },
    { "/Keys/Open _secret Keyring", NULL, keys_openSecret, 0, NULL  },
    { "/Keys/Open _keyring", NULL, keys_open, 0, NULL },
    { "/Keys/sep1", NULL, NULL, 0, "<Separator>" },
    { "/Keys/_Generate Key", NULL, keys_generateKey, 0, NULL },
    { "/Keys/Generate _Revocation Certificate", NULL, keys_generateRevocation, 0, NULL },
    { "/Keys/_Import", NULL, keys_import, 0, NULL },
    { "/Keys/Import _Ownertrust", NULL, keys_importOwnertrust, 0, NULL },
    { "/Keys/_Update Trust Database", NULL, keys_updateTrust, 0, NULL },
    { "/_Options", NULL, NULL, 0, "<Branch>" },
    { "/Options/_Keyserver", NULL, options_keyserver, 0, NULL },
    { "/Options/Default _Recipients", NULL, options_recipients, 0, NULL },
    { "/Options/_Default Key", NULL, options_key, 0, NULL },
    { "/Options/_Home Directory", NULL, options_homedir, 0, NULL },
    { "/Options/sep1", NULL, NULL, 0, "<Separator>" },
    { "/Options/_Load Options File", NULL, options_load, 0, NULL },
    { "/Options/_Save Options File", NULL, options_save, 0, NULL },
    { "/_Help", NULL, NULL, 0, "<Branch>" },
    { "/Help/_Version", NULL, help_version, 0, NULL },
    { "/Help/_License", NULL, help_license, 0, NULL },
    { "/Help/_Warranty", NULL, help_warranty, 0, NULL },
    { "/Help/_Help", "F1", help_help, 0, NULL }
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
  gtk_window_add_accel_group ( GTK_WINDOW ( windowMain ), accelGroup );
  menubar = gtk_item_factory_get_widget ( itemFactory, "<main>" );
  return ( menubar );
} /* gpa_menubar_new */

GtkWidget *gpa_fileFrame_new () {
/* var */
  char *fileListTitle [ 5 ] = {
    "File", "Encrypted", "Sigs total", "Valid Sigs", "Invalid Sigs"
  };
  int i;
/* objects */
  GtkWidget *fileFrame;
    GtkWidget *fileScroller;
      GtkWidget *fileList;
/* commands */
  fileFrame = gtk_frame_new ( "Files in work" );
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
  gtk_widget_show ( fileList );
  gtk_container_add ( GTK_CONTAINER ( fileScroller ), fileList );
  gtk_widget_show ( fileScroller );
  gtk_container_add ( GTK_CONTAINER ( fileFrame ), fileScroller );
  return ( fileFrame );
} /* gpa_fileFrame_new */

GtkWidget *gpa_windowMain_new ( char *title ) {
/* objects */
  GtkWidget *windowMain;
    GtkWidget *vboxMain;
      GtkWidget *menubar;
      GtkWidget *fileBox;
	GtkWidget *fileFrame;
/* commands */
  windowMain = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_title ( GTK_WINDOW ( windowMain ), title );
  gtk_widget_set_usize ( windowMain, 640, 480 );
  vboxMain = gtk_vbox_new ( FALSE, 0 );
  menubar = gpa_menubar_new ( windowMain );
  gtk_widget_show ( menubar );
  gtk_box_pack_start ( GTK_BOX ( vboxMain ), menubar, FALSE, TRUE, 0 );
  fileBox = gtk_hbox_new ( TRUE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( fileBox ), 5 );
  fileFrame = gpa_fileFrame_new ();
  gtk_widget_show ( fileFrame );
  gtk_box_pack_start ( GTK_BOX ( fileBox ), fileFrame, TRUE, TRUE, 0 );
  gtk_widget_show ( fileBox );
  gtk_box_pack_end ( GTK_BOX ( vboxMain ), fileBox, TRUE, TRUE, 0 );
  gtk_widget_show ( vboxMain );
  gtk_container_add ( GTK_CONTAINER ( windowMain ), vboxMain );
  gpa_fileOpenSelect_init ( "Open a file" );
  return ( windowMain );
} /* gpa_windowMain_new */

int main ( int params, char *param [] ) {
/* objects */
  GtkWidget *windowMain;
/* commands */
  gtk_init ( &params, &param );
  windowMain = gpa_windowMain_new ( "GNU Privacy Assistant" );
  gtk_signal_connect (
    GTK_OBJECT ( windowMain ), "delete_event",
    GTK_SIGNAL_FUNC ( file_quit ), NULL
  );
  gtk_widget_show ( windowMain );
  gtk_main ();
  return ( 0 );
} /* main */
