/* gpa_keys.c  -  The GNU Privacy Assistant
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
#include "gpa.h"
#include "gtktools.h"

#include <stdio.h> /*!!!*/

static GtkWidget *ringOpenSelect;
/*!!!*/ static char *text [] = { N_( "Dummy Text" ) }; /*!!!*/
/*!!!*/ static char *text2 [] = { N_( "Dummy Text" ), N_( "Dummy Text" ) }; /*!!!*/
/*!!!*/ static char *text4 [] = { N_( "Dummy Text" ), N_( "Dummy Text" ), N_( "Dummy Text" ), N_( "Dummy Text" ) }; /*!!!*/

GtkWidget *gpa_frameExpire_new ( GtkAccelGroup *accelGroup ) {
/* var */
  GList *unitAfter = NULL;
/* objects */
  GtkWidget *frameExpire;
    GtkWidget *vboxExpire;
      GtkWidget *radioDont;
      GtkWidget *hboxAfter;
	GtkWidget *radioAfter;
	GtkWidget *entryAfter;
	GtkWidget *comboAfter;
      GtkWidget *hboxAt;
	GtkWidget *radioAt;
	GtkWidget *entryAt;
/* commands */
  frameExpire = gtk_frame_new ( _( "Expiration" ) );
  vboxExpire = gtk_vbox_new ( TRUE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxExpire ), 5 );
  radioDont = gpa_radio_button_new ( accelGroup, _( "_indefinitely valid" ) );
  gtk_box_pack_start ( GTK_BOX ( vboxExpire ), radioDont, FALSE, FALSE, 0 );
  hboxAfter = gtk_hbox_new ( FALSE, 0 );
  radioAfter = gpa_radio_button_new_from_widget (
    GTK_RADIO_BUTTON ( radioDont ), accelGroup, _( "expire _after" )
  );
  gtk_box_pack_start ( GTK_BOX ( hboxAfter ), radioAfter, FALSE, FALSE, 0 );
  entryAfter = gtk_entry_new ();
  gtk_signal_connect_object (
    GTK_OBJECT ( radioAfter ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_grab_focus ), (gpointer) entryAfter
  );
  gtk_widget_set_usize ( entryAfter, 50, 22 );
  gtk_box_pack_start ( GTK_BOX ( hboxAfter ), entryAfter, FALSE, FALSE, 0 );
  comboAfter = gtk_combo_new ();
  gtk_editable_set_editable (
    GTK_EDITABLE ( GTK_COMBO ( comboAfter ) -> entry ), FALSE
  );
  unitAfter = g_list_append ( unitAfter, _( "days" ) );
  unitAfter = g_list_append ( unitAfter, _( "weeks" ) );
  unitAfter = g_list_append ( unitAfter, _( "months" ) );
  unitAfter = g_list_append ( unitAfter, _( "years" ) );
  gtk_combo_set_popdown_strings ( GTK_COMBO ( comboAfter ), unitAfter );
  gtk_box_pack_start ( GTK_BOX ( hboxAfter ), comboAfter, FALSE, FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxExpire ), hboxAfter, FALSE, FALSE, 0 );
  hboxAt = gtk_hbox_new ( FALSE, 0 );
  radioAt = gpa_radio_button_new_from_widget (
    GTK_RADIO_BUTTON ( radioDont ), accelGroup, _( "expire a_t" )
  );
  gtk_box_pack_start ( GTK_BOX ( hboxAt ), radioAt, FALSE, FALSE, 0 );
  entryAt = gtk_entry_new ();
  gtk_signal_connect_object (
    GTK_OBJECT ( radioAt ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_grab_focus ), (gpointer) entryAt
  );
  gtk_box_pack_start ( GTK_BOX ( hboxAt ), entryAt, FALSE, FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxExpire ), hboxAt, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( frameExpire ), vboxExpire );
  return ( frameExpire );
} /* gpa_frameExpire_new */

void ring_open_ok ( void ) {
char message [ 100 ];
sprintf ( message, _( "Open key ring '%s'\n" ), gtk_file_selection_get_filename ( GTK_FILE_SELECTION ( ringOpenSelect ) ) ); /*!!!*/
g_print ( message ); /*!!!*/
gtk_widget_hide ( ringOpenSelect );
} /* ring_open_ok */

void gpa_ringOpenSelect_init ( gchar* title ) {
  ringOpenSelect = gtk_file_selection_new ( title );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( ringOpenSelect ) -> ok_button ), "clicked",
    GTK_SIGNAL_FUNC ( ring_open_ok ), (gpointer) ringOpenSelect
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( ringOpenSelect ) -> cancel_button ),
    "clicked", GTK_SIGNAL_FUNC ( gtk_widget_hide ), (gpointer) ringOpenSelect
  );
} /* gpa_ringOpenSelect_init */

void keys_export ( void ) {
g_print ( _( "Export keys\n" ) ); /*!!!*/
} /* keys_export */

void keys_delete ( void ) {
g_print ( _( "Delete keys\n" ) ); /*!!!*/
} /* keys_delete */

void keys_revocation ( void ) {
g_print ( _( "Create revocation certificate\n" ) ); /*!!!*/
} /* keys_revocation */

void keys_openPublic_editTrust_accept ( GtkWidget *windowTrust ) {
g_print ( _( "Accept new ownertrust level\n" ) ); /*!!!*/
  gpa_window_destroy ( windowTrust );
} /* keys_openPublic_editTrust_accept */

