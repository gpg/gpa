/* w32-clipboard.c - W32 API clipboard functions
 *	Copyright (C) 2001, 2002 Timo Schulz
 *
 * This file is part of MyGPGME.
 *
 * MyGPGME is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MyGPGME is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#if defined(__MINGW32__) || defined(HAVE_DOSISH_SYSTEM)
#include <windows.h>

#include "util.h"
#include "ops.h"
#include "context.h"

static char*
get_w32_clip_text(int *r_size)
{
  int rc, size;
  char *private_clip = NULL;
  char *clip = NULL;
  HANDLE cb;

  rc = OpenClipboard(NULL);
  if (!rc)
    return NULL;
  cb = GetClipboardData(CF_TEXT);
  if (!cb)
    goto leave;

  private_clip = GlobalLock(cb);
  if (!private_clip)
    goto leave;
  size = strlen(private_clip);

  clip = xtrymalloc(size + 1);
  if (!clip)
    {
      GlobalUnlock(cb);
      goto leave;
    }	
  memcpy(clip, private_clip, size);
  clip[size] = '\0';
  *r_size = size;
  GlobalUnlock(cb);

leave:
  CloseClipboard();
  return clip;
} /* get_w32_clip_text */

static int
set_w32_clip_text(const char *data, int size)
{
  int rc;
  HANDLE cb;
  char *private_data;

  rc = OpenClipboard( NULL );
  if (!rc)
    return 1;
  EmptyClipboard();
  cb = GlobalAlloc(GHND, size+1);
  if (!cb)
    goto leave;

  private_data = GlobalLock(cb);
  if (!private_data)
    goto leave;
  memcpy(private_data, data, size);
  private_data[size] = '\0';
  SetClipboardData(CF_TEXT, cb);
  GlobalUnlock(cb);

leave:
  CloseClipboard();
  return 0;
} /* set_w32_clip_text */

GpgmeError
gpgme_data_new_from_clipboard(GpgmeData *r_dh, int copy)
{
  GpgmeError err = 0;
  GpgmeData dh;
  char *clip_text;
  int size;

  clip_text = get_w32_clip_text(&size);
  if (!clip_text)
    return mk_error(No_Data);

  err = gpgme_data_new_from_mem(&dh, clip_text, size, copy);

  xfree(clip_text);
  *r_dh = dh;
  return err;
} /* gpgme_data_new_from_clipboard */

void
gpgme_data_release_and_set_clipboard( GpgmeData dh )
{	
  char *clip_text;
  int size;

  if (dh)
    {
      clip_text = _gpgme_data_get_as_string(dh);
      if (clip_text && (size = strlen(clip_text)))
        {
          set_w32_clip_text(clip_text, size);
          xfree(clip_text);
        }
      gpgme_data_release(dh);
    }
} /* gpgme_data_release_and_set_clipboard */
#endif /*__MINGW32__*/










