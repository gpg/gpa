/* gpa_options.c  -  The GNU Privacy Assistant
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
#include "gpa_gtktools.h"

#include <stdio.h> /*!!!*/

static GtkWidget *homeDirSelect;
static GtkWidget *loadOptionsSelect;
static GtkWidget *saveOptionsSelect;

/*!!!*/ static gchar *text [ 1 ] = { _( "Dummy text" ) }; /*!!!*/

void gpa_homeDirSelect_ok ( void ) {
char message [ 100 ]; /*!!!*/
sprintf ( message, _( "Set home directory to %s\n" ), gtk_file_selection_get_filename ( GTK_FILE_SELECTION ( homeDirSelect ) ) ); /*!!!*/
g_print ( message ); /*!!!*/
  gtk_widget_hide ( homeDirSelect );
} /* gpa_homeDirSelect_ok */

void gpa_homeDirSelect_init ( gchar *title ) {
  homeDirSelect = gtk_file_selection_new ( title );
  gtk_signal_connect (
    GTK_OBJECT ( GTK_FILE_SELECTION ( homeDirSelect ) -> ok_button ),
    "clicked", GTK_SIGNAL_FUNC ( gpa_homeDirSelect_ok ), NULL
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( homeDirSelect ) -> cancel_button ),
    "clicked", GTK_SIGNAL_FUNC ( gtk_widget_hide ), (gpointer) homeDirSelect
  );
} /* gpa_homeDirSelect_init */

void gpa_loadOptionsSelect_ok ( void ) {
char message [ 100 ]; /*!!!*/
sprintf ( message, _( "Load options file %s\n" ), gtk_file_selection_get_filename ( GTK_FILE_SELECTION ( loadOptionsSelect ) ) ); /*!!!*/
g_print ( message ); /*!!!*/
  gtk_widget_hide ( loadOptionsSelect );
} /* gpa_loadOptionsSelect_ok */

void gpa_loadOptionsSelect_init ( gchar *title ) {
  loadOptionsSelect = gtk_file_selection_new ( title );
  gtk_signal_connect (
    GTK_OBJECT ( GTK_FILE_SELECTION ( loadOptionsSelect ) -> ok_button ),
    "clicked", GTK_SIGNAL_FUNC ( gpa_loadOptionsSelect_ok ), NULL
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( loadOptionsSelect ) -> cancel_button ),
    "clicked", GTK_SIGNAL_FUNC ( gtk_widget_hide ),
    (gpointer) loadOptionsSelect
  );
} /* gpa_loadOptionsSelect_init */

void gpa_saveOptionsSelect_ok ( void ) {
char message [ 100 ]; /*!!!*/
sprintf ( message, _( "Save options file %s\n" ), gtk_file_selection_get_filename ( GTK_FILE_SELECTION ( saveOptionsSelect ) ) ); /*!!!*/
g_print ( message ); /*!!!*/
  gtk_widget_hide ( saveOptionsSelect );
} /* gpa_saveOptionsSelect_ok */

void gpa_saveOptionsSelect_init ( gchar *title ) {
  saveOptionsSelect = gtk_file_selection_new ( title );
  gtk_signal_connect (
    GTK_OBJECT ( GTK_FILE_SELECTION ( saveOptionsSelect ) -> ok_button ),
    "clicked", GTK_SIGNAL_FUNC ( gpa_saveOptionsSelect_ok ), NULL
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( saveOptionsSelect ) -> cancel_button ),
    "clicked", GTK_SIGNAL_FUNC ( gtk_widget_hide ),
    (gpointer) saveOptionsSelect
  );
} /* gpa_saveOptionsSelect_init */

void options_keyserver_set ( GtkWidget *windowServer ) {
g_print ( _( "Set key server\n" ) ); /*!!!*/
  gtk_widget_destroy ( windowServer );
} /* options_keyserver_set */

void options_keyserver ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowServer;
    GtkWidget *vboxServer;
      GtkWidget *hboxServer;
        GtkWidget *labelServer;
        GtkWidget *comboServer;
      GtkWidget *hButtonBoxServer;
        GtkWidget *buttonCancel;
        GtkWidget *buttonSet;
