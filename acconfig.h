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

@BOTTOM@

#include "gpadefs.h"

#endif /*GPA_CONFIG_H*/
