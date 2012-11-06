// sessiond - SSL session cache daemon, file sessiond.cpp
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

#include "log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#ifdef __WIN32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

// logging is performed every 5 minutes
#define LOG_FREQ 300

void process_request(const int, const u_short, LOG &); // defined in comm.cpp
void my_perror(const char *); // defined in comm.cpp
#ifdef __WIN32__
static void log_thread(void *);
#else
static void signal_handler(int);
#endif
static void send_empty();
static u_short port;
static int s;

int main(int argc, char *argv[]) {
    if(argc<2)
        port=54321;
    else
        port=atoi(argv[1]);
    if(!port) {
        fprintf(stderr, "illegal port number\n");
        return 1;
    }

#ifdef __WIN32__
    // initialize winsock
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2), &wsa)) {
        my_perror("WSAStartup");
        return 1;
    }
#endif

    // create the socket
    s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s==-1) {
        my_perror("socket");
        return 1;
    }

    // bind it to the specified port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(s, (struct sockaddr *)&addr, sizeof addr)==-1) {
        my_perror("bind");
        return 1;
    }

    printf("sessiond " VERSION " started on port %u/UDP\n", port);
    LOG log;
#ifdef __WIN32__
    _beginthread(log_thread, 0, NULL);
#else
    daemon(0, 0);
    signal(SIGUSR1, signal_handler);
    signal(SIGALRM, signal_handler);
    alarm(LOG_FREQ);
    log.msg(LOG_NOTICE, "sessiond version " VERSION " started");
#endif
    for(;;) // the main loop
        process_request(s, port, log);
}


#ifdef __WIN32__

static void log_thread(void *arg) {
    for(;;) {
        Sleep(1000*LOG_FREQ);
        send_empty();
    }
}

#else // defined __WIN32__

static void signal_handler(int sig) {
    // only signal-safe functions are allowed in the signal handler
    alarm(LOG_FREQ);
    send_empty();
}

#endif // defined __WIN32__

static void send_empty() { // send an empty UDP packet
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    sendto(s, "", 0, 0, (struct sockaddr *)&addr, sizeof addr);
}

// end of sessiond.cpp
