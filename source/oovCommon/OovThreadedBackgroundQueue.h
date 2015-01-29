// Provides a queue so a single producer can place tasks into a
// queue and processed by single consumer thread.
//
// On Windows, this module uses MinGW-W64 because it fully supports
// std::thread and atomic functions. MinGW does not at this time. (2014)
// If normal MinGW is used, the interface stays the same, but the queue
// will only work as if it is single threaded.
//  \copyright 2014 DCBlaha.  Distributed under the GPL.

#ifndef OOV_THREADED_BACKGROUND_QUEUE
#define OOV_THREADED_BACKGROUND_QUEUE

// Some references.
// http://en.cppreference.com/w/cpp/thread/condition_variable
// http://www.justsoftwaresolutions.co.uk/threading/
//        implementing-a-thread-safe-queue-using-condition-variables.html
#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "OovProcess.h"		/// @todo - for sleepMs


#define DEBUG_PROC_QUEUE 0
#if(DEBUG_PROC_QUEUE)
void logProc(char const *str, const void *ptr, int val=0);
#define LOG_PROC(str, ptr)	logProc(str, ptr);
#define LOG_PROC_INT(str, ptr, val)	logProc(str, ptr, val);
#else
#define LOG_PROC(str, ptr)
#define LOG_PROC_INT(str, ptr, val)
#endif


// This is not defined on Windows with some versions of MinGW because things
// like std::condition_variable are not defined. If USE_THREADS is not defined,
// then revert to single threading, but provide the same interface.
#ifdef __linux__
#define USE_THREADS 1
#else
#if defined(_GLIBCXX_HAS_GTHREADS)
#define USE_THREADS 1
#else
#define USE_THREADS 0
#endif
#endif

#if(USE_THREADS)

/// This class reduces template code bloat. See  OovThreadSafeQueue for
/// description.
class OovThreadedBackgroundQueuePrivate
    {
    public:
        OovThreadedBackgroundQueuePrivate():
            mQuitPopping(false)
            {}
        virtual ~OovThreadedBackgroundQueuePrivate()
            {}
        void initThreadSafeQueuePrivate()
            { mQuitPopping = false; }
        void pushPrivate(void const *item);
        bool waitPopPrivate(void *item);
        void quitPopsPrivate();
        bool isEmptyPrivate();

    private:
        // Indicates to quit waiting for pops, nothing more will be put on
        // the queue, and the queue will be cleared.
        bool mQuitPopping;
        std::mutex mProcessQueueMutex;
        // A signal that the provider pushed something onto the queue, or
        // the provider wants the consumers to check the queue.
        std::condition_variable  mProviderPushedSignal;
        // A signal that a consumer popped everything from the queue.
        std::condition_variable  mConsumerPoppedQueueEmptySignal;

        virtual bool isQueueEmpty() const = 0;
        virtual void pushBack(void const *item) = 0;
        virtual void getFront(void *item) = 0;
        virtual void clear() = 0;
    };

/// This is a thread safe queue, but does not handle the threads.
template<typename T_ThreadQueueItem>
    class OovThreadedBackgroundQueue:public OovThreadedBackgroundQueuePrivate
    {
    public:
        OovThreadedBackgroundQueue():
	    mGotItemFromQueue(false)
            {}
        virtual ~OovThreadedBackgroundQueue()
            {}

        void initThreadSafeQueue()
            { initThreadSafeQueuePrivate(); }

        // Called by the provider thread.
        // @param item Item that will be pushed onto the queue.
        void push(T_ThreadQueueItem const &item)
            { pushPrivate(&item); }

        // Called by the consumer threads.
        // Waits until something is read from the queue or until quitPops
        // is called.
        // @param item Item to fill from the queue.
        // The return indicates whether a queue item was read.
        bool waitPop(T_ThreadQueueItem &item)
            { return waitPopPrivate(&item); }

        // Called by the provider thread.
        // This will clear all queue items and will not process
        // items that are still in the queue.
        // This will cause the waitPop functions to quit and return false
        // if there are no more queue entries.
        void quitPops()
            { quitPopsPrivate(); }

        bool isEmpty()
            { return isEmptyPrivate(); }

        /// @todo - this is dirty.
        void workCompleted()
            {
            mGotItemFromQueue = false;
            LOG_PROC_INT("workCompleted", this, mGotItemFromQueue);
            }
        bool isWorkingOnItem() const
            {
            LOG_PROC_INT("isWorking", this, mGotItemFromQueue);
            return mGotItemFromQueue;
            }

    private:
        std::list<T_ThreadQueueItem> mQueue;
        bool mGotItemFromQueue;

        virtual bool isQueueEmpty() const override
            { return mQueue.empty(); }
        virtual void pushBack(void const *item) override
            { mQueue.push_back(*static_cast<T_ThreadQueueItem const*>(item)); }
        virtual void getFront(void *item) override
	    {
            *static_cast<T_ThreadQueueItem*>(item) = mQueue.front();
            mGotItemFromQueue = true;
            LOG_PROC_INT("getFront", this, mGotItemFromQueue);
            mQueue.pop_front();
	    }
        virtual void clear() override
            {
            mQueue.clear();
            }
    };
