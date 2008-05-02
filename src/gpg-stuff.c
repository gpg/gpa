/* gpg-stuff.c - Code taken from GnuPG.
 * Copyright (C) 2008 g10 Code GmbH.
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006,
 *               2007, 2008 Free Software Foundation, Inc.
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

#include <config.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "gpa.h"
#include "strlist.h"
#include "gpg-stuff.h"



/* Break a string into successive option pieces.  Accepts single word
   options and key=value argument options. */
static char *
optsep (char **stringp)
{
  char *tok, *end;

  tok = *stringp;
  if (tok)
    {
      end = strpbrk (tok, " ,=");
      if (end)
	{
	  int sawequals = 0;
	  char *ptr = end;

	  /* what we need to do now is scan along starting with *end,
	     If the next character we see (ignoring spaces) is an =
	     sign, then there is an argument. */

	  while (*ptr)
	    {
	      if (*ptr == '=')
		sawequals = 1;
	      else if (*ptr != ' ')
		break;
	      ptr++;
	    }

	  /* There is an argument, so grab that too.  At this point,
	     ptr points to the first character of the argument. */
	  if (sawequals)
	    {
	      /* Is it a quoted argument? */
	      if (*ptr == '"')
		{
		  ptr++;
		  end = strchr (ptr, '"');
		  if (end)
		    end++;
		}
	      else
		end = strpbrk (ptr, " ,");
	    }

	  if (end && *end)
	    {
	      *end = '\0';
	      *stringp = end + 1;
	    }
	  else
	    *stringp = NULL;
	}
      else
	*stringp = NULL;
    }

  return tok;
}

/* Breaks an option value into key and value.  Returns NULL if there
   is no value.  Note that "string" is modified to remove the =value
   part. */
static char *
argsplit(char *string)
{
  char *equals,*arg=NULL;

  equals=strchr(string,'=');
  if(equals)
    {
      char *quote,*space;

      *equals='\0';
      arg=equals+1;

      /* Quoted arg? */
      quote=strchr(arg,'"');
      if(quote)
	{
	  arg=quote+1;

	  quote=strchr(arg,'"');
	  if(quote)
	    *quote='\0';
	}
      else
	{
	  size_t spaces;

	  /* Trim leading spaces off of the arg */
	  spaces=strspn(arg," ");
	  arg+=spaces;
	}

      /* Trim tailing spaces off of the tag */
      space=strchr(string,' ');
      if(space)
	*space='\0';
    }

  return arg;
}



/* 
   Code taken from gnupg-2.0.9/g10.keyserver.c.  Fixme: We should
   replace it by our generic URI parser from gnupg/common/http.c. 
*/

void
free_keyserver_spec(struct keyserver_spec *keyserver)
{
  xfree(keyserver->uri);
  xfree(keyserver->scheme);
  xfree(keyserver->auth);
  xfree(keyserver->host);
  xfree(keyserver->port);
  xfree(keyserver->path);
  xfree(keyserver->opaque);
  free_strlist(keyserver->options);
  xfree(keyserver);
}

static void
add_canonical_option (char *option,strlist_t *list)
{
  char *arg = argsplit(option);

  if (arg)
    {
      char *joined;

      joined = xmalloc(strlen(option)+1+strlen(arg)+1);
      /* Make a canonical name=value form with no spaces */
      strcpy(joined,option);
      strcat(joined,"=");
      strcat(joined,arg);
      append_to_strlist(list,joined);
      xfree(joined);
    }
  else
    append_to_strlist(list,option);
}


