/*
 * OperationDiagram.cpp
 *
 *  Created on: Jul 29, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "OperationDiagram.h"


void OperationDiagram::clearGraphAndAddOperation(OovStringRef const className,
        OovStringRef const opName, bool isConst)
    {
    mOpGraph.clearGraphAndAddOperation(*mModelData,
            mOptions, className, opName, isConst, 2);
    mLastOperDiagramParams.mClassName = className;
    mLastOperDiagramParams.mOpName = opName;
    mLastOperDiagramParams.mIsConst = isConst;
    }

void OperationDiagram::restart()
    {
    clearGraphAndAddOperation(mLastOperDiagramParams.mClassName,
            mLastOperDiagramParams.mOpName, mLastOperDiagramParams.mIsConst);
    }

void OperationDiagram::drawDiagram(DiagramDrawer &drawer)
    {
    mOperationDrawer.drawDiagram(drawer, mOpGraph, mOptions);
    }
