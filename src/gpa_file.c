#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gpa_gtktools.h"

#include <stdio.h> /*!!!*/

static GtkWidget *fileOpenSelect;
/*!!!*/ static char *text [] = { "Dummy Text" }; /*!!!*/
/*!!!*/ static char *text2 [] = { "Dummy Text", "Dummy Text" }; /*!!!*/

void file_open_ok ( void ) {
char message [ 100 ];
sprintf ( message, "Open file '%s'\n", gtk_file_selection_get_filename ( GTK_FILE_SELECTION ( fileOpenSelect ) ) ); /*!!!*/
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
  gtk_widget_show ( fileOpenSelect );
} /* file_open */

void file_showDetail ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  guint accelKey;
  gchar *titlesSignatures [ 2 ] = { "Signature", "Validity" };
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
  gtk_window_set_title ( GTK_WINDOW ( windowDetail ), "Detailed file view" );
  gtk_widget_set_usize ( windowDetail, 280, 400 );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowDetail ), accelGroup );
  vboxDetail = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxDetail ), 5 );
  hboxFilename = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxFilename ), 5 );
  labelFilename = gtk_label_new ( "Filename: " );
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
  checkerEncrypted = gtk_check_button_new_with_label ( " encrypted" );
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
  labelSignatures = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( labelSignatures ), "_Signatures"
  );
  labelJfdSignatures = gpa_widget_hjustified_new (
    labelSignatures, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxSignatures ), labelJfdSignatures, FALSE, FALSE, 0
  );
  scrollerSignatures = gtk_scrolled_window_new ( NULL, NULL );
  clistSignatures = gtk_clist_new_with_titles ( 2, titlesSignatures );
  gtk_widget_add_accelerator (
    clistSignatures, "grab_focus", accelGroup, accelKey, GDK_MOD1_MASK, 0
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
  buttonClose = gtk_button_new_with_label ( "Close" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonClose ) -> child ), "_Close"
  );
  gtk_widget_add_accelerator (
    buttonClose, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_widget_add_accelerator (
    buttonClose, "clicked", accelGroup, GDK_Escape, 0, 0
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonClose ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) windowDetail
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDetail ), buttonClose );
  gtk_box_pack_start (
    GTK_BOX ( vboxDetail ), hButtonBoxDetail, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowDetail ), vboxDetail );
  gtk_widget_show_all ( windowDetail );
} /* file_showDetail */

void file_sign_sign ( GtkWidget *windowSign ) {
g_print ( "Sign a file\n" ); /*!!!*/
  gtk_widget_destroy ( windowSign );
} /* file_sign_sign */

void file_sign ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  guint accelKey;
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
  gtk_window_set_title ( GTK_WINDOW ( windowSign ), "Sign files" );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowSign ), accelGroup );
  vboxSign = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxSign ), 5 );
  frameMode = gtk_frame_new ( "Signing mode" );
  gtk_container_set_border_width ( GTK_CONTAINER ( frameMode ), 5 );
  vboxMode = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxMode ), 5 );
  radioSignComp = gtk_radio_button_new_with_label ( NULL, "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( radioSignComp ) -> child ), "si_gn and compress"
  );
  gtk_widget_add_accelerator (
    radioSignComp, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxMode ), radioSignComp, FALSE, FALSE, 0 );
  radioSign = gtk_radio_button_new_with_label_from_widget ( 
    GTK_RADIO_BUTTON ( radioSignComp ), ""
  );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( radioSign ) -> child ), "sign, do_n't compress"
  );
  gtk_widget_add_accelerator (
    radioSign, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxMode ), radioSign, FALSE, FALSE, 0 );
  radioSignSep = gtk_radio_button_new_with_label_from_widget (
    GTK_RADIO_BUTTON ( radioSignComp ), ""
  );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( radioSignSep ) -> child ), "sign in separate _file"
  );
  gtk_widget_add_accelerator (
    radioSignSep, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxMode ), radioSignSep, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( frameMode ), vboxMode );
  gtk_box_pack_start ( GTK_BOX ( vboxSign ), frameMode, FALSE, FALSE, 0 );
  checkerArmor = gtk_check_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( checkerArmor ) -> child ), "A_rmor"
  );
  gtk_widget_add_accelerator (
    checkerArmor, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_container_set_border_width ( GTK_CONTAINER ( checkerArmor ), 5 );
  gtk_box_pack_start ( GTK_BOX ( vboxSign ), checkerArmor, FALSE, FALSE, 0 );
  tableWho = gtk_table_new ( 2, 2, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tableWho ), 5 );
  labelWho = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline ( GTK_LABEL ( labelWho ), "Sign _as " );
  labelJfdWho = gpa_widget_hjustified_new ( labelWho, GTK_JUSTIFY_RIGHT );
  gtk_table_attach (
    GTK_TABLE ( tableWho ), labelJfdWho, 0, 1, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  comboWho = gtk_combo_new ();
  gtk_widget_add_accelerator (
    GTK_COMBO ( comboWho ) -> entry, "grab_focus", accelGroup, accelKey,
    GDK_MOD1_MASK, 0
  );
  gtk_table_attach (
    GTK_TABLE ( tableWho ), comboWho, 1, 2, 0, 1,
    GTK_SHRINK, GTK_SHRINK, 0, 0
  );
  labelPasswd = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline ( GTK_LABEL ( labelPasswd ), "_Password: " );
  labelJfdPasswd = gpa_widget_hjustified_new (
    labelPasswd, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableWho ), labelJfdPasswd, 0, 1, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryPasswd = gtk_entry_new ();
  gtk_widget_add_accelerator (
    entryPasswd, "grab_focus", accelGroup, accelKey, GDK_MOD1_MASK, 0
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
  buttonCancel = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonCancel ) -> child ), "_Cancel"
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonCancel ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) windowSign
  );
  gtk_widget_add_accelerator (
    buttonCancel, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_widget_add_accelerator (
    buttonCancel, "clicked", accelGroup, GDK_Escape, 0, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSign ), buttonCancel );
  buttonSign = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonSign ) -> child ), "_Sign"
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonSign ), "clicked",
    GTK_SIGNAL_FUNC ( file_sign_sign ), (gpointer) windowSign
  );
  gtk_widget_add_accelerator (
    buttonSign, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSign ), buttonSign );
  gtk_box_pack_start ( GTK_BOX ( vboxSign ), hButtonBoxSign, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( windowSign ), vboxSign );
  gtk_widget_show_all ( windowSign );
} /* file_sign */

