//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "util/asyncjobmanager.h"
#include "debug/assert.h"

namespace AGS
{
namespace Engine
{

//-----------------------------------------------------------------------------
// BaseAsyncJob
//-----------------------------------------------------------------------------

AsyncJobResult BaseAsyncJob::Run()
{
    // Test that job state is runnable
    std::unique_lock<std::mutex> lk(_mutex);
    assert(_state == kAsyncJobState_Undefined || _state == kAsyncJobState_Suspended);
    switch (_state)
    {
    case kAsyncJobState_Undefined:
    case kAsyncJobState_Suspended:
        _state = kAsyncJobState_Running; break;
    case kAsyncJobState_Running:
        return kAsyncJobResult_Busy; // Run called while already running?
    case kAsyncJobState_Aborted:
        return kAsyncJobResult_Aborted;
    case kAsyncJobState_Done:
        return kAsyncJobResult_Done;
    }
    lk.unlock();

    AsyncJobResult result = kAsyncJobResult_Busy;
    while (result == kAsyncJobResult_Busy)
    {
        result = RunStepImpl();

        // Post-run update job's state
        {
            lk.lock();
            if (_state == kAsyncJobState_Running)
            {
                switch (result)
                {
                case kAsyncJobResult_Done:
                    _state = kAsyncJobState_Done; break;
                case kAsyncJobResult_Aborted:
                case kAsyncJobResult_Error:
                    _state = kAsyncJobState_Aborted; break;
                default: break;
                }
            }
            else
            {
                switch (_state)
                {
                case kAsyncJobState_Aborted:
                    result = kAsyncJobResult_Aborted; break;
                case kAsyncJobState_Done:
                    result = kAsyncJobResult_Done; break;
                case kAsyncJobState_Suspended:
                    result = kAsyncJobResult_Suspended; break;
                default: assert(false); break; // should not happen
                }
            }
            lk.unlock();
        }
    }
    return result;
}

void BaseAsyncJob::Abort()
{
    std::lock_guard<std::mutex> lk(_mutex);
    if (_state == kAsyncJobState_Running || _state == kAsyncJobState_Suspended)
    {
        _state = kAsyncJobState_Aborted;
    }
}

void BaseAsyncJob::Suspend()
{
    std::lock_guard<std::mutex> lk(_mutex);
    if (_state == kAsyncJobState_Running)
        _state = kAsyncJobState_Suspended;
}

AsyncJobState BaseAsyncJob::GetState() const
{
    std::lock_guard<std::mutex> lk(_mutex);
    return _state;
}

//-----------------------------------------------------------------------------
// AsyncJobManager
//-----------------------------------------------------------------------------

void AsyncJobManager::Start()
{
    if (!_workThread.joinable())
        return;

    _workThread = std::thread(&AsyncJobManager::Work, this);
}

void AsyncJobManager::Stop()
{
    if (!_workThread.joinable())
        return;

    AbortAll();
    _workThread.join();
    _threadPaused = false;
}

void AsyncJobManager::Suspend()
{
    if (!_workThread.joinable())
        return;

    std::lock_guard<std::mutex> lk(_queueMutex);
    _threadPaused = true;
    if (_currentJob)
        _currentJob->Suspend();
    _workNotify.notify_all();
}

void AsyncJobManager::Resume()
{
    if (!_workThread.joinable())
        return;

    std::lock_guard<std::mutex> lk(_queueMutex);
    _threadPaused = false;
    _workNotify.notify_all();
}

void AsyncJobManager::Work()
{
    bool do_run = true;
    while (do_run)
    {
        IAsyncJob *current_job = nullptr;
        AsyncJobResult job_result = kAsyncJobResult_Busy;
        // Try get current job from queue, under queue mutex
        {
            std::lock_guard<std::mutex> lk(_queueMutex);
            while ((!_currentJob || (_currentJob->GetState() == kAsyncJobState_Aborted))
                && !_jobQueue.empty())
            {
                _currentJob = std::move(_jobQueue.front());
                _jobQueue.pop_front();
                _jobLookup.erase(_currentJob->GetID());
            }
            current_job = _currentJob.get();
        }
        
        // Run current job, not under mutex
        if (current_job)
        {
            job_result = current_job->Run();
        }

        // Test last job's result under queue mutex, wait if thread was paused,
        // check if the thread was aborted by user
        {
            std::unique_lock<std::mutex> lk(_queueMutex);
            if (current_job)
            {
                if (job_result != kAsyncJobResult_Busy && job_result != kAsyncJobResult_Suspended)
                    _currentJob = nullptr;
                _lastJobResult = job_result;
                _workNotify.notify_all(); // in case someone was waiting for this job to complete
            }

            while (_threadPaused || _jobQueue.empty())
            {
                _workNotify.wait_for(lk, std::chrono::milliseconds(_waitTimeoutMs));
            }

            do_run = !_abortThread;
        }
    }
}

uint32_t AsyncJobManager::AddJob(std::unique_ptr<IAsyncJob> &&job)
{
    std::lock_guard<std::mutex> lk(_queueMutex);
    const uint32_t job_id = _jobID++;
    job->SetID(job_id);
    _jobLookup.insert(std::make_pair(job_id, job.get()));
    _jobQueue.push_back(std::move(job));
    _workNotify.notify_all();
    return job_id;
}

IAsyncJob *AsyncJobManager::FindJob(uint32_t job_id)
{
    if (_currentJob && _currentJob->GetID() == job_id)
        return _currentJob.get();
    auto it_job = _jobLookup.find(job_id);
    if (it_job != _jobLookup.end())
        return it_job->second;
    return nullptr;
}

void AsyncJobManager::AbortJob(uint32_t job_id)
{
    std::lock_guard<std::mutex> lk(_queueMutex);
    auto *job = FindJob(job_id);
    if (job)
        job->Abort();
    // keep it in queue, will be popped without running
}

void AsyncJobManager::AbortAll()
{
    std::lock_guard<std::mutex> lk(_queueMutex);
    for (auto &job : _jobQueue)
        job->Abort();
    // keep them in queue, will be popped without running
}

bool AsyncJobManager::RaiseJob(uint32_t job_id)
{
    std::lock_guard<std::mutex> lk(_queueMutex);
    if (_currentJob && _currentJob->GetID() == job_id)
        return true; // already running

    for (auto it_job = _jobQueue.begin(); it_job != _jobQueue.end(); ++it_job)
    {
        if ((*it_job)->GetID() == job_id)
        {
            auto job = std::move(*it_job);
            _jobQueue.erase(it_job);
            _jobQueue.push_front(std::move(job));
            return true;
        }
    }
    return false; // not found
}

AsyncJobResult AsyncJobManager::WaitForJob(uint32_t job_id)
{
    if (!RaiseJob(job_id))
        return kAsyncJobResult_Done;

    bool job_started = false;
    while (true)
    {
        std::unique_lock<std::mutex> lk(_queueMutex);

        if (!_currentJob && _jobQueue.empty())
            return kAsyncJobResult_Done;
        if (job_started && _currentJob->GetID() != job_id)
            return _lastJobResult;

        job_started = _currentJob && _currentJob->GetID() == job_id;

        _workNotify.wait_for(lk, std::chrono::milliseconds(_waitTimeoutMs));
    }
}

void AsyncJobManager::WaitForAll()
{
    while (true)
    {
        std::unique_lock<std::mutex> lk(_queueMutex);

        if (!_currentJob && _jobQueue.empty())
            return;

        _workNotify.wait_for(lk, std::chrono::milliseconds(_waitTimeoutMs));
    }
}

} // namespace Engine
} // namespace AGS
