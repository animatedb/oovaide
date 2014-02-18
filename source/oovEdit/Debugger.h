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
#include <algorithm>
#include <vector>

class DebuggerLocation
    {
    public:
	DebuggerLocation(char const * const fn="", int line=-1):
	    mFileOrFuncName(fn), mLineNum(line)
	    {
	    }
	bool operator==(DebuggerLocation const &rhs) const
	    {
	    return(mLineNum == rhs.mLineNum && mFileOrFuncName == rhs.mFileOrFuncName);
	    }
	void setFileLine(char const * const fn, int line)
	    {
	    mFileOrFuncName = fn;
	    mLineNum = line;
	    }
	void setLine(int line)
	    { mLineNum = line; }
	// For GDB, a location can be:
	//	function
	//	filename:linenum
	//	filename:function
	//	*address
	// WARNING: At this time, location can only be filename:linenum. This
	// 	is because breakpoint numbers for clearing breakpoints use
	//	this to find the matching breakpoint.
	std::string getAsString() const;
	OovString mFileOrFuncName;
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
	bool locationMatch(const DebuggerLocation &loc) const
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
	virtual void DebugStopped(DebuggerLocation const &loc) = 0;
    };

class Debugger:public OovProcessListener
    {
    public:
	Debugger();
	void setListener(DebuggerListener &listener)
	    { mDebuggerListener = &listener; }
	void setDebuggee(char const * const debuggee)
	    { mDebuggee = debuggee; }
	void toggleBreakpoint(const DebuggerBreakpoint &br);
	void stepInto();
	void stepOver();
	void resume();
	void interrupt();
	void sendCommand(char const * const command);
	void viewVariable(char const * const variable);
	// Returns empty filename if not stopped.
	DebuggerLocation getStoppedLocation() const;
	DebuggerBreakpoints const &getBreakpoints() const
	    { return mBreakpoints; }

    private:
	enum GdbChildStates { GCS_GdbChildNotRunning, GCS_GdbChildRunning, GCS_GdbChildPaused };
	std::string mDebuggee;
	std::string mGdbOutputBuffer;
	DebuggerBreakpoints mBreakpoints;
	OovBackgroundPipeProcess mBkgPipeProc;
	GdbChildStates mGdbChildState;
	DebuggerListener *mDebuggerListener;
	int mCommandIndex;
	DebuggerLocation mStoppedLocation;
	bool runDebuggerProcess();
	void ensureGdbChildRunning();
	void sendAddBreakpoint(const DebuggerBreakpoint &br);
	void sendDeleteBreakpoint(const DebuggerBreakpoint &br);
	void sendMiCommand(char const * const command);
	void handleResult(const std::string &resultStr);
	virtual void onStdOut(char const * const out, int len);
	virtual void onStdErr(char const * const out, int len);
    };

#endif /* DEBUGGER_H_ */
