// OovIpc.h
//  Created on: July 16, 2015
//  \copyright 2015 DCBlaha.  Distributed under the GPL.

#ifndef OOV_IPC
#define OOV_IPC

#include "OovString.h"

enum EditorCommands
    {
    EC_ViewFile='v',            // arg1 = fileName, arg2 = lineNum
    };

enum EditorContainerCommands
    {
    ECC_GotoMethodDef = 'm',            // arg1=className, arg2=methodName
    ECC_ViewClassDiagram = 'c',         // arg1=className
    ECC_ViewPortionDiagram = 'p',       // arg1=className
    ECC_RunAnalysis = 'a',              // no args
    ECC_Build = 'b',                    // no args
    ECC_StopAnalysis = 's',             // no args
    };

class OovIpcMsg:public OovString
    {
    public:
        OovIpcMsg()
            {}
        OovIpcMsg(OovString const &str):
            OovString(str)
            {}
        OovIpcMsg(int cmd, OovStringRef arg1=nullptr, OovStringRef arg2=nullptr);
        // argNum=0 is the command
        OovString getArg(size_t argNum) const;
        int getCommand() const;
    };

class OovIpc
    {
    public:
        static void sendMessage(OovStringRef msg);
    };

#endif
