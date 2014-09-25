/*
 * ClassGuiBinding.cpp
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ClassDiagram.h"
#include "Options.h"
#include "Svg.h"
#include "ClassDrawer.h"
#include "CairoDrawer.h"
#include "OptionsDialog.h"

static const int NODE_DEPTH = 2;

static const ClassDrawOptions &getDrawOptions()
    {
    static ClassDrawOptions dopts;
    dopts.drawAttributes = gGuiOptions.getValueBool(OptGuiShowAttributes);
    dopts.drawOperations = gGuiOptions.getValueBool(OptGuiShowOperations);
    dopts.drawOperParams = gGuiOptions.getValueBool(OptGuiShowOperParams);
    dopts.drawAttrTypes = gGuiOptions.getValueBool(OptGuiShowAttrTypes);
    dopts.drawOperTypes = gGuiOptions.getValueBool(OptGuiShowOperTypes);
    dopts.drawPackageName = gGuiOptions.getValueBool(OptGuiShowPackageName);
    dopts.drawOovSymbols = gGuiOptions.getValueBool(OptGuiShowOovSymbols);
    dopts.drawOperParamRelations = gGuiOptions.getValueBool(OptGuiShowOperParamRelations);
    dopts.drawOperBodyVarRelations = gGuiOptions.getValueBool(OptGuiShowOperBodyVarRelations);
    dopts.drawRelationKey = gGuiOptions.getValueBool(OptGuiShowRelationKey);
    return dopts;
    }

void ClassDiagram::initialize(Builder &builder, const ModelData &modelData,
	ClassDiagramListener *listener)
    {
    mBuilder = &builder;
    mModelData = &modelData;
    mListener = listener;
    mClassGraph.initialize(getDiagramWidget());
    }

void ClassDiagram::updateGraph()
    {
    mClassGraph.updateGraph(getModelData(), getDrawOptions());
    updateGraphSize();
    }

void ClassDiagram::updateGraphSize()
    {
    GraphSize size = mClassGraph.getGraphSize().getZoomed(getDesiredZoom(),
	    getDesiredZoom());
    GtkWidget *widget = getDiagramWidget();
    gtk_widget_set_size_request(widget, size.x, size.y);
    }

void ClassDiagram::restart()
    {
    if(mClassGraph.getNodes().size() == 0)
	{
	clearGraphAndAddClass(getLastSelectedClassName().c_str());
	}
    updateGraph();
    }

void ClassDiagram::clearGraphAndAddClass(char const * const className)
    {
    mClassGraph.clearGraphAndAddNode(getModelData(),
	    getDrawOptions(), className, NODE_DEPTH);
    setLastSelectedClassName(className);
    updateGraphSize();
    }

void ClassDiagram::addClass(char const * const className)
    {
    mClassGraph.addNode(getModelData(), getDrawOptions(), className, NODE_DEPTH);
    setLastSelectedClassName(className);
    }

void ClassDiagram::drawSvgDiagram(FILE *fp)
    {
    GtkCairoContext cairo(mClassGraph.getDiagramWidget());
    SvgDrawer svgDrawer(fp, cairo.getCairo());
    ClassDrawer drawer(svgDrawer);

    drawer.setZoom(getDesiredZoom());
    drawer.drawDiagram(mClassGraph, getDrawOptions());
    }

void ClassDiagram::drawToDrawingArea()
    {
    drawDiagram(getDrawOptions());
    }

ClassNode *ClassDiagram::getNode(int x, int y)
    {
    return mClassGraph.getNode(x / getDesiredZoom(), y / getDesiredZoom());
    }

static ClassDiagram *gClassDiagram;
static struct StartPosInfo
    {
    GraphPoint startPos;
    } gStartPosInfo;

void ClassDiagram::buttonPressEvent(const GdkEventButton *event)
    {
    gClassDiagram = this;
    gStartPosInfo.startPos.set(event->x, event->y);
    }

void ClassDiagram::displayContextMenu(guint button, guint32 acttime, gpointer data)
    {
    GdkEventButton *event = static_cast<GdkEventButton*>(data);
    ClassNode *node = getNode(event->x, event->y);
    char const * const nodeMenus[] =
	{
	"GotoClassMenuitem",
	"AddStandardMenuitem", "AddAllMenuitem",
	"AddSuperclassesMenuitem", "AddSubclassesMenuitem",
	"AddMembersUsingMenuitem", "AddMemberUsersMenuitem",
	"AddFuncParamsUsingMenuitem", "AddFuncParamUsersMenuitem",
	"AddFuncBodyVarUsingMenuitem", "AddFuncBodyVarUsersMenuitem",
	"RemoveClassMenuitem", "ViewSourceMenuitem"
	};
    for(size_t i=0; i<sizeof(nodeMenus)/sizeof(nodeMenus[i]); i++)
	{
	gtk_widget_set_sensitive(getBuilder().getWidget(
		nodeMenus[i]), node != nullptr);
	}

    GtkMenu *menu = getBuilder().getMenu("DrawClassPopupMenu");
    gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, button, acttime);
    gStartPosInfo.startPos.set(event->x, event->y);
    }

void ClassDiagram::buttonReleaseEvent(const GdkEventButton *event)
    {
    if(event->button == 1)
	{
//	if(abs(event->x - gPressInfo.startPos.x) > 5 ||
//		abs(event->y - gPressInfo.startPos.y) > 5)
	    {
	    ClassNode *node = getNode(
		    gStartPosInfo.startPos.x, gStartPosInfo.startPos.y);
	    if(node)
		{
		GraphPoint clickOffset(event->x, event->y);
		clickOffset.sub(gStartPosInfo.startPos);
		GraphPoint newPos(node->getPosition());
		newPos.add(clickOffset.getZoomed(1/getDesiredZoom(),
			1/getDesiredZoom()));

		node->setPosition(newPos);
		getClassGraph().setModified();
		drawDiagram(getDrawOptions());
		}
	    }
//	else
	    {
/*
	    DiagramNode *node = getNode(event->x, event->y);
	    if(node)
		{
//		gDiagramGraph.clearGraph();
//		gLastSelectedClassName = node->getClassifier()->getName();
//		gDiagramGraph.addNodes(gModelData, node->getClassifier(),
//			DiagramGraph::AN_All, NODE_DEPTH);
//		gDiagramGraph.setGraph(gModelData, getDrawOptions());
//		gDiagramGraph.drawDiagram(getDrawOptions());
		}
*/
	    }
	}
    else
	{
	displayContextMenu(event->button, event->time, (gpointer)event);
	}
    }

