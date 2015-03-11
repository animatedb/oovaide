// OovThreadedBackgroundQueue.cpp
//  \copyright 2014 DCBlaha.  Distributed under the GPL.

#include "OovThreadedBackgroundQueue.h"
#include <thread>

#if(DEBUG_PROC_QUEUE)
#include "Debug.h"
DebugFile sDebugQueue("DbgQueue.txt", false);
void logProc(char const *str, const void *ptr, int val)
    {
    sDebugQueue.printflush("%s %p %d\n", str, ptr, val);
    }
#endif



void OovThreadedBackgroundQueuePrivate::pushPrivate(void const *item)
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
    LOG_PROC("push lock", this);
    pushBack(item);

    LOG_PROC("push unlock", this);
    lock.unlock();
    // Signal to waitPop that data is ready.
    mProviderPushedSignal.notify_one();
    }

bool OovThreadedBackgroundQueuePrivate::waitPopPrivate(void *item)
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
    LOG_PROC("pop lock", this);
    bool gotItem = false;
    // Wait while empty
    while(isQueueEmpty() && !mQuitPopping)
        {
        // Release lock and wait for signal.
        mProviderPushedSignal.wait(lock);
        // After signaled, lock is reaquired.
        }

    if(mQuitPopping)
	{
	clear();
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
    mConsumerPoppedQueueEmptySignal.notify_one();
    LOG_PROC("pop done", this);
    return gotItem;
    }

void OovThreadedBackgroundQueuePrivate::quitPopsPrivate()
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
    LOG_PROC("quitPops lock", this);
    mQuitPopping = true;
    // Wait to make sure all queue items were processed.
    while(!isQueueEmpty())
        {
        mConsumerPoppedQueueEmptySignal.wait(lock);
        }
    LOG_PROC("quitPops unlock", this);
    lock.unlock();
    mProviderPushedSignal.notify_all();
    LOG_PROC("quitPops done", this);
    }


bool OovThreadedBackgroundQueuePrivate::isEmptyPrivate()
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
    return isQueueEmpty();
    }

