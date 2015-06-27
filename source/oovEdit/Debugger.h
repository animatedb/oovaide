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


#define USE_LLDB 0
#if(USE_LLDB)
#include <LLDB.h>
#endif


enum DebuggerChildStates { DCS_ChildNotRunning, DCS_ChildRunning, DCS_ChildPaused };
enum eDebuggerChangeStatus { DCS_None, DCS_RunState, DCS_Stack, DCS_Value };

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
        //      function
        //      filename:linenum
        //      filename:function
        //      *address
        // WARNING: At this time, location can only be filename:linenum. This
        //      is because breakpoint numbers for clearing breakpoints use
        //      this to find the matching breakpoint.
        //      This returns the format:        filenane:linenum
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
        virtual ~DebuggerListener();
        /// All output from the debugger process is sent to this listener.
        /// Set this up by calling Debugger::setListener.
        virtual void DebugOutput(OovStringRef const str) = 0;
        /// This is called after the state has changed. Use
        /// Debugger::getChangeStatus to get the status.
        virtual void DebugStatusChanged() = 0;
    };


class DebuggerBase
    {
    public:
        DebuggerBase();
        virtual ~DebuggerBase();
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
        ///     Lines are separated with \n.
        virtual void setStackFrame(OovStringRef const frameLine) = 0;
        virtual void toggleBreakpoint(const DebuggerBreakpoint &br) = 0;

        virtual void stepInto()=0;
        virtual void stepOver()=0;
        virtual void resume()=0;
        virtual void interrupt()=0;     // pause
        virtual void stop()=0;
        /// This allows the user to send a typed in debugger command.
        virtual void sendCommand(OovStringRef const command) = 0;
        std::string const &getDebuggerFilePath() const
            { return mDebuggerFilePath; }
        std::string const &getDebuggeeFilePath() const
            { return mDebuggeeFilePath; }
        DebuggerBreakpoints const &getBreakpoints() const
            { return mBreakpoints; }
        // The following info must be thread protected
        eDebuggerChangeStatus getChangeStatus();
        DebuggerChildStates getChildState();
        OovString getStack();
        OovString getVarValue();
        // Returns empty filename if not stopped.
        DebuggerLocation getStoppedLocation();

    protected:
        OovString mDebuggerFilePath;
        OovString mDebuggeeFilePath;
        OovString mDebuggeeArgs;
        OovString mWorkingDir;

        DebuggerBreakpoints mBreakpoints;
        DebuggerListener *mDebuggerListener;

        // Thread protected data
        std::string mDebuggerOutputBuffer;
        OovString mStack;
        std::string mVarValue;
        DebuggerChildStates mDebuggerChildState;
        InProcMutex mStatusLock;
        std::queue<eDebuggerChangeStatus> mChangeStatusQueue;
        DebuggerLocation mStoppedLocation;
        void updateChangeStatus(eDebuggerChangeStatus status);
        void changeChildState(DebuggerChildStates state);
    };


#if(!USE_LLDB)
class DebuggerGdb:public OovProcessListener, public DebuggerBase
    {
    public:
        DebuggerGdb();
        virtual ~DebuggerGdb()
            {}
        /// @param frameLine One line of text returned from getStack
        ///     Lines are separated with \n.
        void setStackFrame(OovStringRef const frameLine) override;
        void toggleBreakpoint(const DebuggerBreakpoint &br) override;
        void stepInto() override;
        void stepOver() override;
        void resume() override;
        void interrupt() override;      // pause
        void stop() override;

        /// This allows the user to send a typed in debugger command.
        void sendCommand(OovStringRef const command) override;
        void startGetVariable(OovStringRef const variable);
        void startGetStack();
        void startGetMemory(OovStringRef const addr);

    private:
        OovBackgroundPipeProcess mBkgPipeProc;
        int mCommandIndex;
        // Frame numbers start at 0
        int mFrameNumber;
        // Thread numbers start at 1
        int mCurrentThread;

        void resetFrameNumber()
            { mFrameNumber = 0; }
        bool runDebuggerProcess();
        void ensureGdbChildRunning();
        void sendAddBreakpoint(const DebuggerBreakpoint &br);
        void sendDeleteBreakpoint(const DebuggerBreakpoint &br);
        void sendMiCommand(OovStringRef const command);
        void handleResult(const std::string &resultStr);
        void handleBreakpoint(const std::string &resultStr);
        void handleStack(const std::string &resultStr);
        void handleValue(const std::string &resultStr);
        virtual void onStdOut(OovStringRef const out, size_t len) override;
        virtual void onStdErr(OovStringRef const out, size_t len) override;
    };
#endif


#if(USE_LLDB)
class DebuggerLlvm:public DebuggerBase
    {
    private:
        virtual ~DebuggerLlvm();
        SBDebugger mDebugger;
        SBTarget mDebuggee;
        SBProcess mDebuggeeProcess;

        void runDebuggee();
    };
#endif


#if(USE_LLDB)
typedef DebuggerLlvm Debugger;
#else
typedef DebuggerGdb Debugger;
#endif

#endif /* DEBUGGER_H_ */
