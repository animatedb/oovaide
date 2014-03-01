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
#include <glib.h>	// For GThread
#ifdef __linux__
#include <memory.h>
#else
#include <windows.h>	// for HANDLE
#endif
//#ifndef __linux		// These are needed for mingw 4.0, but it doesn't work with clang
//#define off64_t _off64_t
//#define off_t int
//#endif


void sleepMs(int ms);

/// @param procPath Path to executable
/// @param argv List of arguments, last one must be nullptr.
int spawnNoWait(char const * const procPath, char const * const *argv);

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

// This is a listener that defaults to sending the output to the
// the stdout and stderr.
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
    private:
	OutputPlaces mStdOutPlace;
	OutputPlaces mStdErrPlace;
	FILE *mStdoutFp;
	FILE *mStderrFp;
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
	    {
	    clearArgs();
	    }

	// WARNING: The arg[0] must be added.
	void addArg(char const * const argStr);
	void clearArgs()
	    {
	    mArgv.clear();
	    mArgv.push_back(nullptr);	// Add one for end null
	    argStrings.clear();
	    }
	char const * const *getArgv() const
	    { return const_cast<char const * const *>(&mArgv[0]); }
	const int getArgc() const
	    { return mArgv.size()-1; }
	void printArgs(FILE *fh) const;

    private:
	std::vector<char*> mArgv;
	// These can hold temporary modified strings during the call to run the process.
	std::vector<std::string> argStrings;
    };


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
	GThread *mThread;
	ThreadStates mThreadState;
	int mChildProcessExitCode;
    };

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

#endif /* PORTABLE_H_ */
