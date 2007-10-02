/* gpa.h  -  main header
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
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

#ifndef GPA_H
#define GPA_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gpgme.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assuan.h>

#include "gpadefs.h"
#include "gpgmetools.h"
#include "options.h"

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


extern GtkWidget *global_windowMain;
extern GtkWidget *global_windowTip;
extern GList *global_defaultRecipients;
extern gchar *gnupg_homedir;
extern int cms_hack;

void gpa_open_keyring_editor (void);
void gpa_open_filemanager (void);
void gpa_open_settings_dialog (void);
GtkWidget * gpa_get_keyring_editor (void);
GtkWidget * gpa_get_settings_dialog (void);

typedef void (*GPADefaultKeyChanged) (gpointer user_data);

void gpa_run_server_continuation (assuan_context_t ctx, gpg_error_t err);
void gpa_start_server (void);


void gpa_show_backend_config (void);

/*-- utils.c --*/
/* We are so used to these function thus provide them.  */
void *xmalloc (size_t n);
void *xcalloc (size_t n, size_t m);
char *xstrdup (const char *str);
#define xfree(a) g_free ((a))

int translate_sys2libc_fd (assuan_fd_t fd, int for_write);


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
#ifndef GPA_ERR_SOURCE_GPA
#define GPA_ERR_SOURCE_GPA 12
#endif

#endif /*GPA_H */

