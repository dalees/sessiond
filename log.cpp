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
#else
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#endif // __WIN32__

#ifdef __WIN32__
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

LOG::LOG() {
#ifdef DAEMONISE
    openlog("sessiond", LOG_CONS, LOG_DAEMON);
#endif
}

LOG::~LOG() {
#ifdef DAEMONISE
    closelog();
#endif
}

void LOG::msg(const int priority, const char *format, ...) {
	char buffer[512];
	buffer[0] = '\0';
	va_list args;
	va_start(args, format);

	// assemble the users message into the buffer
	vsprintf(buffer, format, args);

#ifdef DAEMONISE
	// use syslog if we are daemonised
	syslog(priority, buffer);
#else
	// just use plain printf, otherwise
    printf("%d| %s\n", priority, buffer);
#endif // DAEMONISE

    va_end(args);
}

void LOG::err(const int priority, const char *format, ...) {
	char buffer[512];
	va_list args;
	va_start(args, format);

	// assemble the users error into the buffer
	vsprintf(buffer, format, args);

#ifdef DAEMONISE
    syslog(priority, "%s: error %d: %s", buffer, errno, strerror(errno));
#else
	fprintf(stderr, "%d| %s: error %d: %s\n", priority, buffer, errno, strerror(errno));
#endif

}

#endif // defined __WIN32__

// end of log.cpp
