/*
 * PortionDrawer.h
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef PORTION_DRAWER_H
#define PORTION_DRAWER_H

#include "PortionGraph.h"
#include "DiagramDrawer.h"


class PortionDrawer:public DiagramDependencyDrawer
    {
    public:
	PortionDrawer(DiagramDrawer *drawer=nullptr):
	    DiagramDependencyDrawer(drawer), mGraph(nullptr)
            {
            setDrawer(drawer);
            }
	void updateGraph(PortionGraph const &graph);
        GraphSize getDrawingSize() const;
	void drawGraph();
	void setPosition(size_t nodeIndex, GraphPoint startPoint, GraphPoint newPoint);

	size_t getNodeIndex(GraphPoint p) const
	    {
	    return DiagramDependencyDrawer::getNodeIndex(p,
	            mGraph->getNodes().size());
	    }
        virtual size_t getNumNodes() const override
            { return mNodePositions.size(); }
        virtual void setNodePosition(size_t nodeIndex, GraphPoint pos) override
            { mNodePositions[nodeIndex] = pos; }
        virtual GraphPoint getNodePosition(size_t nodeIndex) const override
            { return mNodePositions[nodeIndex]; }
        virtual OovString const &getNodeName(size_t nodeIndex) const override
            { return mGraph->getNodes()[nodeIndex].getName(); }
	virtual size_t getNumConnections() const override
	    { return mGraph->getConnections().size(); }
        virtual void getConnection(size_t ci, size_t &consumerIndex,
            size_t &supplierIndex) const override
            {
            auto const &conn = mGraph->getConnections()[ci];
            consumerIndex = conn.mConsumerNodeIndex;
            supplierIndex = conn.mSupplierNodeIndex;
            }

	PortionConnection getConnection(size_t index) const
	    { return mGraph->getConnections()[index]; }

    private:
        PortionGraph const *mGraph;
	std::vector<GraphPoint> mNodePositions;

        void drawNodes();
        void drawConnections();
        void drawNodeText();
	void updateNodePositions();
	/// This returns a dependency depth for each node.
	/// Operations start at a depth of 1, and attributes start at 0.
	std::vector<size_t> getCallDepths() const;
	void fillDepths(size_t nodeIndex, std::vector<size_t> &depths) const;
    };


#endif
