/* keyserver.c - W32 keyserver access based on WinPT
 *   Copyright (C) 2000, 2001, 2002 Timo Schulz <twoaday@freakmail.de>
 *   Copyright (C) 2001 Marco Cunha <marco.cunha@ignocorp.com>
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

#if defined(__MINGW32__) || defined(HAVE_DOSISH_SYSTEM)
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "keyserver.h"
#include "xmalloc.h"
#ifdef xfree
#undef xfree
#endif
#define xfree(foo) free (foo)

static char base64code[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char hkp_errmsg[1024];
static int hkp_err = 0;
static char hkp_proxy[256];
static char hkp_proxy_base64user[256];
static int hkp_use_proxy_user = 0;
static int hkp_proxy_port = 0;
static int hkp_use_proxy = 0;

static void
base64_encode(const char *inputstr, char *outputstr, int maxlen)
{
  int index = 0, temp = 0, res = 0, i = 0, inputlen = 0, len = 0;
	
  inputlen = strlen(inputstr);

  /* fixme: check if len > maxlen */
  for (i = 0; i <inputlen; i++)
    {
      res = temp;
      res = (res << 8) | (inputstr[i] & 0xFF);
      switch (index++) {
      case 0: outputstr[len++] = base64code[res >> 2 & 0x3F]; res &= 0x3;
        break;
      case 1:	outputstr[len++] = base64code[res >> 4 & 0x3F];	res &= 0xF;
        break;
      case 2:	outputstr[len++] = base64code[res >> 6 & 0x3F];
        outputstr[len++] = base64code[res & 0x3F];
        res = index = 0;
        break;
      }
      temp = res;
	}

  switch (index)
    {
    case 0: break;
    case 2: outputstr[len++] = base64code[temp << 2 & 0x3F];
      outputstr[len++] = '=';
      break;
    case 1: outputstr[len++] = base64code[temp << 4 & 0x3F];
      outputstr[len++] = '=';
      outputstr[len++] = '=';
    }

  outputstr[len] = '\0';
} /* base64_encode */

static int
check_hkp_response(const char *resp, int recv)
{
  int ec;
  char *p;
	
  ec = recv ? HKPERR_RECVKEY: HKPERR_SENDKEY;

  if (!strstr( resp, "HTTP/1.0 200 OK" )) /* http error */
    return ec;

  if ( strstr( resp, "<title>Public Key Server -- Error</title>" )
       || strstr( resp, "<h1>Public Key Server -- Error</h1>" )
       || strstr( resp, "No matching keys in database" ) )
    {
      p = strstr(resp, "<p>");
      if (p)
        {
          strcpy(hkp_errmsg, p);
          hkp_err = 1;
        }
      return ec;
    }

  return 0;
} /* check_hkp_response */

#if 0  /* not used */

static int
sock_getline( int fd, char *buf, int buflen, int *nbytes )
{
  char ch;
  int count = 0;
	
  *nbytes = 0;
  *buf = 0;

  while ( recv(fd, &ch, 1, 0) > 0 )
    {
      *buf++ = ch;
      count++;	
      if ( ch == '\n' || count == (buflen-1) )
        {
          *buf = 0;
          *nbytes = count;
          return 0;
		}
	}
		
  return 1;
} /* sock_getline */

#endif /* not used */

static int
sock_select(int fd)
{
  FD_SET rfd;
  struct timeval tv = {1, 0};

  FD_ZERO(&rfd);
  FD_SET(fd, &rfd);

  if (select(fd+1, &rfd, NULL, NULL, &tv) == SOCKET_ERROR)
    return SOCKET_ERROR;

  if (FD_ISSET(fd, &rfd))
    return 1;

  return 0;
} /* sock_select */

static int
sock_read(int fd, char *buf, int buflen, int *nbytes)
{	
  DWORD nread;
  int nleft = buflen;
  int rc, n = 0;

  while (nleft > 0)
    {
      if (n >= 10)
        return HKPERR_TIMEOUT;
      if ( (rc = sock_select(fd)) == SOCKET_ERROR )
        return HKPERR_SOCKET;      
      else if (!rc)
        n++;
      else
        {
          nread = recv(fd, buf, nleft, 0);
          if (nread == SOCKET_ERROR)
            return SOCKET_ERROR;
          else if (!nread)
            break;
          nleft -= nread;
          buf += nread;
        }
	}
  if (nbytes)
    *nbytes = buflen - nleft;

  return 0;
} /* sock_read */

static int
sock_write(int fd, const char *buf, int buflen)
{
  DWORD nwritten;
  int nleft = buflen;

  while (nleft > 0)
    {
      nwritten = send (fd, buf, nleft, 0);
      if (nwritten == SOCKET_ERROR)
        return SOCKET_ERROR;
      nleft -= nwritten;
      buf += nwritten;
	}

  return 0;
} /* sock_write */

