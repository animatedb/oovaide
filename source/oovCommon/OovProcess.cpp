/*
 *
 *  Created on: Jun 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "OovProcess.h"
#include "File.h"
#include "Debug.h"
#ifdef __linux__
#include <spawn.h>
#include <unistd.h>	// for usleep
#include <stdlib.h>	// for mktemp
#include <poll.h>
#include <sys/wait.h>
#include "string.h"
#else
#include <process.h>
#include <windows.h>	// for Sleep
#include <stdio.h>	// For _popen
#include <vector>
#endif
#include <sys/stat.h>
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
// Prevent "error: 'off_t' has not been declared"
#define off_t _off_t
#include <unistd.h>
#include <string>
#include <time.h>

#define DEBUG_PROC 0
#if(DEBUG_PROC)
class ProcDebugFile:public DebugFile
    {
    public:
	void open()
	    {
	    char fn[60];
	    snprintf(fn, sizeof(fn), "DebugProcess-%d.txt", getpid());
	    DebugFile::open(fn);
	    }
    };
static ProcDebugFile sDbgFile;
#endif

void sleepMs(int ms)
    {
#ifdef __linux__
    usleep(ms*1000);
#else
    Sleep(ms);
#endif
    }

// Removes clang warning:
// warning: 'OovProcessListener' has no out-of-line virtual method definitions;
// its vtable will be emitted in every translation unit
OovProcessListener::~OovProcessListener()
    {}
OovTaskContinueListener::~OovTaskContinueListener()
    {}
OovTaskStatusListener::~OovTaskStatusListener()
    {}

// Redirected output
// http://msdn.microsoft.com/en-us/library/ms682499%28VS.85%29.aspx

#ifdef __linux__



static void linuxClosePipe(int &fd)
    {
    if(fd != -1)
	{
#if(DEBUG_PROC)
	sDbgFile.printflush("linuxClosePipe %d\n", fd);
#endif
	close(fd);
	fd = -1;
	}
    }

OovPipeProcessLinux::OovPipeProcessLinux():
    mChildProcessId(0)
    {
#if(DEBUG_PROC)
    sDbgFile.open();
    sDbgFile.printflush("OovPipeProcessLinux\n");
#endif
    for(int i=0; i<2; i++)
	{
	mOutPipe[i] = -1;
	mInPipe[i] = -1;
	mErrPipe[i] = -1;
	}
    }

void OovPipeProcessLinux::linuxClosePipes()
    {
    for(int i=0; i<P_NumIndices; i++)
	{
	linuxClosePipe(mOutPipe[i]);
	linuxClosePipe(mInPipe[i]);
	linuxClosePipe(mErrPipe[i]);
	}
    }

bool OovPipeProcessLinux::linuxCreatePipes()
    {
    bool success = false;
    linuxClosePipes();
#if(DEBUG_PROC)
    sDbgFile.printflush("linuxCreatePipes\n");
#endif
    if(pipe(mOutPipe) == 0)	// Where parent is going to write
	{
	if(pipe(mInPipe) == 0)	// Where parent is going to read
	    {
	    if(pipe(mErrPipe) == 0)	// Where parent is going to read
		{
#if(DEBUG_PROC)
		sDbgFile.printflush("linuxOpenPipes %d %d\n", mOutPipe[0], mOutPipe[1]);
		sDbgFile.printflush("linuxOpenPipes %d %d\n", mInPipe[0], mInPipe[1]);
		sDbgFile.printflush("linuxOpenPipes %d %d\n", mErrPipe[0], mErrPipe[1]);
#endif
		success = true;
		}
	    }
	}
    if(!success)
	linuxClosePipes();
    return success;
    }


bool OovPipeProcessLinux::linuxCreatePipeProcess(OovStringRef const procPath,
	char const * const *argv)
    {
    // from http://jineshkj.wordpress.com/2006/12/22/how-to-capture-stdin-stdout-and-stderr-of-child-program
    bool success = linuxCreatePipes();
    if(success)
	{
	int pid = fork();
	if(pid == 0)	// Is this the child?
	    {
	    close(STDOUT_FILENO);
	    close(STDIN_FILENO);
	    close(STDERR_FILENO);
#if(DEBUG_PROC)
	    sDbgFile.printflush("dup pipe %d\n", mOutPipe[P_Read]);
	    sDbgFile.printflush("dup pipe %d\n", mInPipe[P_Write]);
	    sDbgFile.printflush("dup pipe %d\n", mErrPipe[P_Write]);
#endif
	    dup2(mOutPipe[P_Read], STDIN_FILENO); 	// Move read end parent's output pipe to stdin
	    dup2(mInPipe[P_Write], STDOUT_FILENO); 	// Move the write end of parent's input pipe to stdout
	    dup2(mErrPipe[P_Write], STDERR_FILENO); 	// Move the write end of parent's input error pipe to stderr
	    // Not required for the child since std handles have all needed pipe handles.
	    linuxClosePipes();
	    success = (execvp(procPath, const_cast<char**>(argv)) != -1);
	    if(!success)
		{
		fprintf(stderr, "Unable to run process %s\n", procPath.getStr());
		}
//	    pause();
	    _exit(0);
	    }
	else if(pid > 0)		// This is the parent
	    {
	    mChildProcessId = pid;
#if(DEBUG_PROC)
	    sDbgFile.printflush("linuxCreatePipeProc %d\n", mChildProcessId);
#endif
	    linuxClosePipe(mOutPipe[P_Read]); // These are being used by the child
	    linuxClosePipe(mInPipe[P_Write]);
	    linuxClosePipe(mErrPipe[P_Write]);
	    }
	else
	    success = false;
	}
    return success;
    }

void OovPipeProcessLinux::linuxChildProcessListen(OovProcessListener &listener, int &exitCode)
    {
//    write(mOutPipes[P_Write], "\n", 1); // Write to child's stdin
    struct pollfd rfds[2];
    rfds[0].fd = mInPipe[P_Read];
    rfds[1].fd = mErrPipe[P_Read];
    rfds[0].events = POLLIN;
    rfds[1].events = POLLIN;

    pid_t pidStat;

#if(DEBUG_PROC)
    sDbgFile.printflush("linuxChildProcessListen %d\n", mChildProcessId);
#endif
    siginfo_t siginfo;
    while((pidStat = waitid(P_PID, mChildProcessId, &siginfo, WEXITED | WNOHANG)) != -1)
	{
#if(DEBUG_PROC)
	sDbgFile.printflush("linuxChildProcessListen pidstat %d\n", pidStat);
	// 17 0 1 means exited.
	sDbgFile.printflush("%d %d %d\n", siginfo.si_signo, siginfo.si_errno, siginfo.si_code);
#endif
	if(mChildProcessId == pidStat)
	    {
	    if(siginfo.si_signo == SIGCHLD)
		{
		break;
		}
	    }
	rfds[0].revents = 0;
	rfds[1].revents = 0;
#define PPOLL 1
#if(PPOLL)
        sigset_t signals;
        sigemptyset(&signals);
        sigaddset(&signals, POLLIN);
        timespec waittime;
        waittime.tv_sec = 1;
        waittime.tv_nsec = 0;
	int stat = ppoll(rfds, 2, &waittime, &signals);
#else
	int stat = poll(rfds, 2, 250);
#endif
	if(stat > 0)
	    {
#if(!PPOLL)
            bool didSomething = false;
#endif
#if(DEBUG_PROC)
	    sDbgFile.printflush("linuxChildProcessListen stat %d\n", stat);
#endif
	    for(int i=0; i<2; i++)
		{
		if(rfds[i].revents & POLLIN)
		    {
#if(!PPOLL)
		    didSomething = true;
#endif
                    char buf[500];
#if(DEBUG_PROC)
                    sDbgFile.printflush("linuxChildProcessListen read start %d\n", i);
#endif
                    int size = read(rfds[i].fd, buf, sizeof(buf));
#if(DEBUG_PROC)
                    sDbgFile.printflush("linuxChildProcessListen read %d\n", size);
#endif
                    if(size > 0)
			 {
			 if(rfds[i].fd == mInPipe[P_Read])
                            listener.onStdOut(buf, size);
			 else
                            listener.onStdErr(buf, size);
			 }
#if(DEBUG_PROC)
                    sDbgFile.printflush("linuxChildProcessListen done read %d\n", size);
#endif
		    }
		}
	    }
#if(!PPOLL)
	if(!didSomething)
	    sleepMs(10);
#endif
	}
#if(DEBUG_PROC)
    sDbgFile.printflush("linuxChildProcessListen - done waiting\n");
#endif
    linuxClosePipe(mOutPipe[P_Write]);
    linuxClosePipe(mInPipe[P_Read]);
    linuxClosePipe(mErrPipe[P_Read]);
    // If the error pipe has "Unable to run process..." then this should
    // actually return an error.
    int waitStatus;
    wait(&waitStatus);
    if(WIFEXITED(waitStatus) == 0)
	{
	exitCode = 0;
	}
    else
	exitCode = waitStatus;
#if(DEBUG_PROC)
    sDbgFile.printflush("linuxChildProcessListen - done\n");
#endif
    }

void OovPipeProcessLinux::linuxChildProcessSend(OovStringRef const str)
    {
    // Junk code:
    // Compile error for not checking write return, so instead, retry once
    for(int i=0; i<2; i++)
	{
	ssize_t size = write(mOutPipe[P_Write], str, strlen(str));
	if(size > 0)
	    break;
	}
    }

void OovPipeProcessLinux::linuxChildProcessKill()
    {
#if(DEBUG_PROC)
    sDbgFile.printflush("linuxChildProcessKill %d\n", mChildProcessId);
#endif
    if(mChildProcessId != 0)
	{
	kill(mChildProcessId, SIGTERM);
	sleep(2);
	kill(mChildProcessId, SIGKILL);
	mChildProcessId = 0;
	}
    }

static bool addEnvItem(OovStringRef envName, OovString &envStr)
    {
    char *val = getenv(envName.getStr());
    if(val)
	{
	envStr += envName;
	envStr += "=";
	envStr += val;
	}
    return(val != nullptr);
    }

static int linuxSpawnNoWait(OovStringRef const procPath, char const * const *argv)
    {
    pid_t pid;
    char const *env[5];
    OovString envStrs[5];
    int envCount = 0;

    if(addEnvItem("DISPLAY", envStrs[envCount]))
	{
	env[envCount] = envStrs[envCount].getStr();
	envCount++;
	}
    if(addEnvItem("XAUTHORITY", envStrs[envCount]))
	{
	env[envCount] = envStrs[envCount].getStr();
	envCount++;
	}
    env[envCount] = nullptr;
    int ret = posix_spawnp(&pid, procPath, nullptr, nullptr,
	const_cast<char * const *>(argv), const_cast<char * const *>(env));
    return ret;
    }

#else

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
static bool windowsChildProcessCreate(OovStringRef const szCmdline,
	HANDLE hChildStd_IN_Rd,
	HANDLE hChildStd_OUT_Wr, HANDLE hChildStd_ERR_Wr,
	PROCESS_INFORMATION &piProcInfo)
    {
    STARTUPINFO siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_ERR_Wr;
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    siStartInfo.wShowWindow = SW_HIDE;

#if(DEBUG_PROC)
    sDbgFile.printflush("windowsChildProcessCreate %s\n", szCmdline);
#endif
    BOOL bSuccess = CreateProcess(NULL, const_cast<char*>(szCmdline.getStr()),
	NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    return bSuccess;
    }

static bool windowsPeekAndReadFile(HANDLE readHandle, bool writeStdout, bool &gotData,
	OovProcessListener &listener)
    {
    DWORD dwRead;
    DWORD dwAvail;
    const int BUFSIZE = 4096;
    CHAR chBuf[BUFSIZE];

    gotData = false;
    BOOL bSuccess = PeekNamedPipe(readHandle, NULL, 0, &dwRead, &dwAvail, NULL);
    if(bSuccess && dwAvail > 0)
	{
	if(dwAvail > 4096)
	    dwAvail = 4096;
	bSuccess = ReadFile(readHandle, chBuf, dwAvail, &dwRead, NULL);
	}
    if(bSuccess)
	{
	if(dwRead != 0)
	    {
	    gotData = true;
	    if(writeStdout)
		listener.onStdOut(chBuf, dwRead);
	    else
		listener.onStdErr(chBuf, dwRead);
	    }
	}
    else
	{
	int code = GetLastError();
	if(code == ERROR_BROKEN_PIPE)
	    bSuccess = false;
	}
    return bSuccess;
    }

class CmdLine
    {
    public:
	CmdLine(OovStringRef const /*procPath*/, char const * const *argv)
	    {
	    for(int i=0; ; i++)
		{
		if(i != 0)
		    cmdStr += ' ';
		if(argv[i] == nullptr)
		    break;
		cmdStr += argv[i];
		}
	    }
	OovStringRef const getCmdStr()
	    { return cmdStr; }
    private:
	std::string cmdStr;
    };

