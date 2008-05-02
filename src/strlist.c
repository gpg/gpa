/* strlist.c -  string helpers
 * Copyright (C) 2008 g10 Code GmbH.
 * Copyright (C) 1998, 2000, 2001, 2006 Free Software Foundation, Inc.
 *
 * This file is part of JNLIB.
 *
 * JNLIB is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * JNLIB is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "gpa.h"
#include "strlist.h"

void
free_strlist( strlist_t sl )
{
    strlist_t sl2;

    for(; sl; sl = sl2 ) {
	sl2 = sl->next;
	g_free(sl);
    }
}


strlist_t
add_to_strlist( strlist_t *list, const char *string )
{
    strlist_t sl;

    sl = g_malloc ( sizeof *sl + strlen(string));
    sl->flags = 0;
    strcpy(sl->d, string);
    sl->next = *list;
    *list = sl;
    return sl;
}


/* Same as add_to_strlist() but if is_utf8 is *not* set, a conversion
   to UTF-8 is done.  */
#if 0
#ifdef JNLIB_NEED_UTF8CONV
strlist_t
add_to_strlist2( strlist_t *list, const char *string, int is_utf8 )
{
  strlist_t sl;
  
  if (is_utf8)
    sl = add_to_strlist( list, string );
  else 
    {
      char *p = native_to_utf8( string );
      sl = add_to_strlist( list, p );
      jnlib_free ( p );
    }
  return sl;
}
#endif /* JNLIB_NEED_UTF8CONV*/
#endif 

strlist_t
append_to_strlist( strlist_t *list, const char *string )
{
    strlist_t r, sl;

    sl = g_malloc( sizeof *sl + strlen(string));
    sl->flags = 0;
    strcpy(sl->d, string);
    sl->next = NULL;
    if( !*list )
	*list = sl;
    else {
	for( r = *list; r->next; r = r->next )
	    ;
	r->next = sl;
    }
    return sl;
}


#if 0
#ifdef JNLIB_NEED_UTF8CONV
strlist_t
append_to_strlist2( strlist_t *list, const char *string, int is_utf8 )
{
  strlist_t sl;
    
  if( is_utf8 )
    sl = append_to_strlist( list, string );
  else
    {
      char *p = native_to_utf8 (string);
      sl = append_to_strlist( list, p );
      jnlib_free( p );
    }
  return sl;
}
#endif /* JNLIB_NEED_UTF8CONV */
#endif

/* Return a copy of LIST. */
strlist_t
strlist_copy (strlist_t list)
{
  strlist_t newlist = NULL, sl, *last;

  last = &newlist;
  for (; list; list = list->next)
    {
      sl = g_malloc (sizeof *sl + strlen (list->d));
      sl->flags = list->flags;
      strcpy(sl->d, list->d);
      sl->next = NULL;
      *last = sl;
      last = &sl;
    }
  return newlist;
}



strlist_t
strlist_prev( strlist_t head, strlist_t node )
{
    strlist_t n;

    for(n=NULL; head && head != node; head = head->next )
	n = head;
    return n;
}

strlist_t
strlist_last( strlist_t node )
{
    if( node )
	for( ; node->next ; node = node->next )
	    ;
    return node;
}


char *
strlist_pop (strlist_t *list)
{
  char *str=NULL;
  strlist_t sl=*list;

  if(sl)
    {
      str = g_malloc ( strlen (sl->d) + 1 );
      strcpy (str,sl->d);

      *list=sl->next;
      g_free(sl);
    }

  return str;
}