void keys_openPublic_editTrust_export ( GtkWidget *windowTrust ) {
g_print ( _( "Accept and export new ownertrust level\n" ) ); /*!!!*/
  gpa_window_destroy ( windowTrust );
} /* keys_openPublic_editTrust_export */

void keys_openPublic_editTrust ( GtkWidget *parent ) {
/* var */
  GtkAccelGroup *accelGroup;
  GList *valueLevel = NULL;
/* objects */
  GtkWidget *windowTrust;
    GtkWidget *vboxTrust;
      GtkWidget *hboxLevel;
	GtkWidget *labelLevel;
	GtkWidget *comboLevel;
      GtkWidget *hButtonBoxTrust;
	GtkWidget *buttonCancel;
	GtkWidget *buttonAccept;
	GtkWidget *buttonExport;
/* commands */
  windowTrust = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowTrust ), _( "Change key ownertrust" )
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowTrust ), accelGroup );
  vboxTrust = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxTrust ), 5 );
  hboxLevel = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxLevel ), 5 );
  labelLevel = gtk_label_new ( _( "" ) );
  gtk_box_pack_start ( GTK_BOX ( hboxLevel ), labelLevel, FALSE, FALSE, 0 );
  comboLevel = gtk_combo_new ();
  gtk_editable_set_editable (
    GTK_EDITABLE ( GTK_COMBO ( comboLevel ) -> entry ), FALSE
  );
  valueLevel = g_list_append ( valueLevel, _( "don't know" ) );
  valueLevel = g_list_append ( valueLevel, _( "don't trust" ) );
  valueLevel = g_list_append ( valueLevel, _( "trust marginally" ) );
  valueLevel = g_list_append ( valueLevel, _( "trust fully" ) );
  gtk_combo_set_popdown_strings ( GTK_COMBO ( comboLevel ), valueLevel );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelLevel ), GTK_COMBO ( comboLevel ) -> entry,
    accelGroup, _( "_Ownertrust level: " )
  );
  gtk_box_pack_start ( GTK_BOX ( hboxLevel ), comboLevel, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxTrust ), hboxLevel, TRUE, TRUE, 0 );
  hButtonBoxTrust = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxTrust ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxTrust ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxTrust ), 5 );
  buttonCancel = gpa_buttonCancel_new (
    windowTrust, accelGroup, _( "_Cancel" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxTrust ), buttonCancel );
  buttonAccept = gpa_button_new ( accelGroup, _( "_Accept" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonAccept ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_editTrust_accept ),
    (gpointer) windowTrust
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxTrust ), buttonAccept );
  buttonExport = gpa_button_new ( accelGroup, _( "Accept and e_xport" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonExport ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_editTrust_export ),
    (gpointer) windowTrust
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxTrust ), buttonExport );
  gtk_box_pack_start (
    GTK_BOX ( vboxTrust ), hButtonBoxTrust, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowTrust ), vboxTrust );
  gtk_widget_show_all ( windowTrust );
  gpa_widget_set_centered ( windowTrust, parent );
  gpa_windowTip_show ( _( "Dummy text" ) ); /*!!!*/
} /* keys_openPublic_editTrust */

void keys_openPublic_sign ( void ) {
g_print ( _( "Sign keys\n" ) ); /*!!!*/
} /* keys_openPublic_sign */

void keys_openPublic_editKey_check ( void ) {
g_print ( _( "Check key signature validities\n" ) ); /*!!!*/
} /* keys_openPublic_editKey_check */

void keys_openPublic_editKey ( GtkWidget *parent ) {
/* var */
  GtkAccelGroup *accelGroup;
  char *titlesSignatures [] = { N_( "Signature" ), N_( "valid" ) };
/* objects */
  GtkWidget *windowKey;
    GtkWidget *vboxEdit;
      GtkWidget *hboxKey;
	GtkWidget *labelKey;
	GtkWidget *entryKey;
      GtkWidget *vboxFingerprint;
	GtkWidget *labelJfdFingerprint;
	  GtkWidget *labelFingerprint;
	GtkWidget *entryFingerprint;
      GtkWidget *vboxSignatures;
	GtkWidget *labelJfdSignatures;
	  GtkWidget *labelSignatures;
	GtkWidget *scrollerSignatures;
	  GtkWidget *clistSignatures;
      GtkWidget *hboxSignatures;
	GtkWidget *buttonSign;
	GtkWidget *checkerLocally;
	GtkWidget *buttonCheck;
      GtkWidget *tableMisc;
	GtkWidget *labelJfdTrust;
	  GtkWidget *labelTrust;
	GtkWidget *entryTrust;
	GtkWidget *labelJfdDate;
	  GtkWidget *labelDate;
	GtkWidget *entryDate;
      GtkWidget *vboxSubkeys;
	GtkWidget *labelJfdSubkeys;
	  GtkWidget *labelSubkeys;
	GtkWidget *scrollerSubkeys;
	  GtkWidget *clistSubkeys;
      GtkWidget *hButtonBoxEdit;
	GtkWidget *buttonEditTrust;
	GtkWidget *buttonExportKey;
	GtkWidget *buttonClose;
/* commands */
  windowKey = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowKey ), _( "Public key editor" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowKey ), accelGroup );
  vboxEdit = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxEdit ), 5 );
  hboxKey = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxKey ), 5 );
  labelKey = gtk_label_new ( _( "Key: " ) );
  gtk_box_pack_start ( GTK_BOX ( hboxKey ), labelKey, FALSE, FALSE, 0 );
  entryKey = gtk_entry_new ();
  gtk_editable_set_editable ( GTK_EDITABLE ( entryKey ), FALSE );
  gtk_box_pack_start ( GTK_BOX ( hboxKey ), entryKey, FALSE, FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxEdit ), hboxKey, FALSE, FALSE, 0 );
  vboxFingerprint = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxFingerprint ), 5 );
  labelFingerprint = gtk_label_new ( _( "Fingerprint:" ) );
  labelJfdFingerprint = gpa_widget_hjustified_new (
    labelFingerprint, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxFingerprint ), labelJfdFingerprint, FALSE, FALSE, 0
  );
  entryFingerprint = gtk_entry_new ();
  gtk_editable_set_editable ( GTK_EDITABLE ( entryFingerprint ), FALSE );
  gtk_box_pack_start (
    GTK_BOX ( vboxFingerprint ), entryFingerprint, FALSE, FALSE, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxEdit ), vboxFingerprint, FALSE, FALSE, 0
  );
  vboxSignatures = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxSignatures ), 5 );
  labelSignatures = gtk_label_new ( _( "" ) );
  labelJfdSignatures = gpa_widget_hjustified_new (
    labelSignatures, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxSignatures ), labelJfdSignatures, FALSE, FALSE, 0
  );
  scrollerSignatures = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrollerSignatures, 320, 100 );
  clistSignatures = gtk_clist_new_with_titles ( 2, titlesSignatures );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelSignatures ), clistSignatures,
    accelGroup, _( "S_ignatures" )
  );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 0, 200 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistSignatures ), 0, GTK_JUSTIFY_LEFT
  );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 1, 80 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistSignatures ), 1, GTK_JUSTIFY_CENTER
  );