void file_encrypt_detail_check ( void ) {
g_print ( "Check signature validities\n" ); /*!!!*/
} /* file_encrypt_detail_check */

void file_encrypt_detail ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  guint accelKey;
  char *titlesSignatures [] = { "Signature", "valid" };
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
  gtk_window_set_title ( GTK_WINDOW ( windowSigs ), "Show public key detail" );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowSigs ), accelGroup );
  vboxSigs = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxSigs ), 5 );
  vboxSignatures = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxSignatures ), 5 );
  labelSignatures = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( labelSignatures ), "_Signatures"
  );
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
  gtk_widget_add_accelerator (
    clistSignatures, "grab-focus", accelGroup, accelKey, GDK_MOD1_MASK, 0
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
  labelFingerprint = gtk_label_new ( "Fingerprint: " );
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
  buttonCheck = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonCheck ) -> child ), "Chec_k"
  );
  gtk_signal_connect (
    GTK_OBJECT ( buttonCheck ), "clicked",
    GTK_SIGNAL_FUNC ( file_encrypt_detail_check ), NULL
  );
  gtk_widget_add_accelerator (
    buttonCheck, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSigs ), buttonCheck );
  buttonClose = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonClose ) -> child ), "_Close"
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonClose ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) windowSigs
  );
  gtk_widget_add_accelerator (
    buttonClose, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_widget_add_accelerator (
    buttonClose, "clicked", accelGroup, GDK_Escape, 0, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSigs ), buttonClose );
  gtk_box_pack_start ( GTK_BOX ( vboxSigs ), hButtonBoxSigs, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( windowSigs ), vboxSigs );
  gtk_widget_show_all ( windowSigs );
} /* file_encrypt_detail */

void file_encrypt_encrypt ( GtkWidget *windowEncrypt ) {
g_print ( "Encrypt a file\n" ); /*!!!*/
  gtk_widget_destroy ( windowEncrypt );
} /* file_encrypt_encrypt */

