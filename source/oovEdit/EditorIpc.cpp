// EditorIpc.cpp
//  Created on: July 29, 2015
//  \copyright 2015 DCBlaha.  Distributed under the GPL.

#include "EditorIpc.h"


EditorIpc::EditorIpc()
    {
    mBackgroundListener.setListener(this);
    }

bool EditorIpc::getMessage(OovIpcMsg &msg)
    {
    bool gotMsg = false;
    if(!mStdInBuffer.empty())
        {
        gotMsg = true;
        msg = mStdInBuffer.front();
        mStdInBuffer.pop();
        }
    return gotMsg;
    }

void EditorIpc::onStdIn(OovStringRef const in, size_t len)
    {
    // WARNING - This thread cannot do GUI calls
    mStdInBuffer.push(OovString(in));
    }

void EditorIpc::viewClassDiagram(OovStringRef className)
    {
    OovIpcMsg msg(ECC_ViewClassDiagram, className);
    OovIpc::sendMessage(msg);
    }

void EditorIpc::viewPortionDiagram(OovStringRef className)
    {
    OovIpcMsg msg(ECC_ViewPortionDiagram, className);
    OovIpc::sendMessage(msg);
    }

void EditorIpc::analyze()
    {
    OovIpcMsg msg(ECC_RunAnalysis);
    OovIpc::sendMessage(msg);
    }

void EditorIpc::build()
    {
    OovIpcMsg msg(ECC_Build);
    OovIpc::sendMessage(msg);
    }

void EditorIpc::stopAnalyze()
    {
    OovIpcMsg msg(ECC_StopAnalysis);
    OovIpc::sendMessage(msg);
    }

void EditorIpc::goToMethod(OovStringRef className, OovStringRef methodName)
    {
    OovIpcMsg msg(ECC_GotoMethodDef, className, methodName);
    OovIpc::sendMessage(msg);
    }