gtk_clist_append ( GTK_CLIST ( clistSignatures ), text2 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistSignatures ), text2 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistSignatures ), text2 ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerSignatures ), clistSignatures );
  gtk_box_pack_start (
    GTK_BOX ( vboxSignatures ), scrollerSignatures, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxEdit ), vboxSignatures, TRUE, TRUE, 0 );
  hboxSignatures = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxSignatures ), 5 );
  buttonSign = gpa_button_new ( accelGroup, _( "  _Sign  " ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonSign ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_sign ), NULL
  );
  gtk_box_pack_start (
    GTK_BOX ( hboxSignatures ), buttonSign, FALSE, FALSE, 0
  );
  checkerLocally = gpa_check_button_new (
    accelGroup, _( "sign only _locally" )
  );
  gtk_box_pack_start (
    GTK_BOX ( hboxSignatures ), checkerLocally, FALSE, FALSE, 5
  );
  buttonCheck = gpa_button_new ( accelGroup, _( "  Chec_k  " ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonCheck ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_editKey_check ), NULL
  );
  gtk_box_pack_start (
    GTK_BOX ( hboxSignatures ), buttonCheck, FALSE, FALSE, 15
  );
  gtk_box_pack_start ( GTK_BOX ( vboxEdit ), hboxSignatures, FALSE, FALSE, 0 );
  tableMisc = gtk_table_new ( 2, 2, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tableMisc ), 5 );
  labelTrust = gtk_label_new ( _( "Key Trust: " ) );
  labelJfdTrust = gpa_widget_hjustified_new ( labelTrust, GTK_JUSTIFY_RIGHT );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), labelJfdTrust, 0, 1, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryTrust = gtk_entry_new ();
  gtk_editable_set_editable ( GTK_EDITABLE ( entryTrust ), FALSE );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), entryTrust, 1, 2, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  labelDate = gtk_label_new ( _( "Expiry date: " ) );
  labelJfdDate = gpa_widget_hjustified_new ( labelDate, GTK_JUSTIFY_RIGHT );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), labelJfdDate, 0, 1, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryDate = gtk_entry_new ();
  gtk_editable_set_editable ( GTK_EDITABLE ( entryDate ), FALSE );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), entryDate, 1, 2, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxEdit ), tableMisc, FALSE, FALSE, 0 );
  vboxSubkeys = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxSubkeys ), 5 );
  labelSubkeys = gtk_label_new ( _( "" ) );
  labelJfdSubkeys = gpa_widget_hjustified_new (
    labelSubkeys, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxSubkeys ), labelJfdSubkeys, FALSE, FALSE, 0
  );
  scrollerSubkeys = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrollerSubkeys, 320, 100 );
  clistSubkeys = gtk_clist_new ( 1 );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelSubkeys ), clistSubkeys, accelGroup, _( "S_ubkeys" )
  );
gtk_clist_append ( GTK_CLIST ( clistSubkeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistSubkeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistSubkeys ), text ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerSubkeys ), clistSubkeys );
  gtk_box_pack_start (
    GTK_BOX ( vboxSubkeys ), scrollerSubkeys, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxEdit ), vboxSubkeys, FALSE, FALSE, 0 );
  hButtonBoxEdit = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxEdit ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxEdit ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxEdit ), 5 );
  buttonEditTrust = gpa_button_new ( accelGroup, _( "Edit _Ownertrust" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonEditTrust ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_editTrust ), (gpointer) windowKey
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEdit ), buttonEditTrust );
  buttonExportKey = gpa_button_new ( accelGroup, _( "E_xport key" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonExportKey ), "clicked",
    GTK_SIGNAL_FUNC ( keys_export ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEdit ), buttonExportKey );
  buttonClose = gpa_buttonCancel_new ( windowKey, accelGroup, _( "_Close" ) );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEdit ), buttonClose );
  gtk_box_pack_start ( GTK_BOX ( vboxEdit ), hButtonBoxEdit, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( windowKey ), vboxEdit );
  gtk_widget_show_all ( windowKey );
  gpa_widget_set_centered ( windowKey, parent );
  gpa_windowTip_show ( _( "Dummy text" ) ); /*!!!*/
} /* keys_openPublic_editKey */

