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
class ZoneComponentChanges:public std::vector<int>
    {
    public:
	int getComponentIndex(int classIndex);
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
		mCharWidth = mDrawer->getTextExtentWidth("W");
		}
	    }

	// Sets zoom for next drawGraph.
	void setZoom(double zoom)
	    { mZoom = zoom; }

	// This must be called before getting graph size or drawing.
	void updateGraph(ZoneGraph const &graph);
	GraphSize getGraphSize() const;
	void drawGraph(ZoneDrawOptions const &drawOptions);

	ZoneNode const *getZoneNode(GraphPoint screenPos) const;
	ZoneGraph const *getGraph() const
	    { return mGraph; }

    private:
	ZoneComponentChanges mComponentChanges;
        ZoneGraph const *mGraph;
	DiagramDrawer *mDrawer;
	float mScale;		// 1 = full drawing size of about a screen full.
	double mZoom;
	// This is the top left corner of a rectangle enclosing the circle.
	GraphPoint mTopLeftCircleOffset;
	int mCharWidth;
	GraphSize mGraphSize;
	bool mDrawNodeText;

    	void drawNode(size_t nodeIndex);
    	void drawConnections(ZoneDrawOptions const &drawOptions);
    	void drawSortedConnections(std::vector<ZoneConnectIndices> const &connections,
    		ZoneDrawOptions const &drawOptions);
    	void drawConnection(int firstNodeIndex, int secondNodeIndex,
    		eZoneDependencyDirections dir, ZoneDrawOptions const &drawOptions);
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
        float getCircleRadius(int numNodes) const
            {
            float radius = numNodes / 2.0 / M_PI;
            return(radius * mScale);
            }
        int getCircleCenterPosX(float radius) const
            { return(radius + mTopLeftCircleOffset.x); }
        int getCircleCenterPosY(float radius) const
            { return(radius + mTopLeftCircleOffset.y); }
        void drawNodeText();
        enum RelPositions { RL_TopLeftPos, RL_CenterPos };
        GraphPoint getNodePosition(double nodeIndex, int radiusXOffset=0, int radiusYOffset=0,
        	RelPositions=RL_CenterPos) const;
        size_t getNodeIndexFromComponentIndex(size_t componentIndex);

        GraphRect getNodeRect(size_t nodeIndex);
        // This gets the text position relative to the node position, which
        // is relative to the top left position of the graph.
        GraphRect getComponentTextRect(double nodeIndex) const;
        GraphRect getNodeTextRect(double nodeIndex) const;
        GraphRect getTextRect(double nodeIndex, const char *text, int offset) const;

        std::string getNodeComponentText(size_t nodeIndex) const;
        std::vector<ZoneConnectIndices> getSortedConnections(
        	ZoneConnections const &connections) const;
    };

#endif
