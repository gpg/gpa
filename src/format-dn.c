/* format-dn.c  - Functions to format an ASN.1 Distinuguished Name.
 * Copyright (C) 2001, 2004, 2007 Free Software Foundation, Inc.
 * Copyright (C) 2009 g10 Code GmbH.
 *
 * This file is part of GPA.
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

/* 
   This code is based on code taken from GnuPG (sm/certdump.c).  It
   has been converted to use only Glib stuff.
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "gpa.h"
#include "format-dn.h"


struct dn_array_s 
{
  char *key;
  char *value;
  int   multivalued;
  int   done;
};


/* Remove trailing white spaces from STRING.  */
static void
trim_trailing_spaces (char *string)
{
  char *p, *mark;
  
  for (mark=NULL, p=string; *p; p++)
    {
      if (g_ascii_isspace (*p))
        {
          if (!mark)
            mark = p;
	}
      else
        mark = NULL;
    }
  if (mark)
    *mark = '\0' ;
}



/* Helper for the rfc2253 string parser.  */
static const char *
parse_dn_part (struct dn_array_s *array, const char *string)
{
  static struct {
    const char *label;
    const char *oid;
  } label_map[] = 
    {
      /* Note: Take care we expect the LABEL not more than 9 bytes
         longer than the OID.  */
      {"EMail",        "1.2.840.113549.1.9.1" },
      {"T",            "2.5.4.12" },
      {"GN",           "2.5.4.42" },
      {"SN",           "2.5.4.4" },
      {"NameDistinguisher", "0.2.262.1.10.7.20"}, 
      {"ADDR",         "2.5.4.16" },
      {"BC",           "2.5.4.15" },
      {"D",            "2.5.4.13" },
      {"PostalCode",   "2.5.4.17" },
      {"Pseudo",       "2.5.4.65" },
      {"SerialNumber", "2.5.4.5" },
      {NULL, NULL}
    };
  const char *s, *s1;
  size_t n;
  char *p;
  int i;

  /* Parse attributeType */
  for (s = string+1; *s && *s != '='; s++)
    ;
  if (!*s)
    return NULL; /* error */
  n = s - string;
  if (!n)
    return NULL; /* empty key */

  /* We need to allocate a few bytes more due to the possible mapping
     from the shorter OID to the longer label.  */
  array->key = p = g_try_malloc (n+10);
  if (!array->key)
    return NULL;
  memcpy (p, string, n); 
  p[n] = 0;
  trim_trailing_spaces (p);

  if (g_ascii_isdigit (*p))
    {
      for (i=0; label_map[i].label; i++ )
        if ( !strcmp (p, label_map[i].oid) )
          {
            strcpy (p, label_map[i].label);
            break;
          }
    }
  string = s + 1;

  if (*string == '#')
    { 
      /* Hexstring. */
      string++;
      for (s=string; g_ascii_isxdigit (*s); s++)
        s++;
      n = s - string;
      if (!n || (n & 1))
        return NULL; /* Empty or odd number of digits.  */
      n /= 2;
      array->value = p = g_try_malloc (n+1);
      if (!p)
        return NULL;
      for (s1=string; n; s1 += 2, n--, p++)
        {
          *(unsigned char *)p = xtoi_2 (s1);
          if (!*p)
            *p = 0x01; /* Better print a wrong value than truncating
                          the string. */
        }
      *p = 0;
   }
  else
    { 
      /* Regular v3 quoted string.  */
      for (n=0, s=string; *s; s++)
        {
          if (*s == '\\')
            { 
              /* Pair. */
              s++;
              if (*s == ',' || *s == '=' || *s == '+'
                  || *s == '<' || *s == '>' || *s == '#' || *s == ';' 
                  || *s == '\\' || *s == '\"' || *s == ' ')
                n++;
              else if (g_ascii_isxdigit (*s) && g_ascii_isxdigit(s[1]))
                {
                  s++;
                  n++;
                }
              else
                return NULL; /* Invalid escape sequence.  */
            }
          else if (*s == '\"')
            return NULL; /* Invalid encoding.  */
          else if (*s == ',' || *s == '=' || *s == '+'
                   || *s == '<' || *s == '>' || *s == ';' )
            break; 
          else
            n++;
        }
      
      array->value = p = g_try_malloc (n+1);
      if (!p)
        return NULL;
      for (s=string; n; s++, n--)
        {
          if (*s == '\\')
            { 
              s++;
              if (g_ascii_isxdigit (*s))
                {
                  *(unsigned char *)p++ = xtoi_2 (s);
                  s++;
                }
              else
                *p++ = *s;
            }
          else
            *p++ = *s;
        }
      *p = 0;
    }
  return s;
}


/* Parse a DN and return an array-ized one.  This is not a validating
   parser and it does not support any old-stylish syntax; KSBA is
   expected to return only rfc2253 compatible strings. */
