/* gpa_file.c  -  The GNU Privacy Assistant
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

static GtkWidget *fileOpenSelect;
/*!!!*/ static char *text [] = { N_( "Dummy Text" ) }; /*!!!*/
/*!!!*/ static char *text2 [] = { N_( "Dummy Text" ), N_( "Dummy Text" ) }; /*!!!*/

void file_open_ok ( void ) {
char message [ 100 ];
sprintf ( message, _( "Open file '%s'\n" ), gtk_file_selection_get_filename ( GTK_FILE_SELECTION ( fileOpenSelect ) ) ); /*!!!*/
g_print ( message ); /*!!!*/
gtk_widget_hide ( fileOpenSelect );
} /* file_open_ok */

void gpa_fileOpenSelect_init ( char *title ) {
  fileOpenSelect = gtk_file_selection_new ( title );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( fileOpenSelect ) -> ok_button ), "clicked",
    GTK_SIGNAL_FUNC ( file_open_ok ), (gpointer) fileOpenSelect
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( fileOpenSelect ) -> cancel_button ),
    "clicked", GTK_SIGNAL_FUNC ( gtk_widget_hide ), (gpointer) fileOpenSelect
  );
} /* gpa_fileOpenSelect_init */

void file_open ( void ) {
  gtk_window_set_position (
    GTK_WINDOW ( fileOpenSelect ), GTK_WIN_POS_CENTER
  );
  gtk_widget_show ( fileOpenSelect );
} /* file_open */

void file_showDetail ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  gchar *titlesSignatures [ 2 ] = { N_( "Signature" ), N_( "Validity" ) };
/* objects */
  GtkWidget *windowDetail;
    GtkWidget *vboxDetail;
      GtkWidget *hboxFilename;
        GtkWidget *labelFilename;
        GtkWidget *entryFilename;
      GtkWidget *checkerEncrypted;
      GtkWidget *vboxSignatures;
        GtkWidget *labelJfdSignatures;
          GtkWidget *labelSignatures;
        GtkWidget *scrollerSignatures;
          GtkWidget *clistSignatures;
      GtkWidget *hButtonBoxDetail;
        GtkWidget *buttonClose;
/* commands */
  windowDetail = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowDetail ), _( "Detailed file view" )
  );
  gtk_widget_set_usize ( windowDetail, 280, 400 );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowDetail ), accelGroup );
  vboxDetail = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxDetail ), 5 );
  hboxFilename = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxFilename ), 5 );
  labelFilename = gtk_label_new ( _( "Filename: " ) );
  gtk_box_pack_start (
    GTK_BOX ( hboxFilename ), labelFilename, FALSE, FALSE, 0
  );
  entryFilename = gtk_entry_new ();
  gtk_entry_set_editable ( GTK_ENTRY ( entryFilename ), FALSE );
  gtk_box_pack_start (
    GTK_BOX ( hboxFilename ), entryFilename, TRUE, TRUE, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxDetail ), hboxFilename, FALSE, FALSE, 0
  );
  checkerEncrypted = gtk_check_button_new_with_label ( _( " encrypted" ) );
  gtk_container_set_border_width ( GTK_CONTAINER ( checkerEncrypted ), 5 );
  gtk_signal_connect (
    GTK_OBJECT ( checkerEncrypted ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_toggle_button_set_active ),
    (gpointer) gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON ( checkerEncrypted )
    )
  ); /*-- Disables clicking of checkerEncrypted --*/
  gtk_box_pack_start (
    GTK_BOX ( vboxDetail ), checkerEncrypted, FALSE, FALSE, 0
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
  clistSignatures = gtk_clist_new_with_titles ( 2, titlesSignatures );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelSignatures ), clistSignatures,
    accelGroup, _( "_Signatures" )
  );
gtk_clist_append ( GTK_CLIST ( clistSignatures ), text2 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistSignatures ), text2 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistSignatures ), text2 ); /*!!!*/
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 0, 150 );
  gtk_container_add ( GTK_CONTAINER ( scrollerSignatures ), clistSignatures );
  gtk_box_pack_start (
    GTK_BOX ( vboxSignatures ), scrollerSignatures, TRUE, TRUE, 0
  );
  gtk_box_pack_start (
     GTK_BOX ( vboxDetail ), vboxSignatures, TRUE, TRUE, 0
  );
  hButtonBoxDetail = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxDetail ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxDetail ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxDetail ), 5 );
  buttonClose = gpa_buttonCancel_new (
    windowDetail, accelGroup, _( "_Close" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDetail ), buttonClose );
  gtk_box_pack_start (
    GTK_BOX ( vboxDetail ), hButtonBoxDetail, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowDetail ), vboxDetail );
  gtk_widget_show_all ( windowDetail );
  gpa_widget_set_centered ( windowDetail, windowMain );
} /* file_showDetail */

