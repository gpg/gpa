#include <config.h>
#include <gpafile.h>

GpaFile *gpa_file_new ( gchar *anIdentifier, gchar *aName ) {
/* objects */
  GpaFile *fileNew;
/* commands */
  fileNew = (GpaFile*) malloc ( sizeof ( GpaFile ) );
  fileNew -> identifier = (gchar*) malloc ( strlen ( anIdentifier ) + 1 );
  strcpy ( fileNew -> identifier, anIdentifier );
  fileNew -> name = (gchar*) malloc ( strlen ( aName ) + 1 );
  strcpy ( fileNew -> name, aName );
  fileNew -> selected = FALSE;
  return ( fileNew );
} /* gpa_file_new */

gchar *gpa_file_get_identifier ( GpaFile *aFile ) {
  if ( aFile != NULL )
    return ( aFile -> identifier );
  else
    return ( NULL );
} /* gpa_file_get_identifier */

gchar *gpa_file_get_name ( GpaFile *aFile ) {
  if ( aFile != NULL )
    return ( aFile -> name );
  else
    return ( NULL );
} /* gpa_file_get_name */

gboolean gpa_file_is_selected ( GpaFile *aFile ) {
  if ( aFile != NULL )
    return ( aFile -> selected );
  else
    return ( FALSE );
} /* gpa_file_is_selected */

void gpa_file_set_selected ( GpaFile *aFile, gboolean value ) {
  if ( aFile != NULL )
    aFile -> selected = value;
} /* gpa_file_set_selected */
