/*
 * IncludeDiagram.h
 * Created on: June 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#ifndef INCLUDE_DIAGRAM_H
#define INCLUDE_DIAGRAM_H

#include "IncludeDrawer.h"
#include "CairoDrawer.h"
#include "Gui.h"

class IncludeDiagram
    {
    public:
        IncludeDiagram():
	    mIncludeMap(nullptr), mCairoDrawer(nullptr)
            {}
	void initialize(const IncDirDependencyMapReader &includeMap);
	void clearGraphAndAddInclude(OovStringRef incName);
	void redraw();
	void restart()
	    {
	    clearGraphAndAddInclude(mLastIncName);
	    drawToDrawingArea();
	    }
	void relayout()
	    {
	    mIncludeDrawer.updateGraph(mIncludeGraph);
	    redraw();
	    }

	// For use by extern functions.
	void graphButtonPressEvent(const GdkEventButton *event);
	void graphButtonReleaseEvent(const GdkEventButton *event);

	// This should be called from the draw event.
	void drawToDrawingArea();
	void drawSvgDiagram(FILE *fp);
	GtkWidget *getDrawingArea() const;
	void addSuppliers();
	void viewFileSource();

    private:
	const IncDirDependencyMapReader *mIncludeMap;
	GtkCairoContext mCairoContext;
	OovString mLastIncName;
	/// @todo - the drawer should only be needed during drawing and not for positions?
	// The drawer only needs to be a member to get font sizes for positions.
	CairoDrawer mCairoDrawer;
        IncludeGraph mIncludeGraph;
        IncludeDrawer mIncludeDrawer;

        void updateDrawingAreaSize();
    };

#endif
