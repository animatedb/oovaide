// OovIpc.cpp
//  Created on: July 15, 2015
//  \copyright 2015 DCBlaha.  Distributed under the GPL.

#include "OovIpc.h"
#define DEBUG_IPC 0

OovIpcMsg::OovIpcMsg(int cmd, OovStringRef arg1,
    OovStringRef arg2)
    {
    OovString &cmdStr = *this;
    cmdStr += static_cast<char>(cmd);
    if(arg1)
        {
        cmdStr += ',';
        cmdStr += arg1;
        }
    if(arg2)
        {
        cmdStr += ',';
        cmdStr += arg2;
        }
    cmdStr += '\n';
#if(DEBUG_IPC)
    FILE *fp = fopen("IPC.txt", "a");
    fprintf(fp, "SND: %s", cmdStr.getStr());
    fclose(fp);
#endif
    }

// First arg at index 0 is the command.
OovString OovIpcMsg::getArg(size_t argNum) const
    {
    OovStringVec args = split(',');
    OovString arg;
#if(DEBUG_IPC)
    if(argNum == 0)
        {
        FILE *fp = fopen("IPC.txt", "a");
        fprintf(fp, "RCV: %s\n", cmdStr.getStr());
        fclose(fp);
        }
#endif
    if(argNum < args.size())
        {
        arg = args[argNum];
        }
    return arg;
    }

int OovIpcMsg::getCommand() const
    {
    OovString cmd = OovIpcMsg::getArg(0);
    return(cmd[0]);
    }

void OovIpc::sendMessage(OovStringRef msg)
    {
    printf("%s", msg.getStr());
    fflush(stdout);
    }
