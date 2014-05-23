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
    return dopts;
    }

void ClassDiagram::initialize(Builder &builder, const ModelData &modelData,
	ClassDiagramListener *listener)
    {
    mBuilder = &builder;
    mModelData = &modelData;
    mListener = listener;
    mClassGraph.initialize(builder.getWidget("DiagramDrawingarea"));
    }

void ClassDiagram::updateGraph()
    {
    mClassGraph.updateGraph(getModelData(), getDrawOptions());
    }

void ClassDiagram::clearGraphAndAddClass(char const * const className)
    {
    mClassGraph.clearGraphAndAddNode(getModelData(),
	    getDrawOptions(), className, NODE_DEPTH);
    setLastSelectedClassName(className);
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

    drawer.drawDiagram(mClassGraph, getDrawOptions());
    }

void ClassDiagram::drawToDrawingArea()
    {
    mClassGraph.drawDiagram(getDrawOptions());
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
    ClassNode *node = getClassGraph().getNode(event->x, event->y);
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
	    ClassNode *node = getClassGraph().getNode(
		    gStartPosInfo.startPos.x, gStartPosInfo.startPos.y);
	    if(node)
		{
		GraphPoint offset = gStartPosInfo.startPos;
		offset.sub(node->getPosition());

		GraphPoint newPos = GraphPoint(event->x, event->y);
		newPos.sub(offset);

		node->setPosition(newPos);
		getClassGraph().setModified();
		getClassGraph().drawDiagram(getDrawOptions());
		}
	    }
//	else
	    {
/*
	    DiagramNode *node = gDiagramGraph.getNode(event->x, event->y);
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

// Class Diagram Popup menu

extern "C" G_MODULE_EXPORT void on_GotoClassMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    ClassNode *node = gClassDiagram->getClassGraph().getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	gClassDiagram->gotoClass(node->getType()->getName().c_str());
	}
    }

extern "C" G_MODULE_EXPORT void on_RelayoutMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gClassDiagram->getClassGraph().updateGraph(gClassDiagram->getModelData(),
	    getDrawOptions());
    }

void handlePopup(ClassGraph::eAddNodeTypes addType)
    {
    ClassNode *node = gClassDiagram->getClassGraph().getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	gClassDiagram->getClassGraph().addRelatedNodes(gClassDiagram->getModelData(),
		node->getType(), getDrawOptions(), addType, 2);
	gClassDiagram->getClassGraph().updateGraph(gClassDiagram->getModelData(),
		getDrawOptions());
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
    ClassNode *node = gClassDiagram->getClassGraph().getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	gClassDiagram->getClassGraph().removeNode(*node);
	gClassDiagram->getClassGraph().updateConnections(gClassDiagram->getModelData(),
		    getDrawOptions());
	gClassDiagram->getClassGraph().drawDiagram(getDrawOptions());
//	gClassDiagram->getClassGraph().updateGraph(gClassDiagram->getModelData(),
//		getDrawOptions());
	}
    }

extern "C" G_MODULE_EXPORT void on_RemoveAllMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    gClassDiagram->getClassGraph().clearGraph();
    gClassDiagram->getClassGraph().updateGraph(gClassDiagram->getModelData(),
	    getDrawOptions());
    }

extern "C" G_MODULE_EXPORT void on_ViewSourceMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    ClassNode *node = gClassDiagram->getClassGraph().getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	const ModelClassifier *classifier = ModelObject::getClass(node->getType());
	if(classifier)
	    {
	    viewSource(classifier->getModule()->getModulePath().c_str(), classifier->getLineNum());
	    }
	}
    }

extern "C" G_MODULE_EXPORT void on_ClassPreferencesMenuitem_activate(GtkWidget *widget, gpointer data)
    {
    ClassNode *node = gClassDiagram->getClassGraph().getNode(gStartPosInfo.startPos.x,
	    gStartPosInfo.startPos.y);
    if(node)
	{
	ClassPreferencesDialog dlg;
	if(dlg.run(gClassDiagram->getBuilder(), node->getDrawOptions()))
	    {
	    gClassDiagram->getClassGraph().setModified();
	    gClassDiagram->getClassGraph().updateNodeSizes();
	    gClassDiagram->getClassGraph().drawDiagram(getDrawOptions());
	    }
	}
    }