void file_sign_sign ( GtkWidget *windowSign ) {
g_print ( _( "Sign a file\n" ) ); /*!!!*/
  gtk_widget_destroy ( windowSign );
} /* file_sign_sign */

void file_sign ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowSign;
    GtkWidget *vboxSign;
      GtkWidget *frameMode;
        GtkWidget *vboxMode;
          GtkWidget *radioSignComp;
          GtkWidget *radioSign;
          GtkWidget *radioSignSep;
      GtkWidget *checkerArmor;
      GtkWidget *tableWho;
        GtkWidget *labelJfdWho;
          GtkWidget *labelWho;
        GtkWidget *comboWho;
        GtkWidget *labelJfdPasswd;
          GtkWidget *labelPasswd;
        GtkWidget *entryJfdPasswd;
          GtkWidget *entryPasswd;
      GtkWidget *hButtonBoxSign;
        GtkWidget *buttonCancel;
        GtkWidget *buttonSign;
/* commands */
  windowSign = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowSign ), _( "Sign files" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowSign ), accelGroup );
  vboxSign = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxSign ), 5 );
  frameMode = gtk_frame_new ( _( "Signing mode" ) );
  gtk_container_set_border_width ( GTK_CONTAINER ( frameMode ), 5 );
  vboxMode = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxMode ), 5 );
  radioSignComp = gpa_radio_button_new (
    accelGroup, _( "si_gn and compress" )
  );
  gtk_box_pack_start ( GTK_BOX ( vboxMode ), radioSignComp, FALSE, FALSE, 0 );
  radioSign = gpa_radio_button_new_from_widget (
    GTK_RADIO_BUTTON ( radioSignComp ), accelGroup,
    _( "sign, do_n't compress" )
  );
  gtk_box_pack_start ( GTK_BOX ( vboxMode ), radioSign, FALSE, FALSE, 0 );
  radioSignSep = gpa_radio_button_new_from_widget (
    GTK_RADIO_BUTTON ( radioSignComp  ), accelGroup,
    _( "sign in separate _file" )
  );
  gtk_box_pack_start ( GTK_BOX ( vboxMode ), radioSignSep, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( frameMode ), vboxMode );
  gtk_box_pack_start ( GTK_BOX ( vboxSign ), frameMode, FALSE, FALSE, 0 );
  checkerArmor = gpa_check_button_new ( accelGroup, _( "a_rmor" ) );
  gtk_container_set_border_width ( GTK_CONTAINER ( checkerArmor ), 5 );
  gtk_box_pack_start ( GTK_BOX ( vboxSign ), checkerArmor, FALSE, FALSE, 0 );
  tableWho = gtk_table_new ( 2, 2, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tableWho ), 5 );
  labelWho = gtk_label_new ( _( "" ) );
  labelJfdWho = gpa_widget_hjustified_new ( labelWho, GTK_JUSTIFY_RIGHT );
  gtk_table_attach (
    GTK_TABLE ( tableWho ), labelJfdWho, 0, 1, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  comboWho = gtk_combo_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelWho ), GTK_COMBO ( comboWho ) -> entry,
    accelGroup, _( "Sign _as " )
  );
  gtk_table_attach (
    GTK_TABLE ( tableWho ), comboWho, 1, 2, 0, 1,
    GTK_SHRINK, GTK_SHRINK, 0, 0
  );
  labelPasswd = gtk_label_new ( _( "" ) );
  labelJfdPasswd = gpa_widget_hjustified_new (
    labelPasswd, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableWho ), labelJfdPasswd, 0, 1, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryPasswd = gtk_entry_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelPasswd ), entryPasswd, accelGroup, _( "_Password: " )
  );
  gtk_entry_set_visibility ( GTK_ENTRY ( entryPasswd ), FALSE );
  entryJfdPasswd = gpa_widget_hjustified_new ( entryPasswd, GTK_JUSTIFY_LEFT );
  gtk_table_attach (
    GTK_TABLE ( tableWho ), entryJfdPasswd, 1, 2, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxSign ), tableWho, TRUE, FALSE, 0 );
  hButtonBoxSign = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxSign ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxSign ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxSign ), 5 );
  buttonCancel = gpa_buttonCancel_new (
    windowSign, accelGroup, _( "_Cancel" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSign ), buttonCancel );
  buttonSign = gpa_button_new ( accelGroup, "_Sign" );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonSign ), "clicked",
    GTK_SIGNAL_FUNC ( file_sign_sign ), (gpointer) windowSign
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSign ), buttonSign );
  gtk_box_pack_start ( GTK_BOX ( vboxSign ), hButtonBoxSign, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( windowSign ), vboxSign );
  gtk_widget_show_all ( windowSign );
  gpa_widget_set_centered ( windowSign, windowMain );
} /* file_sign */

