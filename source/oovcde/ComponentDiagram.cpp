/*
 * ComponentDiagram.cpp
 *
 *  Created on: Jan 4, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "ComponentDiagram.h"
#include "Svg.h"
#include "ComponentDrawer.h"
#include "CairoDrawer.h"
#include "Options.h"

static const ComponentDrawOptions &getDrawOptions()
    {
    static ComponentDrawOptions dopts;
    dopts.drawImplicitRelations = gGuiOptions.getValueBool(OptGuiShowCompImplicitRelations);
    return dopts;
    }

void ComponentDiagram::initialize(Builder &builder)
    {
    mDrawingArea = builder.getWidget("DiagramDrawingarea");
    updateGraph();

    GraphSize size = mComponentGraph.getGraphSize();
    gtk_widget_set_size_request(mDrawingArea, size.x, size.y);
    }

void ComponentDiagram::updateGraph()
    {
    mComponentGraph.updateGraph(getDrawOptions());
    updatePositionsInGraph();
    }

void ComponentDiagram::updatePositionsInGraph()
    {
    GtkCairoContext cairo(mDrawingArea);
    NullDrawer nulDrawer(cairo.getCairo());
    if(nulDrawer.haveCr())
	{
	ComponentDrawer drawer(nulDrawer);
	int pad = nulDrawer.getPad(1) * 2;

	enum NodeVectorsIndex { NVI_ExtPackage, NVI_Lib, NVI_Exec, NVI_NumVecs };
	struct NodeVectors
	{
	    NodeVectors():
		nodesSizeX(0)
		{}
	    void add(ComponentNode*node, int sizeX, int pad)
		{
		nodeVector.push_back(node);
		nodesSizeX += sizeX + pad;
		}
	    std::vector<ComponentNode*> nodeVector;
	    int nodesSizeX;
	} nodeVectors[NVI_NumVecs];
	int nodeSpacingY = 0;
	for(auto &node : mComponentGraph.getNodes())
	    {
	    GraphSize size = drawer.drawNode(node);
	    node.setSize(size);
	    if(nodeSpacingY == 0)
		nodeSpacingY = size.y * 2;
	    if(node.getComponentNodeType() == ComponentNode::CNT_ExternalPackage)
		{
		nodeVectors[NVI_ExtPackage].add(&node, size.x, pad);
		}
	    else
		{
		if(node.getComponentType() == ComponentTypesFile::CT_StaticLib)
		    {
		    nodeVectors[NVI_Lib].add(&node, size.x, pad);
		    }
		else
		    {
		    nodeVectors[NVI_Exec].add(&node, size.x, pad);
		    }
		}
	    }
	int biggestX = 0;
	for(auto const &vec : nodeVectors)
	    {
	    if(vec.nodesSizeX > biggestX)
		biggestX = vec.nodesSizeX;
	    }
	for(size_t veci=0; veci<sizeof(nodeVectors)/sizeof(nodeVectors[0]); veci++)
	    {
	    int yPos = veci * nodeSpacingY;
	    int xPos = (biggestX - nodeVectors[veci].nodesSizeX) / 2;
	    for(auto const &node : nodeVectors[veci].nodeVector)
		{
		node->setPos(GraphPoint(xPos, yPos));
		xPos += node->getRect().size.x + pad;
		}
	    }
	}
    redraw();
    }

void ComponentDiagram::drawSvgDiagram(FILE *fp)
    {
    GtkCairoContext cairo(mDrawingArea);
    SvgDrawer svgDrawer(fp, cairo.getCairo());
    ComponentDrawer drawer(svgDrawer);
    drawer.drawDiagram(mComponentGraph);
    }

void ComponentDiagram::drawToDrawingArea()
    {
    GtkCairoContext cairo(mDrawingArea);
    cairo_t *cr = cairo.getCairo();
    if(cr)
	{
	cairo_set_source_rgb(cairo.getCairo(), 255,255,255);
	cairo_paint(cairo.getCairo());

	cairo_set_source_rgb(cairo.getCairo(), 0,0,0);
	cairo_set_line_width(cairo.getCairo(), 1.0);
	CairoDrawer cairoDrawer(cr);
	ComponentDrawer drawer(cairoDrawer);
	drawer.drawDiagram(mComponentGraph);
	cairo_stroke(cairo.getCairo());
	}
    }

static GraphPoint gStartPosInfo;
static ComponentDiagram *gComponentDiagram;

void ComponentDiagram::buttonPressEvent(const GdkEventButton *event)
    {
    gComponentDiagram = this;
    gStartPosInfo.set(event->x, event->y);
    }

void ComponentDiagram::buttonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
	{
	ComponentNode *node = mComponentGraph.getNode(
		gStartPosInfo.x, gStartPosInfo.y);
	if(node)
	    {
	    GraphPoint offset = gStartPosInfo;
	    offset.sub(node->getRect().start);

	    GraphPoint newPos = GraphPoint(event->x, event->y);
	    newPos.sub(offset);

	    node->setPos(newPos);
	    mComponentGraph.setModified();
	    drawToDrawingArea();
	    }
	}
    else
	{
	displayContextMenu(event->button, event->time, (gpointer)event);
	}
    }

void ComponentDiagram::displayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GtkMenu *menu = Builder::getBuilder()->getMenu("DrawComponentPopupMenu");
    GtkCheckMenuItem *implicitItem = GTK_CHECK_MENU_ITEM(
	    Builder::getBuilder()->getWidget("DrawComponentPopupMenu"));
    ComponentDrawOptions opts = getDrawOptions();
    gtk_check_menu_item_set_active(implicitItem, opts.drawImplicitRelations);
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    }

extern "C" G_MODULE_EXPORT void on_RestartComponentsMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    gComponentDiagram->restart();
    }

extern "C" G_MODULE_EXPORT void on_RelayoutComponentsMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    gComponentDiagram->relayout();
    }

extern "C" G_MODULE_EXPORT void on_RemoveComponentMenuitem_activate(
	GtkWidget *widget, gpointer data)
    {
    ComponentNode *node = gComponentDiagram->getGraph().getNode(gStartPosInfo.x,
	    gStartPosInfo.y);
    if(node)
	{
	gComponentDiagram->getGraph().removeNode(*node, getDrawOptions());
	gComponentDiagram->relayout();
	}
    }

extern "C" G_MODULE_EXPORT void on_ShowImplicitRelationsMenuitem_toggled(
	GtkWidget *widget, gpointer data)
    {
    bool drawImplicitRelations = gGuiOptions.getValueBool(OptGuiShowCompImplicitRelations);
    gGuiOptions.setNameValueBool(OptGuiShowCompImplicitRelations, !drawImplicitRelations);
    gComponentDiagram->updateGraph();
    gGuiOptions.writeFile();
    }
