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

#include <mutex>
#include <condition_variable>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Semaphore imlementation based on conditional variable.
 */
class Semaphore final
{
public:

    explicit Semaphore(unsigned initialCount = 0);
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator =(const Semaphore&) = delete;

    void notify();
    void wait();
    bool tryWait();
    unsigned count() const;

private:

    mutable std::mutex _mutex;
    std::condition_variable _cv;
    unsigned _counter;
};

AEOLUS_NAMESPACE_END
