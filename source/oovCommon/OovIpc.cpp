// OovIpc.cpp
//  Created on: July 15, 2015
//  \copyright 2015 DCBlaha.  Distributed under the GPL.

#include "OovIpc.h"

OovStringRef OovMsg::buildSendMsg(int cmd, OovStringRef arg1,
    OovStringRef arg2)
    {
    OovString cmdStr;
    cmdStr += static_cast<char>(cmd);
    OovString cmdData = cmdStr;
    cmdData += ',';
    cmdData += arg1;
    if(arg2)
        {
        cmdData += ',';
        cmdData += arg2;
        }
    cmdData += '\n';
    return cmdData;
    }

// First arg at index 0 is the command.
OovString OovMsg::getArg(OovStringRef cmdStr, size_t argNum)
    {
    OovStringVec args = cmdStr.split(',');
    OovString arg;
    if(argNum < args.size())
        {
        arg = args[argNum];
        }
    return arg;
    }

