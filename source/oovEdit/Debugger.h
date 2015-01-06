/*
 * Debugger.h
 *
 *  Created on: Feb 15, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include "OovProcess.h"
#include "OovString.h"
#include "FilePath.h"
#include "DebugResult.h"
#include <algorithm>
#include <vector>
#include <queue>


enum GdbChildStates { GCS_GdbChildNotRunning, GCS_GdbChildRunning, GCS_GdbChildPaused };

class DebuggerLocation
    {
    public:
	DebuggerLocation(OovStringRef const fn="", int line=-1):
	    mFileName(fn), mLineNum(line)
	    {
	    }
	void clear()
	    { mLineNum = -1; }
	bool isEmpty() const
	    { return(mLineNum == -1); }
	bool operator==(DebuggerLocation const &rhs) const
	    {
	    return(mLineNum == rhs.mLineNum &&
		    StringCompareNoCase(mFileName.c_str(), rhs.mFileName.c_str()) == 0);
	    }
	void setFileLine(OovStringRef const fn, int line)
	    {
	    mFileName = fn;
	    mLineNum = line;
	    }
	void setLine(int line)
	    { mLineNum = line; }
	int getLine() const
	    { return mLineNum; }
	std::string const &getFilename() const
	    { return mFileName; }
	// For GDB, a location can be:
	//	function
	//	filename:linenum
	//	filename:function
	//	*address
	// WARNING: At this time, location can only be filename:linenum. This
	// 	is because breakpoint numbers for clearing breakpoints use
	//	this to find the matching breakpoint.
	//	This returns the format: 	filenane:linenum
	std::string getAsString() const;

    private:
	OovString mFileName;
	int mLineNum;
    };

class DebuggerBreakpoint:public DebuggerLocation
    {
    public:
	DebuggerBreakpoint(OovStringRef const fn="", int line=-1):
	    DebuggerLocation(fn, line), mBreakpointNumber(-1)
	    {}
	int mBreakpointNumber;
    };

class DebuggerBreakpoints:public std::vector<DebuggerBreakpoint>
    {
    public:
	bool anyLocationsMatch(const DebuggerLocation &loc) const
	    { return(std::find(begin(), end(), loc) != end()); }
	void setBreakpointNumber(const DebuggerLocation &loc, int bn)
	    {
	    auto iter = std::find(begin(), end(), loc);
	    if(iter != end())
		(*iter).mBreakpointNumber = bn;
	    }
    };

class DebuggerListener
    {
    public:
	virtual ~DebuggerListener()
	    {}
	/// All output from the debugger process is sent to this listener.
	/// Set this up by calling Debugger::setListener.
	virtual void DebugOutput(OovStringRef const str) = 0;
	/// This is called after the state has changed. Use
	/// Debugger::getChangeStatus to get the status.
	virtual void DebugStatusChanged() = 0;
    };

class Debugger:public OovProcessListener
    {
    public:
	enum eChangeStatus { CS_None, CS_RunState, CS_Stack, CS_Value };
	Debugger();
	void setListener(DebuggerListener &listener)
	    { mDebuggerListener = &listener; }
	void setDebuggerFilePath(OovStringRef const dbgPath)
	    { mDebuggerFilePath = dbgPath; }
	/// The debuggee is the program being debugged.
	void setDebuggee(OovStringRef const debuggee)
	    { mDebuggeeFilePath = debuggee; }
	void setDebuggeeArgs(OovStringRef const args)
	    { mDebuggeeArgs = args; }
	/// This is the working directory of the debuggee.
	void setWorkingDir(OovStringRef const dir)
	    { mWorkingDir = dir; }
	/// @param frameLine One line of text returned from getStack
	///	Lines are separated with \n.
	void setStackFrame(OovStringRef const frameLine);
	void toggleBreakpoint(const DebuggerBreakpoint &br);
	void stepInto();
	void stepOver();
	void resume();
	void interrupt();	// pause
	void stop();
	/// This allows the user to send a typed in debugger command.
	void sendCommand(OovStringRef const command);
	void startGetVariable(OovStringRef const variable);
	void startGetStack();
	void startGetMemory(OovStringRef const addr);

	std::string const &getDebuggerFilePath() const
	    { return mDebuggerFilePath; }
	std::string const &getDebuggeeFilePath() const
	    { return mDebuggeeFilePath; }
	DebuggerBreakpoints const &getBreakpoints() const
	    { return mBreakpoints; }

	// The following info must be thread protected
	Debugger::eChangeStatus getChangeStatus();
	GdbChildStates getChildState();
	OovString getStack();
	OovString getVarValue();
	// Returns empty filename if not stopped.
	DebuggerLocation getStoppedLocation();

    private:
	OovString mDebuggerFilePath;
	OovString mDebuggeeFilePath;
	OovString mDebuggeeArgs;
	OovString mWorkingDir;

	DebuggerBreakpoints mBreakpoints;
	OovBackgroundPipeProcess mBkgPipeProc;
	DebuggerListener *mDebuggerListener;
	int mCommandIndex;
	// Frame numbers start at 0
	int mFrameNumber;
	// Thread numbers start at 1
	int mCurrentThread;

	// Thread protected data
	std::string mGdbOutputBuffer;
	OovString mStack;
	std::string mVarValue;
	GdbChildStates mGdbChildState;
	InProcMutex mStatusLock;
	std::queue<Debugger::eChangeStatus> mChangeStatusQueue;
	DebuggerLocation mStoppedLocation;

	void resetFrameNumber()
	    { mFrameNumber = 0; }
	bool runDebuggerProcess();
	void ensureGdbChildRunning();
	void sendAddBreakpoint(const DebuggerBreakpoint &br);
	void sendDeleteBreakpoint(const DebuggerBreakpoint &br);
	void sendMiCommand(OovStringRef const command);
	void changeChildState(GdbChildStates state);
	void updateChangeStatus(Debugger::eChangeStatus);
	void handleResult(const std::string &resultStr);
	void handleBreakpoint(const std::string &resultStr);
	void handleStack(const std::string &resultStr);
	void handleValue(const std::string &resultStr);
	virtual void onStdOut(OovStringRef const out, int len);
	virtual void onStdErr(OovStringRef const out, int len);
    };

#endif /* DEBUGGER_H_ */
