
#include "OovThreadedQueue.h"
#include <thread>


#define DEBUG_PROC_QUEUE 0
#if(DEBUG_PROC_QUEUE)
#include "Debug.h"
DebugFile sDebugQueue("DbgQueue.txt");
#endif

#if(USE_THREADS)

void OovThreadSafeQueuePrivate::waitPushPrivate(void const *item)
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
#if(DEBUG_PROC_QUEUE)
    sDebugQueue.printflush("push lock\n");
#endif
    // Wait while not empty
    while(!isQueueEmpty())
        {
        // Release lock and wait for signal.
        mConsumerPoppedSignal.wait(lock);
        // After signaled, lock is reaquired.
        }
#if(DEBUG_PROC_QUEUE)
    sDebugQueue.printflush("push\n");
#endif
    pushBack(item);

#if(DEBUG_PROC_QUEUE)
    sDebugQueue.printflush("push unlock\n");
#endif
    lock.unlock();
    // Signal to waitPop that data is ready.
    mProviderPushedSignal.notify_one();
    }

bool OovThreadSafeQueuePrivate::waitPopPrivate(void *item)
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
    bool gotItem = false;
#if(DEBUG_PROC_QUEUE)
    sDebugQueue.printflush("pop lock\n");
#endif
    // Wait while empty
    while(isQueueEmpty() && !mQuitPopping)
        {
        // Release lock and wait for signal.
        mProviderPushedSignal.wait(lock);
        // After signaled, lock is reaquired.
        }

    // In the normal case this will be empty.
    // If it is not empty, then there was a signal, but nothing was
    // in the queue. This may mean that the quit function was called,
    // or that a spurious signal was received.
    // See std::condition_variable::wait().
    gotItem = !isQueueEmpty();
#if(DEBUG_PROC_QUEUE)
    sDebugQueue.printflush("pop got item=%d\n", gotItem);
#endif
    if(gotItem)
        {
	getFront(item);
        }

#if(DEBUG_PROC_QUEUE)
        sDebugQueue.printflush("pop unlock %d\n", gotItem);
#endif
    // unlock and signal to provider thread that queue is processed.
    lock.unlock();
    mConsumerPoppedSignal.notify_one();
#if(DEBUG_PROC_QUEUE)
        sDebugQueue.printflush("pop done\n");
#endif
    return gotItem;
    }

void OovThreadSafeQueuePrivate::waitQueueEmptyPrivate()
    {
    std::unique_lock<std::mutex> lock(mProcessQueueMutex);
#if(DEBUG_PROC_QUEUE)
    sDebugQueue.printflush("quitPops lock\n");
#endif
    mQuitPopping = true;
    // Wait to make sure all queue items were processed.
    while(!isQueueEmpty())
        {
        mConsumerPoppedSignal.wait(lock);
        }
#if(DEBUG_PROC_QUEUE)
//    sDebugQueue.printflush("quitPops unlock\n");
#endif
    lock.unlock();
    }

void OovThreadSafeQueuePrivate::quitPopsPrivate()
    {
    waitQueueEmptyPrivate();
    mProviderPushedSignal.notify_all();
#if(DEBUG_PROC_QUEUE)
    sDebugQueue.printflush("quitPops done\n");
#endif
    }

#if(USE_THREADS)
void ThreadedWorkQueuePrivate::joinThreads(std::vector<std::thread> &workerThreads)
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
#endif

#endif

