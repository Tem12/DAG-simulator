//============================================================================//
// htab.h
// Solution: FIT VUT IJC-DU2 - b)
// Date: 09.04.2020
// Author: Tomáš Hladký at BUT FIT
//============================================================================//

#include "htab.h"

htab_iterator_t htab_begin(const htab_t *t)
{
    htab_iterator_t iterator;
    iterator.t = t;
    iterator.ptr = nullptr;
    iterator.idx = 0;

    if (t == nullptr) {
        return iterator;
    }

    for (size_t i = 0; i < t->arr_size; i++) {
        if (t->item[i] != nullptr) {
            iterator.ptr = t->item[i];
            iterator.idx = i;
            break;
        }
    }

    return iterator;
}

// Try to find the closest item to specified index by iterating up and down
htab_iterator_t htab_find_closest(htab_t *t, long index)
{
    htab_iterator_t iterator;
    iterator.t = t;
    iterator.ptr = nullptr;
    iterator.idx = 0;

    if (t == nullptr) {
        return iterator;
    }

    size_t size = t->arr_size;
    size_t max = (size / 2) + 1;
    long down_i = index;
    long up_i = index == size - 1 ? 0 : index + 1;

    for (int i = 0; i < max; i++) {
        // Test positions on and below index
        if (down_i < 0) {
            down_i = size - 1;
        }
        if (t->item[down_i] != nullptr) {
            iterator.ptr = t->item[down_i];
            iterator.idx = down_i;
            return iterator;
        } else {
            down_i--;
        }

        // Test positions above index
        if (up_i > size - 1) {
            up_i = 0;
        }
        if (t->item[up_i] != nullptr) {
            iterator.ptr = t->item[up_i];
            iterator.idx = up_i;
            return iterator;
        } else {
            up_i++;
        }
    }

    // No item was found
    return iterator;
}

size_t htab_bucket_count(const htab_t *t)
{
    if (t == nullptr)
        return 0;

    return t->arr_size;
}

htab_iterator_t htab_end(const htab_t *t)
{
    htab_iterator_t iterator;
    iterator.t = t;
    iterator.ptr = nullptr;
    iterator.idx = 0;

    if (t == nullptr) {
        return iterator;
    }

    for (size_t i = t->arr_size - 1; i > 0; i--) {
        if (t->item[i] != nullptr) {
            iterator.idx = i + 1;
            break;
        }
    }

    return iterator;
}

void htab_erase(htab_t *t, htab_iterator_t it)
{
    if (t == nullptr)
        return;

    if (!htab_iterator_valid(it))
        return;

    // Check if entry is on start of bucket
    htab_iterator_t start;
    start.t = t;
    start.ptr = t->item[it.idx];
    start.idx = it.idx;

    if (htab_iterator_equal(it, start)) {
        // Check if entry is alone in bucket
        if (start.ptr->next == nullptr) {
            t->item[it.idx] = nullptr;
        } else {
            t->item[it.idx] = start.ptr->next;
        }
    } else {
        // Find previous entry in the same bucket
        while (start.ptr->next != it.ptr) {
            if (start.ptr->next == nullptr)
                return;
            start.ptr = start.ptr->next;
        }

        // Check if entry has next entry in same bucket
        if (it.ptr->next != nullptr) {
            start.ptr->next = it.ptr->next;
        } else {
            start.ptr->next = nullptr;
        }
    }
    t->size--;

    free((char *)it.ptr->key);
    free(it.ptr);
}

htab_iterator_t htab_find(htab_t *t, htab_key_t key)
{
    if (t == nullptr)
        return htab_end(t);

    if (key == nullptr)
        return htab_end(t);

    htab_iterator_t iterator;

    // Get index from hash function
    size_t index = (htab_hash_fun(key) % t->arr_size);

    iterator.t = t;
    iterator.idx = index;

    struct htab_item *next = t->item[index];

    // Try to find an existing entry
    if (next != nullptr) {
        do {
            if (strcmp(next->key, key) == 0) {
                iterator.ptr = next;
                return iterator;
            } else {
                next = next->next;
            }
        } while (next != nullptr);
    }

    iterator = htab_end(t);
    return iterator;
}

htab_iterator_t htab_find(htab_t *t, htab_key_content_t key_content)
{
    if (t == nullptr)
        return htab_end(t);

    htab_key_t key = const_cast<htab_key_t>(
        std::to_string(key_content.minerID).append(".").append(std::to_string(key_content.txID)).c_str());

    if (key == nullptr)
        return htab_end(t);

    htab_iterator_t iterator;

    // Get index from hash function
    size_t index = (htab_hash_fun(key) % t->arr_size);

    iterator.t = t;
    iterator.idx = index;

    struct htab_item *next = t->item[index];

    // Try to find an existing entry
    if (next != nullptr) {
        do {
            if (strcmp(next->key, key) == 0) {
                iterator.ptr = next;
                return iterator;
            } else {
                next = next->next;
            }
        } while (next != nullptr);
    }

    iterator = htab_end(t);
    return iterator;
}