void file_encrypt ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  guint accelKey;
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
  gtk_window_set_title ( GTK_WINDOW ( windowEncrypt ), "Encrypt files" );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowEncrypt ), accelGroup );
  vboxEncrypt = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxEncrypt ), 5 );
  vboxRecKeys = gtk_vbox_new ( FALSE, 0 );
  tableRecKeys = gtk_table_new ( 2, 2, FALSE );
  vboxDefault = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxDefault ), 5 );
  checkerDefault = gtk_check_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( checkerDefault ) -> child ),
    "Add _default recipients:"
  );
  gtk_widget_add_accelerator (
    checkerDefault, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
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
  labelRecipients = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( labelRecipients ), "Rec_ipients"
  );
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
  gtk_widget_add_accelerator (
    clistRecipients, "grab_focus", accelGroup, accelKey, GDK_MOD1_MASK, 0
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
  labelKeys = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( labelKeys ), "_Public keys"
  );
  labelJfdKeys = gpa_widget_hjustified_new ( labelKeys, GTK_JUSTIFY_LEFT );
  gtk_box_pack_start (
    GTK_BOX ( vboxKeys ), labelJfdKeys, FALSE, FALSE, 0
  );
  scrollerKeys = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrollerKeys, 180, 200 );
  clistKeys = gtk_clist_new ( 1 );
  gtk_widget_add_accelerator (
    clistKeys, "grab_focus", accelGroup, accelKey, GDK_MOD1_MASK, 0
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
  buttonAdd = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonAdd ) -> child ), "Add _key to recipients"
  );
  gtk_widget_add_accelerator (
    buttonAdd, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecKeys ), buttonAdd );
  buttonDetail = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonDetail ) -> child ), "S_how detail"
  );
  gtk_signal_connect (
    GTK_OBJECT ( buttonDetail ), "clicked",
    GTK_SIGNAL_FUNC ( file_encrypt_detail ), NULL
  );
  gtk_widget_add_accelerator (
    buttonDetail, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecKeys ), buttonDetail );
  gtk_box_pack_start (
    GTK_BOX ( vboxRecKeys ), hButtonBoxRecKeys, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxEncrypt ), vboxRecKeys, TRUE, TRUE, 0 );
  vboxMisc = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxMisc ), 5 );
  hboxSaveAs = gtk_hbox_new ( FALSE, 0 );
  labelSaveAs = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( labelSaveAs ), "Save encrypted file _as: "
  );
  gtk_box_pack_start ( GTK_BOX ( hboxSaveAs ), labelSaveAs, FALSE, FALSE, 0 );
  entrySaveAs = gtk_entry_new ();
  gtk_widget_add_accelerator (
    entrySaveAs, "grab_focus", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_box_pack_start ( GTK_BOX ( hboxSaveAs ), entrySaveAs, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxMisc ), hboxSaveAs, FALSE, FALSE, 0 );
  checkerSign = gtk_check_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( checkerSign ) -> child ), "_sign"
  );
  gtk_widget_add_accelerator (
    checkerSign, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxMisc ), checkerSign, FALSE, FALSE, 0 );
  checkerArmor = gtk_check_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( checkerArmor ) -> child ), "a_rmor"
  );
  gtk_widget_add_accelerator (
    checkerArmor, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxMisc ), checkerArmor, FALSE, FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxEncrypt ), vboxMisc, FALSE, FALSE, 0 );
  hButtonBoxEncrypt = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxEncrypt ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxEncrypt ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxEncrypt ), 5 );
  buttonCancel = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonCancel ) -> child ), "_Cancel"
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonCancel ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) windowEncrypt
  );
  gtk_widget_add_accelerator (
    buttonCancel, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_widget_add_accelerator (
    buttonCancel, "clicked", accelGroup, GDK_Escape, 0, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEncrypt ), buttonCancel );
  buttonEncrypt = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonEncrypt ) -> child ), "_Encrypt"
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonEncrypt ), "clicked",
    GTK_SIGNAL_FUNC ( file_encrypt_encrypt ), (gpointer) windowEncrypt
  );
  gtk_widget_add_accelerator (
    buttonEncrypt, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEncrypt ), buttonEncrypt );
  gtk_box_pack_start (
    GTK_BOX ( vboxEncrypt ), hButtonBoxEncrypt, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowEncrypt ), vboxEncrypt );
  gtk_widget_show_all ( windowEncrypt );
} /* file_encrypt */

void file_protect_protect ( GtkWidget *windowProtect ) {
g_print ( "Protect a file by Password\n" ); /*!!!*/
  gtk_widget_destroy ( windowProtect );
} /* file_protect_protect */

