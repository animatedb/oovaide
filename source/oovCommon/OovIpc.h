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
    ECC_GetMethodDef = 'm',     // arg1=className, arg2=methodName
    ECC_GetClassDef = 'c',      // arg1=className
    ECC_RunAnalysis = 'a',      // no args
    ECC_StopAnalysis = 's',     // no args
    };

class OovMsg
    {
    public:
        static OovStringRef buildSendMsg(int cmd, OovStringRef arg1,
            OovStringRef arg2=nullptr);
        static OovString getArg(OovStringRef cmdStr, size_t argNum);
    };

#endif
