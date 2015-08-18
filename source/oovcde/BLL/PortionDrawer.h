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
        PortionDrawer():
            mGraph(nullptr)
            {}
        void setGraphSource(PortionGraph const &graph)
            { mGraph = &graph; }
        void updateNodePositions(DiagramDrawer &drawer);
        GraphSize getDrawingSize(DiagramDrawer &drawer) const;
        void drawGraph(DiagramDrawer &drawer);
        void setPosition(size_t nodeIndex, GraphPoint startPoint, GraphPoint newPoint);
        void setPosition(size_t nodeIndex, GraphPoint newPoint);

        size_t getNodeIndex(DiagramDrawer &drawer, GraphPoint p) const
            {
            return DiagramDependencyDrawer::getNodeIndex(drawer,
                    p, mGraph->getNodes().size());
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

        void drawNodes(DiagramDrawer &drawer);
        void drawConnections(DiagramDrawer &drawer);
        void getNodeText(DiagramDrawer &drawer,
            std::vector<DrawString> &drawStrings, std::vector<bool> &virtOpers);
        void drawNodeText(DiagramDrawer &drawer, bool drawVirts,
            std::vector<DrawString> const &drawStrings, std::vector<bool> const &virtOpers);
        /// This returns a dependency depth for each node.
        /// Operations start at a depth of 1, and attributes start at 0.
        std::vector<size_t> getCallDepths() const;
        void fillDepths(size_t nodeIndex, std::vector<size_t> &depths) const;
    };


#endif
