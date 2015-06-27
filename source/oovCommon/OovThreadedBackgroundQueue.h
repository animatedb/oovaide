// Provides a queue so a single producer can place tasks into a queue and
// processed by single consumer background thread.  Background thread
// remains around and waits for a signal to process the queue, but can be
// stopped and joined by the client at any time.
//
// On Windows, this module uses MinGW-W64 because it fully supports
// std::thread and atomic functions. MinGW does not at this time. (2014)
//  \copyright 2014 DCBlaha.  Distributed under the GPL.

#ifndef OOV_THREADED_BACKGROUND_QUEUE
#define OOV_THREADED_BACKGROUND_QUEUE

// Some references.
// http://en.cppreference.com/w/cpp/thread/condition_variable
// http://www.justsoftwaresolutions.co.uk/threading/
//        implementing-a-thread-safe-queue-using-condition-variables.html
// https://eugenedruy.wordpress.com/2009/07/19/refactoring-template-bloat/
#include <list>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "OovProcess.h"         /// @todo - for sleepMs, and continueListener


#define DEBUG_PROC_QUEUE 0
#if(DEBUG_PROC_QUEUE)
void logProc(char const *str, const void *ptr, int val=0);
#define LOG_PROC(str, ptr)      logProc(str, ptr);
#define LOG_PROC_INT(str, ptr, val)     logProc(str, ptr, val);
#else
#define LOG_PROC(str, ptr)
#define LOG_PROC_INT(str, ptr, val)
#endif


