// Provides a queue so a single producer can place many tasks into a
// queue and processed by multiple worker/consumer threads. The single
// producer will block if all consumer threads are busy.
//
// On Windows, this module uses MinGW-W64 because it fully supports
// std::thread and atomic functions. MinGW does not at this time. (2014)
// If normal MinGW is used, the interface stays the same, but the queue
// will only work as if it is single threaded.
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#ifndef OOV_THREADED_WAIT_QUEUE
#define OOV_THREADED_WAIT_QUEUE

// Some references.
// http://en.cppreference.com/w/cpp/thread/condition_variable
// http://www.justsoftwaresolutions.co.uk/threading/
//      implementing-a-thread-safe-queue-using-condition-variables.html
// https://eugenedruy.wordpress.com/2009/07/19/refactoring-template-bloat/
#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "OovProcess.h"         // For sleepMs


/// This class reduces template code bloat. See  OovThreadedWaitQueue for
/// interface description.  See top of file for reference for reducing bloat.
class OovThreadedWaitQueuePrivate
    {
    public:
        OovThreadedWaitQueuePrivate():
            mQuitPopping(false)
            {}
        virtual ~OovThreadedWaitQueuePrivate();
        void initThreadSafeQueuePrivate()
            { mQuitPopping = false; }
        void waitPushPrivate(void const *item);
        bool waitPopPrivate(void *item);
        void quitPopsPrivate();

    private:
        // Indicates to quit waiting for pops, nothing more will be put on
        // the queue.
        bool mQuitPopping;
        std::mutex mProcessQueueMutex;
        // A signal that the provider pushed something onto the queue, or
        // the provider wants the consumers to check the queue.
        std::condition_variable  mProviderPushedSignal;
        // A signal that a consumer popped something from the queue.
        std::condition_variable  mConsumerPoppedSignal;

        virtual bool isQueueEmpty() const = 0;
        virtual void pushBack(void const *item) = 0;
        virtual void getFront(void *item) = 0;
    };

/// This is a thread safe queue, but does not handle the threads.
template<typename T_ThreadQueueItem>
    class OovThreadedWaitQueue:public OovThreadedWaitQueuePrivate
    {
    public:
        OovThreadedWaitQueue()
            {}
        virtual ~OovThreadedWaitQueue()
            { quitPops(); }

        void initThreadSafeQueue()
            { initThreadSafeQueuePrivate(); }

        // Called by the provider thread.
        // Waits until queue is empty before pushing more.
        // @param item Item that will be pushed onto the queue.
        void waitPush(T_ThreadQueueItem const &item)
            { waitPushPrivate(&item); }

        // Called by the consumer threads.
        // Waits until something is read from the queue or until quitPops
        // is called.
        // @param item Item to fill from the queue.
        // The return indicates whether a queue item was read.
        bool waitPop(T_ThreadQueueItem &item)
            { return waitPopPrivate(&item); }

        // Called by the provider thread.
        // This will wait to make sure all queue items were processed
        // by a consumer thread.
        // This will cause the waitPop functions to quit and return false
        // if there are no more queue entries.
        void quitPops()
            { quitPopsPrivate(); }

    private:
        std::list<T_ThreadQueueItem> mQueue;

        virtual bool isQueueEmpty() const override
            { return mQueue.empty(); }
        virtual void pushBack(void const *item) override
            { mQueue.push_back(*static_cast<T_ThreadQueueItem const*>(item)); }
        virtual void getFront(void *item) override
            {
            *static_cast<T_ThreadQueueItem*>(item) = mQueue.front();
            mQueue.pop_front();
            }
    };

// This class reduces template code bloat.
class ThreadedWorkWaitPrivate
    {
    public:
        static void joinThreads(std::vector<std::thread> &workerThreads);
    };

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
///     function that returns true to continue processing:
///     bool processItem(T_ThreadQueueItem const &item)
///
/// A usage example:
/// class ThreadedQueue:public ThreadedWorkQueue<std::string, class ThreadedQueue>
///    {
///    public:
///        // The function that will be called by the worker threads.
///        bool processItem(std::string const &item) {}
///    };
template<typename T_ThreadQueueItem, typename T_ProcessItem>
    class ThreadedWorkWaitQueue
    {
    public:
        // The worker threads are joined and cleaned up at destruction.
        ~ThreadedWorkWaitQueue()
            { waitForCompletion(); }
        // Starts the worker/consumer threads.
        // @param numThreads The number of threads to use to process the queue.
        void setupQueue(size_t numThreads)
            {
            mTaskQueue.initThreadSafeQueue();
            setupThreads(numThreads);
            }
        // This will block if the worker threads are busy processing the queue.
        // @param item The item to push onto the queue to process.
        void addTask(T_ThreadQueueItem const &item)
            {
            mTaskQueue.waitPush(item);
            }

        // Wait for all threads to complete work on the queued items.
        // setupQueue must be called each time after waitForCompletion.
        void waitForCompletion()
            {
            mTaskQueue.quitPops();
            ThreadedWorkWaitPrivate::joinThreads(mWorkerThreads);
            }

        // Uses std::thread::hardware_concurrency() to find number of
        // threads.
         static size_t getNumHardwareThreads()
            {
            return std::thread::hardware_concurrency();
            }

    private:
        OovThreadedWaitQueue<T_ThreadQueueItem> mTaskQueue;
        std::vector<std::thread> mWorkerThreads;
        // Only sets the number of threads if there are
        // currently no threads running.
        void setupThreads(size_t numThreads)
            {
            if(mWorkerThreads.size() == 0)
                {
                for(size_t i=0; i<numThreads; i++)
                    {
                    mWorkerThreads.push_back(std::thread(workerThreadProc, this));
                    }
                }
            }
        static void workerThreadProc(
                ThreadedWorkWaitQueue<T_ThreadQueueItem, T_ProcessItem> *workQueue)
            {
            T_ThreadQueueItem item;
            while(workQueue->mTaskQueue.waitPop(item))
                {
                static_cast<T_ProcessItem*>(workQueue)->processItem(item);
                }
            }
    };

#endif
