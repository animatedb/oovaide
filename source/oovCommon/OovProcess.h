/*
 * OovProcess.h
 *
 *  Created on: Jun 22, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVPROCESS_H_
#define OOVPROCESS_H_

#include <vector>
#include <stdio.h>
#define USE_STD_THREAD 1
#if(USE_STD_THREAD)
#include <thread>
#else
#include <glib.h>       // For GThread
#endif
#ifdef __linux__
#include <memory.h>
#else
#include <windows.h>    // for HANDLE
#endif
#include "OovString.h"

#include "OovProcessArgs.h"


void sleepMs(int ms);

/// @param procPath Path to executable
/// @param argv List of arguments, last one must be nullptr.
int spawnNoWait(OovStringRef const procPath, char const * const *argv);


/// Note that on Linux, this is not recursive.
/// Deprecated: Use std::mutex for C++11.
class InProcMutex
    {
    public:
#ifdef __linux__
        InProcMutex()
            { pthread_mutex_init(&mSection, NULL); }
        ~InProcMutex()
            { pthread_mutex_destroy(&mSection); }
        void enter()
            { pthread_mutex_lock(&mSection); }
        void leave()
            { pthread_mutex_unlock(&mSection); }
private:
        pthread_mutex_t mSection;
#else
        InProcMutex()
            { InitializeCriticalSection(&mSection); }
        ~InProcMutex()
            { DeleteCriticalSection(&mSection); }
        void enter()
            { EnterCriticalSection(&mSection); }
        void leave()
            { LeaveCriticalSection(&mSection); }
    private:
        CRITICAL_SECTION mSection;
#endif
    };

/// Deprecated: Use std::lock_quard for C++11.
class LockGuard
    {
    public:
        LockGuard(InProcMutex &mut):
            mMutex(mut)
            { mMutex.enter(); }
        ~LockGuard()
            { mMutex.leave(); }

    private:
        InProcMutex &mMutex;
    };


class OovProcessListener
    {
    public:
        virtual ~OovProcessListener();
        virtual void onStdOut(OovStringRef const out, size_t len) = 0;
        virtual void onStdErr(OovStringRef const out, size_t len) = 0;
        virtual void processComplete()
            {}
    };

class OovTaskContinueListener
    {
    public:
        virtual ~OovTaskContinueListener();
        virtual bool continueProcessingItem() const = 0;
    };

typedef int OovTaskStatusListenerId;

/// Warning - this may be called from a background thread and should not make
/// any direct calls to GUI functions.
class OovTaskStatusListener
    {
    public:
        virtual ~OovTaskStatusListener();
        /// This indicates the task started.  The text indicates to the
        /// user what task is being performed.
        virtual OovTaskStatusListenerId startTask(OovStringRef const &text,
                size_t totalIterations) = 0;
        /// @return false to stop iteration.
        virtual bool updateProgressIteration(OovTaskStatusListenerId id, size_t i,
                OovStringRef const &text=nullptr) = 0;
        /// This is a signal that indicates the task completed.  To end the
        /// task, override updateProcessIteration.
        virtual void endTask(OovTaskStatusListenerId id) = 0;
    };

/// This listener collects the std output from the child process and
/// sends the output to stdout and stderr.
class OovProcessStdListener:public OovProcessListener
    {
    public:
        enum OutputPlaces { OP_OutputNone=0x00, OP_OutputStd=0x01, OP_OutputFile=0x02,
            OP_OutputStdAndFile=OP_OutputStd|OP_OutputFile
            };
        OovProcessStdListener():
            mStdOutPlace(OP_OutputStd), mStdErrPlace(OP_OutputStd),
            mStdoutFp(stdout), mStderrFp(stderr)
            {}
        void setStdOut(FILE *stdoutFp, OutputPlaces op=OP_OutputFile)
            {
            mStdOutPlace = op;
            mStdoutFp = stdoutFp;
            }
        void setErrOut(FILE *stdoutFp, OutputPlaces op=OP_OutputFile)
            {
            mStdErrPlace = op;
            mStderrFp = stdoutFp;
            }
        virtual ~OovProcessStdListener()
            {}
        virtual void onStdOut(OovStringRef const out, size_t len);
        virtual void onStdErr(OovStringRef const out, size_t len);

    protected:
        OutputPlaces mStdOutPlace;
        OutputPlaces mStdErrPlace;
        FILE *mStdoutFp;
        FILE *mStderrFp;
    };

/// This listener collects the std output from the child process and
/// periodically outputs to stdout and stderr.
/// Since all output is not sent out at the same time, the process ID string
/// is prepended each time there is some output.
/// This listener must be destructed to write all buffered output.
class OovProcessBufferedStdListener:public OovProcessStdListener
    {
    public:
        OovProcessBufferedStdListener(InProcMutex &stdMutex):
            mStdMutex(stdMutex), mStdoutTime(0), mStderrTime(0)
            {}
        virtual ~OovProcessBufferedStdListener();
        void setProcessIdStr(OovStringRef const str);
        virtual void onStdOut(OovStringRef const out, size_t len) override;
        virtual void onStdErr(OovStringRef const out, size_t len) override;

    private:
// This isn't available yet.
//      typedef std::chrono::high_resolution_clock clock;
        InProcMutex &mStdMutex;
        OovString mStdoutStr;
        OovString mStderrStr;
        OovString mProcessIdStr;
        time_t mStdoutTime;
        time_t mStderrTime;

        void periodicOutput(FILE *fp, std::string &str, time_t &time);
        /// writeAll = false for write to last cr.
        void output(FILE *fp, std::string &str, bool writeAll);
    };

#ifdef __linux__
class OovPipeProcessLinux
    {
    public:
        OovPipeProcessLinux();
        ~OovPipeProcessLinux()
            {
            linuxChildProcessKill();
            }
        void linuxClosePipes();
        bool linuxCreatePipes();
        bool linuxCreatePipeProcess(OovStringRef const procPath,
                char const * const *argv);
        void linuxChildProcessListen(OovProcessListener &listener, int &exitCode);
        void linuxChildProcessKill();
        void linuxChildProcessSend(OovStringRef const str);
    private:
        int mChildProcessId;
        enum PipeIndices { P_Read=0, P_Write=1, P_NumIndices=2 };
        // These are named from the parent's perspective.
        int mOutPipe[P_NumIndices];
        int mInPipe[P_NumIndices];
        int mErrPipe[P_NumIndices];
    };
#else
class OovPipeProcessWindows
    {
    public:
        OovPipeProcessWindows():
            mChildStd_IN_Rd(NULL), mChildStd_IN_Wr(NULL),
            mChildStd_OUT_Rd(NULL), mChildStd_OUT_Wr(NULL),
            mChildStd_ERR_Rd(NULL), mChildStd_ERR_Wr(NULL)
            {
            setStatusProcNotRunning();
            }
        bool windowsCreatePipeProcess(OovStringRef const procPath,
                char const * const *argv);
        void windowsChildProcessListen(OovProcessListener &listener, int &exitCode);
        void windowsChildProcessClose();
        bool windowsChildProcessSend(OovStringRef const str);
        void windowsChildProcessKill();

    private:
        PROCESS_INFORMATION mProcInfo;
        HANDLE mChildStd_IN_Rd;
        HANDLE mChildStd_IN_Wr;

        HANDLE mChildStd_OUT_Rd;
        HANDLE mChildStd_OUT_Wr;
        HANDLE mChildStd_ERR_Rd;
        HANDLE mChildStd_ERR_Wr;
        void windowsCloseHandle(HANDLE &h);
        bool windowsCreatePipes();
        void windowsClosePipes();
        bool isProcRunning()
            { return mProcInfo.hProcess != nullptr; }
        void setStatusProcNotRunning()
            { mProcInfo.hProcess = nullptr; }
    };
#endif

class OovPipeProcess
    {
    public:
        ~OovPipeProcess()
            { childProcessClose(); }
        bool createProcess(OovStringRef const procPath, char const * const *argv);
        /// This hangs waiting for the process to finish. It reads the output
        /// pipes from the child process, and sends the output to the listener.
        void childProcessListen(OovProcessListener &listener, int &exitCode);
        void childProcessSend(OovStringRef const str);
        // Close should be cleaner shutdown than kill?
        void childProcessClose();
        void childProcessKill();

        bool spawn(OovStringRef const procPath, char const * const * argv,
                OovProcessListener &listener, int &exitCode);
        bool isArgLengthOk(int len) const
            {
            bool ok = true;
#ifndef __linux__
            ok = (len < 32767);
#endif
            return ok;
            }

    private:
#ifdef __linux__
        OovPipeProcessLinux mPipeProcLinux;
#else
        OovPipeProcessWindows mPipeProcWindows;
#endif
    };


/// Runs a background process and redirects stdio
class OovBackgroundPipeProcess:public OovPipeProcess
    {
    public:
        enum ThreadStates { TS_Idle, TS_Running, TS_Stopping };
        /// If the listener is not set, it must be set with setListener()
        OovBackgroundPipeProcess(OovProcessListener *listener=nullptr);
        ~OovBackgroundPipeProcess();
        void setListener(OovProcessListener *listener)
            { mListener = listener; }
        bool startProcess(OovStringRef const procPath, char const * const *argv);
        bool isIdle() const
            { return(mThreadState == TS_Idle || mThreadState == TS_Stopping); }
        void stopProcess();

        // Do not use. Used by background thread.
        void privateBackground();

    private:
        OovProcessListener *mListener;
#if(USE_STD_THREAD)
        std::thread mThread;
#else
        GThread *mThread;
#endif
        ThreadStates mThreadState;
        int mChildProcessExitCode;
    };

#endif /* PORTABLE_H_ */
