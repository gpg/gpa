/* filemenu.c  -  The GNU Privacy Assistant
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
#include <stdlib.h>
#include <gpapa.h>
#include "gpa.h"
#include "gtktools.h"
#include "filemenu.h"
#include "keysmenu.h"

#include <stdio.h> /*!!!*/

static GtkWidget *fileOpenSelect;
/*!!!*/ static char *text [] = { N_( "Dummy Text" ) }; /*!!!*/

GList *filesOpened = NULL;
GList *filesSelected = NULL;

gchar *writtenFileStatus [ 3 ] = {
  N_( "clear" ),
  N_( "encrypted" ),
  N_( "protected" )
};

gint get_file_selection_count ( void ) {
  return g_list_length ( filesSelected );
} /* get_file_selection_count */

gchar *getStringForFileStatus ( GpapaFileStatus fileStatus ) {
  return ( writtenFileStatus [ fileStatus ] );
} /* getStringForFileStatus */

void file_open_countSigs ( gpointer data, gpointer userData ) {
/* var */
  GpapaSignature *signature;
  gint *countSigs;
/* commands */
  signature = (GpapaSignature*) data;
  countSigs = (gint*) userData;
  switch (
    gpapa_signature_get_validity (
      signature, gpa_callback, global_windowMain
    )
  ) {
    case GPAPA_SIG_VALID: countSigs [ 0 ] ++; break;
    case GPAPA_SIG_INVALID: countSigs [ 1 ] ++; break;
    default: break;
  } /* switch */
} /* file_open_countSigs */

void file_open_ok ( void ) {
/* var */
  gchar *anIdentifier;
  gchar *aName;
  gchar *fileEntry [ 5 ];
  GList *indexFile;
  GList *signatures;
  static gint countSigs [ 2 ];
  gchar buffer [ 256 ];
/* objects */
  GpapaFile *aFile;
  GtkWidget *fileList;
/* commands */
  anIdentifier = gtk_file_selection_get_filename (
    GTK_FILE_SELECTION ( fileOpenSelect )
  );
  aName = anIdentifier;
  aFile = gpapa_file_new ( anIdentifier, gpa_callback, global_windowMain );
  signatures = gpapa_file_get_signatures (
    aFile, gpa_callback, global_windowMain
  );
  fileEntry [ 0 ] = gpapa_file_get_name (
    aFile, gpa_callback, global_windowMain
  );
  indexFile = g_list_first ( filesOpened );
  while ( indexFile )
    {
      if (
        ! strcmp (
          fileEntry [ 0 ],
          gpapa_file_get_identifier (
            (GpapaFile*) indexFile -> data, gpa_callback, global_windowMain
          )
        )
      )
        {
          gtk_widget_hide ( fileOpenSelect );
          gpa_window_error (
            _( "The file is already open." ), global_windowMain
          );
          return;
        } /* if */
      indexFile = g_list_next ( indexFile );
    } /* while */
  filesOpened = g_list_append ( filesOpened, aFile );
  fileEntry [ 1 ] = getStringForFileStatus (
    gpapa_file_get_status ( aFile, gpa_callback, global_windowMain )
  );
  sprintf ( buffer, "%d", g_list_length ( signatures ) );
  fileEntry [ 2 ] = xstrdup ( buffer );
  countSigs [ 0 ] = 0;
  countSigs [ 1 ] = 0;
  g_list_foreach ( signatures, file_open_countSigs, countSigs );
  sprintf ( buffer, "%d", countSigs [ 0 ] );
  fileEntry [ 3 ] = xstrdup ( buffer );
  if ( countSigs [ 1 ] )
    {
      sprintf ( buffer, "%d", countSigs [ 1 ] );
      fileEntry [ 4 ] = xstrdup ( buffer );
    } /* if */
  else
    fileEntry [ 4 ] = _( "" );
  fileList = gpa_get_global_clist_file ();
  gtk_clist_append ( GTK_CLIST ( fileList ), fileEntry );
  gtk_widget_grab_focus ( fileList );
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
  gtk_signal_connect (
    GTK_OBJECT ( fileOpenSelect ), "delete_event",
    GTK_SIGNAL_FUNC ( gtk_widget_hide ), NULL
  );
  gtk_signal_connect_object (
    GTK_OBJECT( fileOpenSelect ), "hide",
    GTK_SIGNAL_FUNC ( gtk_widget_hide ), (gpointer) global_windowTip
  );
} /* gpa_fileOpenSelect_init */

void file_open ( void ) {
  gtk_window_set_position (
    GTK_WINDOW ( fileOpenSelect ), GTK_WIN_POS_CENTER
  );
  gtk_widget_show ( fileOpenSelect );
  gpa_windowTip_show ( _( "file_open.tip" ) ); /*!!!*/
} /* file_open */

void file_checkSignatures ( void ) {
g_print ( _( "Check signature validities\n" ) ); /*!!!*/
} /* file_checkSignatures */

