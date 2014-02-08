/*
 * DrawDiagram.cpp
 *
 *  Created on: Jun 20, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ClassGraph.h"
#include "CairoDrawer.h"

#include "Debug.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#define DEBUG_ADD 0
#if(DEBUG_ADD)
    static DebugFile sLog("DebugClassAddNode.txt");
    void DebugAdd(char const * const str, const ModelType *type)
	{
	if(type)
	    fprintf(sLog.mFp, "%s  %s\n", str, type->getName().c_str());
	else	/// @todo - there should be no unknown types.
	    fprintf(sLog.mFp, "Unknown Type\n");
	fflush(sLog.mFp);
	}
#endif

void ClassGraph::initialize(GtkWidget *drawingArea)
    {
    mDrawingArea = drawingArea;
// This is set in glade.
//    gtk_widget_add_events(mDrawingArea, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
//	    GDK_FOCUS_CHANGE_MASK);
    }

void ClassGraph::updateNodeSizes(GtkWidget *widget)
    {
    GtkCairoContext cairo(widget);
    cairo_text_extents_t extents;
    cairo_text_extents(cairo.getCairo(), "W", &extents);
    mPad.x = extents.height;
    mPad.y = extents.width;
    NullDrawer nulDrawer(cairo.getCairo());
    ClassDrawer drawer(nulDrawer);
    for(auto &node : mNodes)
	{
	GraphSize size = drawer.drawNode(node);
	node.setSize(size);
	}
    }

void ClassGraph::updateNodeSizes()
    {
    updateNodeSizes(getDiagramWidget());
    }

int ClassGraph::getAvgNodeSize() const
    {
    int size = 0;
    int totalXSize = 0;
    int totalYSize = 0;
    if(mNodes.size() > 0)
	{
	for(auto &node : mNodes)
	    {
	    GraphSize size = node.getSize();
	    totalXSize += size.x;
	    totalYSize += size.y;
	    }
	size = ((totalXSize + totalYSize) / 2) / mNodes.size();
	}
    return size;
    }

void ClassGraph::updateGenes(const ModelData &modelData,
	const ClassRelationDrawOptions &options)
    {
    updateNodeSizes(getDiagramWidget());
    updateConnections(modelData, options);
    if(mNodes.size() > 1)
	{
	mGenes.initialize(*this, getAvgNodeSize());
	mGenes.updatePositionsInGraph(*this);
	}
    else
	{
	/// @todo - need to place single node
	}
    redraw();
    }

int ClassGraph::getNodeIndex(const ModelType *type) const
    {
    int nodeIndex = -1;
    for(size_t i=0; i<mNodes.size(); i++)
	{
	if(mNodes[i].getType() == type)
	    {
	    nodeIndex = i;
	    break;
	    }
	}
    return nodeIndex;
    }

const ClassConnectItem *ClassGraph::getNodeConnection(int node1, int node2) const
    {
    const ClassConnectItem *connectItem = nullptr;
    if(node1 != node2)
	{
	auto ct = mConnectMap.find(nodePair_t(node1, node2));
	if(ct != mConnectMap.end())
	    connectItem = &ct->second;
	}
    return connectItem;
    }

void ClassGraph::addNode(const ClassNode &node)
    {
    bool present = false;
    for(auto &graphNode : mNodes)
	{
	if(graphNode.getType() == node.getType())
	    {
	    present = true;
	    break;
	    }
	}
    if(!present)
	{
	mModified = true;
	mNodes.push_back(node);
	}
    }

void ClassGraph::removeNode(const ClassNode &node)
    {
    for(size_t i=0; i<mNodes.size(); i++)
	{
	if(mNodes[i].getType() == node.getType())
	    {
	    mNodes.erase(mNodes.begin() + i);
	    break;
	    }
	}
    mModified = true;
    }

void ClassGraph::addRelatedNodes(const ModelData &model, const ModelType *type,
	const ClassNodeDrawOptions &options, eAddNodeTypes addType, int maxDepth)
    {
    -- maxDepth;
    if(maxDepth >= 0 && type /* && type->getObjectType() == otClass*/)
	{
	if(type->isTemplateType())
	    {
	    // Add types pointed to by templates.
	    ConstModelClassifierVector relatedClassifiers;
	    model.getRelatedTemplateClasses(*type, relatedClassifiers);
	    for(const auto &rc : relatedClassifiers)
		{
#if(DEBUG_ADD)
		DebugAdd("Templ User", rc);
#endif
		addRelatedNodes(model, rc, options, addType, maxDepth);
		}
	    addNode(ClassNode(type, options));
	    }
	// Go through all template types and see if they refer to the passed in type.
	for(const auto &templType : model.mTypes)
	    {
	    if(templType->isTemplateType())
		{
		ConstModelClassifierVector relatedClassifiers;
		model.getRelatedTemplateClasses(*templType, relatedClassifiers);
		for(const auto &rc : relatedClassifiers)
		    {
		    if(rc == type)
			{
#if(DEBUG_ADD)
			DebugAdd("Templ Rel", templType);
#endif
			addRelatedNodes(model, templType, options, addType, maxDepth);
			}
		    }
		}
	    }
	// Go through all classes and see if members refer to the passed in type.
	if((addType & AN_MemberUsers) > 0)
	    {
	    for(const auto &modelType : model.mTypes)
		{
		const ModelClassifier*cl = ModelObject::getClass(modelType);
		if(cl)
		    {
		    for(const auto &attr : cl->getAttributes())
			{
			const ModelType *attrType = attr->getDeclType();
			if(attrType == type)
			    {
#if(DEBUG_ADD)
			    DebugAdd("Memb User", cl);
#endif
			    addRelatedNodes(model, cl, options, addType, maxDepth);
			    }
			}
		    }
		}
	    }
	// Go through all classes and see if func params refer to the passed in type.
	if((addType & AN_FuncParamsUsers) > 0)
	    {
	    for(const auto &modelType : model.mTypes)
		{
		const ModelClassifier*cl = ModelObject::getClass(modelType);
		if(cl)
		    {
		    ConstModelDeclClassVector relatedDeclClasses;
		    model.getRelatedFuncParamClasses(*cl, relatedDeclClasses);
		    for(auto &rdc : relatedDeclClasses)
			{
			if(rdc.cl == type)
			    {
#if(DEBUG_ADD)
			    DebugAdd("Param User", cl);
#endif
			    addRelatedNodes(model, cl, options, addType, maxDepth);
			    }
			}
		    }
		}
	    }
	// Go through all classes and see if func body variables refer to the passed in type.
	if((addType & AN_FuncBodyUsers) > 0)
	    {
	    for(const auto &modelType : model.mTypes)
		{
		const ModelClassifier*cl = ModelObject::getClass(modelType);
		if(cl)
		    {
		    ConstModelDeclClassVector relatedDeclClasses;
		    model.getRelatedBodyVarClasses(*cl, relatedDeclClasses);
		    for(auto &rdc : relatedDeclClasses)
			{
			if(rdc.cl == type)
			    {
#if(DEBUG_ADD)
			    DebugAdd("Var User", cl);
#endif
			    addRelatedNodes(model, cl, options, addType, maxDepth);
			    }
			}
		    }
		}
	    }
	const ModelClassifier *classifier = ModelObject::getClass(type);
	if(classifier)
	    {
	    addNode(ClassNode(classifier, options));
	    if((addType & AN_MemberChildren) > 0)
		{
		for(const auto &attr : classifier->getAttributes())
		    {
#if(DEBUG_ADD)
		    DebugAdd("Member", attr->getDeclType());
#endif
		    addRelatedNodes(model, attr->getDeclType(), options, addType, maxDepth);
		    }
		}
	    if((addType & AN_Superclass) > 0)
		{
		for(const auto &assoc : model.mAssociations)
		    {
		    if(assoc->getChild() != nullptr && assoc->getParent() != nullptr)
			{
			if(assoc->getChild() == classifier)
			    {
#if(DEBUG_ADD)
			    DebugAdd("Super", assoc->getParent());
#endif
			    addRelatedNodes(model, assoc->getParent(), options, addType, maxDepth);
			    }
			}
		    }
		}
	    if((addType & AN_Subclass) > 0)
		{
		for(const auto &assoc  : model.mAssociations)
		    {
		    // Normally the child and parent should not be nullptr.
		    if(assoc->getChild() != nullptr && assoc->getParent() != nullptr)
			{
			if(assoc->getParent() == classifier)
			    {
#if(DEBUG_ADD)
			    DebugAdd("Subclass", assoc->getChild());
#endif
			    addRelatedNodes(model, assoc->getChild(), options, addType, maxDepth);
			    }
			}
		    }
		}
	    if((addType & AN_FuncParamsUsing) > 0)
		{
		ConstModelDeclClassVector relatedDeclClasses;
		model.getRelatedFuncParamClasses(*classifier, relatedDeclClasses);
		for(const auto &rdc : relatedDeclClasses)
		    {
#if(DEBUG_ADD)
		    DebugAdd("Param Using", rdc.cl);
#endif
		    addRelatedNodes(model, rdc.cl, options, addType, maxDepth);
		    }
		}
	    if((addType & AN_FuncBodyUsing) > 0)
		{
		ConstModelDeclClassVector relatedDeclClasses;
		model.getRelatedBodyVarClasses(*classifier, relatedDeclClasses);
		for(const auto &rdc : relatedDeclClasses)
		    {
#if(DEBUG_ADD)
		    DebugAdd("Body Using", rdc.cl);
#endif
		    addRelatedNodes(model, rdc.cl, options, addType, maxDepth);
		    }
		}
	    }
	}
    }

