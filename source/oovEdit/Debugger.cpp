/*
 * Debugger.cpp
 *
 *  Created on: Feb 15, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "Debugger.h"
#include "Gui.h"
#include "FilePath.h"
#include "Debug.h"
#include <climits>

#define DEBUG_DBG 0
#if(DEBUG_DBG)
static DebugFile sDbgFile("Dbg.txt");
#endif


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


DebuggerListener::~DebuggerListener()
    {}

DebuggerBase::~DebuggerBase()
    {}


DebuggerBase::DebuggerBase():
    mDebuggerListener(nullptr)
    {
    }

DebuggerLocation DebuggerBase::getStoppedLocation()
    {
    DebuggerLocation loc;
    if(getChildState() == DCS_ChildPaused)
        {
        LockGuard lock(mStatusLock);
        loc = mStoppedLocation;
        }
    return loc;
    }

eDebuggerChangeStatus DebuggerBase::getChangeStatus()
    {
    LockGuard lock(mStatusLock);
    eDebuggerChangeStatus st = DCS_None;
    if(!mChangeStatusQueue.empty())
        {
        st = mChangeStatusQueue.front();
        mChangeStatusQueue.pop();
        }
    return st;
    }

DebuggerChildStates DebuggerBase::getChildState()
    {
    LockGuard lock(mStatusLock);
    DebuggerChildStates cs = mDebuggerChildState;
    return cs;
    }

OovString DebuggerBase::getStack()
    {
    LockGuard lock(mStatusLock);
    OovString str = mStack;
    return str;
    }

OovString DebuggerBase::getVarValue()
    {
    LockGuard lock(mStatusLock);
    std::string str = mVarValue;
    return str;
    }

void DebuggerBase::updateChangeStatus(eDebuggerChangeStatus status)
    {
        {
        LockGuard lock(mStatusLock);
        mChangeStatusQueue.push(status);
        }
    if(mDebuggerListener)
        mDebuggerListener->DebugStatusChanged();
    }

void DebuggerBase::changeChildState(DebuggerChildStates state)
    {
        {
        LockGuard lock(mStatusLock);
        mDebuggerChildState = state;
        }
    updateChangeStatus(DCS_RunState);
    }


/////////////////////////

#if(!USE_LLDB)
DebuggerGdb::DebuggerGdb():
    mBkgPipeProc(this), mCommandIndex(0), mFrameNumber(0), mCurrentThread(1)
    {
    // It seems like "-exec-run" must be used to start the debugger,
    // so this is needed to prevent it from running to the end when
    // single stepping is used to start the program.
    toggleBreakpoint(DebuggerBreakpoint("main"));
    resetFrameNumber();
    }

bool DebuggerGdb::runDebuggerProcess()
    {
    bool started = mBkgPipeProc.isIdle();
    if(started)
        {
        OovProcessChildArgs args;
        args.addArg(mDebuggerFilePath);
        args.addArg(mDebuggeeFilePath);
        args.addArg("--interpreter=mi");
        mBkgPipeProc.startProcess(mDebuggerFilePath, args.getArgv(), false);
#if(DEBUG_DBG)
        sDbgFile.printflush("Starting process\n");
#endif
        }
    return started;
    }

void DebuggerGdb::resume()
    {
    resetFrameNumber();
    ensureGdbChildRunning();
    sendMiCommand("-exec-continue");
    }

void DebuggerGdb::toggleBreakpoint(const DebuggerBreakpoint &br)
    {
    if(getChildState() == DCS_ChildRunning)
        {
        interrupt();
        }
    auto iter = std::find(mBreakpoints.begin(), mBreakpoints.end(), br);
    if(iter == mBreakpoints.end())
        {
        mBreakpoints.push_back(br);
        if(getChildState() == DCS_ChildPaused)
            {
            sendAddBreakpoint(br);
            }
        }
    else
        {
        if(getChildState() == DCS_ChildPaused)
            {
            DebuggerBreakpoint &delBreakpoint = *iter;
            if(delBreakpoint.mBreakpointNumber != -1)
                {
                sendDeleteBreakpoint(delBreakpoint);
                }
            }
        mBreakpoints.erase(iter);
        }
    }

void DebuggerGdb::sendAddBreakpoint(const DebuggerBreakpoint &br)
    {
    OovString command = "-break-insert -f ";
    command += br.getAsString();
    sendMiCommand(command);
    }

void DebuggerGdb::sendDeleteBreakpoint(const DebuggerBreakpoint &br)
    {
    OovString command = "-break-delete ";
    command.appendInt(br.mBreakpointNumber);
    sendMiCommand(command);
    }

void DebuggerGdb::stepInto()
    {
    resetFrameNumber();
    ensureGdbChildRunning();
    sendMiCommand("-exec-step");
    }

void DebuggerGdb::stepOver()
    {
    resetFrameNumber();
    ensureGdbChildRunning();
    sendMiCommand("-exec-next");
    }

void DebuggerGdb::interrupt()
    {
    if(!mBkgPipeProc.isIdle() && getChildState() == DCS_ChildRunning)
        {
        sendMiCommand("-exec-interrupt");
        }
    }

void DebuggerGdb::stop()
    {
    if(!mBkgPipeProc.isIdle())
        {
        sendMiCommand("-gdb-exit");
        mBkgPipeProc.childProcessClose();
        changeChildState(DCS_ChildNotRunning);
        }
    }

void DebuggerGdb::ensureGdbChildRunning()
    {
    if(runDebuggerProcess())
        {
        for(auto const &br : mBreakpoints)
            sendAddBreakpoint(br);
        }
    if(getChildState() == DCS_ChildNotRunning)
        {
        if(mWorkingDir.length())
            {
            std::string dirCmd = "-environment-cd ";
            dirCmd += mWorkingDir;
            sendMiCommand(dirCmd);
            }
        if(mDebuggeeArgs.length())
            {
            std::string argCmd = "-exec-arguments ";
            argCmd += mDebuggeeArgs;
            sendMiCommand(argCmd);
            }
        sendMiCommand("-exec-run");
        }
    }

void DebuggerGdb::startGetVariable(OovStringRef const variable)
    {
    OovString cmd = "-data-evaluate-expression ";
    cmd += "--thread ";
    cmd.appendInt(mCurrentThread, 10);
    cmd += " --frame ";
    cmd.appendInt(mFrameNumber, 10);
    cmd += ' ';
    cmd += variable;
    mGetVariableName = variable;
    sendMiCommand(cmd);
    }

void DebuggerGdb::startGetStack()
    {
    sendMiCommand("-stack-list-frames");
    }

void DebuggerGdb::startGetMemory(OovStringRef const addr)
    {
    OovString cmd = "-data-read-memory-bytes ";
    cmd += addr;
    sendMiCommand(cmd);
    }

void DebuggerGdb::sendMiCommand(OovStringRef const command)
    {
    OovString cmd;
    cmd.appendInt(++mCommandIndex);
    cmd.append(command);
    sendCommand(cmd);
    }

void DebuggerGdb::sendCommand(OovStringRef const command)
    {
    OovString cmd = command;
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
    if(mDebuggerListener)
        {
        mDebuggerListener->DebugOutput(cmd);
        }
    mBkgPipeProc.childProcessSend(cmd);
#if(DEBUG_DBG)
    sDbgFile.printflush("Sent Command %s\n", cmd.c_str());
#endif
    }

void DebuggerGdb::onStdOut(OovStringRef const out, size_t len)\
    {
    mDebuggerOutputBuffer.append(out, len);
    while(1)
        {
        size_t pos = mDebuggerOutputBuffer.find('\n');
        if(pos != std::string::npos)
            {
            std::string res(mDebuggerOutputBuffer, 0, pos+1);
            handleResult(res);
            mDebuggerOutputBuffer.erase(0, pos+1);
            }
        else
            break;
        }
    }

// The docs say that -stack-select-frame is deprecated for the --frame option.
// When --frame is used, --thread must also be specified.
// The --frame option does work with many commands such as -data-evaluate-expression
void DebuggerGdb::setStackFrame(OovStringRef const frameLine)
    {
    OovString line = frameLine;
    size_t pos = line.find(':');
    if(pos != std::string::npos)
        {
        int frameNumber;
        OovString numStr = line;
        numStr.resize(pos);
        if(numStr.getInt(0, 10000, frameNumber))
            {
            mFrameNumber = frameNumber;
            }
        }
    }


void DebuggerGdb::onStdErr(OovStringRef const out, size_t len)
    {
    std::string result(out, len);
    if(mDebuggerListener)
        mDebuggerListener->DebugOutput(result);
    }

// gets value within quotes of
//      tagname="value"
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
    OovString line = getTagValue(resultStr, "line");
    int lineNum = 0;
    line.getInt(0, INT_MAX, lineNum);
// For some reason, "fullname" has doubled slashes on Windows, Sometimes "file"
// contains a full good path, but not all the time.
    FilePath fullFn(FilePathFixFilePath(getTagValue(resultStr, "fullname")),
            FP_File);
    loc.setFileLine(fullFn, lineNum);
//    loc.setFileLine(getTagValue(resultStr, "file"), lineNum);
    return loc;
    }

// Return is end of result tuple.
// A tuple is defined in the GDB/MI output syntax BNF
// Example:             std::vector<WoolBag> mBags
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
// Example:             A class containing mModule and mInterface
// 10^done,value="
//      {
//      mModule = 0x8,
//      mInterface =
//          {
//          getResourceName = 0x7625118e <onexit+97>,
//              putTogether = 0x76251162 <onexit+53>
//          }
//      }"
static size_t getResultTuple(size_t pos, const std::string &resultStr, std::string &tupleStr)
    {
    size_t startPos = resultStr.find('{', pos);
    size_t endPos = resultStr.find('}', startPos);
    if(endPos != std::string::npos)
        {
        endPos++;       // Include the close brace.
        tupleStr = resultStr.substr(startPos, endPos-startPos);
        }
    return endPos;
    }

void DebuggerGdb::handleBreakpoint(const std::string &resultStr)
    {
    OovString brkNumStr = getTagValue(resultStr, "number");
    int brkNum;
    if(brkNumStr.getInt(0, 55555, brkNum))
        {
        DebuggerLocation loc = getLocationFromResult(resultStr);
        mBreakpoints.setBreakpointNumber(loc, brkNum);
        }
    }

void DebuggerGdb::handleValue(const std::string &resultStr)
    {
    cDebugResult debRes;
    debRes.parseResult(resultStr);
    mVarValue = mGetVariableName;
    mGetVariableName.clear();
    mVarValue += " : ";
    mVarValue += debRes.getAsString();
    updateChangeStatus(DCS_Value);
    if(mDebuggerListener)
        mDebuggerListener->DebugOutput(mVarValue);
    }

// 99^done,stack=[
//    frame={level="0",addr="0x00408d0b",func="printf",file="c:/mingw/include/stdio.h",
//      fullname="c:\\mingw\\include\\stdio.h",line="240"},
//    frame={level="1",...
void DebuggerGdb::handleStack(const std::string &resultStr)
    {
        {
        LockGuard lock(mStatusLock);
        mStack.clear();
        int frameNum = 0;
        size_t pos=0;
        do
            {
            std::string tuple;
            pos = getResultTuple(pos, resultStr, tuple);
            if(pos != std::string::npos)
                {
                mStack.appendInt(frameNum++, 10);
                mStack += ':';
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
    updateChangeStatus(DCS_Stack);
    }

static int compareSubstr(const std::string &resultStr, size_t pos, char const *substr)
    {
    return resultStr.compare(pos, strlen(substr), substr);
    }

void DebuggerGdb::handleResult(const std::string &resultStr)
    {
    // Values are: "^running", "^error", "*stop"
    // "^connected", "^exit"
#if(DEBUG_DBG)
    sDbgFile.printflush("%s\n", resultStr.c_str());
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
                //      running, done, connected, error, exit
                if(compareSubstr(resultStr, pos+1, "running") == 0)
                    {
                    changeChildState(DCS_ChildRunning);
                    }
                else if(compareSubstr(resultStr, pos+1, "error") == 0)
                    {
//              if(mDebuggerListener)
//                  mDebuggerListener->DebugOutput(&resultStr[3]);
                    }
                else if(compareSubstr(resultStr, pos+1, "done") == 0)
                    {
                    size_t variableNamePos = resultStr.find(',');
                    if(variableNamePos != std::string::npos)
                        {
                        variableNamePos++;
                        if(compareSubstr(resultStr, variableNamePos, "bkpt=") == 0)
                            {
                            std::string typeStr = getTagValue(resultStr, "type");
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
                    changeChildState(DCS_ChildNotRunning);
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
                            {
                            LockGuard lock(mStatusLock);
                            mStoppedLocation = getLocationFromResult(resultStr);
                            }
                        changeChildState(DCS_ChildPaused);
                        }
                    else if(reason.find("exited-normally") != std::string::npos)
                        {
                        changeChildState(DCS_ChildNotRunning);
                        }
                    }
                else if(resultStr.compare(1, std::string::npos, "stop") == 0)
                    {
                    changeChildState(DCS_ChildNotRunning);
                    }
                }
                break;

            case '~':
            case '@':
            case '&':
    //      if(mDebuggerListener)
    //          mDebuggerListener->DebugOutput(&resultStr[3]);
                break;
            }
        }
    if(mDebuggerListener)
        mDebuggerListener->DebugOutput(resultStr);
    }
#endif


/////////////////////////

#if(USE_LLDB)

bool DebuggerLlvm::runDebuggee()
    {
    if(!mDebuggeeProcess.IsValid())
        {
        mDebuggeeProcess = LaunchSimple(mDebuggerFilePath, nullptr,
            ".");
//        mDebuggeeProcess.GetState();
/*
# Get a handle on the process's broadcaster.
        broadcaster = process.GetBroadcaster()

        # Create an empty event object.
        event = SBEvent()

        # Create a listener object and register with the broadcaster.
        listener = SBListener('my listener')
        rc = broadcaster.AddListener(listener, SBProcess.eBroadcastBitStateChanged)
*/
        }
    }

#endif
