/* filewatch.c -  The file watch facility
   Copyright (C) 2008 g10 Code GmbH

   This file is part of GPA

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

/* 
   Note that we do not want to use the GIO library because this would
   require DBus; which we try to avoid.  The reason we need this file
   watcher facility at all is to get notified on card reader changes
   by scdaemon.  It might be useful for other things too, though.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_INOTIFY_INIT
# include <sys/inotify.h>
#endif /*HAVE_INOTIFY_INIT*/

#include <glib.h>

#include "gpa.h"

/* The object used to identify a file watch within GPA.  */
struct gpa_filewatch_id_s
{
  gpa_filewatch_id_t next;
  int wd;
  gpa_filewatch_cb_t callback;
  void *callback_data;
  char fname[1];
};


/* The file descriptor used for the inotify queue.  */
static int queue_fd = -1;

/* We need to keep a list of active file watches.  */
static gpa_filewatch_id_t watch_list;

/* We set this flag to true while walking thewatch_list.  */
static int walking_watch_list_p;


/* This function is called by the main event loop if the file watcher
   fd is readable.  This is currently only used under Linux if the
   inotify interface is available. */
#ifdef HAVE_INOTIFY_INIT
static gboolean 
filewatch_cb (GIOChannel *channel, 
              GIOCondition condition, void *data)
{
  gsize nread;
  GIOStatus status;
  GError *err = NULL;
  char buffer[512];

  status = g_io_channel_read_chars (channel, buffer, sizeof buffer,
                                    &nread, &err);
  if (err)
    {
      g_debug ("error reading inotify queue: status=%d, err=%s",
               status, err->message);
      g_error_free (err);
    }
/*   g_debug ("new file watch event, nread=%u", (unsigned int)nread); */
  
  if (status == G_IO_STATUS_NORMAL)
    {
      struct inotify_event *ev;
      char *p = buffer;
      gpa_filewatch_id_t watch;
      char reason[20];
      int  reasonidx;
      
      while (nread >= sizeof *ev) 
        {
          ev = (void *)p;
#define MAKEREASON(a,b) do { if ((ev->mask & (b))                   \
                                  && reasonidx < sizeof reason) \
                                reason[reasonidx++] = (a);      \
                           } while (0)
          reasonidx = 0;
          MAKEREASON ('a', IN_ACCESS);
          MAKEREASON ('c', IN_MODIFY);
          MAKEREASON ('e', IN_ATTRIB);
          MAKEREASON ('w', IN_CLOSE_WRITE);
          MAKEREASON ('0', IN_CLOSE_NOWRITE);
          MAKEREASON ('r', IN_OPEN);
          MAKEREASON ('m', IN_MOVED_FROM);
          MAKEREASON ('y', IN_MOVED_TO);
          MAKEREASON ('n', IN_CREATE);
          MAKEREASON ('d', IN_DELETE);
          MAKEREASON ('D', IN_DELETE_SELF);
          MAKEREASON ('M', IN_MOVE_SELF);
          MAKEREASON ('u', IN_UNMOUNT);
          MAKEREASON ('o', IN_Q_OVERFLOW);
          MAKEREASON ('x', IN_IGNORED);
#undef MAKEREASON
          reason[reasonidx] = 0;
/*           g_debug ("event: wd=%d mask=%#x (%s) cookie=%#x len=%u name=`%.*s'", */
/*                    ev->wd, ev->mask, reason, ev->cookie, ev->len,  */
/*                    (int)ev->len, ev->name); */

          walking_watch_list_p++;
          for (watch=watch_list; watch; watch = watch->next)
            {
              if (ev->wd == watch->wd && watch->callback)
                watch->callback (watch->callback_data, watch->fname, reason);
            }
          walking_watch_list_p--;

          nread -= sizeof *ev;
          nread -= (nread > ev->len)?  ev->len : nread;
          p += sizeof *ev + ev->len;
        }
      
    }

  return TRUE; /* Keep the file watcher fd in the event loop.  */
}
#endif /*HAVE_INOTIFY_INIT*/