void ClassGraph::insertConnection(int node1, int node2,
	const ClassConnectItem &connectItem)
    {
    nodePair_t key(node1, node2);
    auto cmi = mConnectMap.find(key);
    if(cmi != mConnectMap.end())
	{
	mConnectMap[key].mConnectType = static_cast<eDiagramConnectType>
	    (mConnectMap[key].mConnectType | connectItem.mConnectType);
	}
    else
	{
	mConnectMap.insert(std::make_pair(key, connectItem));
	}
    }

void ClassGraph::insertConnection(int node1, const ModelType *type,
	const ClassConnectItem &connectItem)
    {
    if(type)
	{
	int n2Index = getNodeIndex(type);
	if(n2Index != -1)
	    {
	    insertConnection(node1, n2Index, connectItem);
	    }
	}
    }

void ClassGraph::updateConnections(const ModelData &modelData, const ClassRelationDrawOptions &options)
    {
    mConnectMap.clear();
//    Diagram *node1, Diagram *node2
    for(size_t ni=0; ni<mNodes.size(); ni++)
	{
	const ModelType *type = mNodes[ni].getType();

	// Go through templates
	if(type->isTemplateType())
	    {
	    ConstModelClassifierVector relatedClassifiers;
	    modelData.getRelatedTemplateClasses(*type, relatedClassifiers);
	    for(auto const &cl : relatedClassifiers)
		{
		insertConnection(ni, cl, ClassConnectItem(ctAssociation));
		}
	    }

	const ModelClassifier *classifier = ModelObject::getClass(type);
	if(classifier)
	    {
	    // Get attributes of classifier, and get the decl type
	    for(const auto &attr : classifier->getAttributes())
		{
		insertConnection(ni, attr->getDeclType(),
			ClassConnectItem(ctAggregation, attr->isConst(),
				    attr->isRefer(), attr->getAccess()));
		}

	    if(options.drawOperParamRelations)
		{
		// Get operations parameters of classifier, and get the decl type
		ConstModelDeclClassVector declClasses;
		modelData.getRelatedFuncParamClasses(*classifier, declClasses);
		for(const auto &declCl : declClasses)
		    {
		    const ModelDeclarator *decl = declCl.decl;
		    insertConnection(ni, declCl.cl, ClassConnectItem(ctFuncParam,
			    decl->isConst(), decl->isRefer(), Visibility()));
		    }
		}

	    if(options.drawOperBodyVarRelations)
		{
		// Get operations parameters of classifier, and get the decl type
		ConstModelDeclClassVector declClasses;
		modelData.getRelatedBodyVarClasses(*classifier, declClasses);
		for(const auto &declCl : declClasses)
		    {
		    const ModelDeclarator *decl = declCl.decl;
		    insertConnection(ni, declCl.cl, ClassConnectItem(ctFuncVar,
			    decl->isConst(), decl->isRefer(), Visibility()));
		    }
		}

	    // Go through associations, and get related classes.
	    for(const auto &assoc : modelData.mAssociations)
		{
		int n1Index = -1;
		int n2Index = -1;
		if(assoc->getChild() == classifier)
		    {
		    n1Index = getNodeIndex(assoc->getParent());
		    n2Index = ni;
		    }
		else if(assoc->getParent() == classifier)
		    {
		    n1Index = ni;
		    n2Index = getNodeIndex(assoc->getChild());
		    }
		if(n1Index != -1 && n2Index != -1)
		    {
		    insertConnection(n1Index, n2Index,
			    ClassConnectItem(ctIneritance, assoc->getAccess()));
		    }
		}
	    }
	}
    }

