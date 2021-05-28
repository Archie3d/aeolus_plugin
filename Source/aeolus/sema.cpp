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

#include "aeolus/sema.h"

AEOLUS_NAMESPACE_BEGIN

Semaphore::Semaphore(unsigned initialCount)
    : _mutex()
    , _cv()
    , _counter{initialCount}
{
}

void Semaphore::notify()
{
    std::unique_lock<decltype(_mutex)> lock(_mutex);
    ++_counter;
    _cv.notify_one();
}

void Semaphore::wait()
{
    std::unique_lock<decltype(_mutex)> lock(_mutex);

    while (_counter == 0)
        _cv.wait (lock);

    --_counter;
}

bool Semaphore::tryWait()
{
    std::unique_lock<decltype(_mutex)> lock(_mutex);

    if (_counter != 0) {
        --_counter;
        return true;
    }

    return false;
}

unsigned Semaphore::count() const
{
    std::unique_lock<decltype(_mutex)> lock(_mutex);
    return _counter;
}

AEOLUS_NAMESPACE_END
