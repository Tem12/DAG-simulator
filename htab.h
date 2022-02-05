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

struct HtabKeyContent {
    uint32_t minerID;
    uint64_t txID;
};
class HtabItem;
class HtabIterator;
class Htab {
  public:
    std::vector<std::shared_ptr<HtabItem>> items;
    std::vector<HtabItem *> buckets;
    size_t itemCount;
    size_t arrSize;

    Htab(size_t n);
    std::shared_ptr<HtabIterator> begin();
    std::shared_ptr<HtabIterator> findClosest(size_t index);
    size_t size();
    size_t bucketCount();
    std::shared_ptr<HtabIterator> end();
    void erase(HtabIterator *it);
    size_t hashFun(const std::string &str);
    std::shared_ptr<HtabIterator> find(const std::string &itemKey);
    std::shared_ptr<HtabIterator> find(HtabKeyContent key_content);
    void clear();
    std::shared_ptr<HtabIterator> insert(HtabKeyContent key_content);
};
class HtabItem {
  public:
    std::string key;
    HtabKeyContent keyContent;
    uint32_t data;
    HtabItem *next;

    HtabItem();
};
class HtabIterator {
  public:
    Htab *t;
    HtabItem *item;
    size_t idx;

    HtabIterator(Htab *_t, HtabItem *_item, size_t _idx);
    bool isValid();
    bool isEqual(HtabIterator *it);
    void next();
    void setValue(uint32_t value);
};

#endif // DAG_SIM_HTAB_H