/* commands */
  windowServer = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowServer ), _( "Set key server" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowServer ), accelGroup );
  vboxServer = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxServer ), 5 );
  hboxServer = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxServer), 5 );
  labelServer = gtk_label_new ( _( "" ) );
  gtk_box_pack_start ( GTK_BOX ( hboxServer ), labelServer, FALSE, FALSE, 0 );
  comboServer = gtk_combo_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelServer ), GTK_COMBO ( comboServer ) -> entry,
    accelGroup, _( "_Key server: " )
  );
  gtk_combo_set_value_in_list ( GTK_COMBO ( comboServer ), FALSE, FALSE );
  gtk_box_pack_start ( GTK_BOX ( hboxServer ), comboServer, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxServer ), hboxServer, TRUE, TRUE, 0 );
  hButtonBoxServer = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxServer ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxServer ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxServer ), 5 );
  buttonCancel = gpa_buttonCancel_new ( windowServer, accelGroup, "_Cancel" );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxServer ), buttonCancel );
  buttonSet = gpa_button_new ( accelGroup, "_Set" );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonSet ), "clicked",
    GTK_SIGNAL_FUNC ( options_keyserver_set ), (gpointer) windowServer
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxServer ), buttonSet );
  gtk_box_pack_start (
    GTK_BOX ( vboxServer ), hButtonBoxServer, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowServer ), vboxServer );
  gtk_widget_show_all ( windowServer );
  gpa_widget_set_centered ( windowServer, windowMain );
  if ( noTips == FALSE )
    gpa_dialog_tip ( _( "Dummy text" ) ); /*!!!*/
} /* options_keyserver */

void options_recipients ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowRecipients;
    GtkWidget *vboxRecipients;
      GtkWidget *hboxRecipients;
        GtkWidget *vboxDefault;
          GtkWidget *labelJfdDefault;
            GtkWidget *labelDefault;
          GtkWidget *scrollerDefault;
            GtkWidget *clistDefault;
        GtkWidget *vboxKeys;
          GtkWidget *labelJfdKeys;
            GtkWidget *labelKeys;
          GtkWidget *scrollerKeys;
            GtkWidget *clistKeys;
      GtkWidget *hButtonBoxRecipients;
        GtkWidget *buttonAdd;
        GtkWidget *buttonDelete;
        GtkWidget *buttonClose;
/* commands */
  windowRecipients = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowRecipients ), _( "Set default recipients" )
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowRecipients ), accelGroup );
  vboxRecipients = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxRecipients ), 5 );
  hboxRecipients = gtk_hbox_new ( TRUE, 0 );
  vboxDefault = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxDefault ), 5 );
  labelDefault = gtk_label_new ( _( "" ) );
  labelJfdDefault = gpa_widget_hjustified_new (
    labelDefault, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxDefault), labelJfdDefault, FALSE, FALSE, 0
  );
  scrollerDefault = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrollerDefault, 200, 140 );
  clistDefault = gtk_clist_new ( 1 );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelDefault ), clistDefault, accelGroup, _( "_Recipients" )
  );
gtk_clist_append ( GTK_CLIST ( clistDefault ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistDefault ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistDefault ), text ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerDefault ), clistDefault );
  gtk_box_pack_start (
    GTK_BOX ( vboxDefault ), scrollerDefault, TRUE, TRUE, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( hboxRecipients ), vboxDefault, TRUE, TRUE, 0
  );
  vboxKeys = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxKeys ), 5 );
  labelKeys = gtk_label_new ( _( "" ) );
  labelJfdKeys = gpa_widget_hjustified_new ( labelKeys, GTK_JUSTIFY_LEFT );
  gtk_box_pack_start ( GTK_BOX ( vboxKeys), labelJfdKeys, FALSE, FALSE, 0 );
  scrollerKeys = gtk_scrolled_window_new ( NULL, NULL );
  clistKeys = gtk_clist_new ( 1 );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelKeys ), clistKeys, accelGroup, _( "_Public keys" )
  );
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerKeys ), clistKeys );
  gtk_box_pack_start ( GTK_BOX ( vboxKeys ), scrollerKeys, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( hboxRecipients ), vboxKeys, TRUE, TRUE, 0 );
  gtk_box_pack_start (
    GTK_BOX ( vboxRecipients ), hboxRecipients, TRUE, TRUE, 0
  );
  hButtonBoxRecipients = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxRecipients ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxRecipients ), 10 );
  gtk_container_set_border_width (
    GTK_CONTAINER ( hButtonBoxRecipients ), 5
  );
  buttonAdd = gpa_button_new ( accelGroup, _( "_Add" ) );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecipients ), buttonAdd );
  buttonDelete = gpa_button_new ( accelGroup, _( "_Delete" ) );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecipients ), buttonDelete );
  buttonClose = gpa_buttonCancel_new (
    windowRecipients, accelGroup, _( "_Close" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecipients ), buttonClose );
  gtk_box_pack_start (
    GTK_BOX ( vboxRecipients ), hButtonBoxRecipients, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowRecipients ), vboxRecipients );
  gtk_widget_show_all ( windowRecipients );
  gpa_widget_set_centered ( windowRecipients, windowMain );
  if ( noTips == FALSE )
    gpa_dialog_tip ( _( "Dummy text" ) ); /*!!!*/
} /* options_recipients */

