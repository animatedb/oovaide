// EditorContainer.cpp
//  Created on: July 15, 2013
//  \copyright 2013 DCBlaha.  Distributed under the GPL.


#include "EditorContainer.h"

EditorContainer::EditorContainer(ModelData const &modelData):
    mModelData(modelData)
    {
    }

ModelClassifier const *EditorContainer::findClass(OovStringRef name)
    {
//    ModelType const *type = mModelData.findType(name);
    ModelClassifier const *classifier = nullptr;
//    if(type)
        {
//        classifier = ModelType::getClass(type);
        }
    return classifier;
    }

