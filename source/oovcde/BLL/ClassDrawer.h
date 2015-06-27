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
        ClassDrawer(DiagramDrawer &drawer):
                mDrawer(drawer), mActualZoomX(1.0), mActualZoomY(1.0)
                {}
        void setZoom(double zoom);
        double getActualZoomX() const
                { return(mActualZoomX); }
        double getActualZoomY() const
                { return(mActualZoomY); }
        void drawDiagram(const ClassGraph &graph);
        // This is used publicly to calculate node sizes.
        GraphSize drawNode(const ClassNode &node);

    private:
        DiagramDrawer &mDrawer;
        double mActualZoomX;
        double mActualZoomY;

        // Typically node1 is consumer, and node2 is producer
        /// @todo - should make this constant and clear
        void drawConnectionLine(const ClassNode &node1, const ClassNode &node2);
        void drawConnectionSymbols(const ClassRelationDrawOptions &options,
            const ClassNode &node1,
            const ClassNode &node2, const ClassConnectItem &connectItem);
        GraphSize drawRelationKey(const ClassNode &node);
    };


#endif /* CLASSDRAWER_H_ */
