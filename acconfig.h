/* acconfig.h - used by autoheader to make config.h.in
 */
#ifndef GPA_CONFIG_H
#define GPA_CONFIG_H

/* Need this, because some autoconf tests rely on this (e.g. stpcpy)
 * and it should be used for new programs anyway. */
#define _GNU_SOURCE  1

@TOP@

/* Define if your locale.h file contains LC_MESSAGES.  */
#undef HAVE_LC_MESSAGES

/* Define to 1 if NLS is requested.  */
#undef ENABLE_NLS

/* Define as 1 if you have catgets and don't want to use GNU gettext.  */
#undef HAVE_CATGETS

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
#undef HAVE_GETTEXT

/* libintl.h is available; this is obsolete because if we don't have
 * this header we use a symlink to the one in intl/ */
#undef HAVE_LIBINTL_H


#undef HAVE_STPCPY

#undef HAVE_BYTE_TYPEDEF
#undef HAVE_USHORT_TYPEDEF
#undef HAVE_ULONG_TYPEDEF
#undef HAVE_U16_TYPEDEF
#undef HAVE_U32_TYPEDEF

/* set this to limit filenames to the 8.3 format */
#undef USE_ONLY_8DOT3
/* defined if we run on some of the PCDOS like systems (DOS, Windoze. OS/2)
 * with special properties like no file modes */
#undef HAVE_DOSISH_SYSTEM
/* because the Unix gettext has to much overhead on MingW32 systems
 *  * and these systems lack Posix functions, we use a simplified version
 *   * of gettext */
#undef USE_SIMPLE_GETTEXT
/* defined if the filesystem uses driver letters */
#undef HAVE_DRIVE_LETTERS
/* Some systems have a mkdir that takes a single argument. */
#undef MKDIR_TAKES_ONE_ARG
/* path to the gpg binary */
#undef GPG_PATH


@BOTTOM@

#include "gpadefs.h"

#endif /*GPA_CONFIG_H*/