/// This class reduces template code bloat. See  OovThreadedBackgroundQueue for
/// interface description.  See top of file for reference for reducing bloat.
class OovThreadedBackgroundQueuePrivate
    {
    public:
        OovThreadedBackgroundQueuePrivate():
            mQuitPopping(false)
            {}
        virtual ~OovThreadedBackgroundQueuePrivate();
        void clearQuitPoppingPrivate()
            { mQuitPopping = false; }
        void pushPrivate(void const *item);
        bool waitPopPrivate(void *item);
        void quitPopsPrivate();
        bool isEmptyPrivate();

    private:
        /// Indicates to quit waiting for pops, nothing more will be put on
        /// the queue, and the queue will be cleared.
        std::atomic_bool mQuitPopping;
        std::mutex mProcessQueueMutex;
        /// A signal that the provider pushed something onto the queue, or
        /// the provider wants the consumers to check the queue.
        std::condition_variable  mProviderPushedSignal;
        /// A signal that a consumer popped everything from the queue.
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

        void clearQuitPopping()
            { clearQuitPoppingPrivate(); }

        /// Called by the provider thread.
        /// @param item Item that will be pushed onto the queue.
        void push(T_ThreadQueueItem const &item)
            {
            pushPrivate(&item);
            }

        /// Called by the consumer thread.
        /// Waits until there is an item on the queue or until quitPops is
        /// called.
        /// @param item Item to fill from the queue.
        /// The return indicates whether a queue item was read.
        bool waitPop(T_ThreadQueueItem &item)
            { return waitPopPrivate(&item); }

        /// Called by the provider thread.
        /// This will clear all queue items and will not process
        /// items that are still in the queue.
        /// This will cause the waitPop functions to quit and return false
        /// if there are no more queue entries.
        void quitPops()
            { quitPopsPrivate(); }

        bool isEmpty()
            { return isEmptyPrivate(); }

        /// @todo - this works, but should be a signal?
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
        std::atomic_bool mGotItemFromQueue;

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

/// This uses a producer consumer model where a single producer places items in
/// the queue and a single consumer thread removes and processes the queue.
///
/// This is meant to be used by a single client thread.
/// The processItem function can be overridden for the work that will be
/// processed by the worker/consumer thread.
///
/// The thread is started when a task is added to the queue, and is cleaned up
/// by this class during stopAndWaitForCompletion().
///
/// @param T_ThreadQueueItem The type of item that will be in the queue.
/// @param T_ProcessItem A type derived from OovThreadedBackgroundQueue that contains a
///     function to process items:
///     void processItem(T_ThreadQueueItem const &item)
///
/// A usage example:
/// class ThreadedQueue:public OovThreadedBackgroundQueue<class ThreadedQueue, std::string>
///    {
///    public:
///        // The function that will be called by the worker threads.
///        void processItem(std::string const &item) {}
///    };
template<typename T_ProcessClass, typename T_ThreadQueueItem>
    class ThreadedWorkBackgroundQueue: public OovTaskContinueListener
    {
    public:
        ThreadedWorkBackgroundQueue(/*int numThreads = 1*/)
            {
            LOG_PROC("ThreadedWorkBackgroundQueue", this);
            }
        /// WARNING - The worker thread is NOT joined at destruction.  This is
        /// because the derived class contains the callback override, and it
        /// must be available. The derived function must call
        /// stopAndWaitForCompletion().
        virtual ~ThreadedWorkBackgroundQueue()
            {
            LOG_PROC("~ThreadedWorkBackgroundQueue", this);
            }

        /// Starts the worker/consumer thread.
        /// This will block if the worker thread is busy processing the queue.
        /// @param item The item to push onto the queue to process.
        void addTask(T_ThreadQueueItem const &item)
            {
            LOG_PROC("addTask", this);
            mContinueProcessingItem = true;
            mTaskQueue.clearQuitPopping();
            if(!mWorkerThread.joinable())
                {
                mWorkerThread = std::thread(workerThreadProc, this);
                }
            mTaskQueue.push(item);
            }

        /// Stop processing background items and wait for any that are being
        /// processed.
        void stopAndWaitForCompletion()
            {
            LOG_PROC("stopAndWaitForCompletion", this);
            mContinueProcessingItem = false;
            mTaskQueue.quitPops();
            while(isQueueBusy())
                {
                sleepMs(100);           /// @todo - cleanup
                }
            LOG_PROC("stopAndWaitForCompletion - this", this);
            LOG_PROC_INT("stopAndWaitForCompletion - work", this, mTaskQueue.isWorkingOnItem());
            LOG_PROC_INT("stopAndWaitForCompletion - queue", this, mTaskQueue.isEmpty());
            if(mWorkerThread.joinable())
                {
                mWorkerThread.join();
                }
            }

        /// Is there something in the queue, or is there some processing of the queue
        bool isQueueBusy()
            { return !mTaskQueue.isEmpty() || mTaskQueue.isWorkingOnItem(); }

        /// This can be called from processItem to see if the process item should be aborted.
        virtual bool continueProcessingItem() const override
            {
            LOG_PROC_INT("continue proc", this, mContinueProcessingItem);
            return mContinueProcessingItem;
            }

    private:
        OovThreadedBackgroundQueue<T_ThreadQueueItem> mTaskQueue;
        std::thread mWorkerThread;
        std::atomic_bool mContinueProcessingItem;
        static void workerThreadProc(
                ThreadedWorkBackgroundQueue<T_ProcessClass, T_ThreadQueueItem> *workQueue)
            {
            LOG_PROC("start workerThreadProc", nullptr);
            T_ThreadQueueItem item;
            while(workQueue->mTaskQueue.waitPop(item))
                {
                LOG_PROC_INT("start processItem", static_cast<T_ProcessClass*>(workQueue),
                        workQueue->mTaskQueue.isWorkingOnItem());
                static_cast<T_ProcessClass*>(workQueue)->processItem(item);
                LOG_PROC_INT("done processItem", static_cast<T_ProcessClass*>(workQueue),
                        workQueue->mTaskQueue.isWorkingOnItem());
                workQueue->mTaskQueue.workCompleted();
                }
            LOG_PROC("done workerThreadProc", nullptr);
            }
    };

#endif
