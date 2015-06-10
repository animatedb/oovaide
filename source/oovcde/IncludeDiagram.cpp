/*
 * IncludeDiagram.cpp
 * Created on: June 5, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "IncludeDiagram.h"
#include "Svg.h"
#include "Journal.h"

static GraphPoint sStartPosInfo;

void IncludeDiagram::initialize(IncDirDependencyMapReader const &includeMap)
    {
    /// @todo - clean up context since it is needed at certain times.
    mCairoContext.setContext(getDrawingArea());
    mCairoDrawer.setGraphicsLib(mCairoContext.getCairo());
    mIncludeDrawer.setDrawer(&mCairoDrawer);
    mIncludeMap = &includeMap;
    mIncludeGraph.setGraphDataSource(includeMap);
    }

void IncludeDiagram::clearGraphAndAddInclude(OovStringRef incName)
    {
    mLastIncName = incName;
    mIncludeGraph.clearAndAddInclude(incName);
    mIncludeDrawer.updateGraph(mIncludeGraph);
    }

void IncludeDiagram::redraw()
    {
    GraphSize size = mIncludeDrawer.getDrawingSize();
    gtk_widget_queue_draw(getDrawingArea());
    gtk_widget_set_size_request(getDrawingArea(), size.x, size.y);
    }

void IncludeDiagram::updateDrawingAreaSize()
    {
    GraphSize size = mIncludeDrawer.getDrawingSize();
    gtk_widget_set_size_request(getDrawingArea(), size.x, size.y);
    }

void IncludeDiagram::drawToDrawingArea()
    {
    // The size must be set whenever a different diagram is displayed.
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

    mIncludeDrawer.setDrawer(&mCairoDrawer);
    mIncludeDrawer.drawGraph();
    cairo_stroke(mCairoContext.getCairo());
    }

void IncludeDiagram::drawSvgDiagram(FILE *fp)
    {
    mCairoContext.setContext(getDrawingArea());
    SvgDrawer svgDrawer(fp, mCairoContext.getCairo());

    mIncludeDrawer.setDrawer(&svgDrawer);
    mIncludeDrawer.drawGraph();
    mIncludeDrawer.setDrawer(&mCairoDrawer);
    }

GtkWidget *IncludeDiagram::getDrawingArea() const
    {
    return Builder::getBuilder()->getWidget("DiagramDrawingarea");
    }

static IncludeDiagram *sIncludeDiagram;

static void portionGraphDisplayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("DrawIncludePopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    }

void IncludeDiagram::graphButtonPressEvent(const GdkEventButton *event)
    {
    sStartPosInfo.set(event->x, event->y);
    sIncludeDiagram = this;
    }

void IncludeDiagram::graphButtonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
	{
	size_t nodeIndex = mIncludeDrawer.getNodeIndex(sStartPosInfo);
	if(nodeIndex != PortionDrawer::NO_INDEX)
	    {
	    mIncludeDrawer.setPosition(nodeIndex, sStartPosInfo,
		    GraphPoint(event->x, event->y));
	    redraw();
	    }
	}
    else
	{
	portionGraphDisplayContextMenu(event->button, event->time, (gpointer)event);
	}
    }

void IncludeDiagram::addSuppliers()
    {
    size_t index = mIncludeDrawer.getNodeIndex(sStartPosInfo);
    if(index != IncludeDrawer::NO_INDEX)
        {
        IncludeNode const &node = mIncludeGraph.getNodes()[index];
        mIncludeGraph.addSuppliers(node.getName());
        relayout();
        }
    }

void IncludeDiagram::viewFileSource()
    {
    size_t index = mIncludeDrawer.getNodeIndex(sStartPosInfo);
    if(index != IncludeDrawer::NO_INDEX)
        {
        IncludeNode const &node = mIncludeGraph.getNodes()[index];
        viewSource(node.getName(), 0);
        }
    }

extern "C" G_MODULE_EXPORT void on_IncludeAddSuppliersMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sIncludeDiagram->addSuppliers();
    }

extern "C" G_MODULE_EXPORT void on_IncludeViewSourceMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    sIncludeDiagram->viewFileSource();
    }

extern "C" G_MODULE_EXPORT void on_IncludeRestartMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sIncludeDiagram->restart();
    }

extern "C" G_MODULE_EXPORT void on_IncludeRelayoutMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    sIncludeDiagram->relayout();
    }

