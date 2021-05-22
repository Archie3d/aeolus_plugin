// ----------------------------------------------------------------------------
//
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#pragma once

#include "aeolus/globals.h"

AEOLUS_NAMESPACE_BEGIN

template <class Item> struct ListItem;
template <class Item> struct List;

template <typename Item>
struct ListNode
{
private:
    Item* _prev = nullptr;
    Item* _next = nullptr;

    template <typename I> friend struct ListItem;
    template <typename I> friend struct List;
};

template <class Item>
struct ListItem : public ListNode<ListItem<Item>>
{
    void appendAfter (ListItem* item) noexcept
    {
        jassert (item != nullptr);

        this->_prev = item;
        this->_next = item->_next;
        item->_next = this;
    }

    void remove() noexcept
    {
        if (this->_prev != nullptr)
            this->_prev->_next = this->_next;

        if (this->_next != nullptr)
            this->_next->_prev = this->_prev;

        this->_prev = nullptr;
        this->_next = nullptr;
    }

    Item* next() noexcept { return static_cast<Item*> (this->_next); }
    Item* prev() noexcept { return static_cast<Item*> (this->_prev); }
};

template <class Item>
struct List
{
    Item* first() noexcept { return _head; }
    Item* last() noexcept { return _tail; }

    void append (Item* item) noexcept
    {
        jassert (item != nullptr);

        if (_head == nullptr)
        {
            _head = item;
            _tail = item;
        }
        else
        {
            item->appendAfter (_tail);
            _tail = item;
        }
    }

    void prepend (Item* item) noexcept
    {
        jassert (item != nullptr);

        item->_next = _head;
        _head = item;

        if (_tail == nullptr)
            _tail = item;
    }

    void remove (Item* item) noexcept
    {
        jassert (item != nullptr);

        if (_head == item)
            _head = item->next();

        if (_tail == item)
            _tail = item->prev();

        item->remove();
    }

    bool contains (Item* item) const noexcept
    {
        auto* i = _head;

        while (i != nullptr)
        {
            if (i == item)
                return true;

            i = i->next();
        }

        return false;
    }

    Item* removeAndReturnNext (Item* item) noexcept
    {
        Item* n = item->next();
        remove (item);
        return n;
    }

    bool isEmpty() const noexcept
    {
        return _head == nullptr;
    }

    Item* operator[] (int index) const
    {
        Item* it = _head;

        if (index >= 0)
        {
            while (index > 0 && it != nullptr)
            {
                --index;
                it = it->next();
            }
        }
        else
        {
            it = _tail;
            ++index;

            while (index < 0 && it != nullptr)
            {
                ++index;
                it = it->prev();
            }
        }

        return it;
    }

private:
    Item* _head = nullptr;
    Item* _tail = nullptr;
};


AEOLUS_NAMESPACE_END
