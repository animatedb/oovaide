/*
 * ZoneDrawer.h
 * Created on: Feb 9, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef ZONE_DRAWER_H
#define ZONE_DRAWER_H

#include "ZoneGraph.h"
#include "DiagramDrawer.h"


// Keeps a set of indices to the class nodes where components have different
// names. The original nodes must be in component name sorted order.
// Each element contains the first class index of the changed component.
// The first class is not considered a change, so zero is never in the vector.
// The last class is considered a change, and is always present.
class ZoneComponentChanges:public std::vector<size_t>
    {
    public:
        size_t getComponentIndex(size_t classIndex) const;
        // The node index from within the component.
        size_t getComponentChildClassIndex(size_t classIndex) const;

        size_t getBaseComponentChildClassIndex(size_t componentIndex) const;
        size_t getMiddleComponentChildClassIndex(size_t componentIndex) const;
        size_t getNumClassesForComponent(size_t componentIndex) const;
    };

/// This is used to draw a zone (circle) diagram. This is a diagram that shows
/// shows relations between either classes or components.
class ZoneDrawer
    {
    public:
        ZoneDrawer(Diagram const  &diagram):
            mDiagram(diagram), mGraph(nullptr), mScale(1), mZoom(1),
            mDrawNodeText(false)
            {}

        // Sets zoom for next drawGraph.
        void setZoom(double zoom)
            { mZoom = zoom; }

        // This must be called before getting graph size or drawing.
        void updateGraph(DiagramDrawer &drawer, ZoneGraph const &graph,
                bool resetPositions);
        GraphSize getDrawingSize() const;
        void drawGraph(DiagramDrawer &drawer);

        ZoneNode const *getZoneNode(GraphPoint screenPos) const;
        // Must redraw the screen after calling this.
        void setPosition(ZoneNode const *node, GraphPoint origScreenPos,
                GraphPoint screenPos);
//        ZoneGraph const *getGraph() const
//            { return mGraph; }

    private:
        Diagram const  &mDiagram;
        ZoneComponentChanges mComponentChanges;
        ZoneGraph const *mGraph;
        // Positions are saved without zoom. This is so that zoom does not
        // need to change positions.
        std::vector<GraphPoint> mNodePositions;
        // mScale is derived from mZoom, 1 = full drawing size of about a screen full.
        float mScale;
        double mZoom;
        // This is the top left corner of a rectangle enclosing all nodes and text.
        GraphPoint GraphDrawOffset;
        GraphSize mDrawingSize;
        bool mDrawNodeText;

        void updateNodePositions();
        void drawNode(DiagramDrawer &drawer, size_t nodeIndex);
        void drawConnections(DiagramDrawer &drawer);
        void drawSortedConnections(DiagramDrawer &drawer,
                std::vector<ZoneConnectIndices> const &connections);
        void drawConnection(DiagramDrawer &drawer, int firstNodeIndex,
                int secondNodeIndex, eZoneDependencyDirections dir);
        void drawConnectionArrow(DiagramDrawer &drawer, GraphPoint firstPoint,
                GraphPoint secondPoint, eZoneDependencyDirections dir);
        void drawComponentBarriers(DiagramDrawer &drawer);
        void drawComponentText(DiagramDrawer &drawer);
        // Goes through the class nodes and finds where the module names
        // change between class nodes. This stores the indices in the returned
        // value.
        ZoneComponentChanges getComponentChanges() const;
        // This sets up the top left position of the graph by calling
        // getTextPosition. Since the top left position has not been set
        // the first time, the text position will return negative positions,
        // so this function adjusts the top left position so they will all
        // be positive.
        GraphRect getGraphObjectsSize(DiagramDrawer &drawer);
        int getNodeRadius(DiagramDrawer &drawer) const;
        float getCircleRadius(int numNodes) const;
        GraphPoint getCirclePos(float nodeIndex, int numNodes, int radiusOffset=0) const;
        GraphPoint getUnzoomedMasterCirclePos(float nodeIndex, int radiusOffset=0) const;
        GraphPoint getMasterCirclePos(float nodeIndex, int radiusOffset=0) const;
        int getMasterCircleCenterPosX(float radius) const
            { return(static_cast<int>(radius)); }
        int getMasterCircleCenterPosY(float radius) const
            { return(static_cast<int>(radius)); }
        void drawNodeText(DiagramDrawer &drawer);
        GraphPoint getChildNodeCircleOffset(double nodeIndex) const;
        enum RelPositions { RL_TopLeftPos, RL_CenterPos };
        GraphPoint getNodePosition(DiagramDrawer &drawer, double nodeIndex,
                RelPositions=RL_CenterPos) const;
        size_t getNodeIndexFromComponentIndex(size_t componentIndex);

        GraphRect getNodeRect(DiagramDrawer &drawer, size_t nodeIndex);
        // This gets the text position relative to the node position, which
        // is relative to the top left position of the graph.
        GraphRect getComponentTextRect(DiagramDrawer &drawer, double nodeIndex) const;
        GraphRect getNodeTextRect(DiagramDrawer &drawer, double nodeIndex) const;
        /// @param component True for component, false for node
        GraphRect getTextRect(DiagramDrawer &drawer, double nodeIndex,
                const char *text, bool component=false) const;

        std::string getNodeComponentText(size_t nodeIndex) const;
        std::vector<ZoneConnectIndices> getSortedConnections(
                ZoneConnections const &connections) const;
    };

#endif
