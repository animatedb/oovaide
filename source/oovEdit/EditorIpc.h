// EditorIpc.h
//  Created on: July 29, 2015
//  \copyright 2015 DCBlaha.  Distributed under the GPL.

#ifndef EDITORIPC_H_
#define EDITORIPC_H_

#include "OovProcess.h"
#include "OovIpc.h"
#include <queue>

class EditorIpc:public OovStdInListener
    {
    public:
        EditorIpc();
        void startStdinListening()
            { mBackgroundListener.start(); }
        bool getMessage(OovIpcMsg &msg);
        void viewClassDiagram(OovStringRef className);
        void viewPortionDiagram(OovStringRef className);
        void analyze();
        void build();
        void stopAnalyze();
        void goToMethod(OovStringRef className, OovStringRef methodName);
        virtual void onStdIn(OovStringRef const in, size_t len) override;

    private:
        OovBackgroundStdInListener mBackgroundListener;
        std::queue<OovIpcMsg> mStdInBuffer;
    };


#endif /* EDITORIPC_H_ */
