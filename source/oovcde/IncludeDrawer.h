/*
 * IncludeDrawer.h
 * Created on: June 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef INCLUDE_DRAWER_H
#define INCLUDE_DRAWER_H

#include "IncludeGraph.h"
#include "DiagramDrawer.h"


class IncludeDrawer
    {
    public:
	IncludeDrawer(DiagramDrawer *drawer=nullptr):
	    mGraph(nullptr)
            {
            setDrawer(drawer);
            }
	void setDrawer(DiagramDrawer *drawer)
	    {
	    if(drawer)
		{
		mDrawer = drawer;
		}
	    }
	void updateGraph(IncludeGraph const &graph);
        GraphSize getDrawingSize() const;
	void drawGraph();
	size_t getNodeIndex(GraphPoint p);
	void setPosition(size_t nodeIndex, GraphPoint startPoint, GraphPoint newPoint);

	GraphPoint getPosition(size_t nodeIndex) const
	    { return mNodePositions[nodeIndex]; }
	void setPosition(size_t nodeIndex, GraphPoint pos)
	    { mNodePositions[nodeIndex] = pos; }
	size_t getNumNodes() const
	    { return mNodePositions.size(); }
	size_t getNumConnections() const
	    { return mGraph->getConnections().size(); }
	IncludeConnection getConnection(size_t index) const
	    { return mGraph->getConnections()[index]; }
        static const size_t NO_INDEX = static_cast<size_t>(-1);

    private:
        IncludeGraph const *mGraph;
	DiagramDrawer *mDrawer;
	std::vector<GraphPoint> mNodePositions;

        void drawNodes();
        void drawConnections();
        void drawNodeText();
        GraphRect getNodeRect(size_t nodeIndex) const;
	void updateNodePositions();
	std::vector<size_t> getCallDepths() const;
	/// This returns positions without the margin.
	std::vector<int> getColumnPositions(std::vector<size_t> const &depths) const;
	bool fillDepths(size_t nodeIndex, std::vector<size_t> &depths) const;
    };


#endif
