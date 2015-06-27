// ZoneDiagram.cpp

#include "ZoneDiagram.h"


void ZoneDiagram::initialize(const ModelData &modelData)
    {
    mModelData = &modelData;
    }

void ZoneDiagram::zoom(bool inc)
    {
    if(inc)
        {
        if(mZoom < 100)
            mZoom *= 1.5;
        }
    else
        {
        if(mZoom > .1)
            mZoom /= 1.5 ;
        }
    mZoneDrawer.setZoom(mZoom);
    updateNodesAndPositions();
    }

void ZoneDiagram::restart()
    {
    mZoom = 1;
    clearGraphAndAddWorldZone();
    }

void ZoneDiagram::drawDiagram(DiagramDrawer &diagDrawer)
    {
    mZoneDrawer.drawGraph(diagDrawer);
    }

void ZoneDiagram::updateNodesAndPositions()
    {
    mZoneGraph.updateGraph();
    }