void file_encrypt_detail_check ( void ) {
g_print ( _( "Check signature validities\n" ) ); /*!!!*/
} /* file_encrypt_detail_check */

void file_encrypt_detail ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  char *titlesSignatures [] = { N_( "Signature" ), N_( "valid" ) };
/* objects */
  GtkWidget *windowSigs;
    GtkWidget *vboxSigs;
      GtkWidget *vboxSignatures;
        GtkWidget *labelJfdSignatures;
          GtkWidget *labelSignatures;
        GtkWidget *scrollerSignatures;
          GtkWidget *clistSignatures;
      GtkWidget *hboxFingerprint;
        GtkWidget *labelFingerprint;
        GtkWidget *entryFingerprint;
      GtkWidget *hButtonBoxSigs;
        GtkWidget *buttonCheck;
        GtkWidget *buttonClose;
/* commands */
  windowSigs = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowSigs ), _( "Show public key detail" )
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowSigs ), accelGroup );
  vboxSigs = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxSigs ), 5 );
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
  gtk_widget_set_usize ( scrollerSignatures, 320, 200 );
  clistSignatures = gtk_clist_new_with_titles ( 2, titlesSignatures );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 0, 200 );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 1, 80 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistSignatures ), 1, GTK_JUSTIFY_CENTER
  );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelSignatures ), clistSignatures,
    accelGroup, _( "_Signatures" )
  );
gtk_clist_append ( GTK_CLIST ( clistSignatures ), text2 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistSignatures ), text2 ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistSignatures ), text2 ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerSignatures ), clistSignatures );
  gtk_box_pack_start (
    GTK_BOX ( vboxSignatures ), scrollerSignatures, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxSigs ), vboxSignatures, TRUE, TRUE, 0 );
  hboxFingerprint = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxFingerprint ), 5 );
  labelFingerprint = gtk_label_new ( _( "Fingerprint: " ) );
  gtk_box_pack_start (
    GTK_BOX ( hboxFingerprint ), labelFingerprint, FALSE, FALSE, 0
  );
  entryFingerprint = gtk_entry_new ();
  gtk_entry_set_editable ( GTK_ENTRY ( entryFingerprint ), FALSE );
  gtk_box_pack_start (
    GTK_BOX ( hboxFingerprint ), entryFingerprint, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxSigs ), hboxFingerprint, FALSE, FALSE, 0 );
  hButtonBoxSigs = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxSigs ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxSigs ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxSigs ), 5 );
  buttonCheck = gpa_button_new ( accelGroup, _( "Chec_k" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonCheck ), "clicked",
    GTK_SIGNAL_FUNC ( file_encrypt_detail_check ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSigs ), buttonCheck );
  buttonClose = gpa_buttonCancel_new ( windowSigs, accelGroup, _( "_Close" ) );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSigs ), buttonClose );
  gtk_box_pack_start ( GTK_BOX ( vboxSigs ), hButtonBoxSigs, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( windowSigs ), vboxSigs );
  gtk_widget_show_all ( windowSigs );
  gpa_widget_set_centered ( windowSigs, windowMain );
} /* file_encrypt_detail */

void file_encrypt_encrypt ( GtkWidget *windowEncrypt ) {
g_print ( _( "Encrypt a file\n" ) ); /*!!!*/
  gtk_widget_destroy ( windowEncrypt );
} /* file_encrypt_encrypt */

