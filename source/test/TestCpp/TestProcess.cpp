// TestProcess.cpp

#include "TestCpp.h"
#include "../../oovCommon/OovThreadedWaitQueue.h"
#include "../../oovCommon/OovProcess.h"     // For sleepMs
// Including this causes oovbuilder to crash!
//#include "../../oovCommon/OovProcess.h"

class cProcessUnitTest:public cTestCppModule
    {
    public:
        cProcessUnitTest():
            cTestCppModule("Process")
            {}
    };

static cProcessUnitTest gProcessUnitTest;


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


class ThreadedQueue:public ThreadedWorkWaitQueue<std::string, class ThreadedQueue>
    {
    public:
        bool processItem(std::string const &item)
            {
            sleepMs(1000);  // Give time for main thread to push items.
            printf("%s\n", item.c_str());
            fflush(stdout);
            return true;
            }
    };

TEST_F(gProcessUnitTest, ThreadQueueItemsTest)
    {
    ThreadedQueue proc;

    int numThreads = ThreadedQueue::getNumHardwareThreads();
    EXPECT_EQ(numThreads>0, true);
    proc.setupQueue(numThreads);
    proc.addTask("Foo1");
    proc.addTask("Foo2");
    proc.waitForCompletion();
    }

TEST_F(gProcessUnitTest, ThreadQueueEmptyTest)
    {
    ThreadedQueue proc;

    int numThreads = ThreadedQueue::getNumHardwareThreads();
    EXPECT_EQ(numThreads>0, true);
    proc.setupQueue(numThreads);
    proc.waitForCompletion();
    }

