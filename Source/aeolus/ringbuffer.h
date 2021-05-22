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
#include <atomic>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Simple lock-free ring buffer.
 *
 * This is an implementation of single-producer single-consumer queue.
 *
 * @note From http://www.vitorian.com/x1/archives/370
 * https://github.com/Vitorian/RedditHelp/blob/master/test_spsc_ring.cpp
 */
template <typename T, size_t Size>
class RingBuffer final
{
public:

    RingBuffer() noexcept
        : readIdx {0},
          writeIdx {0},
          data {{}}
    {
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator =(const RingBuffer&) = delete;

    ~RingBuffer() = default;

    bool send (const T& obj) noexcept
    {
        size_t nextIdx = writeIdx + 1 < Size ? writeIdx + 1 : 0;

        if (nextIdx != readIdx)
        {
            data[writeIdx] = obj;
            writeIdx = nextIdx;
            return true;
        }

        return false;
    }

    bool receive (T& obj) noexcept
    {
        size_t nextIdx = readIdx + 1 < Size ? readIdx + 1 : 0;

        if (readIdx != writeIdx)
        {
            obj = std::move (data[readIdx]);
            readIdx = nextIdx;
            return true;
        }

        return false;
    }

    size_t count() const noexcept
    {
        return (writeIdx - readIdx) % Size;
    }

private:

    std::atomic<size_t> readIdx;
    std::atomic<size_t> writeIdx;
    T data[Size];
};

AEOLUS_NAMESPACE_END