void file_protect ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  guint accelKey;
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
    GTK_WINDOW ( windowProtect ), "Protect files by Password"
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowProtect ), accelGroup );
  vboxProtect = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxProtect ), 5 );
  tablePasswd = gtk_table_new ( 2, 2, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tablePasswd ), 5 );
  labelPasswd = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( labelPasswd ), "P_assword: "
  );
  labelJfdPasswd = gpa_widget_hjustified_new ( labelPasswd, GTK_JUSTIFY_RIGHT );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), labelJfdPasswd, 0, 1, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  entryPasswd = gtk_entry_new ();
  gtk_entry_set_visibility ( GTK_ENTRY ( entryPasswd ), FALSE );
  gtk_widget_add_accelerator (
    entryPasswd, "grab_focus", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), entryPasswd, 1, 2, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  labelRepeat = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( labelRepeat ), "Repeat Pa_ssword: "
  );
  labelJfdRepeat = gpa_widget_hjustified_new (
    labelRepeat, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), labelJfdRepeat, 0, 1, 1, 2,
    GTK_EXPAND, GTK_SHRINK, 0, 0
  );
  entryRepeat = gtk_entry_new ();
  gtk_widget_add_accelerator (
    entryRepeat, "grab_focus", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_entry_set_visibility ( GTK_ENTRY ( entryRepeat ), FALSE );
  gtk_table_attach (
    GTK_TABLE ( tablePasswd ), entryRepeat, 1, 2, 1, 2,
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxProtect ), tablePasswd, TRUE, TRUE, 0
  );
  checkerArmor = gtk_check_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( checkerArmor ) -> child ), "A_rmor"
  );
  gtk_widget_add_accelerator (
    checkerArmor, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
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
  buttonCancel = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonCancel ) -> child ), "_Cancel"
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonCancel ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) windowProtect
  );
  gtk_widget_add_accelerator (
    buttonCancel, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_widget_add_accelerator (
    buttonCancel, "clicked", accelGroup, GDK_Escape, 0, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxProtect ), buttonCancel );
  buttonProtect = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonProtect ) -> child ), "_Protect"
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonProtect ), "clicked",
    GTK_SIGNAL_FUNC ( file_protect_protect ), (gpointer) windowProtect
  );
  gtk_widget_add_accelerator (
    buttonProtect, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxProtect ), buttonProtect );
  gtk_box_pack_start (
    GTK_BOX ( vboxProtect ), hButtonBoxProtect, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowProtect ), vboxProtect );
  gtk_widget_show_all ( windowProtect );
} /* file_protect */

void file_decrypt_decrypt ( GtkWidget *windowDecrypt ) {
g_print ( "Decrypt a file\n" ); /*!!!*/
  gtk_widget_destroy ( windowDecrypt );
} /* file_decrypt_decrypt */

void file_decrypt ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  guint accelKey;
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
  gtk_window_set_title ( GTK_WINDOW ( windowDecrypt ), "Decrypt files" );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowDecrypt ), accelGroup );
  vboxDecrypt = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxDecrypt ), 5 );
  hboxPasswd = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxPasswd ), 5 );
  labelPasswd = gtk_label_new ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( labelPasswd ), "_Password: "
  );
  gtk_box_pack_start (
    GTK_BOX ( hboxPasswd ), labelPasswd, FALSE, FALSE, 0
  );
  entryPasswd = gtk_entry_new ();
  gtk_widget_add_accelerator (
    entryPasswd, "grab_focus", accelGroup, accelKey, GDK_MOD1_MASK, 0
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
  buttonCancel = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonCancel ) -> child ), "_Cancel"
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonCancel ), "clicked",
    GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) windowDecrypt
  );
  gtk_widget_add_accelerator (
    buttonCancel, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_widget_add_accelerator (
    buttonCancel, "clicked", accelGroup, GDK_Escape, 0, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDecrypt ), buttonCancel );
  buttonDecrypt = gtk_button_new_with_label ( "" );
  accelKey = gtk_label_parse_uline (
    GTK_LABEL ( GTK_BIN ( buttonDecrypt ) -> child ), "_Decrypt"
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonDecrypt ), "clicked",
    GTK_SIGNAL_FUNC ( file_decrypt_decrypt ), (gpointer) windowDecrypt
  );
  gtk_widget_add_accelerator (
    buttonDecrypt, "clicked", accelGroup, accelKey, GDK_MOD1_MASK, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDecrypt ), buttonDecrypt );
  gtk_box_pack_start (
    GTK_BOX ( vboxDecrypt ), hButtonBoxDecrypt, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowDecrypt ), vboxDecrypt );
  gtk_widget_show_all ( windowDecrypt );
} /* file_protect */

void file_verify ( void ) {
g_print ( "Verify signatures of a file\n" ); /*!!!*/
} /* file_protect */

void file_close ( void ) {
g_print ( "Close selected files\n" ); /*!!!*/
} /* file_close */

void file_quit ( void ) {
/*!!! Noch umaendern: Falls Aenderungen stattgefunden haben, ein */
/*!!! Frage-Fenster aufkommen lassen ("Aenderungen speichern?"). */
  gtk_main_quit ();
} /* file_quit */