/* Initialize the file watcher.  */
void
gpa_init_filewatch (void)
{
#ifdef HAVE_INOTIFY_INIT
  GIOChannel *channel;
  unsigned int source_id;

  if (queue_fd != -1)
    {
      g_debug ("gpa_init_filewatch called twice\n");
      return;
    }

  queue_fd = inotify_init ();
  if (queue_fd == -1)
    {
      g_debug ("this machine does not support the inotify interface"
               " - some features won't work\n");
      return;
    }

  channel = g_io_channel_unix_new (queue_fd);
  if (!channel)
    {
      g_debug ("error creating a new i/o channel\n");
      close (queue_fd);
      queue_fd = -1;
      return;
    }
  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_buffered (channel, FALSE);

  source_id = g_io_add_watch (channel, G_IO_IN, filewatch_cb, NULL);
  if (!source_id)
    {
      g_debug ("error creating watch for I/O channel\n");
      g_io_channel_shutdown (channel, 0, NULL);
      close (queue_fd);
      queue_fd = -1;
      return;
    }
#endif /*HAVE_INOTIFY_INIT*/
}

/* Add a new filewatch for file FILENAME using MASKSTRING.  MASKSTRING
   is a string of letters, describing what events are to be watched
   watched:

        "a"  File was accessed
	"c"  File was modified
	"e"  Metadata changed
	"w"  Writtable file was closed
	"0"  Unwrittable file closed
	"r"  File was opened
	"m"  File was moved from X
	"y"  File was moved to Y
	"n"  Subfile was created
	"d"  Subfile was deleted
	"D"  Self was deleted
	"M"  Self was moved
	"u"  Backing fs was unmounted 
	"o"  Event queued overflowed
	"x"  File is no longer watched

   CALLBACK is the callback function to be called for all matching
   events.  

   The function returns NULL on error or an object used for other
   operations.
*/
gpa_filewatch_id_t
gpa_add_filewatch (const char *filename, const char *maskstring,
                   gpa_filewatch_cb_t callback, void *callback_data)
{
#ifdef HAVE_INOTIFY_INIT
  unsigned int mask = 0;
  int wd;
  gpa_filewatch_id_t handle;

  for (mask=0; *maskstring; maskstring++)
    switch (*maskstring)
      {
      case 'a': mask |= IN_ACCESS; break;
      case 'c': mask |= IN_MODIFY; break;
      case 'e': mask |= IN_ATTRIB; break;
      case 'w': mask |= IN_CLOSE_WRITE; break;
      case '0': mask |= IN_CLOSE_NOWRITE; break;
      case 'r': mask |= IN_OPEN; break;
      case 'm': mask |= IN_MOVED_FROM; break;
      case 'y': mask |= IN_MOVED_TO; break;
      case 'n': mask |= IN_CREATE; break;
      case 'd': mask |= IN_DELETE; break;
      case 'D': mask |= IN_DELETE_SELF; break;
      case 'M': mask |= IN_MOVE_SELF; break;
      case 'u': mask |= IN_UNMOUNT; break;
      case 'o': mask |= IN_Q_OVERFLOW; break;
      case 'x': mask |= IN_IGNORED; break;
      default:
        g_debug ("invalid filewatch mask character '%c'\n", *maskstring);
        return NULL;
      }

  wd = inotify_add_watch (queue_fd, filename, mask);
  if (wd == -1)
    {
      g_debug ("adding watch for `%s' failed: %s", filename, strerror (errno));
      return NULL;
    }

  handle = xcalloc (1, sizeof *handle + strlen (filename));
  strcpy (handle->fname, filename);
  handle->wd = wd;
  handle->callback = callback;
  handle->callback_data = callback_data;
  
  handle->next = watch_list;
  watch_list = handle;

  return handle;

#else /*!HAVE_INOTIFY_INIT*/
  return NULL;
#endif /*!HAVE_INOTIFY_INIT*/  
}
