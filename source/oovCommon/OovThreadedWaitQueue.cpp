// OovThreadedQueue.cpp
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#include "OovThreadedWaitQueue.h"
#include <thread>


#define DEBUG_PROC_QUEUE 0
#if(DEBUG_PROC_QUEUE)
#include "Debug.h"
DebugFile sDebugQueue("DbgQueue.txt", false);
void logProc(char const *str, const void *ptr, int val)
    {
    sDebugQueue.printflush("%s %p %d\n", str, ptr, val);
    }
#define LOG_PROC(str, ptr)	logProc(str, ptr, 0);
#define LOG_PROC_INT(str, ptr, val)	logProc(str, ptr, val);
#else
#define LOG_PROC(str, ptr)
#define LOG_PROC_INT(str, ptr, val)
#endif


void OovThreadedWaitQueuePrivate::waitPushPrivate(void const *item)
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
    LOG_PROC("push lock", this);
    // Wait while not empty
    while(!isQueueEmpty())
        {
        // Release lock and wait for signal.
        mConsumerPoppedSignal.wait(lock);
        // After signaled, lock is reaquired.
        }
    LOG_PROC("push", this);
    pushBack(item);

    LOG_PROC("push unlock", this);
    lock.unlock();
    // Signal to waitPop that data is ready.
    mProviderPushedSignal.notify_one();
    }

bool OovThreadedWaitQueuePrivate::waitPopPrivate(void *item)
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
    bool gotItem = false;
    LOG_PROC("pop lock", this);
    // Wait while empty
    while(isQueueEmpty() && !mQuitPopping)
        {
        // Release lock and wait for signal.
        mProviderPushedSignal.wait(lock);
        // After signaled, lock is reaquired.
        }

    // In the normal case this will not be empty.
    // If it is empty, then there was a signal, but nothing was
    // in the queue. This means that the quit function was called.
    gotItem = !isQueueEmpty();
    LOG_PROC_INT("pop got item", this, gotItem);
    if(gotItem)
        {
	getFront(item);
        }

    LOG_PROC_INT("pop unlock", this, gotItem);
    // unlock and signal to provider thread that queue is processed.
    lock.unlock();
    mConsumerPoppedSignal.notify_one();
    LOG_PROC("pop done", this);
    return gotItem;
    }

void OovThreadedWaitQueuePrivate::quitPopsPrivate()
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
    LOG_PROC("quitPops lock", this);
    mQuitPopping = true;
    // Wait to make sure all queue items were processed.
    while(!isQueueEmpty())
        {
        mConsumerPoppedSignal.wait(lock);
        }
    LOG_PROC("quitPops unlock", this);
    lock.unlock();
    mProviderPushedSignal.notify_all();
    LOG_PROC("quitPops done", this);
    }

void ThreadedWorkWaitPrivate::joinThreads(std::vector<std::thread> &workerThreads)
    {
    if(workerThreads.size() > 0)
        {
        for(size_t i=0; i<workerThreads.size(); i++)
            {
            workerThreads[i].join();
            }
        workerThreads.clear();
        }
    }

