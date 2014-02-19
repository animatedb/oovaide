/*
 * Debugger.cpp
 *
 *  Created on: Feb 15, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "Debugger.h"
#include "Gui.h"
#include <climits>

#define DEBUG_DBG 1



std::string DebuggerLocation::getAsString() const
    {
    OovString location = mFileOrFuncName;
    if(mLineNum != -1)
	{
	location += ':';
	location.appendInt(mLineNum);
	}
    return location;
    }



Debugger::Debugger():
    mBkgPipeProc(*this), mDebuggerListener(nullptr), mCommandIndex(0)
    {
    // It seems like "-exec-run" must be used to start the debugger,
    // so this is needed to prevent it from running to the end when
    // single stepping is used to start the program.
    toggleBreakpoint("main");
    }

bool Debugger::runDebuggerProcess()
    {
    bool started = mBkgPipeProc.isIdle();
    if(started)
	{
	OovProcessChildArgs args;
	/// @todo - this must come from options config file.
	char const * const dbgProcName = "/usr/bin/gdb";
	args.addArg(dbgProcName);
	args.addArg(mDebuggee.c_str());
	args.addArg("--interpreter=mi");
	mBkgPipeProc.startProcess(dbgProcName, args.getArgv());
#if(DEBUG_DBG)
	printf("Starting process\n");
	fflush(stdout);
#endif
	}
    return started;
    }

void Debugger::resume()
    {
    ensureGdbChildRunning();
    sendMiCommand("-exec-continue");
    }

void Debugger::toggleBreakpoint(const DebuggerBreakpoint &br)
    {
    if(mGdbChildState == GCS_GdbChildRunning)
	{
	interrupt();
	}
    auto iter = std::find(mBreakpoints.begin(), mBreakpoints.end(), br);
    if(iter == mBreakpoints.end())
	{
	mBreakpoints.push_back(br);
	if(mGdbChildState == GCS_GdbChildPaused)
	    {
	    sendAddBreakpoint(br);
	    }
	}
    else
	{
	mBreakpoints.erase(iter);
	if(mGdbChildState == GCS_GdbChildPaused)
	    {
	    if(br.mBreakpointNumber != -1)
		{
		sendDeleteBreakpoint(br);
		}
	    }
	}
    }

void Debugger::sendAddBreakpoint(const DebuggerBreakpoint &br)
    {
    OovString command = "-break-insert -f ";
    command += br.getAsString();
    sendMiCommand(command.c_str());
    }

void Debugger::sendDeleteBreakpoint(const DebuggerBreakpoint &br)
    {
    OovString command = "-break-delete ";
    command.appendInt(br.mBreakpointNumber);
    sendMiCommand(command.c_str());
    }

void Debugger::stepInto()
    {
    ensureGdbChildRunning();
    sendMiCommand("-exec-step");
    }

void Debugger::stepOver()
    {
    ensureGdbChildRunning();
    sendMiCommand("-exec-next");
    }

void Debugger::interrupt()
    {
    if(!mBkgPipeProc.isIdle() && mGdbChildState == GCS_GdbChildRunning)
	{
	sendMiCommand("-exec-interrupt");
	}
    }

void Debugger::ensureGdbChildRunning()
    {
    if(runDebuggerProcess())
	{
	for(auto const &br : mBreakpoints)
	    sendAddBreakpoint(br);
	}
    if(mGdbChildState == GCS_GdbChildNotRunning)
	{
	sendMiCommand("-exec-run");
	}
    }

void Debugger::viewVariable(char const * const variable)
    {
    std::string cmd = "-data-evaluate-expression ";
    cmd += variable;
    sendMiCommand(cmd.c_str());
    }

void Debugger::sendMiCommand(char const * const command)
    {
    OovString cmd;
    cmd.appendInt(++mCommandIndex);
    cmd.append(command);
    sendCommand(cmd.c_str());
    }

void Debugger::sendCommand(char const * const command)
    {
    std::string cmd = command;
    size_t pos = cmd.find('\n');
    if(pos != std::string::npos)
	{
	if(pos > 0)
	    {
	    if(cmd[pos-1] != '\r')
		cmd.insert(pos, "\r");
	    }
	}
    else
	cmd += "\r\n";
    mBkgPipeProc.childProcessSend(cmd.c_str());
#if(DEBUG_DBG)
    printf("Sent Command %s\n", cmd.c_str());
    fflush(stdout);
#endif
    }

void Debugger::onStdOut(char const * const out, int len)\
    {
    mGdbOutputBuffer.append(out, len);
    while(1)
	{
	size_t pos = mGdbOutputBuffer.find('\n');
	if(pos != std::string::npos)
	    {
	    std::string res(mGdbOutputBuffer, 0, pos+1);
	    handleResult(res);
	    mGdbOutputBuffer.erase(0, pos+1);
	    }
	else
	    break;
	}
    }

DebuggerLocation Debugger::getStoppedLocation() const
    {
    DebuggerLocation loc;
    if(mGdbChildState == GCS_GdbChildPaused)
	{
	loc = mStoppedLocation;
	}
    return loc;
    }

void Debugger::onStdErr(char const * const out, int len)
    {
    std::string result(out, len);
    if(mDebuggerListener)
	mDebuggerListener->DebugOutput(result.c_str());
    }

// gets value within quotes of
//	tagname="value"
static std::string getTagValue(std::string const &wholeStr, char const * const tag)
    {
    std::string val;
    std::string tagStr = tag;
    tagStr += "=\"";
    size_t pos = wholeStr.find(tagStr);
    if(pos != std::string::npos)
	{
	pos += tagStr.length();
	size_t endPos = wholeStr.find("\"", pos);
	if(endPos != std::string::npos)
	    val = wholeStr.substr(pos, endPos-pos);
	}
    return val;
    }

static DebuggerLocation getLocationFromResult(const std::string &resultStr)
    {
    DebuggerLocation loc;
    OovString line = getTagValue(resultStr, "line").c_str();
    int lineNum = 0;
    line.getInt(0, INT_MAX, lineNum);
    loc.setFileLine(getTagValue(resultStr, "fullname").c_str(), lineNum);
    return loc;
    }

void Debugger::handleResult(const std::string &resultStr)
    {
    // Values are: "^running", "^error", "*stop"
    // "^connected", "^exit"
#if(DEBUG_DBG)
    printf("%s\n", resultStr.c_str());
    fflush(stdout);
#endif
    if(isdigit(resultStr[0]))
	{
	size_t pos = 0;
	while(isdigit(resultStr[pos]))
	    {
	    pos++;
	    }
	switch(resultStr[pos+0])
	    {
	    case '^':
		{
		if(resultStr.find("running", pos+1) != std::string::npos)
		    {
		    changeChildState(GCS_GdbChildRunning);
		    }
		else if(resultStr.find("error", pos+1) != std::string::npos)
		    {
//		if(mDebuggerListener)
//		    mDebuggerListener->DebugOutput(&resultStr[3]);
		    }
		else if(resultStr.find("done", pos+1) != std::string::npos)
		    {
		    std::string typeStr = getTagValue(resultStr, "type");
		    if(typeStr.compare("breakpoint") == 0)
			{
			OovString brkNumStr = getTagValue(resultStr, "number");
			int brkNum;
			if(brkNumStr.getInt(0, 55555, brkNum))
			    {
			    DebuggerLocation loc = getLocationFromResult(resultStr);
			    mBreakpoints.setBreakpointNumber(loc, brkNum);
			    }
			}
		    }
		else if(resultStr.find("exit", pos+1) != std::string::npos)
		    {
		    changeChildState(GCS_GdbChildNotRunning);
		    }
		}
		break;
	    }
	}
    else
	{
	switch(resultStr[0])
	    {
	    case '*':
		{
		if(resultStr.compare(1, 7, "stopped") == 0)
		    {
		    std::string reason = getTagValue(resultStr, "reason");
		    if((reason.find("end-stepping-range") != std::string::npos) ||
			    (reason.find("breakpoint-hit") != std::string::npos))
			{
			mStoppedLocation = getLocationFromResult(resultStr);
			changeChildState(GCS_GdbChildPaused);
			}
		    else if(reason.find("exited-normally") != std::string::npos)
			{
			changeChildState(GCS_GdbChildNotRunning);
			}
		    }
		else if(resultStr.compare(1, std::string::npos, "stop") == 0)
		    {
		    changeChildState(GCS_GdbChildNotRunning);
		    }
		}
		break;

	    case '~':
	    case '@':
	    case '&':
    //	    if(mDebuggerListener)
    //		mDebuggerListener->DebugOutput(&resultStr[3]);
		break;
	    }
	}
    if(mDebuggerListener)
	mDebuggerListener->DebugOutput(resultStr.c_str());
    }

void Debugger::changeChildState(GdbChildStates state)
    {
    mGdbChildState = state;
    if(mDebuggerListener)
	mDebuggerListener->DebugStatusChanged();
    }
