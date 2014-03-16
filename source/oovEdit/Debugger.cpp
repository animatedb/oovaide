/*
 * Debugger.cpp
 *
 *  Created on: Feb 15, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "Debugger.h"
#include "Gui.h"
#include "FilePath.h"
#include <climits>

#define DEBUG_DBG 1


std::string DebuggerLocation::getAsString() const
    {
    OovString location = getFilename();
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
	args.addArg(mDebuggerFilePath.c_str());
	args.addArg(mDebuggeeFilePath.c_str());
	args.addArg("--interpreter=mi");
	mBkgPipeProc.startProcess(mDebuggerFilePath.c_str(), args.getArgv());
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
    if(getChildState() == GCS_GdbChildRunning)
	{
	interrupt();
	}
    auto iter = std::find(mBreakpoints.begin(), mBreakpoints.end(), br);
    if(iter == mBreakpoints.end())
	{
	mBreakpoints.push_back(br);
	if(getChildState() == GCS_GdbChildPaused)
	    {
	    sendAddBreakpoint(br);
	    }
	}
    else
	{
	mBreakpoints.erase(iter);
	if(getChildState() == GCS_GdbChildPaused)
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
    if(!mBkgPipeProc.isIdle() && getChildState() == GCS_GdbChildRunning)
	{
	sendMiCommand("-exec-interrupt");
	}
    }

void Debugger::stop()
    {
    if(!mBkgPipeProc.isIdle())
	{
	sendMiCommand("-gdb-exit");
	mBkgPipeProc.childProcessClose();
	changeChildState(GCS_GdbChildNotRunning);
	}
    }

void Debugger::ensureGdbChildRunning()
    {
    if(runDebuggerProcess())
	{
	for(auto const &br : mBreakpoints)
	    sendAddBreakpoint(br);
	}
    if(getChildState() == GCS_GdbChildNotRunning)
	{
	if(mWorkingDir.length())
	    {
	    std::string dirCmd = "-environment-cd ";
	    dirCmd += mWorkingDir;
	    sendMiCommand(dirCmd.c_str());
	    }
	if(mDebuggeeArgs.length())
	    {
	    std::string argCmd = "-exec-arguments ";
	    argCmd += mDebuggeeArgs;
	    sendMiCommand(argCmd.c_str());
	    }
	sendMiCommand("-exec-run");
	}
    }

void Debugger::startGetVariable(char const * const variable)
    {
    std::string cmd = "-data-evaluate-expression ";
    cmd += variable;
    sendMiCommand(cmd.c_str());
    }

void Debugger::startGetStack()
    {
    sendMiCommand("-stack-list-frames");
    }

void Debugger::startGetMemory(char const * const addr)
    {
    OovString cmd = "-data-read-memory-bytes ";
    cmd += addr;
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

DebuggerLocation Debugger::getStoppedLocation()
    {
    DebuggerLocation loc;
    if(getChildState() == GCS_GdbChildPaused)
	{
	LockGuard lock(mStatusLock);
	loc = mStoppedLocation;
	}
    return loc;
    }

Debugger::eChangeStatus Debugger::getChangeStatus()
    {
    LockGuard lock(mStatusLock);
    Debugger::eChangeStatus st = CS_None;
    if(!mChangeStatusQueue.empty())
	{
	st = mChangeStatusQueue.front();
	mChangeStatusQueue.pop();
	}
    return st;
    }
GdbChildStates Debugger::getChildState()
    {
    LockGuard lock(mStatusLock);
    GdbChildStates cs = mGdbChildState;
    return cs;
    }
std::string Debugger::getStack()
    {
    LockGuard lock(mStatusLock);
    std::string str = mStack;
    return str;
    }

std::string Debugger::getVarValue()
    {
    LockGuard lock(mStatusLock);
    std::string str = mVarValue;
    return str;
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
        /// @todo - this has to skip escaped quotes
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
// For some reason, "fullname" has doubled slashes on Windows, Sometimes "file"
// contains a full good path, but not all the time.
    FilePath fullFn(fixFilePath(getTagValue(resultStr, "fullname").c_str()).c_str(),
	    FP_File);
    loc.setFileLine(fullFn.c_str(), lineNum);
//    loc.setFileLine(getTagValue(resultStr, "file").c_str(), lineNum);
    return loc;
    }

// Return is end of result tuple.
// A tuple is defined in the GDB/MI output syntax BNF
// Example:		std::vector<WoolBag> mBags
// 15^done,value="{mBags = {
//    <std::_Vector_base<WoolBag, std::allocator<WoolBag> >> =
//      {
//      _M_impl =
//         {
//         <std::allocator<WoolBag>> =
//            {
//            <__gnu_cxx::new_allocator<WoolBag>> =
//               {<No data fields>},
//               <No data fields>
//            },
//         _M_start = 0x5a15a0,
//         _M_finish = 0x5a15a2,
//         _M_end_of_storage = 0x5a15a4
//         }
//      },
//       <No data fields>}}"
//
// Example:		A class containing mModule and mInterface
// 10^done,value="
//      {
//      mModule = 0x8,
//      mInterface =
//          {
//          getResourceName = 0x7625118e <onexit+97>,
//      	putTogether = 0x76251162 <onexit+53>
//          }
//      }"
static size_t getResultTuple(int pos, const std::string &resultStr, std::string &tupleStr)
    {
    size_t startPos = resultStr.find('{', pos);
    size_t endPos = resultStr.find('}', startPos);
    if(endPos != std::string::npos)
	{
	endPos++;	// Include the close brace.
	tupleStr = resultStr.substr(startPos, endPos-startPos);
	}
    return endPos;
    }

void Debugger::handleBreakpoint(const std::string &resultStr)
    {
    OovString brkNumStr = getTagValue(resultStr, "number");
    int brkNum;
    if(brkNumStr.getInt(0, 55555, brkNum))
	{
	DebuggerLocation loc = getLocationFromResult(resultStr);
	mBreakpoints.setBreakpointNumber(loc, brkNum);
	}
    }

void Debugger::handleValue(const std::string &resultStr)
    {
    cDebugResult debRes;
    debRes.parseResult(resultStr.c_str());
    mVarValue = debRes.getAsString();
    updateChangeStatus(Debugger::CS_Value);
mDebuggerListener->DebugOutput(mVarValue.c_str());
    }

// 99^done,stack=[
//    frame={level="0",addr="0x00408d0b",func="printf",file="c:/mingw/include/stdio.h",
//	fullname="c:\\mingw\\include\\stdio.h",line="240"},
//    frame={level="1",...
void Debugger::handleStack(const std::string &resultStr)
    {
    size_t pos=0;

	{
	LockGuard lock(mStatusLock);
	mStack.clear();
	do
	    {
	    std::string tuple;
	    pos = getResultTuple(pos, resultStr, tuple);
	    if(pos != std::string::npos)
		{
		mStack += getTagValue(tuple, "func");
		mStack += "   ";
		DebuggerLocation loc = getLocationFromResult(tuple);
		mStack += loc.getAsString();
		mStack += "\n";
		}
	    else
		break;
	    } while(pos!=std::string::npos);
	}
    updateChangeStatus(Debugger::CS_Stack);
    }

static int compareSubstr(const std::string &resultStr, size_t pos, char const *substr)
    {
    return resultStr.compare(pos, strlen(substr), substr);
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
		// After ^ is the "result-class":
		//	running, done, connected, error, exit
		if(compareSubstr(resultStr, pos+1, "running") == 0)
		    {
		    changeChildState(GCS_GdbChildRunning);
		    }
		else if(compareSubstr(resultStr, pos+1, "error") == 0)
		    {
//		if(mDebuggerListener)
//		    mDebuggerListener->DebugOutput(&resultStr[3]);
		    }
		else if(compareSubstr(resultStr, pos+1, "done") == 0)
		    {
		    size_t variableNamePos = resultStr.find(',');
		    if(variableNamePos != std::string::npos)
			{
			variableNamePos++;
			if(compareSubstr(resultStr, variableNamePos, "type=") == 0)
			    {
			    std::string typeStr = getTagValue(resultStr, "type=");
			    if(typeStr.compare("breakpoint") == 0)
				{
				handleBreakpoint(resultStr);
				}
			    }
			else if(compareSubstr(resultStr, variableNamePos, "stack=") == 0)
			    {
			    handleStack(resultStr);
			    }
			else if(compareSubstr(resultStr, variableNamePos, "value=") == 0)
			    {
			    handleValue(resultStr.substr(variableNamePos));
			    }
			}
		    }
		else if(compareSubstr(resultStr, pos+1, "exit") == 0)
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
			LockGuard lock(mStatusLock);
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

void Debugger::updateChangeStatus(Debugger::eChangeStatus status)
    {
	{
	LockGuard lock(mStatusLock);
	mChangeStatusQueue.push(status);
	}
    if(mDebuggerListener)
	mDebuggerListener->DebugStatusChanged();
    }

void Debugger::changeChildState(GdbChildStates state)
    {
	{
	LockGuard lock(mStatusLock);
	mGdbChildState = state;
	}
    updateChangeStatus(CS_RunState);
    }
