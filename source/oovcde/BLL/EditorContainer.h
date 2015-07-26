// EditorContainer.h
//  Created on: July 15, 2015
//  \copyright 2015 DCBlaha.  Distributed under the GPL.

#ifndef EDITOR_CONTAINER
#define EDITOR_CONTAINER

#define USE_IPC 1
#if(USE_IPC)

#include "ModelObjects.h"
#include "OovProcess.h"
#include "Options.h"

void viewSource(GuiOptions const &guiOptions, OovStringRef const module,
        unsigned int lineNum);

// The communication protocol is point-to-point, full-duplex, asynchronous.
// The editor container can send commands/requests to the editor, and the
// editor can do the same back to the container.  Either side can initiate.
// Neither side should ever wait for a response, but should be able to
// receive a response at any time.  Only a single editor is connected to
// a single editor container.
class EditorContainer:public OovProcessListener
    {
    public:
        EditorContainer(ModelData const &modelData);
        void viewFile(OovStringRef const procPath, char const * const *argv,
            OovStringRef const fn, int lineNum);
        bool okToExit();

    private:
        ModelData const &mModelData;
        OovBackgroundPipeProcess mBackgroundProcess;
        OovString mReceivedData;

        void handleCommand(OovStringRef cmdStr);
        ModelClassifier const *findClass(OovStringRef name);

        virtual void onStdOut(OovStringRef const out, size_t len) override;
        virtual void onStdErr(OovStringRef const out, size_t len) override;
        virtual void processComplete()  override;
    };
#endif

#endif

