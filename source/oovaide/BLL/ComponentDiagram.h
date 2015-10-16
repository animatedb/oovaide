/*
 * ComponentDiagram.h
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTDIAGRAM_H_
#define COMPONENTDIAGRAM_H_

#include "ComponentGraph.h"
#include "DiagramDrawer.h"

/// This defines functions used to interact with a component diagram. The
/// ComponentDiagram uses the ComponentDrawer to draw the ComponentGraph.
class ComponentDiagram
    {
    public:
        void initialize(IncDirDependencyMapReader const &incMap);

        // Adds all nodes/components and repositions
        void restart(ComponentDrawOptions const &options, DiagramDrawer &nullDrawer)
            { updateGraph(options, nullDrawer); }
        // Changes positions
        void relayout(DiagramDrawer &nullDrawer)
            { updatePositionsInGraph(nullDrawer); }
        void removeNode(const ComponentNode &node, const ComponentDrawOptions &options)
            { mComponentGraph.removeNode(node, options); }

        /// This can be used to paint to a window, or to an SVG file.
        void drawDiagram(DiagramDrawer &drawer);

        ComponentNode *getNode(int x, int y)
            { return mComponentGraph.getNode(x, y); }
        GraphSize getGraphSize() const
            { return mComponentGraph.getGraphSize(); }
// DEAD CODE
//        bool isModified() const
//            { return mComponentGraph.isModified(); }
        void setModified()
            { mComponentGraph.setModified(); }

    private:
        ComponentGraph mComponentGraph;

        void updatePositionsInGraph(DiagramDrawer &nullDrawer);
        // Load components and reposition
        void updateGraph(ComponentDrawOptions const &options,
                DiagramDrawer &nullDrawer);
    };


#endif /* COMPONENTDIAGRAM_H_ */