void keys_openPublic_send ( void ) {
g_print ( _( "Send keys\n" ) ); /*!!!*/
} /* keys_openPublic_send */

void keys_openPublic_receive ( void ) {
g_print ( _( "Receive keys\n" ) ); /*!!!*/
} /* keys_openPublic_receive */

void keys_openPublic_exportTrust ( void ) {
g_print ( _( "Export ownertrust\n" ) ); /*!!!*/
} /* keys_openPublic_exportTrust */

void keys_openPublic ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  gchar *titlesKeys [] = {
    N_( "Key" ), N_( "Key trust" ), N_( "Ownertrust" ), N_( "Expiry date" )
  };
  gint i;
/* objects */
  GtkWidget *windowPublic;
    GtkWidget *vboxPublic;
      GtkWidget *hboxTop;
	GtkWidget *labelRingname;
	GtkWidget *entryRingname;
    GtkWidget *scrollerKeys;
      GtkWidget *clistKeys;
    GtkWidget *hboxAction;
      GtkWidget *tableKey;
	GtkWidget *buttonEditKey;
	GtkWidget *buttonSign;
	GtkWidget *buttonSend;
	GtkWidget *buttonReceive;
	GtkWidget *buttonExportKey;
	GtkWidget *buttonDelete;
      GtkWidget *vboxLocally;
	GtkWidget *checkerLocally;
      GtkWidget *tableTrust;
	GtkWidget *toggleShow;
	GtkWidget *buttonEditTrust;
	GtkWidget *buttonExportTrust;
    GtkWidget *hSeparatorPublic;
    GtkWidget *hButtonBoxPublic;
      GtkWidget *buttonClose;
/* commands */
  windowPublic = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowPublic ), _( "Public key ring editor" )
  );
  gtk_widget_set_usize ( windowPublic, 572, 400 );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowPublic ), accelGroup );
  vboxPublic = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxPublic ), 5 );
  hboxTop = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxTop ), 5 );
  labelRingname = gtk_label_new ( _( "" ) );
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), labelRingname, FALSE, FALSE, 0 );
  entryRingname = gtk_entry_new ();
  gtk_editable_set_editable ( GTK_EDITABLE ( entryRingname ), FALSE );
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), entryRingname, FALSE, FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxPublic ), hboxTop, FALSE, FALSE, 0 );
  scrollerKeys = gtk_scrolled_window_new ( NULL, NULL );
  gtk_container_set_border_width ( GTK_CONTAINER ( scrollerKeys ), 5 );
  clistKeys = gtk_clist_new_with_titles ( 4, titlesKeys );
  gtk_clist_set_column_width ( GTK_CLIST ( clistKeys ), 0, 200 );
  for ( i = 1; i < 4; i++ )
    {
      gtk_clist_set_column_width ( GTK_CLIST ( clistKeys ), i, 100 );
      gtk_clist_set_column_justification (
	GTK_CLIST ( clistKeys ), i, GTK_JUSTIFY_CENTER
      );
    } /* for */
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelRingname ), clistKeys, accelGroup, _( "_Key Ring " )
  );
