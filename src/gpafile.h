#include <config.h>
#include <glib.h>

typedef enum {
  GPA_FILE_CLEAR,
  GPA_FILE_ENCRYPTED,
  GPA_FILE_PROTECTED
} GpaFileStatus;

typedef struct {
  gchar *identifier;
  gchar *name;
  gboolean selected;
} GpaFile;

extern GpaFile *gpa_file_new ( gchar *anIdentifier, gchar *aName );
extern gchar *gpa_file_get_identifier ( GpaFile *aFile );
extern gchar *gpa_file_get_name ( GpaFile *aFile );
extern gboolean gpa_file_is_selected ( GpaFile *aFile );
extern void gpa_file_set_selected ( GpaFile *aFile, gboolean value );
