// sessiond - SSL session cache daemon, file log.cpp
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

#ifdef __WIN32__

#include <stdio.h>
#include <winsock2.h>

void my_perror(const char *); // defined in comm.cpp

LOG::LOG() {
}

LOG::~LOG() {
}

void LOG::msg(const int priority, const char *txt) {
    printf("%s\n", txt);
}

void LOG::err(const int priority, const char *txt) {
    my_perror(txt);
}

#else // defined __WIN32__

#include <errno.h>
#include <string.h>

LOG::LOG() {
    openlog("sessiond", LOG_CONS, LOG_DAEMON);
}

LOG::~LOG() {
    closelog();
}

void LOG::msg(const int priority, const char *txt) {
    syslog(priority, "%s", txt);
}

void LOG::err(const int priority, const char *txt) {
    syslog(priority, "%s: error %d: %s",
        txt, errno, strerror(errno));
}

#endif // defined __WIN32__

// end of log.cpp
