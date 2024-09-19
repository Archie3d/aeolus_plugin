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

#include <memory>
#include <functional>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Schedule jobs run on a separate thread.
 *
 * This class uses a lock-free queue to schedulre jobs
 * on audio thread, and execute them on a side thead.
 * This is normally used for samples streaming.
 */
class Worker final
{
public:

    class Job
    {
    public:
        virtual void run() = 0;
        virtual ~Job() = default;
    };

    //------------------------------------------------------

    /// Maximum allowed number of queued jobs
    constexpr static size_t DefaultCapacity = 1024;

    Worker();
    ~Worker();
    Worker(const Worker&) = delete;
    Worker& operator = (const Worker&) = delete;

    void start();
    void stop();

    /**
     * @brief Add job to the queue.
     *
     * @note This must be called from audio thread only.
     */
    bool addJob(Job* job);
    bool hasPendingJobs() noexcept;
    bool isRunning() const noexcept;

    void purge();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

AEOLUS_NAMESPACE_END