#endif

/// This uses a producer consumer model where a single producer places items in
/// the queue and multiple consumer threads remove and process the queue.
/// Also only stores so many items so that queue doesn't get too
/// large/take too much memory.
///
/// This is meant to be used by a single client thread.
/// The processItem function can be overridden for the work that will be
/// processed by the worker/consumer threads.
///
/// The threads are started by setup(), and are cleaned up by this class during
/// waitForCompletion().
///
/// @param T_ThreadQueueItem The type of item that will be in the queue.
/// @param T_ProcessItem A type derived from ThreadedWorkQueue that contains a
/// 	function that returns true to continue processing:
/// 	bool processItem(T_ThreadQueueItem const &item)
///
/// A usage example:
/// class ThreadedQueue:public ThreadedWorkQueue<class ThreadedQueue, std::string>
///    {
///    public:
///        // The function that will be called by the worker threads.
///        bool processItem(std::string const &item) {}
///    };
template<typename T_ProcessClass, typename T_ThreadQueueItem>
    class ThreadedWorkBackgroundQueue
    {
    public:
        ThreadedWorkBackgroundQueue(/*int numThreads = 1*/)
            {
	    LOG_PROC("ThreadedWorkBackgroundQueue", this);
            mTaskQueue.initThreadSafeQueue();
            }
        // The worker thread is joined at destruction.
        virtual ~ThreadedWorkBackgroundQueue()
            {
	    LOG_PROC("~ThreadedWorkBackgroundQueue", this);
//            waitForCompletion();
            }

        // Starts the worker/consumer threads.
        // @param numThreads The number of threads to use to process the queue.
        // This will block if the worker threads are busy processing the queue.
        // @param item The item to push onto the queue to process.
        void addTask(T_ThreadQueueItem const &item)
            {
#if(USE_THREADS)
            if(!mWorkerThread.joinable())
        	{
        	mWorkerThread = std::thread(workerThreadProc, this);
        	}
            mTaskQueue.push(item);
#else
            static_cast<T_ProcessClass*>(ptr)->processItem(item);
#endif
            }

        // Wait for the thread to complete work on the queued items.
        void waitForCompletion()
            {
#if(USE_THREADS)
	    LOG_PROC("waitForCompletion", this);
	    mTaskQueue.quitPops();
	    while(isQueueBusy())
		{
		sleepMs(100);		/// @todo - cleanup
		}
            LOG_PROC("waitForCompletion - this", this);
            LOG_PROC_INT("waitForCompletion - work", this, mTaskQueue.isWorkingOnItem());
            LOG_PROC_INT("waitForCompletion - queue", this, mTaskQueue.isEmpty());
            if(mWorkerThread.joinable())
        	{
        	mWorkerThread.join();
        	}
#endif
            }

//        bool isQueueEmpty()
//            { return mTaskQueue.isEmpty(); }
        // Is there something in the queue, or is there some processing of the queue
        bool isQueueBusy()
            { return !mTaskQueue.isEmpty() || mTaskQueue.isWorkingOnItem(); }

#if(USE_THREADS)
    private:
        OovThreadedBackgroundQueue<T_ThreadQueueItem> mTaskQueue;
        std::thread mWorkerThread;
        static void workerThreadProc(
        	ThreadedWorkBackgroundQueue<T_ProcessClass, T_ThreadQueueItem> *workQueue)
            {
            T_ThreadQueueItem item;
            while(workQueue->mTaskQueue.waitPop(item))
                {
                static_cast<T_ProcessClass*>(workQueue)->processItem(item);
                workQueue->mTaskQueue.workCompleted();
                }
            }
#endif
    };

#endif