void file_showDetail ( void ) {
/* var */
  GpapaFile *file;
  GtkAccelGroup *accelGroup;
  gchar *titlesSignatures [ 3 ] = {
    N_( "Signature" ), N_( "Validity" ), N_( "Key ID" )
  };
  GList *signatures;
  gpointer paramAppend [ 2 ];
  gchar *contentsFilename;
  static gpointer paramClose [ 2 ];
/* objects */
  GtkWidget *windowDetail;
    GtkWidget *vboxDetail;
      GtkWidget *tableTop;
        GtkWidget *labelJfdFilename;
	  GtkWidget *labelFilename;
	GtkWidget *entryFilename;
	GtkWidget *labelJfdEncrypted;
          GtkWidget *labelEncrypted;
        GtkWidget *entryJfdEncrypted;
          GtkWidget *entryEncrypted;
      GtkWidget *vboxSignatures;
	GtkWidget *labelJfdSignatures;
	  GtkWidget *labelSignatures;
	GtkWidget *scrollerSignatures;
	  GtkWidget *clistSignatures;
      GtkWidget *hButtonBoxDetail;
        GtkWidget *buttonCheck;
	GtkWidget *buttonClose;
/* commands */
  if ( ! filesSelected )
    {
      gpa_window_error (
	_( "No file selected for detail view" ), global_windowMain
      );
      return;
    } /* if */
  file = (GpapaFile*) g_list_last ( filesSelected ) -> data;
  windowDetail = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowDetail ), _( "Detailed file view" )
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowDetail ), accelGroup );
  vboxDetail = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxDetail ), 5 );
  tableTop = gtk_table_new ( 2, 2, FALSE );
  gtk_container_set_border_width ( GTK_CONTAINER ( tableTop ), 5 );
  labelFilename = gtk_label_new ( _( "Filename: " ) );
  labelJfdFilename = gpa_widget_hjustified_new (
    labelFilename, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), labelJfdFilename, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 0, 0
  );
  entryFilename = gtk_entry_new ();
  gtk_editable_set_editable ( GTK_EDITABLE ( entryFilename ), FALSE );
  contentsFilename = gpapa_file_get_name (
    file, gpa_callback, global_windowMain
  );
  gtk_entry_set_text ( GTK_ENTRY ( entryFilename ), contentsFilename );
  gtk_widget_set_usize (
    entryFilename,
    gdk_string_width ( entryFilename -> style -> font, contentsFilename ) +
    gdk_string_width ( entryFilename -> style -> font, "  " ) +
    entryFilename -> style -> klass -> xthickness,
    0
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), entryFilename, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 0, 0
  );
  labelEncrypted = gtk_label_new ( _( "Status: " ) );
  labelJfdEncrypted = gpa_widget_hjustified_new (
    labelEncrypted, GTK_JUSTIFY_RIGHT
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), labelJfdEncrypted, 0, 1, 1, 2,
    GTK_FILL, GTK_FILL, 0, 0
  );
  entryEncrypted = gtk_entry_new ();
  gtk_entry_set_text (
    GTK_ENTRY ( entryEncrypted ), getStringForFileStatus (
      gpapa_file_get_status ( file, gpa_callback, global_windowMain )
    )
  );
  gtk_editable_set_editable ( GTK_EDITABLE ( entryEncrypted ), FALSE );
  entryJfdEncrypted = gpa_widget_hjustified_new (
    entryEncrypted, GTK_JUSTIFY_LEFT
  );
  gtk_table_attach (
    GTK_TABLE ( tableTop ), entryJfdEncrypted, 1, 2, 1, 2,
    GTK_FILL, GTK_FILL, 0, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxDetail ), tableTop, FALSE, FALSE, 0 );
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
  gtk_widget_set_usize ( scrollerSignatures, 350, 200 );
  clistSignatures = gtk_clist_new_with_titles ( 3, titlesSignatures );
  paramAppend [ 0 ] = clistSignatures;
  paramAppend [ 1 ] = global_windowMain;
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelSignatures ), clistSignatures,
    accelGroup, _( "_Signatures" )
  );
  signatures = gpapa_file_get_signatures (
    file, gpa_callback, global_windowMain
  );
  g_list_foreach ( signatures, sigs_append, paramAppend );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 0, 180 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistSignatures ), 0, GTK_JUSTIFY_LEFT
  );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 1, 80 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistSignatures ), 1, GTK_JUSTIFY_CENTER
  );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 2, 120 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistSignatures ), 2, GTK_JUSTIFY_LEFT
  );
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
  buttonCheck = gpa_button_new ( accelGroup, _( "Chec_k" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonCheck ), "clicked",
    GTK_SIGNAL_FUNC ( file_checkSignatures ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDetail ), buttonCheck );
  paramClose [ 0 ] = windowDetail;
  paramClose [ 1 ] = NULL;
  buttonClose = gpa_buttonCancel_new (
    accelGroup, _( "_Close" ), paramClose
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDetail ), buttonClose );
  gtk_box_pack_start (
    GTK_BOX ( vboxDetail ), hButtonBoxDetail, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowDetail ), vboxDetail );
  gpa_widget_show (
    windowDetail, global_windowMain, _( "file_showDetail.tip" )
  );
} /* file_showDetail */

void file_sign_sign_exec ( gpointer param ) {
/* var */
  gpointer* localParam;
  gpointer* data;
  GpapaSignType signType;
  GpapaArmor armor;
  gchar *keyID;
  GtkWidget *windowSign;
  GtkWidget *entryPasswd;
  GtkWidget *windowPassphrase;
  GList *indexFile;
  GpapaFile *file;
  gpointer paramDone [ 2 ];
/* commands */
  localParam = (gpointer*) param;
  data = (gpointer*) localParam [ 0 ];
  entryPasswd =      localParam [ 1 ];
  windowPassphrase = localParam [ 2 ];
  signType = *(GpapaSignType*) data [ 0 ];
  armor = *(GpapaArmor*) data [ 1 ];
  keyID = (gchar*) data [ 2 ];
  windowSign = (GtkWidget*) data [ 3 ];
  if ( ! filesSelected )
    {
      gpa_window_error ( _( "No files selected for signing" ), windowSign );
      return;
    } /* if */
  indexFile = g_list_first ( filesSelected );
  while ( indexFile != NULL )
    {
      file = (GpapaFile*) indexFile -> data;
      gpapa_file_sign (
	file, NULL, keyID, gtk_entry_get_text (
	  GTK_ENTRY ( entryPasswd )
	), signType, armor, gpa_callback, windowSign
      );
      indexFile = g_list_next ( indexFile );
    } /* while */
  paramDone [ 0 ] = windowPassphrase;
  paramDone [ 1 ] = NULL;
  gpa_window_destroy ( paramDone );
  paramDone [ 0 ] = windowSign;
  gpa_window_destroy ( paramDone );
} /* file_sign_sign_exec */