static void clearBackground(cairo_t *cr)
    {
    cairo_set_source_rgb(cr, 255,255,255);
    cairo_paint(cr);
    }

void ClassDiagram::drawDiagram(const ClassDrawOptions &options)
{
    GtkWidget *widget = getDiagramWidget();
    GtkCairoContext cairo(widget);
    clearBackground(cairo.getCairo());
    cairo_set_source_rgb(cairo.getCairo(), 0,0,0);
    cairo_set_line_width(cairo.getCairo(), 1.0);
    CairoDrawer cairoDrawer(cairo.getCairo());
    ClassDrawer drawer(cairoDrawer);

    drawer.setZoom(getDesiredZoom());
    drawer.drawDiagram(getClassGraph(), options);
    cairo_stroke(cairo.getCairo());
}


// Class Diagram Popup menu

extern "C" G_MODULE_EXPORT void on_GotoClassMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    ClassNode *node = gClassDiagram->getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	gClassDiagram->gotoClass(node->getType()->getName().c_str());
	}
    }

extern "C" G_MODULE_EXPORT void on_RestartMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gClassDiagram->restart();
    }

void handlePopup(ClassGraph::eAddNodeTypes addType)
    {
    ClassNode *node = gClassDiagram->getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	static int depth = 0;
	depth++;
	if(depth == 1)
	    {
	    gClassDiagram->getClassGraph().addRelatedNodesRecurse(gClassDiagram->getModelData(),
		    node->getType(), getDrawOptions(), addType, 2);
	    gClassDiagram->updateGraph();
	    }
	depth--;
	}
    }

extern "C" G_MODULE_EXPORT void on_AddAllMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_All);
    }

extern "C" G_MODULE_EXPORT void on_AddStandardMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_AllStandard);
    }

extern "C" G_MODULE_EXPORT void on_AddSuperclassesMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_Superclass);
    }

extern "C" G_MODULE_EXPORT void on_AddSubclassesMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_Subclass);
    }

extern "C" G_MODULE_EXPORT void on_AddMembersUsingMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_MemberChildren);
    }

extern "C" G_MODULE_EXPORT void on_AddMemberUsersMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_MemberUsers);
    }

extern "C" G_MODULE_EXPORT void on_AddFuncParamsUsingMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_FuncParamsUsing);
    }

extern "C" G_MODULE_EXPORT void on_AddFuncParamUsersMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_FuncParamsUsers);
    }

extern "C" G_MODULE_EXPORT void on_AddFuncBodyVarUsingMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_FuncBodyUsing);
    }

extern "C" G_MODULE_EXPORT void on_AddFuncBodyVarUsersMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    handlePopup(ClassGraph::AN_FuncBodyUsers);
    }

extern "C" G_MODULE_EXPORT void on_RemoveClassMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    ClassNode *node = gClassDiagram->getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	gClassDiagram->getClassGraph().removeNode(*node);
	gClassDiagram->getClassGraph().updateConnections(gClassDiagram->getModelData(),
		    getDrawOptions());
	gClassDiagram->drawDiagram(getDrawOptions());
//	gClassDiagram->updateGraph();
	}
    }

extern "C" G_MODULE_EXPORT void on_RemoveAllMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gClassDiagram->getClassGraph().clearGraph();
    gClassDiagram->updateGraph();
    }

extern "C" G_MODULE_EXPORT void on_ViewSourceMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    ClassNode *node = gClassDiagram->getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	const ModelClassifier *classifier = node->getType()->getClass();
	if(classifier && classifier->getModule())
	    {
	    viewSource(classifier->getModule()->getModulePath().c_str(), classifier->getLineNum());
	    }
	}
    }

extern "C" G_MODULE_EXPORT void on_ClassPreferencesMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    ClassNode *node = gClassDiagram->getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	ClassPreferencesDialog dlg;
	if(dlg.run(gClassDiagram->getBuilder(), node->getDrawOptions()))
	    {
	    gClassDiagram->getClassGraph().setModified();
	    gClassDiagram->getClassGraph().updateNodeSizes(getDrawOptions());
	    gClassDiagram->drawDiagram(getDrawOptions());
	    }
	}
    }

extern "C" G_MODULE_EXPORT void on_Zoom1Menuitem_activate(GtkWidget *widget, gpointer data)
    {
    gClassDiagram->setZoom(1);
    gClassDiagram->drawDiagram(getDrawOptions());
    gClassDiagram->updateGraphSize();
    }

extern "C" G_MODULE_EXPORT void on_ZoomHalfMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gClassDiagram->setZoom(0.5);
    gClassDiagram->drawDiagram(getDrawOptions());
    gClassDiagram->updateGraphSize();
    }

extern "C" G_MODULE_EXPORT void on_ZoomQuarterMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gClassDiagram->setZoom(0.25);
    gClassDiagram->drawDiagram(getDrawOptions());
    gClassDiagram->updateGraphSize();
    }
