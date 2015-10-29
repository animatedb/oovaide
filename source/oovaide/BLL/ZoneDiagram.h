/*
 * ZoneDiagram.h
 * Created on: Feb 9, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef ZONE_DIAGRAM_H
#define ZONE_DIAGRAM_H

#include "ZoneDrawer.h"


/// This defines functions used to interact with a zone diagram. The
/// ZoneDiagram uses the ZoneDrawer to draw the ZoneGraph.
/// This must remain for the life of the program since GUI events can be
/// generated any time.
class ZoneDiagram:public Diagram
    {
    public:
        ZoneDiagram():
            mModelData(nullptr), mZoneDrawer(*this), mZoom(1),
            mResetPositions(false)
            {}
        void initialize(const ModelData &modelData);
        /// Call updateDiagram after this
        void clearGraph()
            {
            mResetPositions = true;
            mZoneGraph.clearGraph();
            }
        void clearGraphAndAddWorldZone()
            {
            mResetPositions = true;
            mZoneGraph.clearAndAddWorldNodes(*mModelData);
            }
        void updateNodesAndPositions();

        ZoneNode const *getZoneNode(GraphPoint screenPos) const
            { return mZoneDrawer.getZoneNode(screenPos); }
        // The user can manually change positions.
        void setPosition(ZoneNode const *node, GraphPoint origScreenPos,
                GraphPoint screenPos)
            { mZoneDrawer.setPosition(node, origScreenPos, screenPos); }
        void setFilter(std::string moduleName, bool set)
            {
            mResetPositions = true;
            mZoneGraph.setFilter(moduleName, set);
            }
        void setDrawOptions(ZoneDrawOptions const &options)
            {
            if(options.mDrawChildCircles != mZoneGraph.getDrawOptions().mDrawChildCircles)
                {
                mResetPositions = true;
                }
            mZoneGraph.setDrawOptions(options);
            }
        // Updates all requested changes up to this point.
        void updateGraph(DiagramDrawer &drawer)
            {
            mZoneGraph.updateGraph();
            mZoneDrawer.updateGraph(drawer, mZoneGraph, mResetPositions);
            mResetPositions = false;
            }
        ZonePathMap &getPathMap()
            { return mZoneGraph.getPathMap(); }

        /// This can be used to paint to a window, or to an SVG file.
        void drawDiagram(DiagramDrawer &drawer);
        void restart();
        void zoom(bool inc);
        bool isModified() const
            { return mZoneGraph.isModified(); }

        GraphSize getDrawingSize()
            { return mZoneDrawer.getDrawingSize(); }
        ZoneDrawOptions const &getDrawOptions() const
            { return mZoneGraph.getDrawOptions(); }

    private:
        const ModelData *mModelData;
        ZoneGraph mZoneGraph;
        ZoneDrawer mZoneDrawer;
        double mZoom;
        bool mResetPositions;
    };

#endif
