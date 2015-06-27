/*
 * IncludeDiagram.cpp
 * Created on: June 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "IncludeDiagram.h"

void IncludeDiagram::initialize(IncDirDependencyMapReader const &includeMap)
    {
    mIncludeMap = &includeMap;
    mIncludeGraph.setGraphDataSource(includeMap);
    }

void IncludeDiagram::clearGraphAndAddInclude(DiagramDrawer &drawer, OovStringRef incName)
    {
    mLastIncName = incName;
    mIncludeGraph.clearAndAddInclude(incName);
    mIncludeDrawer.updateGraph(drawer, mIncludeGraph);
    }

void IncludeDiagram::drawDiagram(DiagramDrawer &diagDrawer)
    {
    mIncludeDrawer.drawGraph(diagDrawer);
    }