void ClassGraph::clearGraphAndAddNode(const ModelData &model,
	const ClassDrawOptions &options, char const * const className, int nodeDepth)
    {
    const ModelType *type = model.getTypeRef(className);
    const ModelClassifier *classifier = ModelObject::getClass(type);
    if(classifier)
	{
	clearGraph();
	addRelatedNodes(model, classifier, options, ClassGraph::AN_All, nodeDepth);
	updateGraph(model, options);
	}
    mModified = false;
    }

GraphSize ClassGraph::getGraphSize() const
    {
    GraphSize size;
    for(const auto &node : mNodes)
	{
	GraphRect rect;
	node.getRect(rect);
	if(rect.endx() > size.x)
	    size.x = rect.endx();
	if(rect.endy() > size.y)
	    size.y = rect.endy();
	}
    size.x++;
    size.y++;
    return size;
    }

ClassNode *ClassGraph::getNode(int x, int y)
    {
    ClassNode *node = nullptr;
    for(size_t i=0; i<mNodes.size(); i++)
	{
	GraphRect rect;
	mNodes[i].getRect(rect);
	if(rect.isPointIn(GraphPoint(x, y)))
	    node = &mNodes[i];
	}
    return node;
    }

GraphSize ClassGraph::getNodeSizeWithPadding(int nodeIndex) const
    {
    GraphSize size = mNodes[nodeIndex].getSize();
    size.x += mPad.x;
    size.y += mPad.y;
    return size;
    }

static void clearBackground(cairo_t *cr)
    {
    cairo_set_source_rgb(cr, 255,255,255);
    cairo_paint(cr);
    }

void ClassGraph::updateGraph(const ModelData &modelData, const ClassDrawOptions &options)
    {
    updateGenes(modelData, options);
    GraphSize size = getGraphSize();
    GtkWidget *widget = getDiagramWidget();
    gtk_widget_set_size_request(widget, size.x, size.y);
    }

void ClassGraph::drawDiagram(const ClassDrawOptions &options)
{
    GtkWidget *widget = getDiagramWidget();
    GtkCairoContext cairo(widget);
    clearBackground(cairo.getCairo());
    cairo_set_source_rgb(cairo.getCairo(), 0,0,0);
    cairo_set_line_width(cairo.getCairo(), 1.0);
    CairoDrawer cairoDrawer(cairo.getCairo());
    ClassDrawer drawer(cairoDrawer);

    drawer.drawDiagram(*this, options);
    cairo_stroke(cairo.getCairo());
}

