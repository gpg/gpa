/* utils.c -  Utility functions for GPA.
 * Copyright (C) 2008 g10 Code GmbH
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#ifdef HAVE_W32_SYSTEM
# include <windows.h>
#endif /*HAVE_W32_SYSTEM*/

#include "gpa.h"


#define tohex_lower(n) ((n) < 10 ? ((n) + '0') : (((n) - 10) + 'a'))


/* We want our usual xmalloc function.  */
void *
xmalloc (size_t n)
{
  if (!n)
    n++;
  return g_malloc (n);
}


/* We want an xcalloc.  The similar g_new0 macro is not correclty
   implemented as it allows an integer overflow.  */
void *
xcalloc (size_t n, size_t m)
{
  size_t nbytes;
  void *p;

  nbytes = n * m;
  if ( m && nbytes / m != n)
    {
      errno = ENOMEM;
      p = NULL;
    }
  else
    p = g_malloc (nbytes);
  if (p)
    memset (p, 0, nbytes);
  else
    {
      g_error ("%s: failed to allocate %lu bytes",
               G_STRLOC, (unsigned long)nbytes);
      abort (); /* Just in case g_error returns.  */
    }

  return p;
}


char *
xstrdup (const char *str)
{
  char *buf = g_malloc (strlen(str) + 1);
  strcpy (buf, str);
  return buf;
}



/* This function is a NOP for POSIX systems but required under Windows
   as the file handles as returned by OS calls (like CreateFile) are
   different from the libc file descriptors (like open). This function
   translates system file handles to libc file handles.  FOR_WRITE
   gives the direction of the handle.  */
int
translate_sys2libc_fd (assuan_fd_t fd, int for_write)
{
#ifdef HAVE_W32_SYSTEM
  int x;

  if (fd == ASSUAN_INVALID_FD)
    return -1;

  /* Note that _open_osfhandle is currently defined to take and return
     a long.  */
  x = _open_osfhandle ((long)fd, for_write ? 1 : 0);
  if (x == -1)
    g_debug ("failed to translate osfhandle %p\n", (void *) fd);
  return x;
#else /*!HAVE_W32_SYSTEM */
  return fd;
#endif
}


#ifdef HAVE_W32_SYSTEM
int
inet_aton (const char *cp, struct in_addr *inp)
{
  if (!cp || !*cp || !inp)
    {
      errno = EINVAL;
      return 0;
    }

  if (!strcmp(cp, "255.255.255.255"))
    {
      /*  Although this is a valid address, the old inet_addr function
          is not able to handle it.  */
        inp->s_addr = INADDR_NONE;
        return 1;
    }

  inp->s_addr = inet_addr (cp);
  return (inp->s_addr != INADDR_NONE);
}
#endif /*HAVE_W32_SYSTEM*/


/* Convert two hexadecimal digits from STR to the value they
   represent.  Returns -1 if one of the characters is not a
   hexadecimal digit.  */
static int
hextobyte (const char *str)
{
  int val = 0;
  int i;

#define NROFHEXDIGITS 2
  for (i = 0; i < NROFHEXDIGITS; i++)
    {
      if (*str >= '0' && *str <= '9')
	val += *str - '0';
      else if (*str >= 'A' && *str <= 'F')
	val += 10 + *str - 'A';
      else if (*str >= 'a' && *str <= 'f')
	val += 10 + *str - 'a';
      else
	return -1;
      if (i < NROFHEXDIGITS - 1)
	val *= 16;
      str++;
    }
#undef NROFHEXDIGITS
  return val;
}


/* Decode the C formatted string SRC and return the result in a newly
   allocated buffer.  */