gtk_clist_append ( GTK_CLIST ( clistKeys ), text4 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text4 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text4 ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerKeys ), clistKeys );
  gtk_box_pack_start ( GTK_BOX ( vboxPublic ), scrollerKeys, TRUE, TRUE, 0 );
  hboxAction = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxAction ), 5 );
  tableKey = gtk_table_new ( 3, 2, TRUE );
  buttonEditKey = gpa_button_new ( accelGroup, _( "_Edit key" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonEditKey ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_editKey ), (gpointer) windowPublic
  );
  gtk_table_attach (
    GTK_TABLE ( tableKey ), buttonEditKey, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 0, 0
  );
  buttonSign = gpa_button_new ( accelGroup, _( "_Sign keys" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonSign ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_sign ), NULL
  );
  gtk_table_attach (
    GTK_TABLE ( tableKey ), buttonSign, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 0, 0
  );
  buttonSend = gpa_button_new ( accelGroup, _( "Se_nd keys" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonSend ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_send ), NULL
  );
  gtk_table_attach (
    GTK_TABLE ( tableKey ), buttonSend, 0, 1, 1, 2,
    GTK_FILL, GTK_FILL, 0, 0
  );
  buttonReceive = gpa_button_new ( accelGroup, _( "_Receive keys" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonReceive ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_receive ), NULL
  );
  gtk_table_attach (
    GTK_TABLE ( tableKey ), buttonReceive, 1, 2, 1, 2,
    GTK_FILL, GTK_FILL, 0, 0
  );
  buttonExportKey = gpa_button_new ( accelGroup, _( "E_xport keys" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonExportKey ), "clicked",
    GTK_SIGNAL_FUNC ( keys_export ), NULL
  );
  gtk_table_attach (
    GTK_TABLE ( tableKey ), buttonExportKey, 0, 1, 2, 3,
    GTK_FILL, GTK_FILL, 0, 0
  );
  buttonDelete = gpa_button_new ( accelGroup, _( "_Delete keys" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonDelete ), "clicked",
    GTK_SIGNAL_FUNC ( keys_delete ), NULL
  );
  gtk_table_attach (
    GTK_TABLE ( tableKey ), buttonDelete, 1, 2, 2, 3,
    GTK_FILL, GTK_FILL, 0, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( hboxAction ), tableKey, FALSE, FALSE, 0
  );
  vboxLocally = gtk_vbox_new ( FALSE, 0 );
  checkerLocally = gpa_check_button_new (
    accelGroup, _( "sign only _locally" )
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxLocally ), checkerLocally, FALSE, FALSE, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( hboxAction ), vboxLocally, FALSE, FALSE, 5
  );
  tableTrust = gtk_table_new ( 3, 1, TRUE );
  toggleShow = gpa_toggle_button_new ( accelGroup, _( "S_how ownertrust" ) );
  gtk_table_attach (
    GTK_TABLE ( tableTrust ), toggleShow, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 0, 0
  );
  buttonEditTrust = gpa_button_new ( accelGroup, _( "Edit _ownertrust" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonEditTrust ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_editTrust ), (gpointer) windowPublic
  );
  gtk_table_attach (
    GTK_TABLE ( tableTrust ), buttonEditTrust, 0, 1, 1, 2,
    GTK_FILL, GTK_FILL, 0, 0
  );
  buttonExportTrust = gpa_button_new ( accelGroup, _( "Export o_wnertrust" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonExportTrust ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openPublic_exportTrust ), NULL
  );
  gtk_table_attach (
    GTK_TABLE ( tableTrust ), buttonExportTrust, 0, 1, 2, 3,
    GTK_FILL, GTK_FILL, 0, 0
  );
  gtk_box_pack_end (
    GTK_BOX ( hboxAction ), tableTrust, FALSE, FALSE, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxPublic ), hboxAction, FALSE, FALSE, 0
  );
  hSeparatorPublic = gtk_hseparator_new ();
  gtk_box_pack_start (
    GTK_BOX ( vboxPublic ), hSeparatorPublic, FALSE, FALSE, 0
  );
  hButtonBoxPublic = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxPublic ), GTK_BUTTONBOX_END
  );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxPublic ), 5 );
  buttonClose = gpa_buttonCancel_new (
    windowPublic, accelGroup, _( "_Close" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxPublic ), buttonClose );
  gtk_box_pack_start (
    GTK_BOX ( vboxPublic ), hButtonBoxPublic, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowPublic ), vboxPublic );
  gtk_widget_show_all ( windowPublic );
  gpa_widget_set_centered ( windowPublic, windowMain );
  gpa_windowTip_show ( _( "Dummy text" ) ); /*!!!*/
} /* keys_openPublic */

void keys_openSecret_editKey ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowEdit;
    GtkWidget *vboxEdit;
      GtkWidget *hboxTop;
	GtkWidget *labelKey;
	GtkWidget *entryKey;
      GtkWidget *tablePasswd;
	GtkWidget *labelJfdPasswd;
	  GtkWidget *labelPasswd;
	GtkWidget *entryPasswd;
	GtkWidget *labelJfdRepeat;
	  GtkWidget *labelRepeat;
	GtkWidget *entryRepeat;
      GtkWidget *frameExpire;
      GtkWidget *hButtonBoxEdit;
	GtkWidget *buttonRevoc;
	GtkWidget *buttonExport;
	GtkWidget *buttonClose;
/* commands */
  windowEdit = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowEdit ), _( "Secret key editor" )
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowEdit ), accelGroup );
  vboxEdit = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxEdit ), 5 );
  hboxTop = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxTop ), 5 );
  labelKey = gtk_label_new ( "Key: " );
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), labelKey, FALSE, FALSE, 0 );
  entryKey = gtk_entry_new ();
  gtk_editable_set_editable ( GTK_EDITABLE ( entryKey ), FALSE );
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), entryKey, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxEdit ), hboxTop, FALSE, FALSE, 0 );
  tablePasswd = gtk_table_new ( 2, 2, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tablePasswd ), 5 );
  labelPasswd = gtk_label_new ( _( "" ) );
  labelJfdPasswd = gpa_widget_hjustified_new (
    labelPasswd, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), labelJfdPasswd, 0, 1, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryPasswd = gtk_entry_new ();
  gtk_entry_set_visibility ( GTK_ENTRY ( entryPasswd ), FALSE );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelPasswd ), entryPasswd,
    accelGroup, _( "_Password: " )
  );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), entryPasswd, 1, 2, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  labelRepeat = gtk_label_new ( _( "" ) );
  labelJfdRepeat = gpa_widget_hjustified_new (
    labelRepeat, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), labelJfdRepeat, 0, 1, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryRepeat = gtk_entry_new ();
  gtk_entry_set_visibility ( GTK_ENTRY ( entryRepeat ), FALSE );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelRepeat ), entryRepeat,
    accelGroup, _( "_Repeat password: " )
  );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), entryRepeat, 1, 2, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxEdit ), tablePasswd, TRUE, TRUE, 0 );
  frameExpire = gpa_frameExpire_new ( accelGroup );
  gtk_container_set_border_width ( GTK_CONTAINER ( frameExpire ), 5 );
  gtk_box_pack_start ( GTK_BOX ( vboxEdit ), frameExpire, FALSE, FALSE, 0 );
  hButtonBoxEdit = gtk_hbutton_box_new ();
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxEdit ), 5 );
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxEdit ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxEdit ), 10 );
  buttonRevoc = gpa_button_new ( accelGroup, _( "Create Re_vocation" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonRevoc ), "clicked",
    GTK_SIGNAL_FUNC ( keys_revocation ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEdit ), buttonRevoc );
  buttonExport = gpa_button_new ( accelGroup, _( "E_xport key" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonExport ), "clicked",
    GTK_SIGNAL_FUNC ( keys_export ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEdit ), buttonExport );
  buttonClose = gpa_buttonCancel_new ( windowEdit, accelGroup, _( "_Close" ) );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEdit ), buttonClose );
  gtk_box_pack_start (
    GTK_BOX ( vboxEdit ), hButtonBoxEdit, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowEdit ), vboxEdit );
  gtk_widget_show_all ( windowEdit );
  gpa_widget_set_centered ( windowEdit, windowMain );
  gpa_windowTip_show ( _( "Dummy text" ) ); /*!!!*/
} /* keys_openSecret_editKey */

