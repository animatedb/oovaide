/*
 * PortionDiagram.cpp
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "PortionDiagram.h"
#include "Svg.h"
#include "Journal.h"

static GraphPoint sStartPosInfo;

void PortionDiagram::initialize(const ModelData &modelData)
    {
    /// @todo - clean up context since it is needed at certain times.
    mCairoContext.setContext(getDrawingArea());
    mCairoDrawer.setGraphicsLib(mCairoContext.getCairo());
    mPortionDrawer.setDrawer(&mCairoDrawer);
    mModelData = &modelData;
    }

void PortionDiagram::clearGraphAndAddClass(OovStringRef className)
    {
    mPortionGraph.clearAndAddClass(*mModelData, className);
    mPortionDrawer.updateGraph(mPortionGraph);
    }

void PortionDiagram::redraw()
    {
    GraphSize size = mPortionDrawer.getDrawingSize();
    gtk_widget_queue_draw(getDrawingArea());
    gtk_widget_set_size_request(getDrawingArea(), size.x, size.y);
    }

void PortionDiagram::drawToDrawingArea()
    {
    /// @todo - Context must be set every time it needs to draw.
    mCairoContext.setContext(getDrawingArea());
    mCairoDrawer.setGraphicsLib(mCairoContext.getCairo());
    // Clear the background.
    cairo_set_source_rgb(mCairoContext.getCairo(), 255,255,255);
    cairo_paint(mCairoContext.getCairo());

    // Set black stroke
    cairo_set_source_rgb(mCairoContext.getCairo(), 0,0,0);
    cairo_set_line_width(mCairoContext.getCairo(), 1.0);

    mPortionDrawer.setDrawer(&mCairoDrawer);
    mPortionDrawer.drawGraph();
    cairo_stroke(mCairoContext.getCairo());
    }

void PortionDiagram::drawSvgDiagram(FILE *fp)
    {
    mCairoContext.setContext(getDrawingArea());
    SvgDrawer svgDrawer(fp, mCairoContext.getCairo());

    mPortionDrawer.setDrawer(&svgDrawer);
    mPortionDrawer.drawGraph();
    mPortionDrawer.setDrawer(&mCairoDrawer);
    }

GtkWidget *PortionDiagram::getDrawingArea() const
    {
    return Builder::getBuilder()->getWidget("DiagramDrawingarea");
    }

void PortionDiagram::graphButtonPressEvent(const GdkEventButton *event)
    {
    sStartPosInfo.set(event->x, event->y);
    }

void PortionDiagram::graphButtonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
	{
	size_t nodeIndex = mPortionDrawer.getNodeIndex(sStartPosInfo);
	if(nodeIndex != -1)
	    {
	    mPortionDrawer.setPosition(nodeIndex, sStartPosInfo,
		    GraphPoint(event->x, event->y));
	    redraw();
	    }
	}
    else
	{
	// Display context menu.
	}
    }
