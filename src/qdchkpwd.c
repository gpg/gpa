/* QDCHKPWD - Quick and Dirty CHecK for PassWorDs
 *
 * Copyright (C) 2002 G-N-U GmbH - http://www.g-n-u.de
 *
 * QDCHKPWD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * QDCHKPWD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QDCHKPWD; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* Define MAIN if you want to generate a program instead of a library. */

#include <config.h>

#ifdef MAIN
#include <stdio.h>
#endif
#include <math.h>
#include <zlib.h>
#include <glib.h>
#include <string.h>
#include "qdchkpwd.h"

static char *test_str
  = "Dies ist ein streng geheimes Passwort oder Passwortsatz. "
    "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    "!\"§$%&/()=?`QWERTZUIOPÜASDFGHJKLÖÄYXCVBNM;:_"
    "qwertzuiopüasdfghjklöäyxcvbnm,.-"
    "This is a top secret password or passphrase.";

double
qdchkpwd (const char *pwd)
{
  int i, l;
  unsigned hit_num[256];
  unsigned hit_card = 0;
  char *source_buffer, *dest_buffer;
  unsigned long source_length, dest_length, test_comp_length;
  double S, M;

  if (pwd == NULL)
    return 0.0;
  for (i = 0; i < 256; i++)
    hit_num[i] = 0;
  l = strlen (pwd);
  for (i = 0; i < l; i++)
    {
      int c = pwd[i];
      if (hit_num[c] == 0)
        hit_card++;
      hit_num[c]++;
    }
  S = 0.0;
  M = (double) l / (double) hit_card;
  for (i = 0; i < 256; i++)
    S += (hit_num[i] - M) * (hit_num[i] - M);
  S = sqrt (sqrt (S));

  source_buffer = g_strconcat (test_str, test_str, NULL);
  source_length = strlen (source_buffer);
  dest_length = 2 * source_length + 256;
  dest_buffer = g_malloc (dest_length);
  if (compress ((unsigned char *) dest_buffer, &dest_length,
		(unsigned char *) source_buffer, source_length) != 0)
    return -1.0;
  test_comp_length = dest_length;
  g_free (source_buffer);
  g_free (dest_buffer);

  source_buffer = g_strconcat (test_str, pwd, test_str, NULL);
  source_length = strlen (source_buffer);
  dest_length = 2 * source_length + 256;
  dest_buffer = g_malloc (dest_length);
  if (compress ((unsigned char *) dest_buffer, &dest_length,
		(unsigned char *) source_buffer, source_length) != 0)
    return -1.0;
  g_free (source_buffer);
  g_free (dest_buffer);

  return (double) (dest_length - test_comp_length) / (5.0 * S);
}

#ifdef MAIN

int main (int argc, char *argv[])
{
  if (argc < 2)
    printf ("Usage: %s \"<passphrase>\"\n", argv[0]);
  else
    printf ("%0.3f\n", qdchkpwd (argv[1]));
}

#endif /* MAIN */
