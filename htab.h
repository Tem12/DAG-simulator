/**
* @file htab.h
* @brief Header file for htab.cpp
* @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
* @author Martin Peresini <iperesini@fit.vut.cz>
* @date 2021 - 2022
 */
/*
* Copyright (C) BUT Security@FIT, 2021 - 2022
*
* LICENSE TERMS
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided with the
*    distribution.
* 3. Neither the name of the Company nor the names of its contributors
*    may be used to endorse or promote products derived from this
*    software without specific prior written permission.
*
* ALTERNATIVELY, provided that this notice is retained in full, this
* product may be distributed under the terms of the GNU General Public
* License (GPL) version 2 or later, in which case the provisions
* of the GPL apply INSTEAD OF those given above.
*
* This software is provided ``as is'', and any express or implied
* warranties, including, but not limited to, the implied warranties of
* merchantability and fitness for a particular purpose are disclaimed.
* In no event shall the company or contributors be liable for any
* direct, indirect, incidental, special, exemplary, or consequential
* damages (including, but not limited to, procurement of substitute
* goods or services; loss of use, data, or profits; or business
* interruption) however caused and on any theory of liability, whether
* in contract, strict liability, or tort (including negligence or
* otherwise) arising in any way out of the use of this software, even
* if advised of the possibility of such damage.
*
 */

#ifndef DAG_SIM_HTAB_H
#define DAG_SIM_HTAB_H

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <iostream>

typedef struct htab {
    size_t arr_size;
    size_t size;
    struct htab_item *item[];
} htab_t;

typedef char *htab_key_t;
typedef struct htab_key_content {
    int minerID;
    uint64_t txID;
} htab_key_content_t; // key type
typedef uint32_t htab_value_t; // value type

// item in table
struct htab_item {
    htab_key_t key;
    htab_key_content_t key_content;
    htab_value_t data;
    htab_item *next;
};

// iterator
typedef struct htab_iterator {
    struct htab_item *ptr;
    const htab_t *t;
    size_t idx;
} htab_iterator_t;

size_t htab_hash_fun(htab_key_t str);

htab_t *htab_init(size_t n); // htab constructor
size_t htab_size(const htab_t *t); // number of items in whole table
size_t htab_bucket_count(const htab_t *t); // number of buckets in table

htab_iterator_t htab_find(htab_t *t, htab_key_t key);
htab_iterator_t htab_find(htab_t *t, htab_key_content_t key_content);
htab_iterator_t htab_find_closest(htab_t *t, long index); // find item on index or the closest one by iterating both
                                                          // sides

htab_iterator_t htab_lookup_add(htab_t *t, htab_key_content_t key_content); // try to find and insert

void htab_erase(htab_t *t, htab_iterator_t it); // remove

htab_iterator_t htab_begin(const htab_t *t); // iterator on first item
htab_iterator_t htab_end(const htab_t *t); // iterator right after last item

htab_iterator_t htab_iterator_next(htab_iterator_t it); // iterate to next item

// test: iterator != end
inline bool htab_iterator_valid(htab_iterator_t it)
{
    return it.ptr != nullptr;
}

// test: iterator1 == iterator2
inline bool htab_iterator_equal(htab_iterator_t it1, htab_iterator_t it2)
{
    return it1.ptr == it2.ptr && it1.t == it2.t;
}

// Read and write through iterator
htab_key_t htab_iterator_get_key(htab_iterator_t it);
htab_key_content_t htab_iterator_get_key_content(htab_iterator_t it);
htab_value_t htab_iterator_get_value(htab_iterator_t it);
htab_value_t htab_iterator_set_value(htab_iterator_t it, htab_value_t val);

void htab_clear(htab_t *t); // remove all items from htab
void htab_free(htab_t *t); // htab destructor

#endif // DAG_SIM_HTAB_H