void file_sign_sign ( gpointer param ) {
/* var */
  gpointer *localParam;
  GtkWidget *radioSignComp;
  GtkWidget *radioSign;
  GtkWidget *radioSignSep;
  GtkWidget *checkerArmor;
  GtkWidget *clistWho;
  int as;
  GtkWidget *windowSign;
  static GpapaSignType signType;
  static GpapaArmor armor;
  gint errorCode;
  static gchar *keyID;
  static gpointer *newParam;
/* commands */
  localParam = (gpointer*) param;
  radioSignComp = localParam [ 0 ];
  radioSign =	  localParam [ 1 ];
  radioSignSep =  localParam [ 2 ];
  checkerArmor =  localParam [ 3 ];
  clistWho =	  localParam [ 4 ];
  as =	 *(gint*) localParam [ 5 ];
  windowSign =	  localParam [ 6 ];
  if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( radioSignComp ) ) )
    signType = GPAPA_SIGN_NORMAL;
  else if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( radioSign ) ) )
    signType = GPAPA_SIGN_CLEAR;
  else if (
    gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( radioSignSep ) )
  )
    signType = GPAPA_SIGN_DETACH;
  else
    {
      gpa_window_error (
	_( "!FATAL ERROR!\nInvalid sign mode" ), windowSign
      );
      return;
    } /* else */
  if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( checkerArmor ) ) )
    armor = GPAPA_ARMOR;
  else
    armor = GPAPA_NO_ARMOR;
  errorCode = gtk_clist_get_text ( GTK_CLIST ( clistWho ), as, 1, &keyID );
  newParam = (gpointer*) xmalloc ( 4 * sizeof ( gpointer ) );
  newParam [ 0 ] = &signType;
  newParam [ 1 ] = &armor;
  newParam [ 2 ] = keyID;
  newParam [ 3 ] = windowSign;
  gpa_window_passphrase (
    windowSign, file_sign_sign_exec, _( "file_sign.tip" ), newParam
  );
} /* file_sign_sign */

void file_sign_as (
  GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer as
) {
  *(gint*) as = row;
} /* file_sign_as */

void file_sign ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  gchar *titlesWho [ 2 ] = { "User identity / role", "Key ID" };
  gint contentsCountWho;
  GpapaSecretKey *key;
  gchar *contentsWho [ 2 ];
  static gint as = 0;
  static gpointer param [ 7 ];
  static gpointer paramClose [ 2 ];
/* objects */
  GtkWidget *windowSign;
    GtkWidget *vboxSign;
      GtkWidget *frameMode;
	GtkWidget *vboxMode;
	  GtkWidget *radioSignComp;
	  GtkWidget *radioSign;
	  GtkWidget *radioSignSep;
      GtkWidget *checkerArmor;
      GtkWidget *vboxWho;
	GtkWidget *labelJfdWho;
	  GtkWidget *labelWho;
	GtkWidget *scrollerWho;
	  GtkWidget *clistWho;
      GtkWidget *hButtonBoxSign;
	GtkWidget *buttonCancel;
	GtkWidget *buttonSign;
/* commands */
  if ( get_file_selection_count () == 0 )
    {
      gpa_window_error (
	_( "No files selected for signing" ), global_windowMain
      );
      return;
    } /* if */
  contentsCountWho = gpapa_get_secret_key_count (
    gpa_callback, global_windowMain
  );
  if ( contentsCountWho == 0 )
    {
      gpa_window_error (
	_( "No secret keys available for signing.\nImport a secret key first!" ),
	global_windowMain );
      return;
    } /* if */
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
  vboxWho = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxWho ), 5 );
  labelWho = gtk_label_new ( _( "" ) );
  labelJfdWho = gpa_widget_hjustified_new ( labelWho, GTK_JUSTIFY_LEFT );
  gtk_box_pack_start ( GTK_BOX ( vboxWho ), labelJfdWho, FALSE, FALSE, 0 );
  scrollerWho = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrollerWho, 260, 75 );
  clistWho = gtk_clist_new_with_titles ( 2, titlesWho );
  gtk_clist_set_column_width ( GTK_CLIST ( clistWho ), 0, 185 );
  gtk_clist_set_column_width ( GTK_CLIST ( clistWho ), 1, 120 );
  while ( contentsCountWho )
    {
      contentsCountWho--;
      key = gpapa_get_secret_key_by_index (
	contentsCountWho, gpa_callback, windowSign
      );
      contentsWho [ 0 ] = gpapa_key_get_name (
	GPAPA_KEY ( key ), gpa_callback, windowSign
      );
      contentsWho [ 1 ] = gpapa_key_get_identifier (
	GPAPA_KEY ( key ), gpa_callback, windowSign
      );
      gtk_clist_append ( GTK_CLIST ( clistWho ), contentsWho );
    } /* while */
  gtk_clist_set_selection_mode (
    GTK_CLIST ( clistWho ), GTK_SELECTION_SINGLE
  );
  gtk_signal_connect (
    GTK_OBJECT ( clistWho ), "select-row",
    GTK_SIGNAL_FUNC ( file_sign_as ), (gpointer) &as
  );
  gtk_clist_select_row ( GTK_CLIST ( clistWho ), 0, 0 );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelWho ), clistWho, accelGroup, _( "Sign _as " )
  );
  gtk_container_add ( GTK_CONTAINER ( scrollerWho ), clistWho );
  gtk_box_pack_start ( GTK_BOX ( vboxWho ), scrollerWho, TRUE, TRUE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxSign ), vboxWho, TRUE, FALSE, 0 );
  hButtonBoxSign = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxSign ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxSign ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxSign ), 5 );
  paramClose [ 0 ] = windowSign;
  paramClose [ 1 ] = NULL;
  buttonCancel = gpa_buttonCancel_new (
    accelGroup, _( "_Cancel" ), paramClose
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSign ), buttonCancel );
  buttonSign = gpa_button_new ( accelGroup, "_Sign" );
  param [ 0 ] = radioSignComp;
  param [ 1 ] = radioSign;
  param [ 2 ] = radioSignSep;
  param [ 3 ] = checkerArmor;
  param [ 4 ] = clistWho;
  param [ 5 ] = &as;
  param [ 6 ] = windowSign;
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonSign ), "clicked",
    GTK_SIGNAL_FUNC ( file_sign_sign ), (gpointer) param
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSign ), buttonSign );
  gtk_box_pack_start ( GTK_BOX ( vboxSign ), hButtonBoxSign, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( windowSign ), vboxSign );
  gpa_widget_show ( windowSign, global_windowMain, _( "file_sign.tip" ) );
} /* file_sign */