void keys_openSecret ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowSecret;
    GtkWidget *vboxSecret;
      GtkWidget *hboxTop;
	GtkWidget *labelRingname;
	GtkWidget *entryRingname;
      GtkWidget *scrollerKeys;
	GtkWidget *clistKeys;
      GtkWidget *hButtonBoxSecret;
	GtkWidget *buttonExport;
	GtkWidget *buttonDelete;
	GtkWidget *buttonEditKey;
	GtkWidget *buttonClose;
/* commands */
  windowSecret = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowSecret ), _( "Secret key ring editor" )
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowSecret ), accelGroup );
  vboxSecret = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxSecret ), 5 );
  hboxTop = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxTop ), 5 );
  labelRingname = gtk_label_new ( _( "" ) );
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), labelRingname, FALSE, FALSE, 0 );
  entryRingname = gtk_entry_new ();
  gtk_editable_set_editable ( GTK_EDITABLE ( entryRingname ), FALSE );
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), entryRingname, FALSE, FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxSecret ), hboxTop, FALSE, FALSE, 0 );
  scrollerKeys = gtk_scrolled_window_new ( NULL, NULL );
  gtk_container_set_border_width ( GTK_CONTAINER ( scrollerKeys ), 5 );
  gtk_widget_set_usize ( scrollerKeys, 400, 280 );
  clistKeys = gtk_clist_new ( 1 );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelRingname ), clistKeys, accelGroup, _( "_Key ring: " )
  );
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerKeys ), clistKeys );
  gtk_box_pack_start ( GTK_BOX ( vboxSecret ), scrollerKeys, TRUE, TRUE, 0 );
  hButtonBoxSecret = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxSecret ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxSecret ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxSecret ), 5 );
  buttonExport = gpa_button_new ( accelGroup, _( "E_xport keys" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonExport ), "clicked",
    GTK_SIGNAL_FUNC ( keys_export ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSecret ), buttonExport );
  buttonDelete = gpa_button_new ( accelGroup, _( "_Delete keys" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonDelete ), "clicked",
    GTK_SIGNAL_FUNC ( keys_delete ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSecret ), buttonDelete );
  buttonEditKey = gpa_button_new ( accelGroup, _( "_Edit key" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonEditKey ), "clicked",
    GTK_SIGNAL_FUNC ( keys_openSecret_editKey ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSecret ), buttonEditKey );
  buttonClose = gpa_buttonCancel_new (
    windowSecret, accelGroup, _( "_Close" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSecret ), buttonClose );
  gtk_box_pack_start (
    GTK_BOX ( vboxSecret ), hButtonBoxSecret, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowSecret ), vboxSecret );
  gtk_widget_show_all ( windowSecret );
  gpa_widget_set_centered ( windowSecret, windowMain );
  gpa_windowTip_show ( _( "Dummy text" ) ); /*!!!*/
} /* keys_openSecret */

void keys_open ( void ) {
  gtk_widget_show ( ringOpenSelect );
} /* keys_open */

void keys_generateKey_generate ( GtkWidget *windowGenerate ) {
g_print ( _( "Generate a new key\n" ) ); /*!!!*/
  gpa_window_destroy ( windowGenerate );
} /* keys_generateKey_generate */

void keys_generateKey ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  GList *contentsAlgorithm = NULL;
  GList *contentsKeysize = NULL;
/* objects */
  GtkWidget *windowGenerate;
    GtkWidget *vboxGenerate;
      GtkWidget *tableTop;
	GtkWidget *labelJfdAlgorithm;
	  GtkWidget *labelAlgorithm;
	GtkWidget *comboAlgorithm;
	GtkWidget *labelJfdKeysize;
	  GtkWidget *labelKeysize;
	GtkWidget *comboKeysize;
      GtkWidget *frameExpire;
      GtkWidget *tableMisc;
	GtkWidget *labelJfdUserID;
	  GtkWidget *labelUserID;
	GtkWidget *entryUserID;
	GtkWidget *labelJfdEmail;
	  GtkWidget *labelEmail;
	GtkWidget *entryEmail;
	GtkWidget *labelJfdComment;
	  GtkWidget *labelComment;
	GtkWidget *entryComment;
	GtkWidget *labelJfdPassword;
	  GtkWidget *labelPassword;
	GtkWidget *entryPassword;
	GtkWidget *labelJfdRepeat;
	  GtkWidget *labelRepeat;
	GtkWidget *entryRepeat;
      GtkWidget *vboxMisc;
	GtkWidget *checkerRevoc;
	GtkWidget *checkerSend;
      GtkWidget *hButtonBoxGenerate;
	GtkWidget *buttonCancel;
	GtkWidget *buttonGenerate;
