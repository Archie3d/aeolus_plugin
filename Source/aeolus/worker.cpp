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

#include "aeolus/worker.h"
#include "aeolus/ringbuffer.h"
#include "aeolus/sema.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

struct Worker::Impl
{
    RingBuffer<Worker::Job*, DefaultCapacity> jobsQueue;

    Semaphore sema;
    std::atomic_bool running;
    std::unique_ptr<std::thread> thread;

    Impl()
        : jobsQueue()
        , sema(0)
        , running(false)
        , thread(nullptr)
    {        
    }

    ~Impl()
    {
        stop();
    }

    void run()
    {
        while (running) {
            Worker::Job* job = nullptr;

            wait();        

            if (running && jobsQueue.receive (job)) {
                jassert(job != nullptr);
                job->run();
            }
        }
    }

    bool addJob (Job* job)
    {
        jassert(job != nullptr);
        
        const auto ok = jobsQueue.send (job);
        wakeUp();

        return ok;
    }

    void start()
    {
        purge();

        if (thread == nullptr) {
            running = true;
            thread = std::make_unique<std::thread> (&Impl::run, this);
        }
    }

    void stop()
    {
        if (thread != nullptr) {
            running = false;
            wakeUp();

            if (thread->joinable())
                thread->join();
        }
    }

    bool hasPendingJobs() noexcept
    {
        return jobsQueue.count() > 0;
    }

    bool isRunning() const noexcept
    {
        return running;
    }

    void purge()
    {
        Worker::Job* job;

        while (jobsQueue.receive (job)) {
            // Do nothing.
        }
    }

    void wait()
    {
        sema.wait();
    }

    void wakeUp()
    {
        sema.notify();
    }
};

//----------------------------------------------------------

Worker::Worker()
    : d(std::make_unique<Impl>())
{
}

Worker::~Worker() = default;

void Worker::start()
{
    d->start();
}

void Worker::stop()
{
    d->stop();
}

bool Worker::addJob(Job* job)
{
    return d->addJob (job);
}

bool Worker::hasPendingJobs() noexcept
{
    return d->hasPendingJobs();
}

bool Worker::isRunning() const noexcept
{
    return d->isRunning();
}

void Worker::purge()
{
    d->purge();
}

AEOLUS_NAMESPACE_END