/*
 * Initialize the Winsock2 interface.
 */
int
wsock_init(void)
{
  WSADATA wsa;
	
  if (WSAStartup( 0x202, &wsa ))
    return HKPERR_WSOCK_INIT;
  return 0;
} /* wsock_init */

/*
 * Should be called after the keyserver access.
 */
void
wsock_end(void)
{
  WSACleanup( );
} /* wsock_end */

const char*
kserver_strerror(void)
{
  if (hkp_err)
    return hkp_errmsg;
  return NULL;
} /* kserver_strerror */

/*
 * If the request contains the keyid, it have to be
 * always postfix with '0x'+keyid. This code checks
 * if the keyid is a decimal value and if so if it contains
 * the '0x'. If not it is inserted.
 */
const char*
kserver_check_keyid( const char *keyid )
{
  static char id[20];

  if ( strstr( keyid, "@" ) )
    return keyid; /* email address */
  if ( strncmp( keyid, "0x", 2 ) )
    {
      memset( &id, 0, sizeof(id) );
      _snprintf( id, sizeof(id)-1, "0x%s", keyid );
      return id;
	}

  return keyid;
} /* kserver_check_keyid */

void
kserver_update_proxyuser(const char * proxy_user, const char * proxy_pass)
{
  char t[257]; /* user:pass = 127+1+127+1 = 257 */

  _snprintf(t, sizeof(t)-1, "%s:%s", proxy_user, proxy_pass);
  base64_encode(t, hkp_proxy_base64user, 257);
  hkp_use_proxy_user = 1;
} /* kserver_update_proxyuser */

const char*
kserver_get_proxy(int *r_port)
{
  if (hkp_use_proxy)
    {
      *r_port = hkp_proxy_port;
      return hkp_proxy;
	}
  return NULL;
} /* kserver_get_proxy */

/*
 * Connect to the keyserver 'hostname'.
 * We always use the HKP port.
 */
int
kserver_connect(const char *hostname, int *conn_fd)
{
  int rc, fd;
  DWORD iaddr;
  WORD port = 11371;
  char host[128];
  struct hostent *hp;
  struct sockaddr_in sock;

  memset( &sock, 0, sizeof(sock) );
  sock.sin_family = AF_INET;
  if (hkp_use_proxy)
    sock.sin_port = htons(hkp_proxy_port);
  else
    sock.sin_port = htons(port);

  if (hkp_use_proxy)
    strcpy(host, hkp_proxy);
  else
    strcpy(host, hostname);
	
  if ( (iaddr = inet_addr(host)) != SOCKET_ERROR)
    memcpy(&sock.sin_addr, &iaddr, sizeof(iaddr));
  else if ( (hp = gethostbyname(host)) )
    {
      if (hp->h_addrtype != AF_INET)
        return HKPERR_RESOLVE;
      else if (hp->h_length != 4)
        return HKPERR_RESOLVE;
      memcpy(&sock.sin_addr, hp->h_addr, hp->h_length);
    }
  else {
    *conn_fd = 0;
    return HKPERR_RESOLVE;
  }
	
  fd = socket( AF_INET, SOCK_STREAM, 0 );
  if (fd == INVALID_SOCKET)
    {
      *conn_fd = 0;
      return HKPERR_SOCKET;
    }

  rc = connect( fd, (struct sockaddr *) &sock, sizeof(sock) );
  if (rc == SOCKET_ERROR)
    {
      closesocket(fd);
      *conn_fd = 0;
      return HKPERR_CONNECT;
    }

  *conn_fd = fd;
  WSASetLastError(0);

  return 0;
} /* ks_connect */

static char*
kserver_urlencode(const char *pubkey, int octets, int *newlen)
{
  char *p, numbuf[5];	
  int i, j, size;

  p = xmalloc(2*octets);
  if (!p)
    return NULL;

  for (size=0, i=0; i<octets; i++) {
    if (isalnum(pubkey[i]) || pubkey[i] == '-')
      {
        p[size] = pubkey[i];
        size++;
      }
    else if (pubkey[i] == ' ')
      {
        p[size] = '+';
        size++;
      }
    else
      {
        sprintf(numbuf, "%%%02X", pubkey[i]);			
        for (j=0; j<strlen(numbuf); j++)
          {
            p[size] = numbuf[j];
            size++;
          }
      }
  }
  p[size] = '\0';

  if (newlen)
    *newlen = size;

  return p;
} /* kserver_urlencode */

/*
 * Format a request for the given keyid (send).
 */