/* commands */
  windowGenerate = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowGenerate ), _( "Generate key" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowGenerate ), accelGroup );
  vboxGenerate = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxGenerate ), 5 );
  tableTop = gtk_table_new ( 2, 2, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tableTop ), 5 );
  labelAlgorithm = gtk_label_new ( _( "" ) );
  labelJfdAlgorithm = gpa_widget_hjustified_new (
    labelAlgorithm, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), labelJfdAlgorithm, 0, 1, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  comboAlgorithm = gtk_combo_new ();
  gtk_editable_set_editable (
    GTK_EDITABLE ( GTK_COMBO ( comboAlgorithm ) -> entry ), FALSE
  );
  contentsAlgorithm = g_list_append (
    contentsAlgorithm, _( "DSA and ElGamal" )
  );
  contentsAlgorithm = g_list_append (
    contentsAlgorithm, _( "DSA (sign only)" )
  );
  contentsAlgorithm = g_list_append (
    contentsAlgorithm, _( "ElGamal (sign and encrypt)" )
  );
  contentsAlgorithm = g_list_append (
    contentsAlgorithm, _( "ElGamal (encrypt only)" )
  );
  gtk_combo_set_popdown_strings (
    GTK_COMBO ( comboAlgorithm ), contentsAlgorithm
  );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelAlgorithm ), GTK_COMBO ( comboAlgorithm ) -> entry,
    accelGroup, _( "_Encryption algorithm: ")
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), comboAlgorithm, 1, 2, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  labelKeysize = gtk_label_new ( _( "" ) );
  labelJfdKeysize = gpa_widget_hjustified_new (
    labelKeysize, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), labelJfdKeysize, 0, 1, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  comboKeysize = gtk_combo_new ();
  gtk_combo_set_value_in_list ( GTK_COMBO ( comboKeysize ), FALSE, FALSE );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelKeysize ), GTK_COMBO ( comboKeysize ) -> entry,
    accelGroup, _( "_Key size (bits): " )
  );
  contentsKeysize = g_list_append ( contentsKeysize, _( "1024" ) );
  contentsKeysize = g_list_append ( contentsKeysize, _( "768" ) );
  contentsKeysize = g_list_append ( contentsKeysize, _( "2048" ) );
  gtk_combo_set_popdown_strings (
    GTK_COMBO ( comboKeysize ), contentsKeysize
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), comboKeysize, 1, 2, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxGenerate ), tableTop, FALSE, FALSE, 0 );
  frameExpire = gpa_frameExpire_new ( accelGroup );
  gtk_container_set_border_width ( GTK_CONTAINER ( frameExpire ), 5 );
  gtk_box_pack_start (
    GTK_BOX ( vboxGenerate ), frameExpire, FALSE, FALSE, 0
  );
  tableMisc = gtk_table_new ( 5, 2, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tableMisc ), 5 );
  labelUserID = gtk_label_new ( _( "" ) );
  labelJfdUserID = gpa_widget_hjustified_new (
    labelUserID, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), labelJfdUserID, 0, 1, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryUserID = gtk_entry_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelUserID ), entryUserID, accelGroup, _( "_User ID: " )
  );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), entryUserID, 1, 2, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  labelEmail = gtk_label_new ( _( "" ) );
  labelJfdEmail = gpa_widget_hjustified_new ( labelEmail, GTK_JUSTIFY_RIGHT );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), labelJfdEmail, 0, 1, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryEmail = gtk_entry_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelEmail ), entryEmail, accelGroup, _( "E-_Mail: " )
  );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), entryEmail, 1, 2, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  labelComment = gtk_label_new ( _( "" ) );
  labelJfdComment = gpa_widget_hjustified_new (
    labelComment, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), labelJfdComment, 0, 1, 2, 3,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryComment = gtk_entry_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelComment ), entryComment, accelGroup, _( "C_omment: " )
  );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), entryComment, 1, 2, 2, 3,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  labelPassword = gtk_label_new ( _( "" ) );
  labelJfdPassword = gpa_widget_hjustified_new (
    labelPassword, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), labelJfdPassword, 0, 1, 3, 4,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryPassword = gtk_entry_new ();
  gtk_entry_set_visibility ( GTK_ENTRY ( entryPassword ), FALSE );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelPassword ), entryPassword, accelGroup, _( "_Password: " )
  );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), entryPassword, 1, 2, 3, 4,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  labelRepeat = gtk_label_new ( _( "" ) );
  labelJfdRepeat = gpa_widget_hjustified_new (
    labelRepeat, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), labelJfdRepeat, 0, 1, 4, 5,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryRepeat = gtk_entry_new ();
  gtk_entry_set_visibility ( GTK_ENTRY ( entryRepeat ), FALSE );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelRepeat ), entryRepeat,
    accelGroup, _( "_Repeat password: " )
  );
  gtk_table_attach (
    GTK_TABLE ( tableMisc ), entryRepeat, 1, 2, 4, 5,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxGenerate ), tableMisc, TRUE, FALSE, 0
  );
  vboxMisc = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxMisc ), 5 );
  checkerRevoc = gpa_check_button_new (
    accelGroup, _( "generate re_vocation certificate" )
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxMisc ), checkerRevoc, FALSE, FALSE, 0
  );
  checkerSend = gpa_check_button_new (
    accelGroup, _( "_send to key server" )
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxMisc ), checkerSend, FALSE, FALSE, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxGenerate ), vboxMisc, FALSE, FALSE, 0
  );
  hButtonBoxGenerate = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxGenerate ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxGenerate ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxGenerate ), 5 );
  buttonCancel = gpa_buttonCancel_new (
    windowGenerate, accelGroup, _( "_Cancel" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxGenerate ), buttonCancel );
  buttonGenerate = gpa_button_new ( accelGroup, _( "_Generate key" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonGenerate ), "clicked",
    GTK_SIGNAL_FUNC ( keys_generateKey_generate ), (gpointer) windowGenerate
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxGenerate ), buttonGenerate );
  gtk_box_pack_start (
    GTK_BOX ( vboxGenerate ), hButtonBoxGenerate, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowGenerate ), vboxGenerate );
  gtk_widget_show_all ( windowGenerate );
  gpa_widget_set_centered ( windowGenerate, windowMain );
  gpa_windowTip_show ( _( "Dummy text" ) ); /*!!!*/
} /* keys_generateKey */