size_t htab_hash_fun(htab_key_t str)
{
    uint32_t h = 0;
    const unsigned char *p;
    for (p = (const unsigned char *)str; *p != '\0'; p++)
        h = 65599 * h + *p;
    return h;
}

htab_t *htab_init(size_t n)
{
    assert(n > 0);

    auto *htab = static_cast<htab_t *>(malloc(sizeof(htab_t) + sizeof(struct htab_item) * n));

    if (htab == nullptr)
        return nullptr;

    memset(htab, 0, sizeof(htab_t) + sizeof(struct htab_item) * n);

    htab->arr_size = n;
    htab->size = 0;

    return htab;
}

void htab_clear(htab_t *t)
{
    if (t == nullptr)
        return;

    struct htab_item *next;
    for (size_t i = 0; i < t->arr_size; ++i) {
        while (t->item[i] != nullptr) {
            free((char *)t->item[i]->key);
            next = t->item[i]->next;
            free(t->item[i]);
            t->item[i] = next;
        }
    }
    t->size = 0;
}

void htab_free(htab_t *t)
{
    htab_clear(t);
    free(t);
}

htab_key_t htab_iterator_get_key(htab_iterator_t it)
{
    assert(htab_iterator_valid(it));

    return it.ptr->key;
}

htab_key_content_t htab_iterator_get_key_content(htab_iterator_t it)
{
    assert(htab_iterator_valid(it));

    return it.ptr->key_content;
}

htab_value_t htab_iterator_get_value(htab_iterator_t it)
{
    assert(htab_iterator_valid(it));

    return it.ptr->data;
}

htab_iterator_t htab_iterator_next(htab_iterator_t it)
{
    htab_iterator_t iterator;
    iterator.t = nullptr;
    iterator.ptr = nullptr;
    iterator.idx = 0;

    if (!htab_iterator_valid(it))
        return iterator;

    iterator.t = it.t;
    iterator.idx = it.idx;

    // Try to get next entry on same index
    if (it.ptr->next != nullptr) {
        iterator.ptr = it.ptr->next;
        return iterator;
    } else {
        // Else get next entry from array
        if (iterator.idx + 1 < iterator.t->arr_size) {
            for (size_t i = iterator.idx + 1; i < iterator.t->arr_size; i++) {
                if (it.t->item[i] != nullptr) {
                    iterator.ptr = it.t->item[i];
                    iterator.idx = i;
                    return iterator;
                }
            }
        }
    }

    iterator = htab_end(it.t);
    return iterator;
}

htab_value_t htab_iterator_set_value(htab_iterator_t it, htab_value_t val)
{
    assert(htab_iterator_valid(it));

    it.ptr->data = val;

    return val;
}

htab_iterator_t htab_lookup_add(htab_t *t, htab_key_content_t key_content)
{
    if (t == nullptr)
        return htab_end(t);

    htab_key_t key = const_cast<htab_key_t>(
        std::to_string(key_content.minerID).append(".").append(std::to_string(key_content.txID)).c_str());

    if (key == nullptr)
        return htab_end(t);

    htab_iterator_t iterator;

    // Get index from hash function
    size_t index = (htab_hash_fun(key) % t->arr_size);

    iterator.t = t;
    iterator.idx = index;

    struct htab_item *next = t->item[index];

    // Try to find an existing entry
    if (next != nullptr) {
        do {
            if (strcmp(next->key, key) == 0) {
                iterator.ptr = next;
                return iterator;
            } else {
                next = next->next;
            }
        } while (next != nullptr);
    }

    // Create new entry
    iterator.ptr = static_cast<htab_item *>(malloc(sizeof(struct htab_item)));

    if (iterator.ptr == nullptr) {
        return htab_end(t);
    }

    iterator.ptr->key = static_cast<htab_key_t>(malloc(strlen(key) + 1));

    if (iterator.ptr->key == nullptr) {
        return htab_end(t);
    }

    // Set item key
    strcpy((char *)iterator.ptr->key, key);

    // Set item key content
    iterator.ptr->key_content.txID = key_content.txID;
    iterator.ptr->key_content.minerID = key_content.minerID;

    // Set item value
    htab_iterator_set_value(iterator, 0);

    // New entry will in start of the current bucket (list)
    iterator.ptr->next = t->item[index];

    t->item[index] = iterator.ptr;
    t->size++;

    return iterator;
}

size_t htab_size(const htab_t *t)
{
    if (t == nullptr)
        return 0;

    return t->size;
}
