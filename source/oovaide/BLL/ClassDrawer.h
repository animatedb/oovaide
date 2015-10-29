/*
 * ClassDrawer.h
 *
 *  Created on: Jul 23, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CLASSDRAWER_H_
#define CLASSDRAWER_H_

#include "DiagramDrawer.h"
#include "ClassGraph.h"

/// This is used to draw a class diagram.
class ClassDrawer
    {
    public:
        ClassDrawer(Diagram const &diagram, DiagramDrawer &drawer):
            mDiagram(diagram), mDrawer(drawer), mActualZoom(1.0)
            {}
        void setZoom(double zoom);
// DEAD CODE
//        double getActualZoom() const
//                { return(mActualZoom); }

        void drawDiagram(const ClassGraph &graph);
        // This is used publicly to calculate node sizes.
        GraphSize drawNode(const ClassNode &node);

    private:
        Diagram const &mDiagram;
        DiagramDrawer &mDrawer;
        double mActualZoom;

        // Typically node1 is consumer, and node2 is producer
        /// @todo - should make this constant and clear
        void drawConnectionLine(const ClassNode &node1, const ClassNode &node2,
            bool dashed = false);
        void drawConnectionSymbols(const ClassRelationDrawOptions &options,
            const ClassNode &node1,
            const ClassNode &node2, const ClassConnectItem &connectItem);
        GraphSize drawRelationKey(const ClassNode &node);
    };


#endif /* CLASSDRAWER_H_ */
