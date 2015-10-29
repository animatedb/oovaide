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


/// Spawns a process and does not wait or retain any control for the process.
/// @param procPath Path to executable
/// @param argv List of arguments, last one must be nullptr.
int spawnNoWait(OovStringRef const procPath, char const * const *argv);
bool setWorkingDirectory(char const *path);


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

/// Provides a listener interface for listening to the stanard out and
/// error of a process.
class OovProcessListener
    {
    public:
        virtual ~OovProcessListener();
        /// Called for some standard out received from the process.
        /// @param out The received data.
        /// @param len The number of bytes of data.
        virtual void onStdOut(OovStringRef const out, size_t len) = 0;
        /// Called for some standard error received from the process.
        /// @param out The received data.
        /// @param len The number of bytes of data.
        virtual void onStdErr(OovStringRef const out, size_t len) = 0;
        /// Called when it is detected that the process completes.
        virtual void processComplete()
            {}
    };

/// Provides a listener interface for detecting whether to continue
/// to loop while doing processing.
class OovTaskContinueListener
    {
    public:
        virtual ~OovTaskContinueListener();
        /// Returns whether to continue processing an item
        virtual bool continueProcessingItem() const = 0;
    };

/// This is used to uniquely identify tasks so that progress can be indicated
/// for multiple ongoing tasks.
typedef int OovTaskStatusListenerId;

/// This is a listener to some long term background task.
/// Warning - this may be called from a background thread and should not make
/// any direct calls to GUI functions.
class OovTaskStatusListener
    {
    public:
        virtual ~OovTaskStatusListener();
        /// This indicates the task started.  The text indicates to the
        /// user what task is being performed.
        /// @param text The text to display to the user about the task that
        ///         is being performed.
        /// @param totalIterations The total number of expected iterations.
        virtual OovTaskStatusListenerId startTask(OovStringRef const &text,
                size_t totalIterations) = 0;
        /// Called periodically to indicate the progress.
        /// @param id The id of the task
        /// @param i The indicator of the amount of progress
        /// @param text Some text to display to the user about the progress
        /// @return false to stop iteration.
        virtual bool updateProgressIteration(OovTaskStatusListenerId id, size_t i,
                OovStringRef const &text=nullptr) = 0;
        /// This is a signal that indicates the task completed.  To end the
        /// task, override updateProcessIteration.
        /// @param id The task id
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
        /// Use to set the received standard out to a file
        /// @param stdoutFp The destination file
        /// @param op Whether to output the file
        void setStdOut(FILE *stdoutFp, OutputPlaces op=OP_OutputFile)
            {
            mStdOutPlace = op;
            mStdoutFp = stdoutFp;
            }
        /// Use to set the received standard error to a file
        /// @param stdoutFp The destination file
        /// @param op Whether to output the file
        void setErrOut(FILE *stdoutFp, OutputPlaces op=OP_OutputFile)
            {
            mStdErrPlace = op;
            mStderrFp = stdoutFp;
            }
        virtual ~OovProcessStdListener()
            {}
        /// Called when some standard output is received.
        virtual void onStdOut(OovStringRef const out, size_t len);
        /// Called when some standard error is received.
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
/// The output is flushed based on time.
class OovProcessBufferedStdListener:public OovProcessStdListener
    {
    public:
        OovProcessBufferedStdListener(InProcMutex &stdMutex):
            mStdMutex(stdMutex), mStdoutTime(0), mStderrTime(0)
            {}
        virtual ~OovProcessBufferedStdListener();
        /// Set the string to uniquely identify the process
        /// @param str The unique string
        void setProcessIdStr(OovStringRef const str);
        /// Called when some standard output is received.
        virtual void onStdOut(OovStringRef const out, size_t len) override;
        /// Called when some standard output is received.
        virtual void onStdErr(OovStringRef const out, size_t len) override;

    private:
// This isn't available yet.
//      typedef std::chrono::high_resolution_clock clock;
        InProcMutex &mStdMutex;     // Prevents output from getting interspersed.
        OovString mStdoutStr;       // A buffer for std out
        OovString mStderrStr;       // A buffer for std error
        OovString mProcessIdStr;    // The process id
        time_t mStdoutTime;         // The time of the last received std out
        time_t mStderrTime;         // The time of the last received std error

        void periodicOutput(FILE *fp, std::string &str, time_t &time);
        /// writeAll = false for write to last cr.
        void output(FILE *fp, std::string &str, bool writeAll);
    };

#ifdef __linux__
/// A child process with pipes for Linux
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
                char const * const *argv, char const *workingDir);
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
/// A child process with pipes for Windows
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
                char const * const *argv, bool showWindows, char const *workingDir);
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

