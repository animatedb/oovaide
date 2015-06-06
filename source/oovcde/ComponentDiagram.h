/*
 * ComponentDiagram.h
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef COMPONENTDIAGRAM_H_
#define COMPONENTDIAGRAM_H_

#include "ComponentGraph.h"
#include "Builder.h"

/// This defines functions used to interact with a component diagram. The
/// ComponentDiagram uses the ComponentDrawer to draw the ComponentGraph.
class ComponentDiagram
    {
    public:
	ComponentDiagram():
	    mDrawingArea(nullptr)
	    {}
	void initialize(Builder &builder, IncDirDependencyMapReader const &incMap);
	void updateGraph();
	void drawSvgDiagram(FILE *fp);
	void drawToDrawingArea();
	const ComponentGraph &getGraph() const
	    { return mComponentGraph; }
	ComponentGraph &getGraph()
	    { return mComponentGraph; }

	// For use by extern functions.
	void buttonPressEvent(const GdkEventButton *event);
	void buttonReleaseEvent(const GdkEventButton *event);
	void restart()
	    { updateGraph(); }
	void relayout()
	    { updatePositionsInGraph(); }
	void removeComponent();

    private:
	GtkWidget *mDrawingArea;
	ComponentGraph mComponentGraph;

	void updatePositionsInGraph();
	void displayContextMenu(guint button, guint32 acttime, gpointer data);
	void redraw()
	    { gtk_widget_queue_draw(mDrawingArea); }
        void updateDrawingAreaSize();
    };


#endif /* COMPONENTDIAGRAM_H_ */
