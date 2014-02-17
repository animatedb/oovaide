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

static Debugger *sDebugger;

Debugger::Debugger():
    mBkgPipeProc(*this), mDebuggerListener(nullptr), mCommandIndex(0)
    {
    sDebugger = this;
    addBreakpoint("main");
    }

void Debugger::runDebuggerProcess()
    {
    if(mBkgPipeProc.isIdle())
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
    }

void Debugger::resume()
    {
    ensureGdbChildRunning();
    sendMiCommand("-exec-continue");
    }

DebugBreakpoint::DebugBreakpoint(char const * const moduleName, int lineNum)
    {
    OovString location = moduleName;
    location += ':';
    location.appendInt(lineNum);
    }

void Debugger::addBreakpoint(const DebugBreakpoint &br)
    {
    if(mGdbChildState == GCS_GdbChildNotRunning)
	mBreakpoints.push_back(br);
    else
	{
	interrupt();
	}
    }

void Debugger::sendBreakpoint(const DebugBreakpoint &br)
    {
    OovString command = "-break-insert -f ";
    command += br.getLocation();
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
    if(!mBkgPipeProc.isIdle() && mGdbChildState != GCS_GdbChildNotRunning)
	{
	sendMiCommand("-exec-interrupt");
	}
    }

void Debugger::ensureGdbChildRunning()
    {
    runDebuggerProcess();
    if(mGdbChildState == GCS_GdbChildNotRunning)
	{
	for(auto const &br : mBreakpoints)
	    sendBreakpoint(br);
	mBreakpoints.clear();

	sendMiCommand("-exec-run");
	}
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

void Debugger::onStdErr(char const * const out, int len)
    {
    std::string result(out, len);
    if(mDebuggerListener)
	mDebuggerListener->DebugOutput(result.c_str());
    }

// gets value within quotes of
//	tagname="value"
std::string Debugger::getTagValue(std::string const &wholeStr, char const * const tag)
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

void Debugger::handleResult(const std::string &resultStr)
    {
    // Values are: "^running", "^error", "*stop"
    // "^connected", "^exit"
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
		    mGdbChildState = GCS_GdbChildRunning;
		    }
		else if(resultStr.find("error", pos+1) != std::string::npos)
		    {
//		if(mDebuggerListener)
//		    mDebuggerListener->DebugOutput(&resultStr[3]);
		    }
		else if(resultStr.find("exit", pos+1) != std::string::npos)
		    {
		    mGdbChildState = GCS_GdbChildNotRunning;
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
		    std::string reason = getTagValue(resultStr, "exit");
		    if(reason.compare(0, 4, "exit") != 0)
			{
			std::string fileName = getTagValue(resultStr, "fullname");
			OovString line = getTagValue(resultStr, "line").c_str();
			int lineNum = 0;
			line.getInt(0, INT_MAX, lineNum);
			if(mDebuggerListener)
			    mDebuggerListener->DebugStopped(fileName.c_str(), lineNum);
			}
		    }
		else if(resultStr.compare(1, std::string::npos, "stop") == 0)
		    {
		    mGdbChildState = GCS_GdbChildNotRunning;
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
	if(mDebuggerListener)
	    mDebuggerListener->DebugOutput(resultStr.c_str());
	}
#if(DEBUG_DBG)
    printf("%s\n", resultStr.c_str());
    fflush(stdout);
#endif
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugGo_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sDebugger)
	sDebugger->resume();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugStepOver_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sDebugger)
	sDebugger->stepOver();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugStepInto_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sDebugger)
	sDebugger->stepInto();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugStop_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    if(sDebugger)
	sDebugger->interrupt();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugAddBreakpoint_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
//    if(sDebugger)
//	sDebugger->addBreakpoint();
    return false;
    }

extern "C" G_MODULE_EXPORT gboolean on_DebugViewVariable_activate(GtkWidget *widget,
	GdkEvent *event, gpointer user_data)
    {
    return false;
    }
