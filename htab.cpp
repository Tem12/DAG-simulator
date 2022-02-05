/**
 * @file htab.cpp
 * @brief Module of custom implementation of hashtab with fixed size
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

#include <vector>
#include "htab.h"

HtabItem::HtabItem()
{
    keyContent = { 0, 0 };
    data = 0;
    next = nullptr;
}

Htab::Htab(size_t n)
{
    assert(n > 0);

    items.reserve(n);

    std::vector<HtabItem *> _buckets(n, nullptr);
    buckets = _buckets;

    arrSize = n;
    itemCount = 0;
}

std::shared_ptr<HtabIterator> Htab::begin()
{
    std::shared_ptr<HtabIterator> it(new HtabIterator(this, nullptr, 0));

    for (size_t i = 0; i < this->arrSize; i++) {
        if (this->buckets[i] != nullptr) {
            it->item = this->buckets[i];
            it->idx = i;
            break;
        }
    }

    return it;
}

// Try to find the closest item to specified index by iterating up and down
std::shared_ptr<HtabIterator> Htab::findClosest(size_t index)
{
    std::shared_ptr<HtabIterator> it(new HtabIterator(this, nullptr, 0));

    size_t max = (this->arrSize / 2) + 1;
    size_t down_i = index;
    size_t up_i = index == this->arrSize - 1 ? 0 : index + 1;

    for (int i = 0; i < max; i++) {
        // Test positions on and below index
        if (down_i < 0) {
            down_i = this->arrSize - 1;
        }
        if (this->buckets[down_i] != nullptr) {
            it->item = this->buckets[down_i];
            it->idx = down_i;
            return it;
        } else {
            down_i--;
        }

        // Test positions above index
        if (up_i > this->arrSize - 1) {
            up_i = 0;
        }
        if (this->buckets[up_i] != nullptr) {
            it->item = this->buckets[up_i];
            it->idx = up_i;
            return it;
        } else {
            up_i++;
        }
    }

    // No item was found
    return it;
}

size_t Htab::size()
{
    return this->itemCount;
}

size_t Htab::bucketCount()
{
    return this->arrSize;
}

std::shared_ptr<HtabIterator> Htab::end()
{
    std::shared_ptr<HtabIterator> it(new HtabIterator(this, nullptr, 0));

    for (size_t i = this->arrSize - 1; i > 0; i--) {
        if (this->buckets[i] != nullptr) {
            it->idx = i + 1;
            break;
        }
    }

    return it;
}

void Htab::erase(HtabIterator *it)
{
    if (!it->isValid())
        return;

    // Check if entry is on start of bucket
    std::shared_ptr<HtabIterator> start(new HtabIterator(this, this->buckets[it->idx], it->idx));

    if (it->isEqual(start.get())) {
        // Check if entry is alone in bucket
        if (start->item->next == nullptr) {
            this->buckets[it->idx] = nullptr;
        } else {
            this->buckets[it->idx] = start->item->next;
        }
    } else {
        // Find previous entry in the same bucket
        while (start->item->next != it->item) {
            if (start->item->next == nullptr)
                return;
            start->item = start->item->next;
        }

        // Check if entry has next entry in same bucket
        if (it->item->next != nullptr) {
            start->item->next = it->item->next;
        } else {
            start->item->next = nullptr;
        }
    }

    this->itemCount--;
//    this->items.erase(std::remove(this->items.begin(), this->items.end(), it->item), this->items.end());

    auto itr = std::find_if(this->items.begin(),
                            this->items.end(),
                            [it](auto &element) { return element.get() == it->item;});

    this->items.erase(itr);
}

size_t Htab::hashFun(const std::string &str)
{
    uint32_t h = 0;
    const unsigned char *p;
    for (p = (const unsigned char *)str.c_str(); *p != '\0'; p++)
        h = 65599 * h + *p;
    return h;
}

std::shared_ptr<HtabIterator> Htab::find(const std::string &itemKey)
{
    if (itemKey.empty())
        return this->end();

    // Get index from hash function
    size_t index = (hashFun(itemKey) % this->arrSize);

    HtabItem *next = this->buckets[index];

    // Try to find an existing entry
    if (next != nullptr) {
        do {
            if (next->key == itemKey) {
                std::shared_ptr<HtabIterator> it(new HtabIterator(this, next, index));
                return it;
            } else {
                next = next->next;
            }
        } while (next != nullptr);
    }

    return this->end();
}

std::shared_ptr<HtabIterator> Htab::find(HtabKeyContent key_content)
{
    std::string itemKey = std::to_string(key_content.minerID).append(".").append(std::to_string(key_content.txID));

    // Get index from hash function
    size_t index = (hashFun(itemKey) % this->arrSize);

    HtabItem *next = this->buckets[index];

    // Try to find an existing entry
    if (next != nullptr) {
        do {
            if (next->key == itemKey) {
                std::shared_ptr<HtabIterator> it(new HtabIterator(this, next, index));
                return it;
            } else {
                next = next->next;
            }
        } while (next != nullptr);
    }

    return this->end();
}

void Htab::clear()
{
    this->items.clear();
    std::fill(this->buckets.begin(), this->buckets.end(), nullptr);
    this->itemCount = 0;
}

std::shared_ptr<HtabIterator> Htab::insert(HtabKeyContent key_content)
{
    std::string key = std::to_string(key_content.minerID).append(".").append(std::to_string(key_content.txID));

    // Get index from hash function
    size_t index = (hashFun(key) % this->arrSize);

    std::shared_ptr<HtabIterator> it(new HtabIterator(this, nullptr, index));
    HtabItem *next = this->buckets[index];

    // Try to find an existing entry
    if (next != nullptr) {
        do {
            if (next->key == key) {
                it->item = next;
                return it;
            } else {
                next = next->next;
            }
        } while (next != nullptr);
    }

    // Create new entry
//    auto *htabItem = new HtabItem();
//    std::shared_ptr<HtabItem> htabItem(new HtabItem());
    std::shared_ptr<HtabItem> htabItem = std::make_shared<HtabItem>();
    this->items.push_back(htabItem);

    it->item = htabItem.get();
    if (it->item == nullptr) {
        return this->end();
    }

    // Set item key
    htabItem->key = key;

    // Set item key content
    htabItem->keyContent.txID = key_content.txID;
    htabItem->keyContent.minerID = key_content.minerID;

    // Set item value
    htabItem->data = 0;

    // New entry will be at start of the current bucket (list)
    htabItem->next = this->buckets[index];

    this->buckets[index] = htabItem.get();
    this->itemCount++;

    return it;
}

HtabIterator::HtabIterator(Htab *_t, HtabItem *_item, size_t _idx)
{
    t = _t;
    item = _item;
    idx = _idx;
}

// test: iterator != end
bool HtabIterator::isValid()
{
    return this->item != nullptr;
}

// test: iterator1 == iterator2
bool HtabIterator::isEqual(HtabIterator *it)
{
    return this->t == it->t && this->item == it->item;
}

void HtabIterator::next()
{
    if (!isValid())
        return;

    // Try to get next entry on same index
    if (item->next != nullptr) {
        item = item->next;
        return;
    } else {
        // Else get next entry from array
        if (idx + 1 < t->arrSize) {
            for (size_t i = idx + 1; i < t->arrSize; i++) {
                if (t->buckets[i] != nullptr) {
                    item = t->buckets[i];
                    idx = i;
                    return;
                }
            }
        }
    }

    // End of the hashtable, return non-existing item
    item = nullptr;
    idx = 0;
}

void HtabIterator::setValue(uint32_t value)
{
    assert(isValid());

    item->data = value;
}
