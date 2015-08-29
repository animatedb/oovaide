/*
 * OperationDrawer.h
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPERATIONDRAWER_H_
#define OPERATIONDRAWER_H_

#include "DiagramDrawer.h"
#include "OperationGraph.h"
#include <set>


class BlockPolygon:public OovPolygon
    {
    public:
        BlockPolygon():
            mDepth(0), mCenterLineX(0), mPad(0)
            {}
        void setup(int centerLineX, int pad)
            {
            mCenterLineX = centerLineX;
            mPad = pad;
            }
        void incDepth(int y);
        void decDepth(int y);
        void finishBlock();
        int getDepth() const
            { return mDepth; }

    private:
        int mDepth;
        int mCenterLineX;
        int mPad;
    };


/// Used to draw an operation graph.
class OperationDrawer
    {
    public:
        OperationDrawer():
            mPad(0), mCharHeight(0)
            {}

        // Graph is not const because class positions get updated.
        GraphSize drawDiagram(DiagramDrawer &drawer, OperationGraph &graph,
                const OperationDrawOptions &options);
        GraphSize getDrawingSize(DiagramDrawer &drawer, OperationGraph &graph,
                const OperationDrawOptions &options);

    private:
        static const size_t NO_INDEX = static_cast<size_t>(-1);
        int mPad;
        int mCharHeight;
        std::vector<BlockPolygon> mLifelinePolygons;
        GraphSize drawClass(DiagramDrawer &drawer, const OperationClass &node,
                const OperationDrawOptions &options, bool draw);
        void drawLifeLines(DiagramDrawer &drawer,
                const std::vector<OperationClass> &classes,
                std::vector<int> const &classEndY, int endy);
        /// @todo - This stuff is pretty ugly. The draw flag should be removed,
        /// and some functions for getting size should be separately created.
        GraphSize drawOperation(DiagramDrawer &drawer, GraphPoint pos,
                OperationDefinition &operDef, const OperationGraph &graph,
                const OperationDrawOptions &options,
                std::set<const OperationDefinition*> &drawnOperations, bool draw);
        GraphSize drawOperationNoText(DiagramDrawer &drawer, GraphPoint pos,
                OperationDefinition &operDef,
                const OperationGraph &graph, const OperationDrawOptions &options,
                std::set<const OperationDefinition*> &drawnOperations,
                std::vector<DrawString> &drawStrings, bool draw, int callDepth=0);
        GraphSize drawOrSizeDiagram(DiagramDrawer &drawer, OperationGraph &graph,
                const OperationDrawOptions &options, bool draw);
    };

#endif /* OPERATIONDRAWER_H_ */