void options_key_set ( GtkWidget *windowKey ) {
g_print ( _( "Set Default Key\n" ) ); /*!!!*/
  gtk_widget_destroy ( windowKey );
} /*!!!*/

void options_key ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowKey;
    GtkWidget *vboxKey;
      GtkWidget *hboxKey;
        GtkWidget *labelKey;
        GtkWidget *comboKey;
      GtkWidget *hButtonBoxKey;
        GtkWidget *buttonCancel;
        GtkWidget *buttonSet;
/* commands */
  windowKey = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowKey ), _( "Set default key" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowKey ), accelGroup );
  vboxKey = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxKey ), 5 );
  hboxKey = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxKey ), 5 );
  labelKey = gtk_label_new ( _( "" ) );
  gtk_box_pack_start ( GTK_BOX ( hboxKey ), labelKey, FALSE, FALSE, 0 );
  comboKey = gtk_combo_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelKey ), GTK_COMBO ( comboKey ) -> entry,
    accelGroup, _( "Default _key: " )
  );
  gtk_box_pack_start ( GTK_BOX ( hboxKey ), comboKey, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxKey ), hboxKey, TRUE, TRUE, 0 );
  hButtonBoxKey = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxKey ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxKey ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxKey ), 5 );
  buttonCancel = gpa_buttonCancel_new (
    windowKey, accelGroup, _( "_Cancel" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxKey ), buttonCancel );
  buttonSet = gpa_button_new ( accelGroup, _( "_Set" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonSet ), "clicked",
    GTK_SIGNAL_FUNC ( options_key_set ), (gpointer) windowKey
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxKey ), buttonSet );
  gtk_box_pack_start ( GTK_BOX ( vboxKey ), hButtonBoxKey, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( windowKey ), vboxKey );
  gtk_widget_show_all ( windowKey );
  gpa_widget_set_centered ( windowKey, windowMain );
  if ( noTips == FALSE )
    gpa_dialog_tip ( _( "Dummy text" ) ); /*!!!*/
} /* options_key */

void options_homedir ( void ) {
  gtk_widget_show ( homeDirSelect );
  if ( noTips == FALSE )
    gpa_dialog_tip ( _( "Dummy text" ) ); /*!!!*/
} /* options_homedir */

void options_tips ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowTips;
    GtkWidget *hButtonBoxTips;
      GtkWidget *toggleTips;
      GtkWidget *buttonClose;
/* commands */
  windowTips = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowTips ), _( "Show GPA Tips" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowTips ), accelGroup );
  hButtonBoxTips = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxTips ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxTips ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxTips ), 10 );
  toggleTips = gpa_toggle_button_new ( accelGroup, _( "_Show GPA tips" ) );
  if ( noTips == FALSE )
    gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( toggleTips ), TRUE );
  else
    gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( toggleTips ), FALSE );
  gtk_signal_connect (
    GTK_OBJECT ( toggleTips ), "clicked",
    GTK_SIGNAL_FUNC ( gpa_switch_tips ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxTips ), toggleTips );
  buttonClose = gpa_buttonCancel_new (
    windowTips, accelGroup, _( "_Close" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxTips ), buttonClose );
  gtk_container_add ( GTK_CONTAINER ( windowTips ), hButtonBoxTips );
  gtk_widget_show_all ( windowTips );
  gpa_widget_set_centered ( windowTips, windowMain );
  if ( noTips == FALSE )
    gpa_dialog_tip ( _( "Dummy text" ) );
} /* options_tips */

void options_load ( void ) {
  gtk_widget_show ( loadOptionsSelect );
  if ( noTips == FALSE )
    gpa_dialog_tip ( _( "Dummy text" ) ); /*!!!*/
} /* options_load */

void options_save ( void ) {
  gtk_widget_show ( saveOptionsSelect );
  if ( noTips == FALSE )
    gpa_dialog_tip ( _( "Dummy text" ) ); /*!!!*/
} /* options_save */
