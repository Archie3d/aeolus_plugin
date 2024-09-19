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

#include <memory>

#include "aeolus/globals.h"
#include "aeolus/list.h"
#include "aeolus/ringbuffer.h"

AEOLUS_NAMESPACE_BEGIN

/**
 * Allocate memory with predefined alignment.
 *
 * Ported from https://embeddedartistry.com/blog/2017/2/20/implementing-aligned-malloc
 */
template <size_t Align>
struct AlignedMemory
{
    typedef uint16_t offset_t;
    constexpr static size_t PTR_OFFSET_SIZE = sizeof(offset_t);
    constexpr static size_t alignment = Align;

    /// Allocate aligned memory block.
    static void* alloc(size_t size)
    {
        void* ptr = nullptr;

        if (alignment && size) {
            uint32_t hdr_size = PTR_OFFSET_SIZE + (alignment - 1);
            void* p = ::malloc (size + hdr_size);

            if (p) {
                ptr = (void*) alignUp(((uintptr_t)p + PTR_OFFSET_SIZE));

                //Calculate the offset and store it behind our aligned pointer
                *((offset_t *)ptr - 1) = (offset_t) ((uintptr_t)ptr - (uintptr_t)p);
            }
        }

        return ptr;
    }

    /// Release aligned memory block allocated by \ref alloc()
    static void free(void* ptr)
    {
        if (ptr) {
            offset_t offset = *((offset_t *)ptr - 1);

            // Once we have the offset, we can get our original pointer and call free
            void* p = (void*) ((uint8_t*)ptr - offset);
            ::free (p);
        }
    }

private:

    inline static size_t alignUp(size_t num)
    {
        return (num + (alignment - 1)) & ~(alignment - 1);
    }

    AlignedMemory()
    {
        static_assert ((alignment & (alignment - 1)) == 0);
    }
};

AEOLUS_NAMESPACE_END
