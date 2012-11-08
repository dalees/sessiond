// sessiond - SSL session cache daemon, file comm.cpp
// Copyright (C) 2009 Michal Trojnara <Michal.Trojnara@mirt.net>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, see <http://www.gnu.org/licenses>.
//
// Linking sessiond statically or dynamically with other modules is making
// a combined work based on sessiond. Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.

#include "data.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __WIN32__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef __WIN32__
static const char *winsock_error(); // defined in comm.cpp
#endif

static void mem2bytes(BYTES &dst, const unsigned char *src, const unsigned l);
static void bytes2mem(unsigned char *dst, const BYTES &src);
static void stats(LOG &);

#define CACHE_CMD_NEW     0x00
#define CACHE_CMD_GET     0x01
#define CACHE_CMD_REMOVE  0x02
#define CACHE_RESP_ERR    0x80
#define CACHE_RESP_OK     0x81

#define KEY_LEN 32
#define MAX_VAL_LEN 512
typedef struct {
    u_char version, type;
    u_short timeout;
    u_char key[KEY_LEN];
    u_char val[MAX_VAL_LEN];
} CACHE_PACKET;
static DATA data;
static unsigned long long delta_hits=0, delta_misses=0, delta_trans=0;

void process_request(const int s, const unsigned short port, LOG &log) {
    CACHE_PACKET packet;
    struct sockaddr addr;
    socklen_t addrlen=sizeof addr;
    //log.msg(LOG_DEBUG, "waiting for packet");
    ssize_t len=recvfrom(s, (char *)&packet, sizeof packet, 0, &addr, &addrlen);
    //log.msg(LOG_DEBUG, "Recieved packet");
    if(len==-1) {
        log.err(LOG_ERR, "recvfrom");
#ifdef __WIN32__
        Sleep(1000); // limit the error rate
#else
        sleep(1); // limit the error rate
#endif
        return;
    }
    const sockaddr_in *in_addr=(sockaddr_in *)&addr;
    // check for logging packet
    if( len == 0 &&
            in_addr->sin_family==AF_INET &&
            in_addr->sin_port==htons(port) &&
            in_addr->sin_addr.s_addr==htonl(INADDR_LOOPBACK) ) {
        stats(log);
        return;
    }
    if(len<(int)(sizeof(packet)-MAX_VAL_LEN) || packet.version != 1) {
        log.msg(LOG_ERR, "Malformed packet received from %s", inet_ntoa(in_addr->sin_addr));
        return;
    }
    ++delta_trans;
	BYTES k(KEY_LEN);
    mem2bytes(k, packet.key, KEY_LEN);
    if(packet.type==CACHE_CMD_NEW) {
        BYTES v;
		mem2bytes(v, packet.val, len-(sizeof packet-MAX_VAL_LEN));
        data.insert(k, v, ntohs(packet.timeout));
        //log.msg(LOG_DEBUG, "Added new value for key '%s'", packet.key);
    } else if(packet.type==CACHE_CMD_GET) {
        //log.msg(LOG_DEBUG, "Recieved GET packet.");
        len=sizeof(packet)-(sizeof(u_char) * MAX_VAL_LEN);
		BYTES v;
        if(data.find(k, v)) {
            ++delta_hits;
            bytes2mem(packet.val, v);
            len+=v.size();
            packet.type=CACHE_RESP_OK;
        } else {
            ++delta_misses;
            packet.type=CACHE_RESP_ERR;
        }
        //log.msg(LOG_DEBUG, "Replying to GET packet for '%s' with '%s'. Packet size %d.", packet.key, packet.val, len);
        if(sendto(s, (char *)&packet, len, 0, &addr, addrlen)==-1)
            log.err(LOG_ERR, "Sendto failed to send packet to %s", inet_ntoa(in_addr->sin_addr));
        //else
            //log.msg(LOG_DEBUG, "Sent packet");
    } else if(packet.type==CACHE_CMD_REMOVE) {
        data.erase(k);
        //log.msg(LOG_DEBUG, "Removed key '%s'", packet.key);
    } else {
        //log.msg(LOG_ERR, "Incorrect packet type");
        --delta_trans;
    }
}

static void mem2bytes(BYTES &dst, const unsigned char *src, const unsigned l) {
    dst.clear();
    for(unsigned int i=0; i<l; ++i)
    {
        dst.push_back(src[i]);
    }
}

static void bytes2mem(unsigned char *dst, const BYTES &src) {
    const unsigned length=src.size();
    for(unsigned i=0; i<length; ++i)
    {
        dst[i]=src[i];
    }
}

static unsigned long long total_hits=0, total_misses=0, total_trans=0;
static time_t start_time=time(NULL); // initialized at startup
static time_t prev_time=start_time;

