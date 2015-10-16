// EditorContainer.h
//  Created on: July 15, 2015
//  \copyright 2015 DCBlaha.  Distributed under the GPL.

#ifndef EDITOR_CONTAINER
#define EDITOR_CONTAINER


#include "ModelObjects.h"
#include "OovProcess.h"
#include "Options.h"
#include "OovIpc.h"

void viewSource(GuiOptions const &guiOptions, OovStringRef const module,
        unsigned int lineNum);

class EditorListener
    {
    public:
        virtual ~EditorListener();
        virtual void handleEditorMessage(OovIpcMsg const &msg) = 0;
    };

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
        void setListener(EditorListener *listener)
            { mListener = listener; }
        void viewFile(OovStringRef const procPath, char const * const *argv,
            OovStringRef const fn, int lineNum);
        bool okToExit();

    private:
        ModelData const &mModelData;
        OovBackgroundPipeProcess mBackgroundProcess;
        OovString mReceivedData;
        EditorListener *mListener;

        /// WARNING: this is called from a background thread.
        void handleMessage(OovIpcMsg const &cmd);
        ModelClassifier const *findClass(OovStringRef name);

        /// WARNING: this is called from a background thread.
        virtual void onStdOut(OovStringRef const out, size_t len) override;
        virtual void onStdErr(OovStringRef const out, size_t len) override;
        virtual void processComplete()  override;
    };

#endif