void file_encrypt ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowEncrypt;
    GtkWidget *vboxEncrypt;
      GtkWidget *vboxRecKeys;
        GtkWidget *tableRecKeys;
          GtkWidget *vboxDefault;
            GtkWidget *checkerJfdDefault;
              GtkWidget *checkerDefault;
            GtkWidget *scrollerDefault;
              GtkWidget *clistDefault;
          GtkWidget *vboxRecipients;
            GtkWidget *labelJfdRecipients;
              GtkWidget *labelRecipients;
            GtkWidget *scrollerRecipients;
              GtkWidget *clistRecipients;
          GtkWidget *vboxKeys;
            GtkWidget *labelJfdKeys;
              GtkWidget *labelKeys;
            GtkWidget *scrollerKeys;
              GtkWidget *clistKeys;
        GtkWidget *hButtonBoxRecKeys;
          GtkWidget *buttonAdd;
          GtkWidget *buttonDetail;
      GtkWidget *vboxMisc;
        GtkWidget *hboxSaveAs;
          GtkWidget *labelSaveAs;
          GtkWidget *entrySaveAs;
        GtkWidget *checkerSign;
        GtkWidget *checkerArmor;
      GtkWidget *hButtonBoxEncrypt;
        GtkWidget *buttonCancel;
        GtkWidget *buttonEncrypt;
/* commands */
  windowEncrypt = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowEncrypt ), _( "Encrypt files" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowEncrypt ), accelGroup );
  vboxEncrypt = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxEncrypt ), 5 );
  vboxRecKeys = gtk_vbox_new ( FALSE, 0 );
  tableRecKeys = gtk_table_new ( 2, 2, FALSE );
  vboxDefault = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxDefault ), 5 );
  checkerDefault = gpa_check_button_new (
    accelGroup, _( "Add _default recipients:"  )
  );
  checkerJfdDefault = gpa_widget_hjustified_new (
    checkerDefault, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxDefault ), checkerJfdDefault, FALSE, FALSE, 0
  );
  scrollerDefault = gtk_scrolled_window_new ( NULL, NULL );
  clistDefault = gtk_clist_new ( 1 );
  gtk_signal_connect_object (
    GTK_OBJECT ( checkerDefault ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_grab_focus ), (gpointer) clistDefault
  );
gtk_clist_append ( GTK_CLIST ( clistDefault ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistDefault ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistDefault ), text ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerDefault ), clistDefault );
  gtk_box_pack_start (
    GTK_BOX ( vboxDefault ), scrollerDefault, TRUE, TRUE, 0
  );
  gtk_table_attach (
    GTK_TABLE ( tableRecKeys ), vboxDefault, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 0, 0
  );
  vboxRecipients = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxRecipients ), 5 );
  labelRecipients = gtk_label_new ( _( "" ) );
  labelJfdRecipients = gpa_widget_hjustified_new (
    labelRecipients, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxRecipients ), labelJfdRecipients, FALSE, FALSE, 0
  );
  scrollerRecipients = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrollerRecipients, 300, 200 );
  clistRecipients = gtk_clist_new ( 1 );
