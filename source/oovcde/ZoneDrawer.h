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
	ZoneDrawer(DiagramDrawer *drawer):
            mGraph(nullptr), mDrawer(drawer), mScale(1), mZoom(1),
            mCharWidth(0), mDrawNodeText(false)
	    {
	    setDrawer(drawer);
	    }
	// Either the constructor or this must be called before any drawing.
	void setDrawer(DiagramDrawer *drawer)
	    {
	    if(drawer)
		{
		mDrawer = drawer;
		mCharWidth = static_cast<int>(mDrawer->getTextExtentWidth("W"));
		}
	    }
	DiagramDrawer *getDrawer()
	    { return mDrawer; }

	// Sets zoom for next drawGraph.
	void setZoom(double zoom)
	    { mZoom = zoom; }

	// This must be called before getting graph size or drawing.
	void updateGraph(ZoneGraph const &graph, bool resetPositions);
	GraphSize getDrawingSize() const;
	void drawGraph();

	ZoneNode const *getZoneNode(GraphPoint screenPos) const;
	// Must redraw the screen after calling this.
	void setPosition(ZoneNode const *node, GraphPoint origScreenPos,
		GraphPoint screenPos);
	ZoneGraph const *getGraph() const
	    { return mGraph; }

    private:
	ZoneComponentChanges mComponentChanges;
        ZoneGraph const *mGraph;
	DiagramDrawer *mDrawer;
	std::vector<GraphPoint> mNodePositions;
	// mScale is derived from mZoom, 1 = full drawing size of about a screen full.
	float mScale;
	double mZoom;
	// This is the top left corner of a rectangle enclosing all nodes and text.
	GraphPoint GraphDrawOffset;
	int mCharWidth;
	GraphSize mDrawingSize;
	bool mDrawNodeText;

	void updateNodePositions();
    	void drawNode(size_t nodeIndex);
    	void drawConnections();
    	void drawSortedConnections(std::vector<ZoneConnectIndices> const &connections);
    	void drawConnection(int firstNodeIndex, int secondNodeIndex,
    		eZoneDependencyDirections dir);
    	void drawConnectionArrow(GraphPoint firstPoint, GraphPoint secondPoint,
    		eZoneDependencyDirections dir);
    	void drawComponentBarriers();
        void drawComponentText();
        // Goes through the class nodes and finds where the module names
        // change between class nodes. This stores the indices in the returned
        // value.
        ZoneComponentChanges getComponentChanges() const;
        // This sets up the top left position of the graph by calling
        // getTextPosition. Since the top left position has not been set
        // the first time, the text position will return negative positions,
        // so this function adjusts the top left position so they will all
        // be positive.
        GraphRect getGraphObjectsSize();
        int getNodeRadius() const;
        float getCircleRadius(int numNodes) const;
        GraphPoint getCirclePos(float nodeIndex, int numNodes, int radiusOffset=0) const;
        GraphPoint getMasterCirclePos(float nodeIndex, int radiusOffset=0) const;
        int getMasterCircleCenterPosX(float radius) const
            { return(static_cast<int>(radius)); }
        int getMasterCircleCenterPosY(float radius) const
            { return(static_cast<int>(radius)); }
        void drawNodeText();
        GraphPoint getChildNodeCircleOffset(double nodeIndex) const;
        enum RelPositions { RL_TopLeftPos, RL_CenterPos };
        GraphPoint getNodePosition(double nodeIndex, RelPositions=RL_CenterPos) const;
        size_t getNodeIndexFromComponentIndex(size_t componentIndex);

        GraphRect getNodeRect(size_t nodeIndex);
        // This gets the text position relative to the node position, which
        // is relative to the top left position of the graph.
        GraphRect getComponentTextRect(double nodeIndex) const;
        GraphRect getNodeTextRect(double nodeIndex) const;
        /// @param component True for component, false for node
        GraphRect getTextRect(double nodeIndex, const char *text, bool component=false) const;

        std::string getNodeComponentText(size_t nodeIndex) const;
        std::vector<ZoneConnectIndices> getSortedConnections(
        	ZoneConnections const &connections) const;
    };

#endif