static struct dn_array_s *
parse_dn (const char *string)
{
  struct dn_array_s *array;
  size_t arrayidx, arraysize;
  int i;

  arraysize = 7; /* C,ST,L,O,OU,CN,email */
  arrayidx = 0;
  array = g_try_malloc ((arraysize+1) * sizeof *array);
  if (!array)
    return NULL;

  while (*string)
    {
      while (*string == ' ')
        string++;
      if (!*string)
        break; /* Ready.  */
      if (arrayidx >= arraysize)
        { 
          struct dn_array_s *a2;

          arraysize += 5;
          a2 = g_try_realloc (array, (arraysize+1) * sizeof *array);
          if (!a2)
            goto failure;
          array = a2;
        }
      array[arrayidx].key = NULL;
      array[arrayidx].value = NULL;
      string = parse_dn_part (array+arrayidx, string);
      if (!string)
        goto failure;
      while (*string == ' ')
        string++;
      array[arrayidx].multivalued = (*string == '+');
      array[arrayidx].done = 0;
      arrayidx++;
      if (*string && *string != ',' && *string != ';' && *string != '+')
        goto failure; /* Invalid delimiter. */
      if (*string)
        string++;
    }
  array[arrayidx].key = NULL;
  array[arrayidx].value = NULL;
  return array;

 failure:
  for (i=0; i < arrayidx; i++)
    {
      g_free (array[i].key);
      g_free (array[i].value);
    }
  g_free (array);
  return NULL;
}


/* Append BUFFER to OUTOPUT while replacing all control characters and
   the characters in DELIMITERS by standard C escape sequences.  */
static void 
append_sanitized (GString *output, const void *buffer, size_t length,
                  const char *delimiters)
{
  const unsigned char *p = buffer;
  size_t count = 0;
  
  for (; length; length--, p++, count++)
    {
      if (*p < 0x20 
          || *p == 0x7f
          || (delimiters 
              && (strchr (delimiters, *p) || *p == '\\')))
        {
          g_string_append_c (output, '\\');
          if (*p == '\n')
            g_string_append_c (output, 'n');
          else if (*p == '\r')
            g_string_append_c (output, 'r');
          else if (*p == '\f')
            g_string_append_c (output, 'f');
          else if (*p == '\v')
            g_string_append_c (output, 'v');
          else if (*p == '\b')
            g_string_append_c (output, 'b');
          else if (!*p)
            g_string_append_c (output, '0');
          else
            g_string_append_printf (output, "x%02x", *p);
	}
      else
        g_string_append_c (output, *p);
    }
}


/* Print a DN part to STREAM or if STREAM is NULL to FP. */
static void
print_dn_part (GString *output, struct dn_array_s *dn, const char *key)
{
  struct dn_array_s *first_dn = dn;

  for (; dn->key; dn++)
    {
      if (!dn->done && !strcmp (dn->key, key))
        {
          /* Forward to the last multi-valued RDN, so that we can
             print them all in reverse in the correct order.  Note
             that this overrides the the standard sequence but that
             seems to a reasonable thing to do with multi-valued
             RDNs. */
          while (dn->multivalued && dn[1].key)
            dn++;
        next:
          if (!dn->done && dn->value && *dn->value)
            {
              g_string_append_printf (output, "/%s=", dn->key);
              append_sanitized (output, dn->value, strlen (dn->value), "/");
            }
          dn->done = 1;
          if (dn > first_dn && dn[-1].multivalued)
            {
              dn--;
              goto next;
            }
        }
    }
}


/* Print all parts of a DN in a "standard" sequence.  We first print
   all the known parts, followed by the uncommon ones.  */
static void
print_dn_parts (GString *output, struct dn_array_s *dn)
{
  const char *stdpart[] = {
    "CN", "OU", "O", "STREET", "L", "ST", "C", "EMail", NULL 
  };
  int i;
  
  for (i=0; stdpart[i]; i++)
    print_dn_part (output, dn, stdpart[i]);

  /* Now print the rest without any specific ordering */
  for (; dn->key; dn++)
    print_dn_part (output, dn, dn->key);
}


/* Format an RFC2253 encoded DN or GeneralName.  Caller needs to
   release the return ed string.  This function will never return
   NULL.  */
char *
gpa_format_dn (const char *name) 
{
  char *retval = NULL;
  
  if (!name)
    retval = g_strdup (_("[Error - No name]"));
  else if (*name == '<')
    {
      const char *s = strchr (name+1, '>');
      if (s)
        retval = g_strndup (name+1, s - (name+1));
    }
  else if (*name == '(')
    retval = g_strdup (_("[Error - Encoding not supported]"));
  else if (g_ascii_isalnum (*name))
    {
      struct dn_array_s *dn;
      int i;

      dn = parse_dn (name);
      if (dn)
        {
          GString *output = g_string_sized_new (strlen (name));
          print_dn_parts (output, dn);          
          retval = g_string_free (output, FALSE);
          for (i=0; dn[i].key; i++)
            {
              g_free (dn[i].key);
              g_free (dn[i].value);
            }
          g_free (dn);
        }
    }
 
  if (!retval)
    retval = g_strdup (_("[Error - Invalid encoding]"));

  return retval;
}