void file_browse_ok ( GtkWidget *param [] ) {
  gtk_entry_set_text (
    GTK_ENTRY ( param [ 0 ] ),
    gtk_file_selection_get_filename ( GTK_FILE_SELECTION ( param [ 1 ] ) )
  );
  gtk_widget_destroy ( param [ 1 ] );
} /* file_browse_ok */

void file_browse ( GtkWidget *entryFilename ) {
/* var */
  static GtkWidget *param [ 2 ];
/* objects */
  GtkWidget *browseSelect;
/* commands */
  browseSelect = gtk_file_selection_new ( "Save decrypted file as" );
  param [ 0 ] = entryFilename;
  param [ 1 ] = browseSelect;
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( browseSelect ) -> ok_button ),
    "clicked", GTK_SIGNAL_FUNC ( file_browse_ok ),
    (gpointer) param
  );
  gtk_signal_connect_object (
    GTK_OBJECT ( GTK_FILE_SELECTION ( browseSelect ) -> cancel_button ),
    "clicked", GTK_SIGNAL_FUNC ( gtk_widget_destroy ), (gpointer) browseSelect
  );
  gtk_widget_show ( browseSelect );
} /* file_browse */

void file_encrypt_selectEncryptAs (
  GtkCList *clist, gint row, gint column, GdkEventButton *event,
  gpointer userData
) {
/* var */
  gpointer *localParam;
  GpapaSecretKey **key;
  GtkWidget *windowEncrypt;
  gchar *keyID;
/* commands */
  localParam = (gpointer*) userData;
  key =     (GpapaSecretKey**) localParam [ 0 ];
  windowEncrypt = (GtkWidget*) localParam [ 1 ];
  gtk_clist_get_text ( clist, row, 1, &keyID );
  *key = gpapa_get_secret_key_by_ID ( keyID, gpa_callback, windowEncrypt );
g_print ( "Encrypt: " ); /*!!!*/
g_print ( gpapa_key_get_name ( GPAPA_KEY ( *key ), gpa_callback, windowEncrypt ) ); /*!!!*/
g_print ( " (" ); /*!!!*/
g_print ( gpapa_key_get_identifier ( GPAPA_KEY ( *key ), gpa_callback, windowEncrypt ) ); /*!!!*/
g_print ( ")\n" ); /*!!!*/
} /* file_encrypt_selectEncryptAs */

void file_encrypt_detail ( gpointer param ) {
/* var */
  gpointer *localParam;
  GList **keysSelected;
  GtkWidget *windowEncrypt;
  gchar *tip;
  GpapaPublicKey *key;
  GList *signatures;
  GtkAccelGroup *accelGroup;
  gchar *titlesSignatures [] = {
    N_( "Signature" ), N_( "Validity" ), N_( "Key ID" )
  };
  static gpointer paramSigs [ 2 ];
  gchar *contentsFingerprint;
  static gpointer paramClose [ 2 ];
/* objects */
  GtkWidget *windowSigs;
    GtkWidget *vboxSigs;
      GtkWidget *tableKey;
      GtkWidget *vboxSignatures;
	GtkWidget *labelJfdSignatures;
	  GtkWidget *labelSignatures;
	GtkWidget *scrollerSignatures;
	  GtkWidget *clistSignatures;
      GtkWidget *vboxFingerprint;
        GtkWidget *labelJfdFingerprint;
	  GtkWidget *labelFingerprint;
	GtkWidget *entryFingerprint;
      GtkWidget *hButtonBoxSigs;
	GtkWidget *buttonCheck;
	GtkWidget *buttonClose;
/* commands */
  localParam = (gpointer*) param;
  keysSelected =     (GList**) localParam [ 0 ];
  windowEncrypt = (GtkWidget*) localParam [ 1 ];
  tip =               (gchar*) localParam [ 2 ];
  if ( ! *keysSelected )
    {
      gpa_window_error (
        _( "No key selected for detail view." ), windowEncrypt
      );
      return;
    } /* if */
  key = gpapa_get_public_key_by_ID (
    (gchar*) g_list_last ( *keysSelected ) -> data,
    gpa_callback, windowEncrypt
  );
  windowSigs = gtk_window_new ( GTK_WINDOW_DIALOG );
  gtk_window_set_title (
    GTK_WINDOW ( windowSigs ), _( "Show public key detail" )
  );
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group ( GTK_WINDOW ( windowSigs ), accelGroup );
  vboxSigs = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxSigs ), 5 );
  tableKey = gpa_tableKey_new ( GPAPA_KEY ( key ), windowEncrypt );
  gtk_container_set_border_width ( GTK_CONTAINER ( tableKey ), 5 );
  gtk_box_pack_start ( GTK_BOX ( vboxSigs ), tableKey, FALSE, FALSE, 0 );
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
  gtk_widget_set_usize ( scrollerSignatures, 350, 200 );
  clistSignatures = gtk_clist_new_with_titles ( 3, titlesSignatures );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 0, 180 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistSignatures ), 0, GTK_JUSTIFY_LEFT
  );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 1, 80 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistSignatures ), 1, GTK_JUSTIFY_CENTER
  );
  gtk_clist_set_column_width ( GTK_CLIST ( clistSignatures ), 2, 120 );
  gtk_clist_set_column_justification (
    GTK_CLIST ( clistSignatures ), 2, GTK_JUSTIFY_LEFT
  );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelSignatures ), clistSignatures,
    accelGroup, _( "_Signatures" )
  );
  signatures = gpapa_public_key_get_signatures (
    key, gpa_callback, windowEncrypt
  );
  paramSigs [ 0 ] = clistSignatures;
  paramSigs [ 1 ] = windowEncrypt;
  g_list_foreach ( signatures, sigs_append, (gpointer) paramSigs );
  gtk_container_add ( GTK_CONTAINER ( scrollerSignatures ), clistSignatures );
  gtk_box_pack_start (
    GTK_BOX ( vboxSignatures ), scrollerSignatures, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxSigs ), vboxSignatures, TRUE, TRUE, 0 );
  vboxFingerprint = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxFingerprint ), 5 );
  labelFingerprint = gtk_label_new ( _( "Fingerprint: " ) );
  labelJfdFingerprint = gpa_widget_hjustified_new (
    labelFingerprint, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxFingerprint ), labelJfdFingerprint, FALSE, FALSE, 0
  );
  entryFingerprint = gtk_entry_new ();
  contentsFingerprint = gpapa_public_key_get_fingerprint (
    key, gpa_callback, windowEncrypt
  );
  gtk_entry_set_text ( GTK_ENTRY ( entryFingerprint ), contentsFingerprint );
  gtk_editable_set_editable ( GTK_EDITABLE ( entryFingerprint ), FALSE );
  gtk_widget_set_usize (
    entryFingerprint,
    gdk_string_width (
      entryFingerprint -> style -> font, contentsFingerprint
    ) +
    gdk_string_width ( entryFingerprint -> style -> font, "  " ) +
    entryFingerprint -> style -> klass -> xthickness,
    0
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxFingerprint ), entryFingerprint, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxSigs ), vboxFingerprint, FALSE, FALSE, 0 );
  hButtonBoxSigs = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxSigs ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxSigs ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxSigs ), 5 );
  buttonCheck = gpa_button_new ( accelGroup, _( "Chec_k" ) );
  gtk_signal_connect (
    GTK_OBJECT ( buttonCheck ), "clicked",
    GTK_SIGNAL_FUNC ( file_checkSignatures ), NULL
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSigs ), buttonCheck );
  paramClose [ 0 ] = windowSigs;
  paramClose [ 1 ] = tip;
  buttonClose = gpa_buttonCancel_new (
    accelGroup, _( "_Close" ), paramClose
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxSigs ), buttonClose );
  gtk_box_pack_start ( GTK_BOX ( vboxSigs ), hButtonBoxSigs, FALSE, FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( windowSigs ), vboxSigs );
  gpa_widget_show (
    windowSigs, global_windowMain, _( "file_encrypt_detail.tip" )
  );
} /* file_encrypt_detail */