keyserver_spec_t
parse_keyserver_uri(const char *string,int require_scheme,
		    const char *configname,unsigned int configlineno)
{
  int assume_hkp=0;
  struct keyserver_spec *keyserver;
  const char *idx;
  int count;
  char *uri,*options;

  assert(string!=NULL);

  keyserver = g_malloc0 (sizeof(struct keyserver_spec));

  uri=xstrdup(string);

  options=strchr(uri,' ');
  if(options)
    {
      char *tok;

      *options='\0';
      options++;

      while((tok=optsep(&options)))
	add_canonical_option(tok,&keyserver->options);
    }

  /* Get the scheme */

  for(idx=uri,count=0;*idx && *idx!=':';idx++)
    {
      count++;

      /* Do we see the start of an RFC-2732 ipv6 address here?  If so,
	 there clearly isn't a scheme so get out early. */
      if(*idx=='[')
	{
	  /* Was the '[' the first thing in the string?  If not, we
	     have a mangled scheme with a [ in it so fail. */
	  if(count==1)
	    break;
	  else
	    goto fail;
	}
    }

  if(count==0)
    goto fail;

  if(*idx=='\0' || *idx=='[')
    {
      if(require_scheme)
	return NULL;

      /* Assume HKP if there is no scheme */
      assume_hkp=1;
      keyserver->scheme=xstrdup("hkp");

      keyserver->uri=xmalloc(strlen(keyserver->scheme)+3+strlen(uri)+1);
      strcpy(keyserver->uri,keyserver->scheme);
      strcat(keyserver->uri,"://");
      strcat(keyserver->uri,uri);
    }
  else
    {
      int i;

      keyserver->uri=xstrdup(uri);

      keyserver->scheme=xmalloc(count+1);

      /* Force to lowercase */
      for(i=0;i<count;i++)
	keyserver->scheme[i] = g_ascii_tolower(uri[i]);

      keyserver->scheme[i]='\0';

      /* Skip past the scheme and colon */
      uri+=count+1;
    }

  if (!g_ascii_strcasecmp(keyserver->scheme,"x-broken-hkp"))
    {
      xfree(keyserver->scheme);
      keyserver->scheme=xstrdup("hkp");
    }
  else if (!g_ascii_strcasecmp(keyserver->scheme,"x-hkp"))
    {
      /* Canonicalize this to "hkp" so it works with both the internal
	 and external keyserver interface. */
      xfree(keyserver->scheme);
      keyserver->scheme=xstrdup("hkp");
    }

  if (uri[0]=='/' && uri[1]=='/' && uri[2] == '/')
    {
      /* Three slashes means network path with a default host name.
         This is a hack because it does not crok all possible
         combiantions.  We should better repalce all code bythe parser
         from http.c.  */
      keyserver->path = xstrdup (uri+2);
    }
  else if(assume_hkp || (uri[0]=='/' && uri[1]=='/'))
    {
      /* Two slashes means network path. */

      /* Skip over the "//", if any */
      if(!assume_hkp)
	uri+=2;

      /* Do we have userinfo auth data present? */
      for(idx=uri,count=0;*idx && *idx!='@' && *idx!='/';idx++)
	count++;

      /* We found a @ before the slash, so that means everything
	 before the @ is auth data. */
      if(*idx=='@')
	{
	  if(count==0)
	    goto fail;

	  keyserver->auth=xmalloc(count+1);
	  strncpy(keyserver->auth,uri,count);
	  keyserver->auth[count]='\0';
	  uri+=count+1;
	}

      /* Is it an RFC-2732 ipv6 [literal address] ? */
      if(*uri=='[')
	{
	  for(idx=uri+1,count=1;*idx
		&& ((isascii (*idx) && isxdigit(*idx))
                    || *idx==':' || *idx=='.');idx++)
	    count++;

	  /* Is the ipv6 literal address terminated? */
	  if(*idx==']')
	    count++;
	  else
	    goto fail;
	}
      else
	for(idx=uri,count=0;*idx && *idx!=':' && *idx!='/';idx++)
	  count++;

      if(count==0)
	goto fail;

      keyserver->host=xmalloc(count+1);
      strncpy(keyserver->host,uri,count);
      keyserver->host[count]='\0';

      /* Skip past the host */
      uri+=count;

      if(*uri==':')
	{
	  /* It would seem to be reasonable to limit the range of the
	     ports to values between 1-65535, but RFC 1738 and 1808
	     imply there is no limit.  Of course, the real world has
	     limits. */

	  for(idx=uri+1,count=0;*idx && *idx!='/';idx++)
	    {
	      count++;

	      /* Ports are digits only */
	      if(!digitp(idx))
		goto fail;
	    }

	  keyserver->port=xmalloc(count+1);
	  strncpy(keyserver->port,uri+1,count);
	  keyserver->port[count]='\0';

	  /* Skip past the colon and port number */
	  uri+=1+count;
	}

      /* Everything else is the path */
      if(*uri)
	keyserver->path=xstrdup(uri);
      else
	keyserver->path=xstrdup("/");

      if(keyserver->path[1])
	keyserver->flags.direct_uri=1;
    }
  else if(uri[0]!='/')
    {
      /* No slash means opaque.  Just record the opaque blob and get
	 out. */
      keyserver->opaque=xstrdup(uri);
    }
  else
    {
      /* One slash means absolute path.  We don't need to support that
	 yet. */
      goto fail;
    }

  return keyserver;

 fail:
  free_keyserver_spec(keyserver);

  return NULL;
}



static void
gpg_free_akl (akl_t akl)
{
  if (akl->spec)
    free_keyserver_spec (akl->spec);

  xfree (akl);
}

void
gpg_release_akl (akl_t akl)
{
  while (akl)
    {
      akl_t tmp = akl->next;
      gpg_free_akl (akl);
      akl = tmp;
    }
}

/* Takes a string OPTIONS with the argument as used for gpg'c
   --auto-key-locate and returns an AKL list or NULL on error.  Note
   that this function may modify OPTIONS. */
akl_t
gpg_parse_auto_key_locate (char *options)
{
  char *tok;
  akl_t result = NULL;

  while ( (tok = optsep (&options)) )
    { 
      akl_t akl, check;
      akl_t last = NULL;
      int dupe = 0;

      if (!tok[0])
	continue;

      akl = xcalloc (1, sizeof (*akl));

      if (!g_ascii_strcasecmp (tok, "nodefault"))
	akl->type = AKL_NODEFAULT;
      else if (!g_ascii_strcasecmp (tok, "local"))
	akl->type = AKL_LOCAL;
      else if (!g_ascii_strcasecmp (tok, "ldap"))
	akl->type = AKL_LDAP;
      else if (!g_ascii_strcasecmp (tok, "keyserver"))
	akl->type = AKL_KEYSERVER;
      else if (!g_ascii_strcasecmp (tok, "cert"))
	akl->type = AKL_CERT;
      else if (!g_ascii_strcasecmp (tok,"pka"))
	akl->type = AKL_PKA;
      else if ((akl->spec = parse_keyserver_uri (tok, 1, NULL, 0)))
	akl->type = AKL_SPEC;
      else
	{
	  gpg_free_akl (akl);
	  gpg_release_akl (result);
	  return NULL;
	}

      /* Check for duplicates.  We must maintain the order the user
         gave us */
      for (check = result; check; last=check, check=check->next)
	{
	  if(check->type==akl->type
	     && (akl->type!=AKL_SPEC
		 || (akl->type==AKL_SPEC
		     && !strcmp(check->spec->uri,akl->spec->uri))))
	    {
	      dupe=1;
	      gpg_free_akl(akl);
	      break;
	    }
	}

      if (!dupe)
	{
	  if (last)
	    last->next = akl;
	  else
	    result = akl;
	}
    }

  return result;
}



