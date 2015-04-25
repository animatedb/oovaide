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

    private:
        PortionGraph const *mGraph;
	DiagramDrawer *mDrawer;
	std::vector<GraphPoint> mNodePositions;

        GraphRect getNodeRect(int nodeIndex) const;
        void drawNodes();
        void drawConnections();
        void drawNodeText();
	void updateNodePositions();
	void fillDepths(std::vector<int> &depths) const;
	bool attemptTofillDepth(size_t nodeIndex, std::vector<int> &depths) const;
    };


#endif
