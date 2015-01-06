/*
 * OperationDiagram.cpp
 *
 *  Created on: Jul 29, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "OperationDiagram.h"
#include "CairoDrawer.h"
#include "Svg.h"

void OperationDiagram::initialize(Builder &builder, const ModelData &modelData,
	OperationDiagramListener *listener)
    {
    mBuilder = &builder;
    mModelData = &modelData;
    mListener = listener;
    }

void OperationDiagram::clearGraph()
    {
    getOpGraph().clearGraph();
    }

void OperationDiagram::clearGraphAndAddOperation(OovStringRef const className,
	OovStringRef const opName, bool isConst)
    {
    getOpGraph().clearGraphAndAddOperation(getModelData(),
	    getOptions(), className, opName, isConst, 2);
    mLastOperDiagramParams.mClassName = className;
    mLastOperDiagramParams.mOpName = opName;
    mLastOperDiagramParams.mIsConst = isConst;
    updateDiagram();
    }

void OperationDiagram::restart()
    {
    clearGraphAndAddOperation(mLastOperDiagramParams.mClassName,
	    mLastOperDiagramParams.mOpName, mLastOperDiagramParams.mIsConst);
    }

void OperationDiagram::drawSvgDiagram(FILE *fp)
    {
    GtkCairoContext cairo(getBuilder().getWidget("DiagramDrawingarea"));

    NullDrawer nulDrawer(cairo.getCairo());
    OperationDrawer nulopDrawer(nulDrawer);
    GraphSize diagramSize = nulopDrawer.drawDiagram(getOpGraph(), getOptions());

    SvgDrawer svgDrawer(fp, cairo.getCairo());
    OperationDrawer opDrawer(svgDrawer);

    opDrawer.setDiagramSize(diagramSize);
    opDrawer.drawDiagram(getOpGraph(), getOptions());
    }

void OperationDiagram::updateDiagram()
    {
    GtkWidget *drawingArea = getBuilder().getWidget("DiagramDrawingarea");

    GtkCairoContext cairo(drawingArea);
    if(cairo.getCairo())	// Drawing area is not constructed initially.
	{
	NullDrawer nulDrawer(cairo.getCairo());
	OperationDrawer opDrawer(nulDrawer);
	GraphSize size = opDrawer.drawDiagram(getOpGraph(), getOptions());
	gtk_widget_queue_draw(drawingArea);
	gtk_widget_set_size_request(drawingArea, size.x, size.y);
	}
    }

void OperationDiagram::drawToDrawingArea()
    {
    GtkWidget *drawingArea = getBuilder().getWidget("DiagramDrawingarea");
    GtkCairoContext cairo(drawingArea);
//    clearBackground(cairo.getCairo());
    cairo_set_source_rgb(cairo.getCairo(), 255,255,255);
    cairo_paint(cairo.getCairo());

    cairo_set_source_rgb(cairo.getCairo(), 0,0,0);
    cairo_set_line_width(cairo.getCairo(), 1.0);
    CairoDrawer cairoDrawer(cairo.getCairo());
    ClassDrawer drawer(cairoDrawer);

//    drawer.drawDiagram(*this, options);
    OperationDrawer opDrawer(cairoDrawer);
    opDrawer.drawDiagram(getOpGraph(), getOptions());
    cairo_stroke(cairo.getCairo());
    }

static OperationDiagram *gOperationDiagram;

static GraphPoint gStartPosInfo;

void OperationDiagram::buttonPressEvent(const GdkEventButton *event)
    {
    gOperationDiagram = this;
    gStartPosInfo.set(event->x, event->y);
    }

static void displayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GdkEventButton *event = static_cast<GdkEventButton*>(data);
    const OperationClass *node = gOperationDiagram->getOpGraph().getNode(event->x, event->y);
    const OperationCall *opcall = gOperationDiagram->getOpGraph().getOperation(
	    gStartPosInfo.x, gStartPosInfo.y);
    OovStringRef const nodeitems[] =
	{
	"OperGotoClassMenuitem",
	"RemoveOperClassMenuitem",
	};
    for(size_t i=0; i<sizeof(nodeitems)/sizeof(nodeitems[i]); i++)
	{
	gtk_widget_set_sensitive(gOperationDiagram->getBuilder().getWidget(nodeitems[i]),
		node != nullptr);
	}
    OovStringRef const operitems[] =
	{
	"OperGotoOperationMenuitem",
	"AddCallsMenuitem",
	"AddCallersMenuitem",
	"RemoveCallsMenuitem"
	};
    for(size_t i=0; i<sizeof(operitems)/sizeof(operitems[i]); i++)
	{
	gtk_widget_set_sensitive(gOperationDiagram->getBuilder().getWidget(operitems[i]),
		opcall != nullptr);
	}
    gtk_widget_set_sensitive(gOperationDiagram->getBuilder().getWidget("ViewOperSourceMenuitem"),
	    opcall != nullptr || node != nullptr);

    GtkMenu *menu = gOperationDiagram->getBuilder().getMenu("DrawOperationPopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    gStartPosInfo.set(event->x, event->y);
    }

void OperationDiagram::buttonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
	{
	const OperationClass *node = gOperationDiagram->getOpGraph().getNode(
		gStartPosInfo.x, gStartPosInfo.y);
	if(node)
	    {
	    /*
	    DiagramPoint offset = gStartPosInfo.startPos;
	    offset.sub(node->getPosition());

	    DiagramPoint newPos = DiagramPoint(event->x, event->y);
	    newPos.sub(offset);

	    node->setPosition(newPos);
	    gOperationDiagram->getOpGraph().drawDiagram();
	    */
	    }
	}
    else
	{
	displayContextMenu(event->button, event->time, (gpointer)event);
	}
    }

