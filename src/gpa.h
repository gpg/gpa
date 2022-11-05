/* gpa.h  -  main header
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2008, 2009 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef GPA_H
#define GPA_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gpgme.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assuan.h>

#include "gpgmetools.h"
#include "options.h"
#include "icons.h"

/* For mkdir() */
#ifdef G_OS_WIN32
#include <direct.h>
#include <io.h>
#define mkdir(p,m) _mkdir(p)
#ifndef S_ISDIR
#define S_ISDIR(mode) ((mode)&_S_IFDIR)
#endif
#endif

#include "i18n.h"   /* fixme: file should go into each source file */
#include "options.h" /* ditto */


#define ENABLE_TOFU_INFO 1

/* Global constants.  */
#define GPA_MAX_UID_WIDTH 50  /* # of chars after wich a user id is
                                 truncated in dialog boxes.  */



/* Some variable declarations.  */
extern GtkWidget *global_windowMain;
extern GtkWidget *global_windowTip;
extern GList *global_defaultRecipients;
extern gchar *gnupg_homedir;
extern gboolean cms_hack;
extern gboolean disable_ticker;
extern gboolean debug_edit_fsm;
extern gboolean verbose;

/* Show the keyring editor dialog.  */
void gpa_open_key_manager (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

/* Show the filemanager dialog.  */
void gpa_open_filemanager (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

/* Show the cardmanager dialog.  */
void gpa_open_cardmanager (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

/* Show the filemanager dialog.  */
void gpa_open_clipboard (GSimpleAction *simple, GVariant *parameter, gpointer user_data);


static const GActionEntry gpa_windows_menu_g_action_entries[] =
  {
    { "Windows", NULL },

    { "windows_keyring_editor", gpa_open_key_manager, NULL, NULL, NULL, { 0, 0, 0 } },
    { "windows_file_manager", gpa_open_filemanager, NULL, NULL, NULL, { 0, 0, 0 } },
    { "windows_clipboard", gpa_open_clipboard, NULL, NULL, NULL, { 0,0,0 } },
#ifdef ENABLE_CARD_MANAGER
    { "windows_card_manager", gpa_open_cardmanager, NULL, NULL, NULL, { 0,0,0 } },
#endif /* ENABLE_CARD_MANAGER */
  };


/* Show the settings dialog.  */
void gpa_open_settings_dialog (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

/* Show the backend configuration dialog.  */
void gpa_open_backend_config_dialog (GSimpleAction *simple, GVariant *parameter, gpointer user_data);


static const GActionEntry gpa_preferences_menu_g_action_entries[] =
  {
    { "edit_preferences", gpa_open_settings_dialog, NULL, NULL, NULL, { 0, 0, 0 } },
    { "edit_backend_preferences", gpa_open_backend_config_dialog, NULL, NULL, NULL, { 0, 0, 0 } },
  };

typedef void (*GPADefaultKeyChanged) (gpointer user_data);

void gpa_run_server_continuation (assuan_context_t ctx, gpg_error_t err);
void gpa_start_server (void);
void gpa_stop_server (void);
int  gpa_check_server (void);
gpg_error_t gpa_send_to_server (const char *cmd);


typedef struct gpa_filewatch_id_s *gpa_filewatch_id_t;
typedef void (*gpa_filewatch_cb_t)
     (void *user_data, const char *filename, const char *reason);
void gpa_init_filewatch (void);
gpa_filewatch_id_t gpa_add_filewatch (const char *filename,
                                      const char *maskstring,
                                      gpa_filewatch_cb_t cb,
                                      void *cb_data);

GtkApplication *get_gpa_application();

/*-- utils.c --*/
/* We are so used to these function thus provide them.  */
void *xmalloc (size_t n);
void *xcalloc (size_t n, size_t m);
char *xstrdup (const char *str);
#define xfree(a) g_free ((a))

int translate_sys2libc_fd (assuan_fd_t fd, int for_write);

char *decode_c_string (const char *src);

char *percent_escape (const char *string,
                      const char *delimiters, int space2plus);
size_t percent_unescape (char *string, int plus2space);
void decode_percent_string (char *str);


/*-- Convenience macros. -- */
#define DIM(v)		     (sizeof(v)/sizeof((v)[0]))
#define DIMof(type,member)   DIM(((type *)0)->member)

/*-- Macros to replace ctype ones to avoid locale problems. --*/
#define spacep(p)   (*(p) == ' ' || *(p) == '\t')
#define digitp(p)   (*(p) >= '0' && *(p) <= '9')
#define hexdigitp(a) (digitp (a)                     \
                      || (*(a) >= 'A' && *(a) <= 'F')  \
                      || (*(a) >= 'a' && *(a) <= 'f'))
  /* Note this isn't identical to a C locale isspace() without \f and
     \v, but works for the purposes used here. */
#define ascii_isspace(a) ((a)==' ' || (a)=='\n' || (a)=='\r' || (a)=='\t')

/* The atoi macros assume that the buffer has only valid digits. */
#define atoi_1(p)   (*(p) - '0' )
#define atoi_2(p)   ((atoi_1(p) * 10) + atoi_1((p)+1))
#define atoi_4(p)   ((atoi_2(p) * 100) + atoi_2((p)+2))
#define xtoi_1(p)   (*(p) <= '9'? (*(p)- '0'): \
                     *(p) <= 'F'? (*(p)-'A'+10):(*(p)-'a'+10))
#define xtoi_2(p)   ((xtoi_1(p) * 16) + xtoi_1((p)+1))
#define xtoi_4(p)   ((xtoi_2(p) * 256) + xtoi_2((p)+2))

#define tohex(n) ((n) < 10 ? ((n) + '0') : (((n) - 10) + 'A'))


/*--  Error codes not yet available --*/
#ifndef GPG_ERR_UNFINISHED
#define GPG_ERR_UNFINISHED 199
#endif

#endif /*GPA_H */

