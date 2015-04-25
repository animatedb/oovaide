/*
 * PortionDiagram.h
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef PORTION_DIAGRAM_H
#define PORTION_DIAGRAM_H

#include "PortionDrawer.h"
#include "CairoDrawer.h"
#include "Gui.h"

class PortionDiagram
    {
    public:
        PortionDiagram():
	    mModelData(nullptr), mCairoDrawer(nullptr)
            {}
	void initialize(const ModelData &modelData);
	void clearGraphAndAddClass(OovStringRef className);
	void redraw();

	// For use by extern functions.
	void graphButtonPressEvent(const GdkEventButton *event);
	void graphButtonReleaseEvent(const GdkEventButton *event);

	// This should be called from the draw event.
	void drawToDrawingArea();
	void drawSvgDiagram(FILE *fp);
	GtkWidget *getDrawingArea() const;

    private:
	const ModelData *mModelData;
	GtkCairoContext mCairoContext;
	/// @todo - the drawer should only be needed during drawing and not for positions?
	// The drawer only needs to be a member to get font sizes for positions.
	CairoDrawer mCairoDrawer;
	PortionGraph mPortionGraph;
	PortionDrawer mPortionDrawer;
    };

#endif
