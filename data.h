// sessiond - SSL session cache daemon, file data.h
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

// common headers
#include <time.h>

// STL headers
#include <vector>
#include <set>
#include <map>
using namespace std;

// data definitions
typedef vector<unsigned char> BYTES;

// DATA class
class DATA {
    typedef struct {
        time_t t;
        BYTES v;
    } ITEM;
    map<BYTES, ITEM> storage;
    map<time_t, set<BYTES> > log;
public:
    const BYTES find(const BYTES &);
    const unsigned count(const BYTES &);
    const unsigned size();
    void insert(const BYTES &, const BYTES &, const unsigned);
    void erase(const BYTES &);
    void cleanup(const time_t);
};

// end of data.h
