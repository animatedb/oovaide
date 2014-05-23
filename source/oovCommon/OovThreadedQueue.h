// Provides a process queue so that many tasks can be put into a queue and
// run by multiple worker/consumer threads.
// On Windows, this module uses MinGW-builds because it fully supports
// std::thread and atomic functions. MinGW does not at this time. (2014)
// If normal MinGW is used, the interface stays the same, but the queue
// will only work as if it is single threaded.
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#ifndef OOV_THREADED_QUEUE
#define OOV_THREADED_QUEUE

// Some references.
// http://en.cppreference.com/w/cpp/thread/condition_variable
// http://www.justsoftwaresolutions.co.uk/threading/
//        implementing-a-thread-safe-queue-using-condition-variables.html
#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

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

// This class reduces template code bloat. See  OovThreadSafeQueue for
// description.
class OovThreadSafeQueuePrivate
    {
    public:
        OovThreadSafeQueuePrivate():
            mQuitPopping(false)
            {}
        virtual ~OovThreadSafeQueuePrivate()
            {}
        void initThreadSafeQueuePrivate()
            { mQuitPopping = false; }
        void waitPushPrivate(void const *item);
        bool waitPopPrivate(void *item);
        void waitQueueEmptyPrivate();
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

template<typename T_ThreadQueueItem>
    class OovThreadSafeQueue:public OovThreadSafeQueuePrivate
    {
    public:
        OovThreadSafeQueue()
            {}
        virtual ~OovThreadSafeQueue()
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
        void waitQueueEmpty()
            { waitQueueEmptyPrivate(); }

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
#endif

#if(USE_THREADS)
// This class reduces template code bloat.
class ThreadedWorkQueuePrivate
    {
    public:
	static void joinThreads(std::vector<std::thread> &workerThreads);
    };
#endif

// This uses a producer consumer model where a single producer places items in
// the queue and multiple consumer threads remove and process the queue.
// Also only stores so many items so that queue doesn't get too
// large/take too much memory.
//
// This is meant to be used by a single client thread.
// The processItem function can be overridden for the work that will be
// processed by the worker/consumer threads.
//
// The threads are started by setup(), and are cleaned up by this class during
// waitForCompletion().
//
// @param T_ThreadQueueItem The type of item that will be in the queue.
// @param T_ProcessItem A type derived from ThreadedWorkQueue that contains a
// 	function that returns true to continue processing:
// 	bool processItem(T_ThreadQueueItem const &item)
//
// A usage example:
// class ThreadedQueue:public ThreadedWorkQueue<std::string, class ThreadedQueue>
//    {
//    public:
//        // The function that will be called by the worker threads.
//        bool processItem(std::string const &item) {}
//    };
template<typename T_ThreadQueueItem, typename T_ProcessItem>
    class ThreadedWorkQueue
    {
    public:
        // The worker threads are joined and cleaned up at destruction.
        ~ThreadedWorkQueue()
            { waitForCompletion(); }
        // Starts the worker/consumer threads.
        // @param numThreads The number of threads to use to process the queue.
        void setupQueue(int numThreads)
            {
            mTaskQueue.initThreadSafeQueue();
#if(USE_THREADS)
            setupThreads(numThreads);
#endif
            }
        // This will block if the worker threads are busy processing the queue.
        // @param item The item to push onto the queue to process.
        void addTask(T_ThreadQueueItem const &item)
            {
#if(USE_THREADS)
            mTaskQueue.waitPush(item);
#else
            static_cast<T_ProcessItem*>(this)->processItem(item);
#endif
            }

        // Wait for all threads to complete work on the queued items.
        void waitForCompletion()
            {
#if(USE_THREADS)
            mTaskQueue.waitQueueEmpty();
#endif
            }

        // Wait for all threads to complete work on the queued items.
        // setupQueue must be called each time after waitForCompletion.
        void waitForCompletionAndEndThreads()
            {
#if(USE_THREADS)
	    mTaskQueue.quitPops();
            ThreadedWorkQueuePrivate::joinThreads(mWorkerThreads);
#endif
            }

        // Uses std::thread::hardware_concurrency() to find number of
        // threads.
         static int getNumHardwareThreads()
            {
#if(USE_THREADS)
            return std::thread::hardware_concurrency();
#else
            return 1;
#endif
            }

#if(USE_THREADS)
    private:
        OovThreadSafeQueue<T_ThreadQueueItem> mTaskQueue;
        std::vector<std::thread> mWorkerThreads;
        // Only sets the number of threads if there are
        // currently no threads running.
        void setupThreads(int numThreads)
            {
            if(mWorkerThreads.size() == 0)
        	{
		for(int i=0; i<numThreads; i++)
		    {
		    mWorkerThreads.push_back(std::thread(workerThreadProc, this));
		    }
        	}
            }
        static void workerThreadProc(
        	ThreadedWorkQueue<T_ThreadQueueItem, T_ProcessItem> *workQueue)
            {
            T_ThreadQueueItem item;
            while(workQueue->mTaskQueue.waitPop(item))
                {
                static_cast<T_ProcessItem*>(workQueue)->processItem(item);
                }
            }
#endif
    };

#endif
