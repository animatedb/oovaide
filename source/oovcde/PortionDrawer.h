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

	GraphPoint getPosition(int nodeIndex) const
	    { return mNodePositions[nodeIndex]; }
	void setPosition(int nodeIndex, GraphPoint pos)
	    { mNodePositions[nodeIndex] = pos; }
	size_t getNumNodes() const
	    { return mNodePositions.size(); }
	size_t getNumConnections() const
	    { return mGraph->getConnections().size(); }
	PortionConnection getConnection(int index) const
	    { return mGraph->getConnections()[index]; }

    private:
        PortionGraph const *mGraph;
	DiagramDrawer *mDrawer;
	std::vector<GraphPoint> mNodePositions;

        void drawNodes();
        void drawConnections();
        void drawNodeText();
        GraphRect getNodeRect(int nodeIndex) const;
	void updateNodePositions();
	void findCallDepths(std::vector<int> &depths) const;
	bool attemptTofillDepth(size_t nodeIndex, std::vector<int> &depths) const;
    };


#endif
