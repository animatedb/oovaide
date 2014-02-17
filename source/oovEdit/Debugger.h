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
#include <vector>

class DebuggerListener
    {
    public:
	virtual ~DebuggerListener()
	    {}
	virtual void DebugOutput(char const * const str) = 0;
	virtual void DebugStopped(char const * const fileName, int lineNum) = 0;
    };

class DebugBreakpoint
    {
    public:
	DebugBreakpoint(char const * const moduleName, int lineNumber);
	// location can be a function name
	DebugBreakpoint(char const * const location):
	    mLocation(location)
	    {}
	const OovString &getLocation() const
	    { return mLocation; }
    private:
	OovString mLocation;
    };

class Debugger:public OovProcessListener
    {
    public:
	Debugger();
	void setListener(DebuggerListener &listener)
	    { mDebuggerListener = &listener; }
	void setDebuggee(char const * const debuggee)
	    { mDebuggee = debuggee; }
	void addBreakpoint(const DebugBreakpoint &br);
	void stepInto();
	void stepOver();
	void resume();
	void interrupt();
	void sendCommand(char const * const command);

    private:
	enum GdbChildStates { GCS_GdbChildNotRunning , GCS_GdbChildRunning };
	std::string mDebuggee;
	std::string mGdbOutputBuffer;
	std::vector<DebugBreakpoint> mBreakpoints;
	OovBackgroundPipeProcess mBkgPipeProc;
	GdbChildStates mGdbChildState;
	DebuggerListener *mDebuggerListener;
	int mCommandIndex;
	void runDebuggerProcess();
	void ensureGdbChildRunning();
	void sendBreakpoint(const DebugBreakpoint &br);
	void sendMiCommand(char const * const command);
	void handleResult(const std::string &resultStr);
	std::string getTagValue(std::string const &wholeStr, char const * const tag);
	virtual void onStdOut(char const * const out, int len);
	virtual void onStdErr(char const * const out, int len);
    };

#endif /* DEBUGGER_H_ */