gtk_clist_append ( GTK_CLIST ( clistRecipients ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistRecipients ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistRecipients ), text ); /*!!!*/
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelRecipients ),
    clistRecipients, accelGroup, _( "Rec_ipients" )
  );
  gtk_container_add ( GTK_CONTAINER ( scrollerRecipients ), clistRecipients );
  gtk_box_pack_start (
    GTK_BOX ( vboxRecipients ), scrollerRecipients, TRUE, TRUE, 0
  );
  gtk_table_attach (
    GTK_TABLE ( tableRecKeys ), vboxRecipients, 0, 1, 1, 2,
    GTK_FILL, GTK_FILL, 0, 0
  );
  vboxKeys = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxKeys ), 5 );
  labelKeys = gtk_label_new ( _( "" ) );
  labelJfdKeys = gpa_widget_hjustified_new ( labelKeys, GTK_JUSTIFY_LEFT );
  gtk_box_pack_start (
    GTK_BOX ( vboxKeys ), labelJfdKeys, FALSE, FALSE, 0
  );
  scrollerKeys = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrollerKeys, 180, 200 );
  clistKeys = gtk_clist_new ( 1 );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelKeys ), clistKeys, accelGroup, _( "_Public keys" )
  );
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
gtk_clist_append ( GTK_CLIST ( clistKeys ), text ); /*!!!*/
  gtk_container_add ( GTK_CONTAINER ( scrollerKeys ), clistKeys );
  gtk_box_pack_start (
    GTK_BOX ( vboxKeys ), scrollerKeys, TRUE, TRUE, 0
  );
  gtk_table_attach (
    GTK_TABLE ( tableRecKeys ), vboxKeys, 1, 2, 1, 2,
    GTK_FILL, GTK_FILL, 0, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxRecKeys ), tableRecKeys, TRUE, TRUE, 0 );
  hButtonBoxRecKeys = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxRecKeys ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxRecKeys ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxRecKeys ), 5 );
  buttonAdd = gpa_button_new ( accelGroup, _( "Add _key to recipients" ) );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecKeys ), buttonAdd );
  buttonDetail = gpa_button_new ( accelGroup, _( "S_how detail" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonDetail ), "clicked",
    GTK_SIGNAL_FUNC ( file_encrypt_detail ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecKeys ), buttonDetail );
  gtk_box_pack_start (
    GTK_BOX ( vboxRecKeys ), hButtonBoxRecKeys, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxEncrypt ), vboxRecKeys, TRUE, TRUE, 0 );
  vboxMisc = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxMisc ), 5 );
  hboxSaveAs = gtk_hbox_new ( FALSE, 0 );
  labelSaveAs = gtk_label_new ( _( "" ) );
  gtk_box_pack_start ( GTK_BOX ( hboxSaveAs ), labelSaveAs, FALSE, FALSE, 0 );
  entrySaveAs = gtk_entry_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelSaveAs ), entrySaveAs,
    accelGroup, _( "Save encrypted file _as: " )
  );
  gtk_box_pack_start ( GTK_BOX ( hboxSaveAs ), entrySaveAs, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxMisc ), hboxSaveAs, FALSE, FALSE, 0 );
  checkerSign = gpa_check_button_new ( accelGroup, _( "_sign" ) );
  gtk_box_pack_start ( GTK_BOX ( vboxMisc ), checkerSign, FALSE, FALSE, 0 );
  checkerArmor = gpa_check_button_new ( accelGroup, _( "a_rmor" ) );
  gtk_box_pack_start ( GTK_BOX ( vboxMisc ), checkerArmor, FALSE, FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxEncrypt ), vboxMisc, FALSE, FALSE, 0 );
  hButtonBoxEncrypt = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxEncrypt ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxEncrypt ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxEncrypt ), 5 );
  buttonCancel = gpa_buttonCancel_new (
    windowEncrypt, accelGroup, _( "_Cancel" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEncrypt ), buttonCancel );
  buttonEncrypt = gpa_button_new ( accelGroup, _( "_Encrypt" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonEncrypt ), "clicked",
    GTK_SIGNAL_FUNC ( file_encrypt_encrypt ), (gpointer) windowEncrypt
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEncrypt ), buttonEncrypt );
  gtk_box_pack_start (
    GTK_BOX ( vboxEncrypt ), hButtonBoxEncrypt, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowEncrypt ), vboxEncrypt );
  gtk_widget_show_all ( windowEncrypt );
  gpa_widget_set_centered ( windowEncrypt, windowMain );
} /* file_encrypt */

void file_protect_protect ( GtkWidget *windowProtect ) {
g_print ( _( "Protect a file by Password\n" ) ); /*!!!*/
  gtk_widget_destroy ( windowProtect );
} /* file_protect_protect */

void file_protect ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowProtect;
    GtkWidget *vboxProtect;
      GtkWidget *tablePasswd;
        GtkWidget *labelJfdPasswd;
          GtkWidget *labelPasswd;
        GtkWidget *entryPasswd;
        GtkWidget *labelJfdRepeat;
          GtkWidget *labelRepeat;
        GtkWidget *entryRepeat;
      GtkWidget *checkerArmor;
      GtkWidget *hButtonBoxProtect;
        GtkWidget *buttonCancel;
        GtkWidget *buttonProtect;