static void stats(LOG &log) {
    total_hits+=delta_hits;
    total_misses+=delta_misses;
    total_trans+=delta_trans;

    const time_t now=time(NULL);
    const time_t start_diff=now>start_time ? now-start_time : 1;
    const time_t prev_diff=now>prev_time ? now-prev_time : 1;
    const unsigned long long delta_get=delta_hits+delta_misses;
    const unsigned long long total_get=total_hits+total_misses;

    data.cleanup(now);

    char stats_txt[256];
    snprintf(stats_txt, sizeof stats_txt,
        "cache entries=%u, transactions=%llu/%llu, "
        "tps=%.2f/%.2f, hit ratio=%2.2f%%/%2.2f%%",
        data.size(),
        total_trans, delta_trans,
        1.0*total_trans/start_diff, 1.0*delta_trans/prev_diff,
        total_get>0 ? 100.0*total_hits/total_get : 0.0,
        delta_get>0 ? 100.0*delta_hits/delta_get : 0.0);
    log.msg(LOG_INFO, stats_txt); // log statistics

    delta_hits=delta_misses=delta_trans=0L;
    prev_time=now;
}

void my_perror(const char *txt) {
#ifdef __WIN32__
    fprintf(stderr, "%s: error %d: %s\n",
        txt, WSAGetLastError(), winsock_error());
#else
    fprintf(stderr, "%s: error %d: %s\n",
        txt, errno, strerror(errno));
#endif
}

#ifdef __WIN32__

static const char *winsock_error() {
    switch(WSAGetLastError()) {
    case 10004:
        return "Interrupted system call (WSAEINTR)";
    case 10009:
        return "Bad file number (WSAEBADF)";
    case 10013:
        return "Permission denied (WSAEACCES)";
    case 10014:
        return "Bad address (WSAEFAULT)";
    case 10022:
        return "Invalid argument (WSAEINVAL)";
    case 10024:
        return "Too many open files (WSAEMFILE)";
    case 10035:
        return "Operation would block (WSAEWOULDBLOCK)";
    case 10036:
        return "Operation now in progress (WSAEINPROGRESS)";
    case 10037:
        return "Operation already in progress (WSAEALREADY)";
    case 10038:
        return "Socket operation on non-socket (WSAENOTSOCK)";
    case 10039:
        return "Destination address required (WSAEDESTADDRREQ)";
    case 10040:
        return "Message too long (WSAEMSGSIZE)";
    case 10041:
        return "Protocol wrong type for socket (WSAEPROTOTYPE)";
    case 10042:
        return "Bad protocol option (WSAENOPROTOOPT)";
    case 10043:
        return "Protocol not supported (WSAEPROTONOSUPPORT)";
    case 10044:
        return "Socket type not supported (WSAESOCKTNOSUPPORT)";
    case 10045:
        return "Operation not supported on socket (WSAEOPNOTSUPP)";
    case 10046:
        return "Protocol family not supported (WSAEPFNOSUPPORT)";
    case 10047:
        return "Address family not supported by protocol family (WSAEAFNOSUPPORT)";
    case 10048:
        return "Address already in use (WSAEADDRINUSE)";
    case 10049:
        return "Can't assign requested address (WSAEADDRNOTAVAIL)";
    case 10050:
        return "Network is down (WSAENETDOWN)";
    case 10051:
        return "Network is unreachable (WSAENETUNREACH)";
    case 10052:
        return "Net dropped connection or reset (WSAENETRESET)";
    case 10053:
        return "Software caused connection abort (WSAECONNABORTED)";
    case 10054:
        return "Connection reset by peer (WSAECONNRESET)";
    case 10055:
        return "No buffer space available (WSAENOBUFS)";
    case 10056:
        return "Socket is already connected (WSAEISCONN)";
    case 10057:
        return "Socket is not connected (WSAENOTCONN)";
    case 10058:
        return "Can't send after socket shutdown (WSAESHUTDOWN)";
    case 10059:
        return "Too many references, can't splice (WSAETOOMANYREFS)";
    case 10060:
        return "Connection timed out (WSAETIMEDOUT)";
    case 10061:
        return "Connection refused (WSAECONNREFUSED)";
    case 10062:
        return "Too many levels of symbolic links (WSAELOOP)";
    case 10063:
        return "File name too long (WSAENAMETOOLONG)";
    case 10064:
        return "Host is down (WSAEHOSTDOWN)";
    case 10065:
        return "No Route to Host (WSAEHOSTUNREACH)";
    case 10066:
        return "Directory not empty (WSAENOTEMPTY)";
    case 10067:
        return "Too many processes (WSAEPROCLIM)";
    case 10068:
        return "Too many users (WSAEUSERS)";
    case 10069:
        return "Disc Quota Exceeded (WSAEDQUOT)";
    case 10070:
        return "Stale NFS file handle (WSAESTALE)";
    case 10091:
        return "Network SubSystem is unavailable (WSASYSNOTREADY)";
    case 10092:
        return "WINSOCK DLL Version out of range (WSAVERNOTSUPPORTED)";
    case 10093:
        return "Successful WSASTARTUP not yet performed (WSANOTINITIALISED)";
    case 10071:
        return "Too many levels of remote in path (WSAEREMOTE)";
    case 11001:
        return "Host not found (WSAHOST_NOT_FOUND)";
    case 11002:
        return "Non-Authoritative Host not found (WSATRY_AGAIN)";
    case 11003:
        return "Non-Recoverable errors: FORMERR, REFUSED, NOTIMP (WSANO_RECOVERY)";
    case 11004:
        return "Valid name, no data record of requested type (WSANO_DATA)";
    default:
        return "Unknown error";
    }
}

#endif

// end of comm.cpp
