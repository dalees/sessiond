// sessiond - SSL session cache daemon, file data.cpp
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

const BYTES DATA::find(const BYTES &k) {
    return storage[k].v;
}

const unsigned DATA::count(const BYTES &k) {
    return storage.count(k);
}

const unsigned DATA::size() {
    return storage.size();
}

void DATA::insert(const BYTES &k, const BYTES &v, const unsigned timeout) {
    const time_t t=time(NULL);
    cleanup(t); // purge expired entries
    if(storage.count(k)) // the session is already in cache
        return;
    ITEM i={t+timeout, v};
    storage[k]=i;
    log[t+timeout].insert(k);
}

void DATA::erase(const BYTES &k) {
    if(!storage.count(k)) // the session is not in cache
        return;
    const time_t t=storage[k].t;
    log[t].erase(k);
    if(log[t].empty()) // no more entries for this second
        log.erase(t);
    storage.erase(k);
}

void DATA::cleanup(const time_t t) {
    typedef map<time_t, set<BYTES> >::iterator log_iterator;
    typedef set<BYTES>::iterator set_iterator;

    // erase expired entries
    log_iterator begin=log.begin(), end=log.lower_bound(t);
    for(log_iterator i=begin; i!=end; ++i) // erase expired data entries
        for(set_iterator j=i->second.begin(); j!=i->second.end(); ++j)
            storage.erase(*j);
    log.erase(begin, end); // erase all log entries expiring within the range

    // enforce cache size limit (DoS protection)
    while(storage.size()>100000) {
        log_iterator i=log.begin(); // earliest second
        for(set_iterator j=i->second.begin(); j!=i->second.end(); ++j)
            storage.erase(*j);
        log.erase(i); // erase all log entires expiring within 1 second
    }
}

// end of data.cpp