/* commands */
  windowProtect = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowProtect ), _( "Protect files by Password" )
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowProtect ), accelGroup );
  vboxProtect = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxProtect ), 5 );
  tablePasswd = gtk_table_new ( 2, 2, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tablePasswd ), 5 );
  labelPasswd = gtk_label_new ( _( "" ) );
  labelJfdPasswd = gpa_widget_hjustified_new ( labelPasswd, GTK_JUSTIFY_RIGHT );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), labelJfdPasswd, 0, 1, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryPasswd = gtk_entry_new ();
  gtk_entry_set_visibility ( GTK_ENTRY ( entryPasswd ), FALSE );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelPasswd ), entryPasswd, accelGroup, _( "P_assword: " )
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
    GTK_EXPAND, GTK_SHRINK, 0, 0
  );
  entryRepeat = gtk_entry_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelRepeat ), entryRepeat,
    accelGroup, _( "Repeat Pa_ssword: " )
  );
  gtk_entry_set_visibility ( GTK_ENTRY ( entryRepeat ), FALSE );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), entryRepeat, 1, 2, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxProtect ), tablePasswd, TRUE, TRUE, 0
  );
  checkerArmor = gpa_check_button_new ( accelGroup, _( "a_rmor" ) );
  gtk_container_set_border_width ( GTK_CONTAINER ( checkerArmor ), 5 );
  gtk_box_pack_start (
    GTK_BOX ( vboxProtect ), checkerArmor, FALSE, FALSE, 0
  );
  hButtonBoxProtect = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxProtect ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxProtect ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxProtect ), 5 );
  buttonCancel = gpa_buttonCancel_new (
    windowProtect, accelGroup, _( "_Cancel" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxProtect ), buttonCancel );
  buttonProtect = gpa_button_new ( accelGroup, _( "_Protect" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonProtect ), "clicked",
    GTK_SIGNAL_FUNC ( file_protect_protect ), (gpointer) windowProtect
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxProtect ), buttonProtect );
  gtk_box_pack_start (
    GTK_BOX ( vboxProtect ), hButtonBoxProtect, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowProtect ), vboxProtect );
  gtk_widget_show_all ( windowProtect );
  gpa_widget_set_centered ( windowProtect, windowMain );
} /* file_protect */

void file_decrypt_decrypt ( GtkWidget *windowDecrypt ) {
g_print ( _( "Decrypt a file\n" ) ); /*!!!*/
  gtk_widget_destroy ( windowDecrypt );
} /* file_decrypt_decrypt */

void file_decrypt ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowDecrypt;
    GtkWidget *vboxDecrypt;
      GtkWidget *hboxPasswd;
        GtkWidget *labelPasswd;
        GtkWidget *entryPasswd;
      GtkWidget *hButtonBoxDecrypt;
        GtkWidget *buttonCancel;
        GtkWidget *buttonDecrypt;
/* commands */
  windowDecrypt = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowDecrypt ), _( "Decrypt files" ) );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowDecrypt ), accelGroup );
  vboxDecrypt = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxDecrypt ), 5 );
  hboxPasswd = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxPasswd ), 5 );
  labelPasswd = gtk_label_new ( _( "" ) );
  gtk_box_pack_start (
    GTK_BOX ( hboxPasswd ), labelPasswd, FALSE, FALSE, 0
  );
  entryPasswd = gtk_entry_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelPasswd ), entryPasswd,
    accelGroup, _( "_Password: " )
  );
  gtk_entry_set_visibility ( GTK_ENTRY ( entryPasswd ), FALSE );
  gtk_box_pack_start (
    GTK_BOX ( hboxPasswd ), entryPasswd, TRUE, TRUE, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxDecrypt ), hboxPasswd, TRUE, TRUE, 0
  );
  hButtonBoxDecrypt = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxDecrypt ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxDecrypt ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxDecrypt ), 5 );
  buttonCancel = gpa_buttonCancel_new (
    windowDecrypt, accelGroup, _( "_Cancel" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDecrypt ), buttonCancel );
  buttonDecrypt = gpa_button_new ( accelGroup, _( "_Decrypt" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonDecrypt ), "clicked",
    GTK_SIGNAL_FUNC ( file_decrypt_decrypt ), (gpointer) windowDecrypt
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDecrypt ), buttonDecrypt );
  gtk_box_pack_start (
    GTK_BOX ( vboxDecrypt ), hButtonBoxDecrypt, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowDecrypt ), vboxDecrypt );
  gtk_widget_show_all ( windowDecrypt );
  gpa_widget_set_centered ( windowDecrypt, windowMain );
} /* file_decrypt */

void file_decryptAs_browse_ok ( GtkWidget *browseSelect ) {
  gtk_widget_destroy ( browseSelect );
} /* file_decryptAs_browse_ok */