void file_encrypt_encrypt ( gpointer param ) {
g_print ( _( "Encrypt a file\n" ) ); /*!!!*/
  gpa_recipientWindow_close ( param );
} /* file_encrypt_encrypt */

void file_encrypt_dialog ( gboolean withSaveAs, gchar *tip ) {
/* var */
  gint publicKeyCount;
  gint secretKeyCount;
  gchar *titlesAnyClist [ 2 ] = { N_( "Key owner" ), N_( "Key ID" ) };
  GtkAccelGroup *accelGroup;
  static GList *recipientsSelected = NULL;
  static gpointer paramRemove [ 3 ];
  GpapaPublicKey *publicKey;
  GpapaSecretKey *secretKey;
  static GpapaSecretKey *keyEncryptAs;
  gchar *contentsKeys [ 2 ];
  static GList *keysSelected = NULL;
  static gint columnKeyID = 1;
  static gpointer paramEncryptAs [ 2 ];
  static gpointer paramKeys [ 3 ];
  static gpointer paramAdd [ 4 ];
  static gpointer paramDetail [ 3 ];
  static gpointer paramClose [ 2 ];
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
	  GtkWidget *vboxEncryptAs;
	    GtkWidget *labelJfdEncryptAs;
	      GtkWidget *labelEncryptAs;
	    GtkWidget *scrollerEncryptAs;
	      GtkWidget *clistEncryptAs;
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
	  GtkWidget *buttonRemove;
	  GtkWidget *buttonAdd;
	  GtkWidget *buttonDetail;
      GtkWidget *vboxMisc;
	GtkWidget *hboxSaveAs;
	  GtkWidget *labelSaveAs;
	  GtkWidget *entrySaveAs;
	  GtkWidget *spaceSaveAs;
	  GtkWidget *buttonBrowse;
	GtkWidget *checkerSign;
	GtkWidget *checkerArmor;
      GtkWidget *hButtonBoxEncrypt;
	GtkWidget *buttonCancel;
	GtkWidget *buttonEncrypt;
/* commands */
  publicKeyCount = gpapa_get_public_key_count (
    gpa_callback, global_windowMain
  );
  if ( ! publicKeyCount )
    {
      gpa_window_error (
	_( "No public keys available.\nCurrently, there is nobody who could read a\nfile encrypted by you." ),
	global_windowMain
      );
      return;
    } /* if */
  secretKeyCount = gpapa_get_secret_key_count (
    gpa_callback, global_windowMain
  );
  if ( ! secretKeyCount )
    {
      gpa_window_error (
        _( "No secret keys available to encrypt." ), global_windowMain
      );
      return;
    } /* if */
  if ( ! filesSelected )
    {
      gpa_window_error (
	_( "No files selected for encryption." ), global_windowMain
      );
      return;
    } /* if */
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
  gtk_widget_set_usize ( scrollerDefault, 300, 90 );
  clistDefault = gtk_clist_new_with_titles ( 2, titlesAnyClist );
  gtk_clist_set_column_width ( GTK_CLIST ( clistDefault ), 0, 230 );
  gtk_clist_set_column_width ( GTK_CLIST ( clistDefault ), 1, 120 );
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
    GTK_FILL, GTK_SHRINK, 0, 0
  );
  vboxEncryptAs = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxEncryptAs ), 5 );
  labelEncryptAs = gtk_label_new ( _( "" ) );
  labelJfdEncryptAs = gpa_widget_hjustified_new (
    labelEncryptAs, GTK_JUSTIFY_LEFT
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxEncryptAs ), labelJfdEncryptAs, FALSE, FALSE, 0
  );
  scrollerEncryptAs = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrollerEncryptAs, 300, 90 );
  clistEncryptAs = gtk_clist_new_with_titles ( 2, titlesAnyClist );
  gtk_clist_set_column_width ( GTK_CLIST ( clistEncryptAs ), 0, 230 );
  gtk_clist_set_column_width ( GTK_CLIST ( clistEncryptAs ), 1, 120 );
  gtk_clist_set_selection_mode (
    GTK_CLIST ( clistEncryptAs ), GTK_SELECTION_SINGLE
  );
  paramEncryptAs [ 0 ] = &keyEncryptAs;
  paramEncryptAs [ 1 ] = windowEncrypt;
  gtk_signal_connect (
    GTK_OBJECT ( clistEncryptAs ), "select-row",
    GTK_SIGNAL_FUNC ( file_encrypt_selectEncryptAs ),
    (gpointer) paramEncryptAs
  );
  while ( secretKeyCount )
    {
      secretKeyCount--;
      secretKey = gpapa_get_secret_key_by_index (
	secretKeyCount, gpa_callback, global_windowMain
      );
      contentsKeys [ 0 ] = gpapa_key_get_name (
	GPAPA_KEY ( secretKey ), gpa_callback, global_windowMain
      );
      contentsKeys [ 1 ] = gpapa_key_get_identifier (
	GPAPA_KEY ( secretKey ), gpa_callback, global_windowMain
      );
      gtk_clist_prepend ( GTK_CLIST ( clistEncryptAs ), contentsKeys );
    } /* while */
  gtk_container_add ( GTK_CONTAINER ( scrollerEncryptAs ), clistEncryptAs );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelEncryptAs ), clistEncryptAs,
    accelGroup, _( "Encr_ypt as" )
  );
  gtk_box_pack_start (
    GTK_BOX ( vboxEncryptAs ), scrollerEncryptAs, TRUE, TRUE, 0
  );
  gtk_table_attach (
    GTK_TABLE ( tableRecKeys ), vboxEncryptAs, 1, 2, 0, 1,
    GTK_FILL, GTK_SHRINK, 0, 0
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
  gtk_widget_set_usize ( scrollerRecipients, 300, 120 );
  clistRecipients = gtk_clist_new_with_titles ( 2, titlesAnyClist );
  gtk_clist_set_column_width ( GTK_CLIST ( clistRecipients ), 0, 230 );
  gtk_clist_set_column_width ( GTK_CLIST ( clistRecipients ), 1, 120 );
  gtk_clist_set_selection_mode (
    GTK_CLIST ( clistRecipients ), GTK_SELECTION_EXTENDED
  );
  recipientsSelected = NULL;
  gtk_signal_connect (
    GTK_OBJECT ( clistRecipients ), "select-row",
    GTK_SIGNAL_FUNC ( gpa_selectRecipient ), (gpointer) &recipientsSelected
  );
  gtk_signal_connect (
    GTK_OBJECT ( clistRecipients ), "unselect-row",
    GTK_SIGNAL_FUNC ( gpa_unselectRecipient ), (gpointer) &recipientsSelected
  );
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
  gtk_widget_set_usize ( scrollerKeys, 300, 120 );
  clistKeys = gtk_clist_new_with_titles ( 2, titlesAnyClist );
  gtk_clist_set_selection_mode (
    GTK_CLIST ( clistKeys ), GTK_SELECTION_EXTENDED
  );
  gtk_clist_set_column_width ( GTK_CLIST ( clistKeys ), 0, 230 );
  gtk_clist_set_column_width ( GTK_CLIST ( clistKeys ), 1, 120 );
  keysSelected = NULL;
  paramKeys [ 0 ] = &keysSelected;
  paramKeys [ 1 ] = &columnKeyID;
  paramKeys [ 2 ] = windowEncrypt;
  gtk_signal_connect (
    GTK_OBJECT ( clistKeys ), "select-row",
    GTK_SIGNAL_FUNC ( keys_selectKey ), (gpointer) paramKeys
  );
  gtk_signal_connect (
    GTK_OBJECT ( clistKeys ), "unselect-row",
    GTK_SIGNAL_FUNC ( keys_unselectKey ), (gpointer) paramKeys
  );
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelKeys ), clistKeys, accelGroup, _( "_Public keys" )
  );
  while ( publicKeyCount )
    {
      publicKeyCount--;
      publicKey = gpapa_get_public_key_by_index (
	publicKeyCount, gpa_callback, global_windowMain
      );
      contentsKeys [ 0 ] = gpapa_key_get_name (
	GPAPA_KEY ( publicKey ), gpa_callback, global_windowMain
      );
      contentsKeys [ 1 ] = gpapa_key_get_identifier (
	GPAPA_KEY ( publicKey ), gpa_callback, global_windowMain
      );
      gtk_clist_prepend ( GTK_CLIST ( clistKeys ), contentsKeys );
    } /* while */
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
  buttonRemove = gpa_button_new (
    accelGroup, _( "Remo_ve keys from recipients" )
  );
  paramRemove [ 0 ] = &recipientsSelected;
  paramRemove [ 1 ] = clistRecipients;
  paramRemove [ 2 ] = windowEncrypt;
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonRemove ), "clicked",
    gpa_removeRecipients, (gpointer) paramRemove
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecKeys ), buttonRemove );
  buttonAdd = gpa_button_new ( accelGroup, _( "Add _keys to recipients" ) );
  paramAdd [ 0 ] = &keysSelected;
  paramAdd [ 1 ] = clistKeys;
  paramAdd [ 2 ] = clistRecipients;
  paramAdd [ 3 ] = windowEncrypt;
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonAdd ), "clicked",
    gpa_addRecipients, (gpointer) paramAdd
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecKeys ), buttonAdd );
  buttonDetail = gpa_button_new ( accelGroup, _( "S_how detail" ) );
  paramDetail [ 0 ] = &keysSelected;
  paramDetail [ 1 ] = windowEncrypt;
  paramDetail [ 2 ] = tip;
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonDetail ), "clicked",
    GTK_SIGNAL_FUNC ( file_encrypt_detail ), (gpointer) paramDetail
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxRecKeys ), buttonDetail );
  gtk_box_pack_start (
    GTK_BOX ( vboxRecKeys ), hButtonBoxRecKeys, TRUE, TRUE, 0
  );
  gtk_box_pack_start ( GTK_BOX ( vboxEncrypt ), vboxRecKeys, TRUE, TRUE, 0 );
  vboxMisc = gtk_vbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( vboxMisc ), 5 );
  if ( withSaveAs )
    {
      hboxSaveAs = gtk_hbox_new ( FALSE, 0 );
      labelSaveAs = gtk_label_new ( _( "" ) );
      gtk_box_pack_start (
        GTK_BOX ( hboxSaveAs ), labelSaveAs, FALSE, FALSE, 0
      );
      entrySaveAs = gtk_entry_new ();
      gpa_connect_by_accelerator (
        GTK_LABEL ( labelSaveAs ), entrySaveAs,
        accelGroup, _( "Save encrypted file _as: " )
      );
      gtk_box_pack_start (
        GTK_BOX ( hboxSaveAs ), entrySaveAs, TRUE, TRUE, 0
      );
      spaceSaveAs = gpa_space_new ();
      gtk_box_pack_start (
        GTK_BOX ( hboxSaveAs ), spaceSaveAs, FALSE, FALSE, 5
      );
      buttonBrowse = gpa_button_new ( accelGroup, _( "   _Browse   " ) );
      gtk_signal_connect_object (
        GTK_OBJECT ( buttonBrowse ), "clicked",
        GTK_SIGNAL_FUNC ( file_browse ), (gpointer) entrySaveAs
      );
      gtk_box_pack_start (
        GTK_BOX ( hboxSaveAs ), buttonBrowse, FALSE, FALSE, 0
      );
      gtk_box_pack_start (
        GTK_BOX ( vboxMisc ), hboxSaveAs, FALSE, FALSE, 0
      );
    } /* if */
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
  buttonCancel = gpa_button_new ( accelGroup, _( "_Cancel" ) );
  paramClose [ 0 ] = &recipientsSelected;
  paramClose [ 1 ] = &keysSelected;
  paramClose [ 2 ] = windowEncrypt;
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonCancel ), "clicked",
    GTK_SIGNAL_FUNC ( gpa_recipientWindow_close ), (gpointer) paramClose
  );
  gtk_widget_add_accelerator (
    buttonCancel, "clicked", accelGroup, GDK_Escape, 0, 0
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEncrypt ), buttonCancel );
  buttonEncrypt = gpa_button_new ( accelGroup, _( "_Encrypt" ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonEncrypt ), "clicked",
    GTK_SIGNAL_FUNC ( file_encrypt_encrypt ), (gpointer) paramClose
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxEncrypt ), buttonEncrypt );
  gtk_box_pack_start (
    GTK_BOX ( vboxEncrypt ), hButtonBoxEncrypt, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowEncrypt ), vboxEncrypt );
  gpa_widget_show ( windowEncrypt, global_windowMain, tip );
  gtk_clist_select_row ( GTK_CLIST ( clistEncryptAs ), 0, 0 );
} /* file_encrypt_dialog */

