// TestProcess.cpp

#include "TestCpp.h"
#include "../../oovCommon/OovThreadedBackgroundQueue.h"
#include "../../oovCommon/OovThreadedWaitQueue.h"
#include "../../oovCommon/OovProcess.h"     // For sleepMs

class WaitQueueUnitTest:public TestCppModule
    {
    public:
        WaitQueueUnitTest():
            TestCppModule("WaitQueue")
            {}
    };

static WaitQueueUnitTest gWaitQueueUnitTest;

class BackgroundQueueUnitTest:public TestCppModule
    {
    public:
        BackgroundQueueUnitTest():
            TestCppModule("BackgroundQueue")
            {}
    };

static BackgroundQueueUnitTest gBackgroundQueueUnitTest;


/*
TEST_F(gProcessUnitTest, ThreadQueueSingleItemTest)
    {
#if (QUEUED_PROCESSES)
    ThreadedWorkQueue<char> queue;
    char argsIn;
    char argsOut;

    queue.waitPush(argsIn);
    bool didit = queue.waitPop(argsOut);
    EXPECT_EQ(didit, true);
// Doing a second time will hang, because quit must be called on another thread.
//    didit = queue.waitPop(argsOut);
//    EXPECT_EQ(didit, false);
#endif
    }
*/


class ThreadedWaitQueue:public ThreadedWorkWaitQueue<int,
    class ThreadedWaitQueue>
    {
    public:
        ThreadedWaitQueue():
            mItems{0}
            {}
        bool processItem(int item)
            {
            mItems[item]++;
            sleepMs(250);  // Give time for main thread to push items.
            printf("wait processed %d\n", item);
            fflush(stdout);
            return true;
            }
        int mItems[10];
    };

// Create two tasks and wait for both to complete.
// Check that processItem was called twice.
TEST_F(gWaitQueueUnitTest, WaitThreadQueueItemsTest)
    {
    ThreadedWaitQueue queue;

    int numThreads = ThreadedWaitQueue::getNumHardwareThreads();
    EXPECT_EQ(numThreads > 0, true);
    queue.setupQueue(numThreads);
    queue.addTask(0);
    queue.addTask(1);
    queue.waitForCompletion();
    EXPECT_EQ(queue.mItems[0] == 1, true);
    EXPECT_EQ(queue.mItems[1] == 1, true);
    }

// Create zero tasks and wait for completion.
// Check that processItem was not called.
TEST_F(gWaitQueueUnitTest, WaitThreadQueueEmptyTest)
    {
    ThreadedWaitQueue queue;

    int numThreads = ThreadedWaitQueue::getNumHardwareThreads();
    EXPECT_EQ(numThreads > 0, true);
    queue.setupQueue(numThreads);
    queue.waitForCompletion();
    EXPECT_EQ(queue.mItems[0] == 0, true);
    }


class ThreadedBackgroundQueue:public ThreadedWorkBackgroundQueue<
    class ThreadedBackgroundQueue, int>
    {
    public:
        ThreadedBackgroundQueue():
            mItems{0}
            {}
        void processItem(int item)
            {
            mItems[item]++;
            sleepMs(250);  // Give time for main thread to push items.
            printf("back processed %d\n", item);
            fflush(stdout);
            }
        int mItems[10];
    };

// Create two tasks and wait for both to complete.
// Check that processItem was called twice.
TEST_F(gBackgroundQueueUnitTest, BackgroundThreadQueueItemsTest)
    {
    ThreadedBackgroundQueue queue;

    queue.addTask(0);
    EXPECT_EQ(queue.isQueueBusy(), true);
    queue.addTask(1);
    sleepMs(700);   // Wait for both tasks to complete.
    EXPECT_EQ(queue.isQueueBusy(), false);
    queue.stopAndWaitForCompletion();
    EXPECT_EQ(queue.mItems[0] == 1, true);
    EXPECT_EQ(queue.mItems[1] == 1, true);
    EXPECT_EQ(queue.isQueueBusy(), false);
    }

// Create two tasks and stop early.
// Check that processItem was called once.
TEST_F(gBackgroundQueueUnitTest, BackgroundThreadQueueStopTest)
    {
    ThreadedBackgroundQueue queue;

    queue.addTask(0);
    queue.addTask(1);
    sleepMs(100);   // Wait for first task to get started.
    queue.stopAndWaitForCompletion();
    sleepMs(700);   // Wait for both tasks to complete.
    EXPECT_EQ(queue.isQueueBusy(), false);
    EXPECT_EQ(queue.mItems[0] + queue.mItems[1] == 1, true);
    }
