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
#ifndef __AGS_EE_UTIL__ASYNCJOBMANAGER_H
#define __AGS_EE_UTIL__ASYNCJOBMANAGER_H

#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace AGS
{
namespace Engine
{

enum AsyncJobState
{
    kAsyncJobState_Undefined = 0,
    kAsyncJobState_Running,
    kAsyncJobState_Suspended,
    kAsyncJobState_Aborted,
    kAsyncJobState_Done
};

enum AsyncJobResult
{
    kAsyncJobResult_Done = 0,
    kAsyncJobResult_Error,
    kAsyncJobResult_Aborted,
    kAsyncJobResult_Busy,
    kAsyncJobResult_Suspended
};

// IAsyncJob - a async job interface
class IAsyncJob
{
public:
    // Starts the job. This function exits when:
    // - job is complete; returns kAsyncJobResult_Done
    // - error occured; returns kAsyncJobResult_Error
    // - job was aborted; returns kAsyncJobResult_Aborted
    // - job was suspended; returns kAsyncJobResult_Suspended
    virtual AsyncJobResult Run() = 0;
    // Aborts this job. The Run() function will be exited asap,
    // but in pratice this may happen with a certain delay.
    virtual void Abort() = 0;
    // Suspends the job. The Run() function will be exited asap,
    // but in pratice this may happen with a certain delay.
    // Call Run again to resume the job.
    virtual void Suspend() = 0;

    // Gets current job state
    virtual AsyncJobState GetState() const = 0;
    // Assigns an arbitrary ID to this job
    virtual void SetID(uint32_t job_id) = 0;
    // Gets a previously assigned ID, returns 0 if no ID was assigned
    virtual uint32_t GetID() const = 0;
};

// BaseAsyncJob - the simplest IAsyncJob implementation
class BaseAsyncJob : public IAsyncJob
{
public:
    AsyncJobResult Run() override;
    void Abort() override;
    void Suspend() override;

    AsyncJobState GetState() const override;
    void SetID(uint32_t job_id) override { _id = job_id; }
    uint32_t GetID() const override { return _id; }

protected:
    virtual AsyncJobResult RunStepImpl() = 0;

private:
    AsyncJobState _state = kAsyncJobState_Undefined;
    uint32_t      _id = 0u;
    mutable std::mutex _mutex;
};

class AsyncJobManager final
{
public:
    void     Start();
    void     Stop();
    void     Suspend();
    void     Resume();

    uint32_t AddJob(std::unique_ptr<IAsyncJob> &&job);
    void     AbortJob(uint32_t job_id);
    void     AbortAll();
    bool     RaiseJob(uint32_t job_id);
    // Synchronously wait for the job to complete,
    // this will raise this job if it's not first in queue
    AsyncJobResult WaitForJob(uint32_t job_id);
    // Synchronously wait for all the job queue to complete
    void     WaitForAll();

private:
    // TODO: let configure this?
    static const int64_t _waitTimeoutMs = 16;

    IAsyncJob *FindJob(uint32_t job_id);
    void     Work();

    std::thread _workThread;
    bool _threadPaused = false;
    bool _abortThread = false;
    std::deque<std::unique_ptr<IAsyncJob>> _jobQueue;
    std::unordered_map<uint32_t, IAsyncJob*> _jobLookup;
    std::mutex _queueMutex;
    std::condition_variable _workNotify;
    uint32_t _jobID = 0u;
    std::unique_ptr<IAsyncJob> _currentJob;
    AsyncJobResult _lastJobResult = kAsyncJobResult_Busy;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__ASYNCJOBMANAGER_H