void file_encrypt ( void ) {
  file_encrypt_dialog ( FALSE, _( "file_encrypt.tip" ) );
} /* file_encrypt */

void file_encryptAs ( void ) {
  file_encrypt_dialog ( TRUE, _( "file_encryptAs.tip" ) );
} /* file_encryptAs */

void file_protect_protect ( GtkWidget *windowProtect ) {
/* var */
  gpointer paramDone [ 2 ];
/* commands */
g_print ( _( "Protect a file by Password\n" ) ); /*!!!*/
  paramDone [ 0 ] = windowProtect;
  paramDone [ 1 ] = NULL;
  gpa_window_destroy ( paramDone );
} /* file_protect_protect */

void file_protect_dialog ( gboolean withSaveAs, gchar *tip ) {
/* var */
  GtkAccelGroup *accelGroup;
  static gpointer paramClose [ 2 ];
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
	GtkWidget *labelJfdSaveAs;
          GtkWidget *labelSaveAs;
        GtkWidget *entrySaveAs;
	GtkWidget *spaceBrowse;
	GtkWidget *buttonBrowse;
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
  if ( withSaveAs )
    tablePasswd = gtk_table_new ( 2, 4, FALSE );
  else
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
    GTK_FILL, GTK_SHRINK, 0, 0
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
  if ( withSaveAs )
    {
      labelSaveAs = gtk_label_new ( _( "" ) );
      labelJfdSaveAs = gpa_widget_hjustified_new (
        labelSaveAs, GTK_JUSTIFY_RIGHT
      );
      gtk_table_attach (
        GTK_TABLE ( tablePasswd ), labelJfdSaveAs, 0, 1, 2, 3,
        GTK_FILL, GTK_SHRINK, 0, 10
      );
      entrySaveAs = gtk_entry_new ();
      gpa_connect_by_accelerator (
        GTK_LABEL ( labelSaveAs ), entrySaveAs,
        accelGroup, _( "Save protected _file as: " )
      );
      gtk_table_attach (
        GTK_TABLE ( tablePasswd ), entrySaveAs, 1, 2, 2, 3,
        GTK_FILL, GTK_SHRINK, 0, 0
      );
      spaceBrowse = gpa_space_new ();
      gtk_table_attach (
        GTK_TABLE ( tablePasswd ), spaceBrowse, 2, 3, 2, 3,
        GTK_FILL, GTK_SHRINK, 5, 0
      );
      buttonBrowse = gpa_button_new ( accelGroup, _( "   _Browse   " ) );
      gtk_signal_connect_object (
        GTK_OBJECT ( buttonBrowse ), "clicked",
        GTK_SIGNAL_FUNC ( file_browse ), (gpointer) entrySaveAs
      );
      gtk_table_attach (
        GTK_TABLE ( tablePasswd ), buttonBrowse, 3, 4, 2, 3,
        GTK_FILL, GTK_SHRINK, 0, 0
      );
    } /* if */
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
  paramClose [ 0 ] = windowProtect;
  paramClose [ 1 ] = NULL;
  buttonCancel = gpa_buttonCancel_new (
    accelGroup, _( "_Cancel" ), paramClose
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
  gpa_widget_show ( windowProtect, global_windowMain, tip );
} /* file_protect_dialog */

void file_protect ( void ) {
  file_protect_dialog ( FALSE, _( "file_protect.tip" ) );
} /* file_protect */

void file_protectAs ( void ) {
  file_protect_dialog ( TRUE, _( "file_protectAs.tip" ) );
} /* file_protectAs */

void file_decrypt ( void ) {
g_print ( _( "Decrypt a file\n" ) ); /*!!!*/
} /* file_decrypt */

void file_decryptAs_decrypt ( GtkWidget *param [] ) {
/* var */
  gchar *filename;
  gpointer paramDone [ 2 ];
char message [ 100 ]; /*!!!*/
/* commands */
  filename = gtk_entry_get_text ( GTK_ENTRY ( param [ 0 ] ) );
sprintf ( message, "Decrypt file and save as %s\n", filename ); /*!!!*/
g_print ( message ); /*!!!*/
  paramDone [ 0 ] = param [ 1 ];
  paramDone [ 1 ] = NULL;
  gpa_window_destroy ( paramDone );
} /* file_decryptAs_decrypt */

void file_decryptAs ( void ) {
/* var */
  GtkAccelGroup *accelGroup;
  static GtkWidget *param [ 2 ];
  static gpointer paramClose [ 2 ];
/* objects */
  GtkWidget *windowDecrypt;
    GtkWidget *vboxDecrypt;
      GtkWidget *hboxTop;
	GtkWidget *labelFilename;
	GtkWidget *entryFilename;
	GtkWidget *spaceFilename;
	GtkWidget *buttonFilename;
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
  hboxTop = gtk_hbox_new ( FALSE, 0 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hboxTop ), 5 );
  labelFilename = gtk_label_new ( _( "" ) );
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), labelFilename, FALSE, FALSE, 0 );
  entryFilename = gtk_entry_new ();
  gpa_connect_by_accelerator (
    GTK_LABEL ( labelFilename ), entryFilename,
    accelGroup, _( "Save file _as: " )
  );
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), entryFilename, TRUE, TRUE, 0 );
  spaceFilename = gpa_space_new ();
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), spaceFilename, FALSE, FALSE, 5 );
  buttonFilename = gpa_button_new ( accelGroup, _( "   _Browse   " ) );
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonFilename ), "clicked",
    GTK_SIGNAL_FUNC ( file_browse ), (gpointer) entryFilename
  );
  gtk_box_pack_start ( GTK_BOX ( hboxTop ), buttonFilename, FALSE, FALSE, 0 );
  gtk_box_pack_start ( GTK_BOX ( vboxDecrypt ), hboxTop, TRUE, FALSE, 0 );
  hButtonBoxDecrypt = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (
    GTK_BUTTON_BOX ( hButtonBoxDecrypt ), GTK_BUTTONBOX_END
  );
  gtk_button_box_set_spacing ( GTK_BUTTON_BOX ( hButtonBoxDecrypt ), 10 );
  gtk_container_set_border_width ( GTK_CONTAINER ( hButtonBoxDecrypt ), 5 );
  paramClose [ 0 ] = windowDecrypt;
  paramClose [ 1 ] = NULL;
  buttonCancel = gpa_buttonCancel_new (
    accelGroup, _( "_Cancel" ), paramClose
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDecrypt ), buttonCancel );
  buttonDecrypt = gpa_button_new ( accelGroup, _( "_Decrypt" ) );
  param [ 0 ] = entryFilename;
  param [ 1 ] = windowDecrypt;
  gtk_signal_connect_object (
    GTK_OBJECT ( buttonDecrypt ), "clicked",
    GTK_SIGNAL_FUNC ( file_decryptAs_decrypt ), (gpointer) param
  );
  gtk_container_add ( GTK_CONTAINER ( hButtonBoxDecrypt ), buttonDecrypt );
  gtk_box_pack_start (
    GTK_BOX ( vboxDecrypt ), hButtonBoxDecrypt, FALSE, FALSE, 0
  );
  gtk_container_add ( GTK_CONTAINER ( windowDecrypt ), vboxDecrypt );
  gpa_widget_show (
    windowDecrypt, global_windowMain, _( "file_decryptAs.tip" )
  );
} /* file_decryptAs */