void file_decryptAs_browse ( GtkWidget *entryFilename ) {
/* objects */
  GtkWidget *browseSelect;
/* commands */
  browseSelect = gtk_file_selection_new ( "Save decrypted file as" );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( browseSelect ) -> ok_button ),
    "clicked", GTK_SIGNAL_FUNC ( file_decryptAs_browse_ok ),
    (gpointer) browseSelect
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( browseSelect ) -> cancel_button ),
    "clicked", GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) browseSelect
  );
  gtk_widget_show ( browseSelect );
} /* file_decryptAs_browse */

void file_decryptAs_decrypt ( GtkWidget *windowDecrypt ) {
g_print ( "Decrypt file and save with new name\n" ); /*!!!*/
  gtk_widget_destroy ( windowDecrypt );
} /* file_decryptAs_decrypt */

void file_decryptAs ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
/* objects */
  GtkWidget *windowDecrypt;
    GtkWidget *vboxDecrypt;
      GtkWidget *tableTop;
        GtkWidget *labelJfdFilename;
          GtkWidget *labelFilename;
        GtkWidget *entryFilename;
        GtkWidget *buttonFilename;
        GtkWidget *labelJfdPasswd;
          GtkWidget *labelPasswd;
        GtkWidget *entryPasswd;
      GtkWidget *hButtonBoxDecrypt;
        GtkWidget *buttonCancel;
        GtkWidget *buttonDecrypt;
/* commands */
  windowDecrypt = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title ( GTK_WINDOW ( windowDecrypt ), "Decrypt file" );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowDecrypt ), accelGroup );
  vboxDecrypt = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxDecrypt ), 5 );
  tableTop = gtk_table_new ( 2, 3, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tableTop ), 5 );
  labelFilename = gtk_label_new ( _( "" ) );
  labelJfdFilename = gpa_widget_hjustified_new (
    labelFilename, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), labelJfdFilename, 0, 1, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryFilename = gtk_entry_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelFilename ), entryFilename,
    accelGroup, _( "Save file _as: " )
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), entryFilename, 1, 2, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  buttonFilename = gpa_button_new ( accelGroup, _( "_Browse" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonFilename ), "clicked",
    GTK_SIGNAL_FUNC ( file_decryptAs_browse ), (gpointer) entryFilename
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), buttonFilename, 2, 3, 0, 1,
    GTK_FILL, GTK_SHRINK, 10, 0
  );
  labelPasswd = gtk_label_new ( _( "" ) );
  labelJfdPasswd = gpa_widget_hjustified_new (
    labelPasswd, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), labelJfdPasswd, 0, 1, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryPasswd = gtk_entry_new ();
  gtk_entry_set_visibility ( GTK_ENTRY ( entryPasswd ), FALSE );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelPasswd ), entryPasswd, accelGroup, _( "_Password: " )
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), entryPasswd, 1, 2, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxDecrypt ), tableTop, TRUE, FALSE, 0
  );
  hButtonBoxDecrypt = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxDecrypt ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxDecrypt ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxDecrypt ), 5 );
  buttonCancel = gpa_buttonCancel_new (
    windowDecrypt, accelGroup, _( "_Cancel" )
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDecrypt ), buttonCancel );
  buttonDecrypt = gpa_button_new ( accelGroup, _( "_Decrypt" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonDecrypt ), "clicked",
    GTK_SIGNAL_FUNC ( file_decryptAs_decrypt ),
    (gpointer) windowDecrypt
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDecrypt ), buttonDecrypt );
  gtk_box_pack_start (
    GTK_BOX ( vboxDecrypt ), hButtonBoxDecrypt, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowDecrypt ), vboxDecrypt );
  gtk_widget_show_all ( windowDecrypt );
  gpa_widget_set_centered ( windowDecrypt, windowMain );
} /* file_decryptAs */

void file_verify ( void ) {
g_print ( _( "Verify signatures of a file\n" ) ); /*!!!*/
} /* file_verify */

void file_close ( void ) {
g_print ( _( "Close selected files\n" ) ); /*!!!*/
} /* file_close */

void file_quit ( void ) {
/*!!! Noch umaendern: Falls Aenderungen stattgefunden haben, ein */
/*!!! Frage-Fenster aufkommen lassen ("Aenderungen speichern?"). */
  gtk_main_quit ();
} /* file_quit */