bool OovPipeProcessWindows::windowsCreatePipes()
    {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    bool success = false;

    windowsClosePipes();
    if (CreatePipe(&mChildStd_IN_Rd, &mChildStd_IN_Wr, &saAttr, 0))
	{
	if (CreatePipe(&mChildStd_OUT_Rd, &mChildStd_OUT_Wr, &saAttr, 0))
	    {
	    if(CreatePipe(&mChildStd_ERR_Rd, &mChildStd_ERR_Wr, &saAttr, 0))
		{
		// Ensure the read handle to the pipe for STDOUT is not inherited.
		if(SetHandleInformation(mChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		    {
		    if(SetHandleInformation(mChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0))
			{
			if(SetHandleInformation(mChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
			    success = true;
			}
		    }
		}
	    }
	}
    if(!success)
	{
	windowsClosePipes();
	}
    return success;
    }

bool OovPipeProcessWindows::windowsCreatePipeProcess(OovStringRef const procPath,
	char const * const *argv)
    {
    /// @todo - procPath is not used.
    CmdLine cmdLine(procPath, argv);
    bool success = false;
    if(windowsCreatePipes())
	{
	success = windowsChildProcessCreate(cmdLine.getCmdStr(),
		mChildStd_IN_Rd, mChildStd_OUT_Wr, mChildStd_ERR_Wr, mProcInfo);
	}
    return success;
    }

void OovPipeProcessWindows::windowsChildProcessListen(OovProcessListener &listener, int &exitCode)
    {
    exitCode = -1;
    bool exit = false;
    bool gotData1 = false;
    bool gotData2 = false;
    while(!exit)
	{
	DWORD dwExitCode;
	if(GetExitCodeProcess(mProcInfo.hProcess, &dwExitCode))
	    {
	    if(dwExitCode != STILL_ACTIVE)
		{
		exitCode = dwExitCode;
		exit = true;
		// Read the pipes one last time before quitting the loop.
		}
	    }
	do
	    {
	    if(!windowsPeekAndReadFile(mChildStd_OUT_Rd, true, gotData1, listener))
		{
		exit = true;
		}
	    if(!windowsPeekAndReadFile(mChildStd_ERR_Rd, false, gotData2, listener))
		{
		exit = true;
		}
	    } while(gotData1 || gotData2);
	if(!gotData1 && !gotData2)
	    sleepMs(10);
	}
    }

void OovPipeProcessWindows::windowsChildProcessClose()
    {
    if(isProcRunning())
	{
	CloseHandle(mProcInfo.hProcess);
	CloseHandle(mProcInfo.hThread);
	windowsClosePipes();
	setStatusProcNotRunning();
	}
    }

void OovPipeProcessWindows::windowsClosePipes()
    {
    windowsCloseHandle(mChildStd_ERR_Rd);
    windowsCloseHandle(mChildStd_ERR_Wr);
    windowsCloseHandle(mChildStd_OUT_Rd);
    windowsCloseHandle(mChildStd_OUT_Wr);
    windowsCloseHandle(mChildStd_IN_Rd);
    windowsCloseHandle(mChildStd_IN_Wr);
    }

bool OovPipeProcessWindows::windowsChildProcessSend(OovStringRef const str)
    {
    DWORD written;
    BOOL success = WriteFile(mChildStd_IN_Wr, str, strlen(str), &written, NULL);
#if(DEBUG_PROC)
    if(!success)
	{
	DWORD err = GetLastError();
	sDbgFile.printflush("WriteFile Error %d\n", err);
	}
#endif
    return(success == TRUE);
    }

void OovPipeProcessWindows::windowsCloseHandle(HANDLE &h)
    {
    if(h != NULL)
	{
	CloseHandle(h);
	h = NULL;
	}
    }

void OovPipeProcessWindows::windowsChildProcessKill()
    {
    // This is not a clean shutdown of Windows apps.
    sleepMs(1000);
    TerminateProcess(mProcInfo.hProcess, 0);
    }

#endif

bool OovPipeProcess::createProcess(OovStringRef const procPath,
	char const * const *argv)
    {
#ifdef __linux__
    bool success = mPipeProcLinux.linuxCreatePipeProcess(procPath, argv);
#else
    bool success = mPipeProcWindows.windowsCreatePipeProcess(procPath, argv);
#endif
    return success;
    }

void OovPipeProcess::childProcessListen(OovProcessListener &listener, int &exitCode)
    {
#ifdef __linux__
    mPipeProcLinux.linuxChildProcessListen(listener, exitCode);
#else
    mPipeProcWindows.windowsChildProcessListen(listener, exitCode);
#endif
    listener.processComplete();
    }

void OovPipeProcess::childProcessSend(OovStringRef const str)
    {
#ifdef __linux__
    mPipeProcLinux.linuxChildProcessSend(str);
#else
    mPipeProcWindows.windowsChildProcessSend(str);
#endif
    }

void OovPipeProcess::childProcessClose()
    {
#ifdef __linux__
    mPipeProcLinux.linuxClosePipes();
#else
    mPipeProcWindows.windowsChildProcessClose();
#endif
    }

void OovPipeProcess::childProcessKill()
    {
#ifdef __linux__
    mPipeProcLinux.linuxChildProcessKill();
#else
    mPipeProcWindows.windowsChildProcessKill();
#endif
    }

bool OovPipeProcess::spawn(OovStringRef const procPath, char const * const * argv,
	OovProcessListener &listener, int &exitCode)
    {
    exitCode = -1;

    bool success = createProcess(procPath, argv);
    if(success)
	{
	childProcessListen(listener, exitCode);
	childProcessClose();
	}
    return success;
    }

int spawnNoWait(OovStringRef const procPath, char const * const *argv)
    {
    int ret = 0;
#ifdef __linux__
    ret = linuxSpawnNoWait(procPath.getStr(), argv);
#else
    ret = _spawnv(_P_NOWAIT, procPath.getStr(), argv);
#endif
/*
    switch(ret)
	{
	case E2BIG: printf("E2BIG\n"); break;
	case EINVAL: printf("EINVAL\n"); break;
	case ENOENT: printf("ENOENT\n"); break;
	case ENOEXEC: printf("ENOEXEC\n"); break;
	case -1073741511: printf("DLL Entry Missing\n"); break;
	}
*/
    return ret;
    }

void OovProcessStdListener::onStdOut(OovStringRef const out, size_t len)
    {
    if(mStdOutPlace == OP_OutputStd || mStdOutPlace == OP_OutputStdAndFile)
	fprintf(stdout, "%s", OovString(out, len).getStr());
    if(mStdOutPlace == OP_OutputFile || mStdOutPlace == OP_OutputStdAndFile)
	fprintf(mStdoutFp, "%s", OovString(out, len).getStr());
    }

void OovProcessStdListener::onStdErr(OovStringRef const out, size_t len)
    {
    if(mStdErrPlace == OP_OutputStd || mStdErrPlace == OP_OutputStdAndFile)
	fprintf(stderr, "%s", OovString(out, len).getStr());
    if(mStdErrPlace == OP_OutputFile || mStdErrPlace == OP_OutputStdAndFile)
	fprintf(mStderrFp, "%s", OovString(out, len).getStr());
    }


////////

OovProcessBufferedStdListener::~OovProcessBufferedStdListener()
    {
    output(mStdoutFp, mStdoutStr, true);
    output(mStderrFp, mStderrStr, true);
    }

void OovProcessBufferedStdListener::onStdOut(OovStringRef const out, size_t len)
    {
	{
	LockGuard lock(mStdMutex);
	if((mStdErrPlace & OP_OutputStd) || (mStdErrPlace & OP_OutputFile))
	    mStdoutStr += OovString(out, len);
	}
    periodicOutput(mStdoutFp, mStdoutStr, mStdoutTime);
    }

void OovProcessBufferedStdListener::onStdErr(OovStringRef const out, size_t len)
    {
	{
	LockGuard lock(mStdMutex);
	if((mStdErrPlace & OP_OutputStd) || (mStdErrPlace & OP_OutputFile))
	    mStderrStr += std::string(out, len);
	}
    periodicOutput(mStderrFp, mStderrStr, mStderrTime);
    }

void OovProcessBufferedStdListener::setProcessIdStr(OovStringRef const str)
    {
    LockGuard lock(mStdMutex);
    mProcessIdStr = str;
    }

void OovProcessBufferedStdListener::periodicOutput(FILE *fp, std::string &str, time_t &refTime)
    {

    time_t curTime;
    time(&curTime);
    if(refTime == 0)
	{
	refTime = curTime;
	}
    if(curTime-refTime > 0)
//    if(str.length() > 20000)
	{
	output(fp, str, false);
	refTime = curTime;
	}
    }

void OovProcessBufferedStdListener::output(FILE *fp, std::string &str, bool writeAll)
    {
    LockGuard lock(mStdMutex);
    if(str.length())
	{
	OovString tempStr;
	if(writeAll)
	    {
	    tempStr = str;
	    str.clear();
	    }
	else
	    {
	    size_t lastCrPos = str.rfind('\n');
	    OovString tempStr = str.substr(0, lastCrPos);
	    str.erase(0, lastCrPos);
	    }

	fprintf(fp, "%s", mProcessIdStr.getStr());
	fprintf(fp, "%s", tempStr.getStr());
	fflush(fp);
	}
    }


#if(USE_STD_THREAD)
static void BackgroundThreadFunc(OovBackgroundPipeProcess *proc)
    {
    proc->privateBackground();
    }
#else
static gpointer BackgroundThreadFunc(gpointer data)
    {
    OovBackgroundPipeProcess *backProc = reinterpret_cast<OovBackgroundPipeProcess *>(data);
    backProc->privateBackground();
    return 0;
    }
#endif

OovBackgroundPipeProcess::OovBackgroundPipeProcess(OovProcessListener *listener):
	mListener(listener),
#if(USE_STD_THREAD)
#else
            mThread(nullptr),
#endif
        mThreadState(TS_Idle),
	mChildProcessExitCode(-1)
    {
#if(DEBUG_PROC)
	sDbgFile.open();
	sDbgFile.printflush("bkg-OovBackgroundPipeProcess\n");
#endif
    }

OovBackgroundPipeProcess::~OovBackgroundPipeProcess()
    {
    stopProcess();
#if(DEBUG_PROC)
	sDbgFile.printflush("bkg-~OovBackgroundPipeProcess - done\n");
#endif
    }

bool OovBackgroundPipeProcess::startProcess(OovStringRef const procPath,
	char const * const *argv)
    {
    bool success = false;
    if(mThreadState == TS_Stopping)
	{
	stopProcess();
	}
    if(isIdle())
	{
#if(DEBUG_PROC)
	sDbgFile.printflush("bkg-startProcess\n");
#endif
#if(USE_STD_THREAD)
        mThread = std::thread(BackgroundThreadFunc, this);
#else
	mThread = g_thread_new("BackThread", BackgroundThreadFunc, this);
#endif
	success = OovPipeProcess::createProcess(procPath, argv);
	mThreadState = TS_Running;
#if(DEBUG_PROC)
	sDbgFile.printflush("bkg-startProcess - TS_Running\n");
#endif
	}
    return success;
    }

void OovBackgroundPipeProcess::stopProcess()
    {
    if(mThreadState == TS_Running || mThreadState == TS_Stopping)
	{
#if(DEBUG_PROC)
	sDbgFile.printflush("bkg-stopProcess - join\n");
#endif
	childProcessKill();
#if(USE_STD_THREAD)
        mThread.join();
#else
	g_thread_join(mThread);
#endif
	mThreadState = TS_Idle;
#if(DEBUG_PROC)
	sDbgFile.printflush("bkg-stopProcess - TS_NotRunning\n");
#endif
	}
    }

void OovBackgroundPipeProcess::privateBackground()
    {
    while(mThreadState != TS_Running)
	{
	sleepMs(10);
	}
    if(mThreadState == TS_Running)
	{
#if(DEBUG_PROC)
	sDbgFile.printflush("bkg-background - listening\n");
#endif
	if(mListener)
	    {
	    childProcessListen(*mListener, mChildProcessExitCode);
	    }
	childProcessClose();
#if(DEBUG_PROC)
	sDbgFile.printflush("bkg-background - T_Stopping\n");
#endif
	mThreadState = TS_Stopping;
#if(USE_STD_THREAD)
#else
	g_thread_exit(0);
#endif
	}
    }