static char*
kserver_send_request( const char *hostname, const char *pubkey, int octets )
{
  char *request, *enc_pubkey;
  int reqlen, enc_octets;

  reqlen = 512 + strlen(hostname) + 2*strlen(pubkey);
  request = xmalloc(reqlen);
  if (!request)
    return NULL;
  
  enc_pubkey = kserver_urlencode(pubkey, octets, &enc_octets);
  if (!enc_pubkey || !enc_octets)
    {
      xfree(request);
      return NULL;
	}

  if (hkp_use_proxy_user)
    {
      _snprintf( request, reqlen-1,
                 "POST http://%s:%d/pks/add HTTP/1.0\r\n"
                 "Referer: \r\n"
                 "User-Agent: WinPT/W32\r\n"
                 "Host: %s:%d\r\n"
                 "Proxy-Authorization: Basic %s\r\n"
                 "Content-type: application/x-www-form-urlencoded\r\n"
                 "Content-length: %d\r\n"
                 "\r\n"
                 "keytext=%s"
                 "\n",
                 hostname, 11371, hostname, 11371, hkp_proxy_base64user,
                 enc_octets+9, enc_pubkey);
    }
  else
    {
      _snprintf( request, reqlen-1,
                 "POST /pks/add HTTP/1.0\r\n"
                 "Referer: \r\n"
                 "User-Agent: WinPT/W32\r\n"
                 "Host: %s:%d\r\n"
                 "Content-type: application/x-www-form-urlencoded\r\n"
                 "Content-length: %d\r\n"
                 "\r\n"
                 "keytext=%s"
                 "\n",
                 hostname, 11371, enc_octets+9, enc_pubkey);
    }
  xfree(enc_pubkey);

  return request;
} /* kserver_send_request */

/*
 * Interface receiving a public key.
 */
int
kserver_recvkey( const char *hostname, const char *keyid, char *key, 
				 int maxkeylen )
{	
  int rc, n;
  int conn_fd;
  char *request = NULL;

  hkp_err = 0; /* reset */
	
  rc = kserver_connect( hostname, &conn_fd );
  if (rc)
    goto leave;
		
  request = xmalloc(256+1);
  if (!request)
    {
      rc = HKPERR_GENERAL;
      goto leave;
    }

  if (hkp_use_proxy)
    {
      if (hkp_use_proxy_user)
        {
          _snprintf(request, 255,
                    "GET http://%s:%d/pks/lookup?op=get&search=%s "
                    "HTTP/1.0\r\nProxy-Authorization: Basic %s\r\n\r\n",
                    hostname, 11371, keyid, hkp_proxy_base64user);
        }
      else
        {
          _snprintf(request, 255, 
                    "GET http://%s:%d/pks/lookup?op=get&search=%s "
                    "HTTP/1.0\r\n\r\n",
                    hostname, 11371, keyid);
        }
    }
  else
    {
      _snprintf(request, 255,
                "GET /pks/lookup?op=get&search=%s HTTP/1.0\r\n\r\n",
                keyid);
    }


  rc = sock_write( conn_fd, request, lstrlen(request) );
  if ( rc == SOCKET_ERROR )
    {
      rc = HKPERR_RECVKEY;
      goto leave;
    }
	
  rc = sock_read( conn_fd, key, maxkeylen, &n );
  if ( rc == SOCKET_ERROR )
    {
      rc = HKPERR_RECVKEY;
      goto leave;
    }

  rc = check_hkp_response( key, 1 );
  if (rc)
    goto leave;

  WSASetLastError(0);

leave:
  closesocket(conn_fd);
  xfree(request);
  return rc;
} /* kserver_recvkey */

/* 
 * Interface to sending a public key.
 */
int
kserver_sendkey( const char *hostname, const char *pubkey, int len )
{
  int n, rc;
  int conn_fd;
  char *request = NULL;
  char log[2048];

  hkp_err = 0; /* reset */
		
  rc = kserver_connect( hostname, &conn_fd );
  if (rc)
    goto leave;	

  if ( !(request = kserver_send_request( hostname, pubkey, len )) )
    {
      rc = HKPERR_GENERAL;      
      goto leave;
    }

  rc = sock_write( conn_fd, request, lstrlen(request) );
  if ( rc == SOCKET_ERROR )
    {
      rc = HKPERR_SENDKEY;
      goto leave;
	}

  rc = sock_read( conn_fd, log, sizeof(log), &n );
  if ( rc == SOCKET_ERROR )
    {
      rc = HKPERR_SENDKEY;
      goto leave;
    }

  rc = check_hkp_response(log, 0);
  if (rc)
    goto leave;
	
  WSASetLastError(0);

leave:
  closesocket(conn_fd);
  xfree(request);  
  return rc;
} /* kserver_sendkey */

#endif /*__MINGW32__*/