void file_verify ( void ) {
g_print ( _( "Verify signatures of a file\n" ) ); /*!!!*/
} /* file_verify */

void file_close ( void ) {
/* var */
  GList *indexFile, *previous;
  GpapaFile *file;
  GtkWidget *clistFile;
  gint position;
/* commands */
  if ( ! filesSelected )
    {
      gpa_window_error (
	_( "No files selected for closing" ), global_windowMain
      );
      return;
    } /* if */
  clistFile = gpa_get_global_clist_file ();
  indexFile = g_list_last ( filesOpened );
  while ( indexFile != NULL )
    {
      previous = g_list_previous ( indexFile );
      file = (GpapaFile*) indexFile -> data;
      if ( g_list_find ( filesSelected, file ) )
	{
	  position = g_list_position ( filesOpened, indexFile );
	  gtk_clist_remove ( GTK_CLIST ( clistFile ), position );
	  filesOpened = g_list_remove_link ( filesOpened, indexFile );
	  filesSelected = g_list_remove ( filesSelected, file );
	  gpapa_file_release ( file, gpa_callback, global_windowMain );
	  g_list_free_1 ( indexFile );
	} /* if */
      indexFile = previous;
    } /* while */
} /* file_close */

void file_quit ( void ) {
/*!!! Noch umaendern: Falls Aenderungen stattgefunden haben, ein */
/*!!! Frage-Fenster aufkommen lassen ("Aenderungen speichern?"). */
  gtk_main_quit ();
} /* file_quit */
