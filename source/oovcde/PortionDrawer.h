/*
 * PortionDrawer.h
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef PORTION_DRAWER_H
#define PORTION_DRAWER_H

#include "PortionGraph.h"
#include "DiagramDrawer.h"


class PortionDrawer
    {
    public:
	PortionDrawer(DiagramDrawer *drawer=nullptr):
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
	void updateGraph(PortionGraph const &graph);
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
	PortionConnection getConnection(size_t index) const
	    { return mGraph->getConnections()[index]; }

    private:
        static const size_t NO_INDEX = static_cast<size_t>(-1);
        PortionGraph const *mGraph;
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
