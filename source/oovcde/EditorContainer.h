// EditorContainer.h
//  Created on: July 15, 2015
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#ifndef EDITOR_CONTAINER
#define EDITOR_CONTAINER

#include "ModelObjects.h"
#include "OovProcess.h"

class EditorContainer:public OovBackgroundPipeProcess
    {
    public:
        EditorContainer(ModelData const &modelData);

    private:
        ModelData const &mModelData;
        ModelClassifier const *findClass(OovStringRef name);
    };


#endif

