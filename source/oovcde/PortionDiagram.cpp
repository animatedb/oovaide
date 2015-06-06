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
    mPortionGraph.setGraphDataSource(modelData);
    }

void PortionDiagram::clearGraphAndAddClass(OovStringRef className)
    {
    mCurrentClassName = className;
    mPortionGraph.clearAndAddClass(className);
    mPortionDrawer.updateGraph(mPortionGraph);
    }

void PortionDiagram::redraw()
    {
    gtk_widget_queue_draw(getDrawingArea());
    }

void PortionDiagram::updateDrawingAreaSize()
    {
    GraphSize size = mPortionDrawer.getDrawingSize();
    gtk_widget_set_size_request(getDrawingArea(), size.x, size.y);
    }

void PortionDiagram::drawToDrawingArea()
    {
    /// @todo - Telling the widget to change size inside of the repaint
    /// function is probably not a great idea, but the size must be set
    /// whenever a different diagram is displayed.  This seems to work
    /// ok, but I am not sure if it causes a double paint.
    updateDrawingAreaSize();

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

static PortionDiagram *sPortionDiagram;

static void portionGraphDisplayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("DrawPortionPopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    }

void PortionDiagram::graphButtonPressEvent(const GdkEventButton *event)
    {
    sStartPosInfo.set(event->x, event->y);
    sPortionDiagram = this;
    }

void PortionDiagram::graphButtonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
	{
	size_t nodeIndex = mPortionDrawer.getNodeIndex(sStartPosInfo);
	if(nodeIndex != PortionDrawer::NO_INDEX)
	    {
	    mPortionDrawer.setPosition(nodeIndex, sStartPosInfo,
		    GraphPoint(event->x, event->y));
	    redraw();
	    }
	}
    else
	{
	portionGraphDisplayContextMenu(event->button, event->time, (gpointer)event);
	}
    }

void PortionDiagram::viewClassSource()
    {
    const ModelClassifier *classifier = nullptr;
    ModelType const *type = mModelData->getTypeRef(mCurrentClassName);
    if(type)
	{
	classifier = type->getClass();
	}
    if(classifier && classifier->getModule())
	{
	viewSource(classifier->getModule()->getModulePath(),
	classifier->getLineNum());
	}
    }


extern "C" G_MODULE_EXPORT void on_PortionViewSourceMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    sPortionDiagram->viewClassSource();
    }

extern "C" G_MODULE_EXPORT void on_PortionRelayoutMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    sPortionDiagram->relayout();
    }

