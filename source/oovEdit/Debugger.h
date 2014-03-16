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
	DebuggerLocation(char const * const fn="", int line=-1):
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
	void setFileLine(char const * const fn, int line)
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
	DebuggerBreakpoint(char const * const fn="", int line=-1):
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
	virtual void DebugOutput(char const * const str) = 0;
	virtual void DebugStatusChanged() = 0;
    };

class Debugger:public OovProcessListener
    {
    public:
	enum eChangeStatus { CS_None, CS_RunState, CS_Stack, CS_Value };
	Debugger();
	void setListener(DebuggerListener &listener)
	    { mDebuggerListener = &listener; }
	void setDebuggerFilePath(char const * const dbgPath)
	    { mDebuggerFilePath = dbgPath; }
	void setDebuggee(char const * const debuggee)
	    { mDebuggeeFilePath = debuggee; }
	void setDebuggeeArgs(char const * const args)
	    { mDebuggeeArgs = args; }
	void setWorkingDir(char const * const dir)
	    { mWorkingDir = dir; }
	void toggleBreakpoint(const DebuggerBreakpoint &br);
	void stepInto();
	void stepOver();
	void resume();
	void interrupt();	// pause
	void stop();
	void sendCommand(char const * const command);
	void startGetVariable(char const * const variable);
	void startGetStack();
	void startGetMemory(char const * const addr);

	std::string const &getDebuggerFilePath() const
	    { return mDebuggerFilePath; }
	std::string const &getDebuggeeFilePath() const
	    { return mDebuggeeFilePath; }
	DebuggerBreakpoints const &getBreakpoints() const
	    { return mBreakpoints; }

	// The following info must be thread protected
	Debugger::eChangeStatus getChangeStatus();
	GdbChildStates getChildState();
	std::string getStack();
	std::string getVarValue();
	// Returns empty filename if not stopped.
	DebuggerLocation getStoppedLocation();

    private:
	std::string mDebuggerFilePath;
	std::string mDebuggeeFilePath;
	std::string mDebuggeeArgs;
	std::string mWorkingDir;

	DebuggerBreakpoints mBreakpoints;
	OovBackgroundPipeProcess mBkgPipeProc;
	DebuggerListener *mDebuggerListener;
	int mCommandIndex;

	// Thread protected data
	std::string mGdbOutputBuffer;
	std::string mStack;
	std::string mVarValue;
	GdbChildStates mGdbChildState;
	InProcMutex mStatusLock;
	std::queue<Debugger::eChangeStatus> mChangeStatusQueue;
	DebuggerLocation mStoppedLocation;

	bool runDebuggerProcess();
	void ensureGdbChildRunning();
	void sendAddBreakpoint(const DebuggerBreakpoint &br);
	void sendDeleteBreakpoint(const DebuggerBreakpoint &br);
	void sendMiCommand(char const * const command);
	void changeChildState(GdbChildStates state);
	void updateChangeStatus(Debugger::eChangeStatus);
	void handleResult(const std::string &resultStr);
	void handleBreakpoint(const std::string &resultStr);
	void handleStack(const std::string &resultStr);
	void handleValue(const std::string &resultStr);
	virtual void onStdOut(char const * const out, int len);
	virtual void onStdErr(char const * const out, int len);
    };

#endif /* DEBUGGER_H_ */