/// This creates a process, but does not create a separate thread for
/// listening to the pipes for the process.
class OovPipeProcess
    {
    public:
        ~OovPipeProcess()
            { childProcessClose(); }
        /// Start a process. See spawn for listening to pipes
        /// @param procPath The path to the process
        /// @param argv The arguments for the process
        /// @param showWindows Set true to show the window, or false to run as
        ///     a background hidden process
        /// @param workingDir The current working directory for the child process.
        bool createProcess(OovStringRef const procPath, char const * const *argv,
                bool showWindows, char const *workingDir=nullptr);
        /// This hangs waiting for the process to finish. It reads the output
        /// pipes from the child process, and sends the output to the listener.
        /// @param listener The listener that will be called when pipe data
        ///     is received
        /// @param exitCode The exit code of the child process
        void childProcessListen(OovProcessListener &listener, int &exitCode);
        /// Sends some data to the standard in of the child process
        /// @param str The data to send to the child
        void childProcessSend(OovStringRef const str);
        /// Close should be cleaner shutdown than kill?
        void childProcessClose();
        /// Kill the child process
        void childProcessKill();

        /// Creates the process, listens to the pipes, and closes when done.
        /// @param procPath The path to the process
        /// @param argv The arguments for the process
        /// @param listener The listener that will be called when pipe data
        ///     is received
        /// @param exitCode The exit code of the child process
        /// @param workingDir The current working directory for the child process.
        bool spawn(OovStringRef const procPath, char const * const * argv,
                OovProcessListener &listener, int &exitCode, char const *workingDir=nullptr);
        /// Checks if the argument length is ok on Windows. On Linux, it
        /// is always ok.
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


/// Creates a thread, and listens to pipes on the background thread.
/// Runs a background process and redirects std output to the listener
class OovBackgroundPipeProcess:public OovPipeProcess
    {
    public:
        enum ThreadStates { TS_Idle, TS_Running, TS_Stopping };
        /// Initialize and set the listener.
        /// If the listener is not set, it must be set with setListener()
        OovBackgroundPipeProcess(OovProcessListener *listener=nullptr);
        ~OovBackgroundPipeProcess();
        /// Set the listener
        /// @param listener The listener that is called when standard out, error
        ///     or the process completes.
        void setListener(OovProcessListener *listener)
            { mListener = listener; }
        /// Start a process. See spawn for listening to pipes
        /// @param procPath The path to the process
        /// @param argv The arguments for the process
        /// @param showWindows Set true to show the window, or false to run as
        ///     a background hidden process
        bool startProcess(OovStringRef const procPath, char const * const *argv,
            bool showWindows);
        /// Check if the process is idle or shutting down.
        bool isIdle() const
            { return(mThreadState == TS_Idle || mThreadState == TS_Stopping); }
        /// Check if the process is running.
        bool isRunning() const
            { return(mThreadState == TS_Running); }
        /// Stop the child process
        void stopProcess();

        /// Do not use. Used by background thread.
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

/// A simple listener for listening to standard input
class OovStdInListener
    {
    public:
        virtual ~OovStdInListener();
        virtual void onStdIn(OovStringRef const in, size_t len) = 0;
        virtual void threadComplete()
            {}
    };

/// Creates a thread and reads stdin. Any string input (terminated by \n)
/// is sent to the listener.
class OovBackgroundStdInListener
    {
    public:
        OovBackgroundStdInListener();
        ~OovBackgroundStdInListener();
        /// This starts the thread. It is not started by default because
        /// it messes with the GDB or Eclipse session to listen to stdin.
        void start();
        /// Set the listener that is called when there is standard input
        void setListener(OovStdInListener *listener)
            { mListener = listener; }

        /// Do not use. Used by background thread.
        void privateBackground();

    private:
        enum ThreadStates { TS_Running, TS_Stopping };
        OovStdInListener *mListener;
        std::thread mThread;
        ThreadStates mThreadState;
    };

#endif /* PORTABLE_H_ */
