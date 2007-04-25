/* i18n.h 
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
 *
 * This file is part of GPA.
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

#ifndef I18N_H
#define I18N_H

#ifdef USE_SIMPLE_GETTEXT
   int set_gettext_file (const char *filename);
   char *gettext (const char *msgid);
   char *ngettext (const char *msgid1, const char *msgid2,
                   unsigned long int n);

#  define _(a) gettext (a)
#  define N_(a) (a)

#else /* !USE_SIMPLE_GETTEXT */
#  ifdef HAVE_LOCALE_H
#    include <locale.h>
#  endif

#  ifdef ENABLE_NLS
#    include <libintl.h>
#    define _(a) gettext (a)
#    ifdef gettext_noop
#      define N_(a) gettext_noop (a)
#    else
#      define N_(a) (a)
#    endif
#  else
#    define _(a) (a)
#    define N_(a) (a)
#    define ngettext(a,b,c) ((c)==1? (a):(b))
#  endif
#endif /* !USE_SIMPLE_GETTEXT */


#endif /*I18N_H*/

