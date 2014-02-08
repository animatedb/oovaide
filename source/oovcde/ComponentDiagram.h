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

class ComponentDiagram
    {
    public:
	ComponentDiagram():
	    mDrawingArea(nullptr)
	    {}
	void initialize(Builder &builder);
	void updateGraph()
	    { mComponentGraph.updateGraph(); }
	void drawSvgDiagram(FILE *fp);
	void drawToDrawingArea();
	const ComponentGraph &getGraph() const
	    { return mComponentGraph; }

	// For use by extern functions.
	void buttonPressEvent(const GdkEventButton *event);
	void buttonReleaseEvent(const GdkEventButton *event);

    private:
	GtkWidget *mDrawingArea;
	ComponentGraph mComponentGraph;
	void updatePositionsInGraph();
    };


#endif /* COMPONENTDIAGRAM_H_ */
