/*
 * IncludeDiagramView.cpp
 *
 *  Created on: Jun 18, 2015
 *  \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "IncludeDiagramView.h"
#include "Svg.h"
#include "Options.h"

static GraphPoint sStartPosInfo;


/*
static const IncludeDrawOptions &getDrawOptions()
    {
    static IncludeDrawOptions iopts;
    return iopts;
    }
*/

void IncludeDiagramView::restart()
    {
    setCairoContext();
    mIncludeDiagram.restart(mNullDrawer);
    requestRedraw();
    }

void IncludeDiagramView::drawToDrawingArea()
    {
    GtkCairoContext cairo(getDiagramWidget());
    CairoDrawer cairoDrawer(cairo.getCairo());
    cairoDrawer.clearAndSetDefaults();

    mIncludeDiagram.drawDiagram(cairoDrawer);
    updateDrawingAreaSize();
    }

void IncludeDiagramView::updateDrawingAreaSize()
    {
    GraphSize size = mIncludeDiagram.getDrawingSize(mNullDrawer);
    gtk_widget_set_size_request(getDiagramWidget(), size.x, size.y);
    }

OovStatusReturn IncludeDiagramView::drawSvgDiagram(File &file)
    {
    GtkCairoContext cairo(getDiagramWidget());
    SvgDrawer svgDrawer(file, cairo.getCairo());
    mIncludeDiagram.drawDiagram(svgDrawer);
    return svgDrawer.writeFile();
    }

static IncludeDiagramView *sIncludeDiagramView;

static void includeGraphDisplayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("DrawIncludePopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    }

void IncludeDiagramView::graphButtonPressEvent(const GdkEventButton *event)
    {
    sStartPosInfo.set(event->x, event->y);
    sIncludeDiagramView = this;
    }

void IncludeDiagramView::graphButtonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
        {
        size_t nodeIndex = mIncludeDiagram.getNodeIndex(mNullDrawer,
                sStartPosInfo);
        if(nodeIndex != IncludeDrawer::NO_INDEX)
            {
            mIncludeDiagram.setPosition(nodeIndex, sStartPosInfo,
                    GraphPoint(event->x, event->y));
            requestRedraw();
            }
        }
    else
        {
        includeGraphDisplayContextMenu(event->button, event->time, (gpointer)event);
        }
    }

void IncludeDiagramView::addSuppliers()
    {
    size_t index = mIncludeDiagram.getNodeIndex(mNullDrawer, sStartPosInfo);
    if(index != IncludeDrawer::NO_INDEX)
        {
        IncludeNode const &node = mIncludeDiagram.getNodes()[index];
        mIncludeDiagram.addSuppliers(node.getName());
        relayout();
        }
    }

void IncludeDiagramView::viewFileSource()
    {
    size_t index = mIncludeDiagram.getNodeIndex(mNullDrawer, sStartPosInfo);
    if(index != IncludeDrawer::NO_INDEX)
        {
        IncludeNode const &node = mIncludeDiagram.getNodes()[index];
        viewSource(mGuiOptions, node.getName(), 0);
        }
    }

extern "C" G_MODULE_EXPORT void on_IncludeAddSuppliersMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sIncludeDiagramView->addSuppliers();
    }

extern "C" G_MODULE_EXPORT void on_IncludeViewSourceMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sIncludeDiagramView->viewFileSource();
    }

extern "C" G_MODULE_EXPORT void on_IncludeRestartMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sIncludeDiagramView->restart();
    }

extern "C" G_MODULE_EXPORT void on_IncludeRelayoutMenuitem_activate(
        GtkWidget *widget, gpointer data)
    {
    sIncludeDiagramView->relayout();
    }

