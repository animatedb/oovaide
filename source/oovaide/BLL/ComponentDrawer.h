/*
 * ComponentDrawer.h
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTDRAWER_H_
#define COMPONENTDRAWER_H_

#include "DiagramDrawer.h"
#include "ComponentGraph.h"

/// This is used to draw a component diagram
class ComponentDrawer
    {
    public:
        ComponentDrawer(Diagram const &diagram, DiagramDrawer &drawer):
            mDiagram(diagram), mDrawer(drawer)
            {}
        void drawDiagram(const ComponentGraph &graph);
        // This is used publicly to calculate node sizes.
        GraphSize drawNode(const ComponentNode &node);

    private:
        Diagram const &mDiagram;
        DiagramDrawer &mDrawer;
        void drawConnection(const ComponentGraph &graph,
                ComponentConnection const &connect);
    };

#endif /* COMPONENTDRAWER_H_ */