extern "C" G_MODULE_EXPORT void on_OperGotoOperationmenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    const OperationCall *opcall = gOperationDiagram->getOpGraph().getOperation(
	    gStartPosInfo.x, gStartPosInfo.y);
    if(opcall)
	{
	std::string className = gOperationDiagram->getOpGraph().getClassName(*opcall);
	gOperationDiagram->clearGraphAndAddOperation(className,
		opcall->getName(), opcall->isConst());
	gOperationDiagram->updateDiagram();
	}
    }

extern "C" G_MODULE_EXPORT void on_OperGotoClassMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    const OperationClass *node = gOperationDiagram->getOpGraph().getNode(
	    gStartPosInfo.x, gStartPosInfo.y);
    if(node)
	{
	gOperationDiagram->gotoClass(node->getType()->getName());
	}
    }

extern "C" G_MODULE_EXPORT void on_RemoveOperClassMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    const OperationClass *node = gOperationDiagram->getOpGraph().getNode(
	    gStartPosInfo.x, gStartPosInfo.y);
    if(node)
	{
	gOperationDiagram->getOpGraph().removeNode(node);
	gOperationDiagram->updateDiagram();
	}
    }

extern "C" G_MODULE_EXPORT void on_AddCallsMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    const OperationCall *opcall = gOperationDiagram->getOpGraph().getOperation(
	    gStartPosInfo.x, gStartPosInfo.y);
    if(opcall)
	{
	gOperationDiagram->getOpGraph().addOperDefinition(*opcall);
	gOperationDiagram->updateDiagram();
	}
    }

extern "C" G_MODULE_EXPORT void on_RemoveCallsMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    const OperationCall *opcall = gOperationDiagram->getOpGraph().getOperation(
	    gStartPosInfo.x, gStartPosInfo.y);
    if(opcall)
	{
	gOperationDiagram->getOpGraph().removeOperDefinition(*opcall);
	gOperationDiagram->updateDiagram();
	}
    }

extern "C" G_MODULE_EXPORT void on_AddCallersMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    const OperationCall *opcall = gOperationDiagram->getOpGraph().getOperation(
	    gStartPosInfo.x, gStartPosInfo.y);
    if(opcall)
	{
	gOperationDiagram->getOpGraph().addOperCallers(
		gOperationDiagram->getModelData(), *opcall);
	gOperationDiagram->updateDiagram();
	}
    }

extern "C" G_MODULE_EXPORT void on_ViewOperSourceMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    const OperationCall *opcall = gOperationDiagram->getOpGraph().getOperation(
	    gStartPosInfo.x, gStartPosInfo.y);
    if(opcall)
	{
	const ModelOperation &oper = opcall->getOperation();
	if(oper.getModule())
	    viewSource(oper.getModule()->getModulePath(), oper.getLineNum());
	}
    const OperationClass *node = gOperationDiagram->getOpGraph().getNode(
	    gStartPosInfo.x, gStartPosInfo.y);
    if(node)
	{
	const ModelClassifier *cls = node->getType()->getClass();
	if(cls->getModule())
	    viewSource(cls->getModule()->getModulePath(), cls->getLineNum());
	}
    }

extern "C" G_MODULE_EXPORT void on_RestartOperationsMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    gOperationDiagram->restart();
    }

extern "C" G_MODULE_EXPORT void on_OperRemoveAllMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    gOperationDiagram->getOpGraph().clearGraph();
    gOperationDiagram->updateDiagram();
    }
