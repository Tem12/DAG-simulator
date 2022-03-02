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

    std::vector<HtabItem *> _items(n, nullptr);
    items = _items;

    arrSize = n;
    itemCount = 0;
}

Htab::~Htab()
{
    this->clear();
}

std::shared_ptr<HtabIterator> Htab::begin()
{
    std::shared_ptr<HtabIterator> it(new HtabIterator(this, nullptr, 0));

    for (size_t i = 0; i < this->arrSize; i++) {
        if (this->items[i] != nullptr) {
            it->item = this->items[i];
            it->idx = i;
            break;
        }
    }

    return it;
}

// Try to find the closest item to specified index by iterating up and down
std::shared_ptr<HtabIterator> Htab::findRandom(boost::random::mt19937 &rng)
{
    std::uniform_int_distribution<> rand_index_distr(0, (int)this->arrSize - 1);
    size_t index = rand_index_distr(rng);

    std::shared_ptr<HtabIterator> it(new HtabIterator(this, nullptr, 0));

    size_t max = (this->arrSize / 2) + 1;
    size_t down_i = index;
    size_t up_i = index == this->arrSize - 1 ? 0 : index + 1;

    for (int i = 0; i < max; i++) {
        // Test positions on and below index
        if (down_i == UINT64_MAX) {
            down_i = this->arrSize - 1;
        }
        if (this->items[down_i] != nullptr) {
            it->item = this->items[down_i];
            it->idx = down_i;
            break;
        } else {
            down_i--;
        }

        // Test positions above index
        if (up_i > this->arrSize - 1) {
            up_i = 0;
        }
        if (this->items[up_i] != nullptr) {
            it->item = this->items[up_i];
            it->idx = up_i;
            break;
        } else {
            up_i++;
        }
    }

    // If some item was found, select random one in selected bucket
    if (it->item != nullptr) {
        // Count number of items in bucket
        int bucketCount = 0;
        HtabItem *next = it->item;

        while (next != nullptr) {
            bucketCount++;
            next = next->next;
        }

        std::uniform_int_distribution<> bucketSelectionDistr(0, bucketCount - 1);
        int bucketSelectionIndex = bucketSelectionDistr(rng);

        for (int i = 0; i < bucketSelectionIndex; i++) {
            it->next();
        }

//        printf("From size %zu selected index %zu; selected in bucket %d (%d)\n", this->size(), it->idx,
//               bucketSelectionIndex, bucketCount - 1);
    }

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
    return it;
}

void Htab::erase(HtabIterator *it)
{
    if (!it->isValid())
        return;

    // Check if entry is on start of bucket
    std::shared_ptr<HtabIterator> start(new HtabIterator(this, this->items[it->idx], it->idx));

    if (it->isEqual(start.get())) {
        // Check if entry is alone in bucket
        if (start->item->next == nullptr) {
            this->items[it->idx] = nullptr;
        } else {
            this->items[it->idx] = start->item->next;
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
    delete it->item;
}

size_t Htab::hashFun(const std::string &str)
{
    return std::hash<std::string>{}(str);
}

std::shared_ptr<HtabIterator> Htab::find(const std::string &itemKey)
{
    if (itemKey.empty())
        return this->end();

    // Get index from hash function
    size_t index = (hashFun(itemKey) % this->arrSize);

    HtabItem *next = this->items[index];

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

    HtabItem *next = this->items[index];

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
    HtabItem *next;
    for (auto it : this->items) {
        next = it;
        while (next != nullptr) {
            next = it->next;
            delete it;
        }
    }

    std::fill(this->items.begin(), this->items.end(), nullptr);
    this->itemCount = 0;
}

std::shared_ptr<HtabIterator> Htab::insert(HtabKeyContent key_content)
{
    std::string key = std::to_string(key_content.minerID).append(".").append(std::to_string(key_content.txID));
    size_t index = (hashFun(key) % this->arrSize);

    std::shared_ptr<HtabIterator> it(new HtabIterator(this, nullptr, index));
    HtabItem *next = this->items[index];

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
    auto *htabItem = new HtabItem();

    it->item = htabItem;
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
    htabItem->next = this->items[index];

    this->items[index] = htabItem;
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
                if (t->items[i] != nullptr) {
                    item = t->items[i];
                    idx = i;
                    return;
                }
            }
        }
    }
}

void HtabIterator::setValue(uint32_t value)
{
    assert(isValid());

    item->data = value;
}
