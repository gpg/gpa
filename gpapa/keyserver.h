/* hkp-keyserver.h - W32 keyserver access based on WinPT
 *   Copyright (C) 2000, 2001, 2002 Timo Schulz <twoaday@freakmail.de>
 *
 * This file is part of WinPT.
 *
 * WinPT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * WinPT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WinPT; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef HKP_KEYSERVER_H
#define HKP_KEYSERVER_H

#define HKP_PORT 11371

enum
{
  HKPERR_GENERAL = 1,
  HKPERR_RECVKEY = 2,
  HKPERR_SENDKEY = 3,
  HKPERR_TIMEOUT = 4,
  HKPERR_WSOCK_INIT = 5,
  HKPERR_RESOLVE = 6,
  HKPERR_SOCKET = 7,
  HKPERR_CONNECT = 8
};

typedef struct _keyserver_key
{
  int bits;
  char date[16];
  char keyid[16];
  char uid[1024];
} keyserver_key;

int wsock_init(void);
void wsock_end(void);

const char* kserver_strerror(void);
const char* kserver_check_keyid( const char *keyid );
void kserver_update_proxyuser(const char* proxy_user, const char* proxy_pass);
const char* kserver_get_proxy(int *r_port);
int kserver_connect(const char *hostname, int *conn_fd);
int kserver_recvkey( const char *hostname, const char *keyid, char *key,
                     int maxkeylen );
int kserver_sendkey( const char *hostname, const char *pubkey, int len );
int kserver_search_init(const char *hostname, const char *keyid, int *conn_fd);
int kserver_search( int fd, keyserver_key *key );

#endif /*HKP_KEYSERVER_H*/