char *
decode_c_string (const char *src)
{
  char *buffer, *dest;

  /* The converted string will never be larger than the original
     string.  */
  dest = buffer = xmalloc (strlen (src) + 1);

  while (*src)
    {
      if (*src != '\\')
	{
	  *(dest++) = *(src++);
	  continue;
	}

#define DECODE_ONE(match,result)	\
	case match:			\
	  src += 2;			\
	  *(dest++) = result;		\
	  break;

      switch (src[1])
	{

	  DECODE_ONE ('\'', '\'');
	  DECODE_ONE ('\"', '\"');
	  DECODE_ONE ('\?', '\?');
	  DECODE_ONE ('\\', '\\');
	  DECODE_ONE ('a', '\a');
	  DECODE_ONE ('b', '\b');
	  DECODE_ONE ('f', '\f');
	  DECODE_ONE ('n', '\n');
	  DECODE_ONE ('r', '\r');
	  DECODE_ONE ('t', '\t');
	  DECODE_ONE ('v', '\v');

	case 'x':
	  {
	    int val = hextobyte (&src[2]);

	    if (val == -1)
	      {
		/* Should not happen.  */
		*(dest++) = *(src++);
		*(dest++) = *(src++);
		if (*src)
		  *(dest++) = *(src++);
		if (*src)
		  *(dest++) = *(src++);
	      }
	    else
	      {
		if (!val)
		  {
		    /* A binary zero is not representable in a C
		       string thus we keep the C-escaping.  Note that
		       this will also never be larger than the source
		       string.  */
		    *(dest++) = '\\';
		    *(dest++) = '0';
		  }
		else
		  *((unsigned char *) dest++) = val;
		src += 4;
	      }
	  }
	  break;

	default:
	  {
	    /* Should not happen.  */
	    *(dest++) = *(src++);
	    *(dest++) = *(src++);
	  }
        }
#undef DECODE_ONE
    }
  *(dest++) = 0;

  return buffer;
}


/* Percent-escape all characters from the set in DELIMITERS in STRING.
   If SPACE2PLUS is true, spaces (0x20) are converted to plus signs
   and PLUS signs are percent escaped.  If DELIMITERS is NULL all
   characters less or equal than 0x20 are escaped.  However if
   SPACE2PLUS istrue spaces are still converted to plus signs.
   Returns a newly allocated string.  */
char *
percent_escape (const char *string, const char *delimiters, int space2plus)
{
  const unsigned char *s;
  size_t needed;
  char *buffer, *ptr;

  if (!delimiters)
    delimiters = ("\x20\x02\x03\x04\x05\x06\x07\x08"
                  "\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10"
                  "\x11\x12\x13\x14\x15\x16\x17\x18"
                  "\x19\x1a\x1b\x1c\x1d\x1e\x1f\x01");

  for (s=(const unsigned char*)string, needed=0; *s; s++, needed++)
    if (*s == '%'
        || (space2plus && *s == '+')
        || strchr (delimiters, *s))
      needed += 2;

  buffer = g_malloc (needed + 1);

  for (s=(const unsigned char *)string, ptr=buffer; *s; s++)
    {
      if (*s == '%')
	{
	  *ptr++ = '%';
	  *ptr++ = '2';
	  *ptr++ = '5';
	}
      else if (space2plus && *s == ' ')
	{
	  *ptr++ = '+';
	}
      else if (space2plus && *s == '+')
	{
	  *ptr++ = '%';
	  *ptr++ = '2';
	  *ptr++ = 'b';
	}
      else if (strchr (delimiters, *s))
        {
	  *ptr++ = '%';
          *ptr++ = tohex_lower ((*s>>4)&15);
          *ptr++ = tohex_lower (*s&15);
        }
      else
	*ptr++ = *s;
    }
  *ptr = 0;

  return buffer;
}


/* Remove percent escapes from STRING.  If PLUS2SPACE is true, also
   convert '+' back to space.  Returns the length of the unescaped
   string.  */
size_t
percent_unescape (char *string, int plus2space)
{
  unsigned char *p = (unsigned char *)string;
  size_t n = 0;

  while (*string)
    {
      if (*string == '%' && string[1] && string[2])
        {
          string++;
          *p++ = xtoi_2 (string);
          n++;
          string+= 2;
        }
      else if (*string == '+' && plus2space)
        {
          *p++ = ' ';
          n++;
          string++;
        }
      else
        {
          *p++ = *string++;
          n++;
        }
    }
  *p = 0;

  return n;
}


/* Decode the percent escaped string STR in place.  In contrast to
   percent_unescape, this make sure that the result is a string and in
   addition escapes embedded nuls. */
void
decode_percent_string (char *str)
{
  char *src = str;
  char *dest = str;

  /* Convert the string.  */
  while (*src)
    {
      if (*src != '%')
        {
          *(dest++) = *(src++);
          continue;
        }
      else
        {
          int val = hextobyte (&src[1]);

          if (val == -1)
            {
              /* Should not happen.  */
              *(dest++) = *(src++);
              if (*src)
                *(dest++) = *(src++);
              if (*src)
                *(dest++) = *(src++);
            }
          else
            {
              if (!val)
                {
                  /* A binary zero is not representable in a C
                     string.  */
                  *(dest++) = '\\';
                  *(dest++) = '0';
                }
              else
                *((unsigned char *) dest++) = val;
              src += 3;
            }
        }
    }
  *(dest++) = 0;
}
