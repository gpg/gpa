/* filetype.c -  Identify file types
 * Copyright (C) 2012 g10 Code GmbH
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpgme.h>

#include "parsetlv.h"
#include "filetype.h"


/* The size of the buffer we use to identify CMS objects.  */
#define CMS_BUFFER_SIZE 2048


/* Warning: DATA may be binary but there must be a Nul before DATALEN.  */
#ifndef HAVE_GPGME_DATA_IDENTIFY
static int
detect_cms (const char *data, size_t datalen)
{
  tlvinfo_t ti;
  const char *s;
  size_t n;

  if (datalen < 24) /* Object is probably too short for CMS.  */
    return 0;

  s = data;
  n = datalen;
  if (parse_tlv (&s, &n, &ti))
    goto try_pgp; /* Not properly BER encoded.  */
  if (!(ti.cls == ASN1_CLASS_UNIVERSAL && ti.tag == ASN1_TAG_SEQUENCE
        && ti.is_cons))
    goto try_pgp; /* A CMS object always starts witn a sequence.  */
  if (parse_tlv (&s, &n, &ti))
    goto try_pgp; /* Not properly BER encoded.  */
  if (!(ti.cls == ASN1_CLASS_UNIVERSAL && ti.tag == ASN1_TAG_OBJECT_ID
        && !ti.is_cons && ti.length) || ti.length > n)
    goto try_pgp; /* This is not an OID as expected.  */
  if (ti.length == 9)
    {
      if (!memcmp (s, "\x2A\x86\x48\x86\xF7\x0D\x01\x07\x03", 9))
        return 1; /* Encrypted (aka Enveloped Data).  */
      if (!memcmp (s, "\x2A\x86\x48\x86\xF7\x0D\x01\x07\x02", 9))
        return 1; /* Signed.  */
    }

 try_pgp:
  /* Check whether this might be a non-armored PGP message.  We need
     to do this before checking for armor lines, so that we don't get
     fooled by armored messages inside a signed binary PGP message.  */
  if ((data[0] & 0x80))
    {
      /* That might be a binary PGP message.  At least it is not plain
         ASCII.  Of course this might be certain lead-in text of
         armored CMS messages.  However, I am not sure whether this is
         at all defined and in any case it is uncommon.  Thus we don't
         do any further plausibility checks but stupidly assume no CMS
         armored data will follow.  */
      return 0;
    }

  /* Now check whether there are armor lines.  */
  for (s = data; s && *s; s = (*s=='\n')?(s+1):((s=strchr (s,'\n'))?(s+1):s))
    {
      if (!strncmp (s, "-----BEGIN ", 11))
        {
          if (!strncmp (s+11, "PGP ", 4))
            return 0; /* This is PGP */
          return 1; /* Not PGP, thus we assume CMS.  */
        }
    }

  return 0;
}
#endif /*!HAVE_GPGME_DATA_IDENTIFY*/


/* Return true if the file FNAME looks like an CMS file.  There is no
   error return, just a best effort try to identify CMS in a file with
   a CMS object.  */
int
is_cms_file (const char *fname)
{
#ifdef HAVE_GPGME_DATA_IDENTIFY
  FILE *fp;
  gpgme_data_t dh;
  gpgme_data_type_t dt;

  fp = fopen (fname, "rb");
  if (!fp)
    return 0; /* Not found - can't be a CMS file.  */
  if (gpgme_data_new_from_stream (&dh, fp))
    {
      fclose (fp);
      return 0;
    }
  dt = gpgme_data_identify (dh, 0);
  gpgme_data_release (dh);
  fclose (fp);
  switch (dt)
    {
    case GPGME_DATA_TYPE_CMS_SIGNED:
    case GPGME_DATA_TYPE_CMS_ENCRYPTED:
    case GPGME_DATA_TYPE_CMS_OTHER:
    case GPGME_DATA_TYPE_X509_CERT:
    case GPGME_DATA_TYPE_PKCS12:
      return 1;
    default:
      return 0;
    }
#else
  int result;
  FILE *fp;
  char *data;
  size_t datalen;

  fp = fopen (fname, "rb");
  if (!fp)
    return 0; /* Not found - can't be a CMS file.  */

  data = malloc (CMS_BUFFER_SIZE);
  if (!data)
    {
      fclose (fp);
      return 0; /* Oops */
    }

  datalen = fread (data, 1, CMS_BUFFER_SIZE - 1, fp);
  data[datalen] = 0;
  fclose (fp);

  result = detect_cms (data, datalen);
  free (data);
  return result;
#endif
}


/* Return true if the data (DATA,DATALEN) looks like an CMS object.
   There is no error return, just a best effort try to identify CMS.  */
int
is_cms_data (const char *data, size_t datalen)
{
#ifdef HAVE_GPGME_DATA_IDENTIFY
  gpgme_data_t dh;
  gpgme_data_type_t dt;

  if (gpgme_data_new_from_mem (&dh, data, datalen, 0))
    return 0;
  dt = gpgme_data_identify (dh, 0);
  gpgme_data_release (dh);
  switch (dt)
    {
    case GPGME_DATA_TYPE_CMS_SIGNED:
    case GPGME_DATA_TYPE_CMS_ENCRYPTED:
    case GPGME_DATA_TYPE_CMS_OTHER:
    case GPGME_DATA_TYPE_X509_CERT:
    case GPGME_DATA_TYPE_PKCS12:
      return 1;
    default:
      return 0;
    }
#else
  int result;
  char *buffer;
  if (datalen < 24)
    return 0; /* Too short - don't bother to copy the buffer.  */

  if (datalen > CMS_BUFFER_SIZE - 1)
    datalen = CMS_BUFFER_SIZE - 1;

  buffer = malloc (datalen + 1);
  if (!buffer)
    return 0; /* Oops */
  memcpy (buffer, data, datalen);
  buffer[datalen] = 0;

  result = detect_cms (buffer, datalen);
  free (buffer);
  return result;
#endif
}

/* New version of is_cms_data which requires a decent version of
   gpgme.  In contrast to is_cms_data this works directly on a gpgme
   data object.  */
int
is_cms_data_ext (gpgme_data_t dh)
{
#ifdef HAVE_GPGME_DATA_IDENTIFY
  switch (gpgme_data_identify (dh, 0))
    {
    case GPGME_DATA_TYPE_CMS_SIGNED:
    case GPGME_DATA_TYPE_CMS_ENCRYPTED:
    case GPGME_DATA_TYPE_CMS_OTHER:
    case GPGME_DATA_TYPE_X509_CERT:
    case GPGME_DATA_TYPE_PKCS12:
      return 1;
    default:
      return 0;
    }
#else
  return 0;
#endif
}