void keys_generateRevocation_generate ( GtkWidget *windowRevoc ) {
  keys_revocation (); /*!!!*/
  gpa_window_destroy ( windowRevoc );
} /* keys_generateRevocation_generate */

void keys_generateRevocation ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowRevoc;
    GtkWidget *vboxRevoc;
      GtkWidget *vboxKeys;
	GtkWidget *labelJfdKeys;
	  GtkWidget *labelKeys;
	GtkWidget *scrollerKeys;
	  GtkWidget *clistKeys;
      GtkWidget *hButtonBoxRevoc;
	GtkWidget *buttonCancel;
	GtkWidget *buttonGenerate;
/* commands */
  windowRevoc = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowRevoc ),
    _( "Generate revocation certificate" )
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowRevoc ), accelGroup );
  vboxRevoc = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxRevoc ), 5 );
  vboxKeys = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxKeys ), 5 );
  labelKeys = gtk_label_new ( _( "" ) );
  labelJfdKeys = gpa_widget_hjustified_new ( labelKeys, GTK_JUSTIFY_LEFT );
  gtk_box_pack_start ( GTK_BOX ( vboxKeys ), labelJfdKeys, FALSE, FALSE, 0 );
  scrollerKeys = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrollerKeys, 280, 200 );
  clistKeys = gtk_clist_new ( 1 );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelKeys ), clistKeys, accelGroup, _( "_Secret keys" )
  );
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerKeys ), clistKeys );
  gtk_box_pack_start ( GTK_BOX ( vboxKeys ), scrollerKeys, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxRevoc ), vboxKeys, TRUE, TRUE, 0 );
  hButtonBoxRevoc = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxRevoc ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxRevoc ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxRevoc ), 5 );
  buttonCancel = gpa_buttonCancel_new (
    windowRevoc, accelGroup, _( "_Cancel" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRevoc ), buttonCancel );
  buttonGenerate = gpa_button_new ( accelGroup, _( "_Generate" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonGenerate ), "clicked",
    GTK_SIGNAL_FUNC ( keys_generateRevocation_generate ),
    (gpointer) windowRevoc
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRevoc ), buttonGenerate );
  gtk_box_pack_start (
    GTK_BOX ( vboxRevoc ), hButtonBoxRevoc, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowRevoc ), vboxRevoc );
  gtk_widget_show_all ( windowRevoc );
  gpa_widget_set_centered ( windowRevoc, windowMain );
  gpa_windowTip_show ( _( "Dummy text" ) ); /*!!!*/
} /* keys_generateRevocation */

void keys_import_ok ( GtkWidget *windowImport ) {
char message [ 100 ];
sprintf ( message, _( "Import keys from '%s'\n" ), gtk_file_selection_get_filename ( GTK_FILE_SELECTION ( windowImport ) ) ); /*!!!*/
g_print ( message ); /*!!!*/
  gpa_window_destroy ( windowImport );
} /* keys_import_ok */

void keys_import ( void ) {
/* objects */
  GtkWidget *windowImport;
/* commands */
  windowImport = gtk_file_selection_new ( "Import keys" );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( windowImport ) -> ok_button ), "clicked",
    GTK_SIGNAL_FUNC ( keys_import_ok ), (gpointer) windowImport
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( windowImport ) -> cancel_button ),
    "clicked", GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) windowImport
  );
  gtk_widget_show ( windowImport );
} /* keys_import */

void keys_importOwnertrust_ok ( GtkWidget *windowImport ) {
char message [ 100 ];
sprintf ( message, _( "Import ownertrust from '%s'\n" ), gtk_file_selection_get_filename ( GTK_FILE_SELECTION ( windowImport ) ) ); /*!!!*/
g_print ( message ); /*!!!*/
  gpa_window_destroy ( windowImport );
} /*!!!*/

void keys_importOwnertrust ( void ) {
/* objects */
  GtkWidget *windowImport;
/* commands */
  windowImport = gtk_file_selection_new ( "Import ownertrust" );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( windowImport ) -> ok_button ), "clicked",
    GTK_SIGNAL_FUNC ( keys_importOwnertrust_ok ), (gpointer) windowImport
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( windowImport ) -> cancel_button ),
    "clicked", GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) windowImport
  );
  gtk_widget_show ( windowImport );
} /* keys_importOwnertrust */

void keys_updateTrust ( void ) {
g_print ( _( "Update Trust Database\n" ) ); /*!!!*/
} /* keys_updateTrust */
