/*
 * OovProcess.h
 *
 *  Created on: Jun 22, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVPROCESS_H_
#define OOVPROCESS_H_

#include <vector>
#include <string>
#include <stdio.h>
#define USE_STD_THREAD 1
#if(USE_STD_THREAD)
#include <thread>
#else
#include <glib.h>	// For GThread
#endif
#ifdef __linux__
#include <memory.h>
#else
#include <windows.h>	// for HANDLE
#endif


void sleepMs(int ms);

/// @param procPath Path to executable
/// @param argv List of arguments, last one must be nullptr.
int spawnNoWait(char const * const procPath, char const * const *argv);


/// Note that on Linux, this is not recursive.
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
	virtual ~OovProcessListener()
	    {}
	virtual void onStdOut(char const * const out, int len) = 0;
	virtual void onStdErr(char const * const out, int len) = 0;
	virtual void processComplete()
	    {}
    };

/// This is a listener that defaults to sending the output to the
/// the stdout and stderr.
class OovProcessStdListener:public OovProcessListener
    {
    public:
	enum OutputPlaces { OP_OutputStd, OP_OutputFile, OP_OutputStdAndFile,
	    OP_OutputNone };
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
	virtual void onStdOut(char const * const out, int len);
	virtual void onStdErr(char const * const out, int len);

    protected:
	OutputPlaces mStdOutPlace;
	OutputPlaces mStdErrPlace;
	FILE *mStdoutFp;
	FILE *mStderrFp;
    };

class OovProcessBufferedStdListener:public OovProcessStdListener
    {
    public:
	OovProcessBufferedStdListener(InProcMutex &stdMutex):
	    mStdMutex(stdMutex), mStdoutTime(0), mStderrTime(0)
	    {}
	virtual ~OovProcessBufferedStdListener()
	    {}
	void setProcessIdStr(char const * const str);
	virtual void processComplete();
	virtual void onStdOut(char const * const out, int len);
	virtual void onStdErr(char const * const out, int len);

    private:
// This isn't available yet.
//	typedef std::chrono::high_resolution_clock clock;
	InProcMutex &mStdMutex;
	std::string mStdoutStr;
	std::string mStderrStr;
	std::string mProcessIdStr;
	time_t mStdoutTime;
	time_t mStderrTime;

	void output(FILE *fp, std::string &str, time_t &time);
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
	bool linuxCreatePipeProcess(char const * const procPath,
		char const * const *argv);
	void linuxChildProcessListen(OovProcessListener &listener, int &exitCode);
	void linuxChildProcessKill();
	void linuxChildProcessSend(char const * const str);
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
	bool windowsCreatePipeProcess(char const * const procPath,
		char const * const *argv);
	void windowsChildProcessListen(OovProcessListener &listener, int &exitCode);
	void windowsChildProcessClose();
	bool windowsChildProcessSend(char const * const str);
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
	bool createProcess(char const * const procPath, char const * const *argv);
	/// This hangs waiting for the process to finish. It reads the output
	/// pipes from the child process, and sends the output to the listener.
	void childProcessListen(OovProcessListener &listener, int &exitCode);
	void childProcessSend(char const * const str);
	// Close should be cleaner shutdown than kill?
	void childProcessClose();
	void childProcessKill();

	bool spawn(char const * const procPath, char const * const *argv,
		OovProcessListener &listener, int &exitCode);

    private:
#ifdef __linux__
	OovPipeProcessLinux mPipeProcLinux;
#else
	OovPipeProcessWindows mPipeProcWindows;
#endif
    };

/// Builds arguments for a child process.
class OovProcessChildArgs
    {
    public:
	OovProcessChildArgs()
	    { clearArgs(); }

	// WARNING: The arg[0] must be added.
	void addArg(char const * const argStr);
	void clearArgs()
	    { mArgStrings.clear(); }
	char const * const *getArgv() const;
	const int getArgc() const
	    { return mArgStrings.size(); }
	std::string getArgsAsStr() const;
	void printArgs(FILE *fh) const;

    private:
	std::vector<std::string> mArgStrings;
	mutable std::vector<char const*> mArgv;	// These are created temporarily during getArgv
    };


/// Runs a background process and redirects stdio
class OovBackgroundPipeProcess:public OovPipeProcess
    {
    public:
	enum ThreadStates { TS_Idle, TS_Running, TS_Stopping };
	OovBackgroundPipeProcess(OovProcessListener &listener);
	~OovBackgroundPipeProcess();
	bool startProcess(char const * const procPath, char const * const *argv);
	bool isIdle()
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
