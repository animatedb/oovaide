/*
 * ClassGraph.cpp
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

void ClassGraph::updateNodeSizes(GtkWidget *widget, const ClassDrawOptions &options)
    {
    GtkCairoContext cairo(widget);
    CairoDrawer cairoDrawer(cairo.getCairo());
    mPad.x = cairoDrawer.getTextExtentWidth("W");
    mPad.y = cairoDrawer.getTextExtentHeight("W");
    NullDrawer nulDrawer(cairo.getCairo());
    ClassDrawer drawer(nulDrawer);
    for(auto &node : mNodes)
	{
	GraphSize size = drawer.drawNode(node, options);
	node.setSize(size);
	}
    }

void ClassGraph::updateNodeSizes(const ClassDrawOptions &options)
    {
    updateNodeSizes(getDiagramWidget(), options);
    }

int ClassGraph::getAvgNodeSize() const
    {
    int size = 0;
    if(mNodes.size() > 0)
	{
        int totalXSize = 0;
        int totalYSize = 0;
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
	const ClassDrawOptions &options)
    {
    updateNodeSizes(options);
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

void ClassGraph::addRelatedNodesRecurseUser(const ModelData &model, const ModelType *type,
	const ModelType *modelType, const ClassNodeDrawOptions &options,
	eAddNodeTypes addType, int maxDepth)
    {
    // Add nodes for template types if they refer to the passed in type.
    if(modelType->isTemplateType())
	{
	ConstModelClassifierVector relatedClassifiers;
	model.getRelatedTemplateClasses(*modelType, relatedClassifiers);
	for(const auto &rc : relatedClassifiers)
	    {
	    if(rc == type)
		{
#if(DEBUG_ADD)
		DebugAdd("Templ Rel", modelType);
#endif
		addRelatedNodesRecurse(model, modelType, options, addType, maxDepth);
		}
	    }
	}
    // Add nodes if members refer to the passed in type.
    if((addType & AN_MemberUsers) > 0)
	{
	const ModelClassifier*cl = modelType->getClass();
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
		    addRelatedNodesRecurse(model, cl, options, addType, maxDepth);
		    }
		}
	    }
	}
    // Add nodes if func params refer to the passed in type.
    if((addType & AN_FuncParamsUsers) > 0)
	{
	const ModelClassifier*cl = modelType->getClass();
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
		    addRelatedNodesRecurse(model, cl, options, addType, maxDepth);
		    }
		}
	    }
	}
    // Add nodes if func body variables refer to the passed in type.
    if((addType & AN_FuncBodyUsers) > 0)
	{
	const ModelClassifier*cl = modelType->getClass();
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
		    addRelatedNodesRecurse(model, cl, options, addType, maxDepth);
		    }
		}
	    }
	}
    }

void ClassGraph::addRelatedNodesRecurse(const ModelData &model, const ModelType *type,
	const ClassNodeDrawOptions &options, eAddNodeTypes addType, int maxDepth)
    {
    RecursiveBackgroundDialog backDlg(mBackgroundDialogLevel);
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
		addRelatedNodesRecurse(model, rc, options, addType, maxDepth);
		}
	    addNode(ClassNode(type, options));
	    }
	backDlg.setDialogText("Adding user relations.");
	backDlg.setProgressIterations(model.mTypes.size());
	for(size_t i=0; i<model.mTypes.size(); i++)
	    {
	    addRelatedNodesRecurseUser(model, type, model.mTypes[i].get(), options,
		    addType, maxDepth);
	    if(!backDlg.updateProgressIteration(i))
		{
		break;
		}
	    }
	const ModelClassifier *classifier = type->getClass();
	if(classifier)
	    {
	    backDlg.setDialogText("Adding member relations.");
	    addNode(ClassNode(classifier, options));
	    if((addType & AN_MemberChildren) > 0)
		{
		for(const auto &attr : classifier->getAttributes())
		    {
#if(DEBUG_ADD)
		    DebugAdd("Member", attr->getDeclType());
#endif
		    addRelatedNodesRecurse(model, attr->getDeclType(), options, addType, maxDepth);
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
			    addRelatedNodesRecurse(model, assoc->getParent(), options, addType, maxDepth);
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
			    addRelatedNodesRecurse(model, assoc->getChild(), options, addType, maxDepth);
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
		    addRelatedNodesRecurse(model, rdc.cl, options, addType, maxDepth);
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
		    addRelatedNodesRecurse(model, rdc.cl, options, addType, maxDepth);
		    }
		}
	    }
	backDlg.setDialogText("Done adding relations.");
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
	if(!mNodes[node1].isKey() && !mNodes[node2].isKey())
	    {
	    mConnectMap.insert(std::make_pair(key, connectItem));
	    }
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

	if(type)
	    {
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

	    const ModelClassifier *classifier = type->getClass();
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
    }

void ClassGraph::addNode(const ModelData &model, const ClassDrawOptions &options,
	char const * const className, int nodeDepth, bool clear)
    {
    static int depth = 0;
    depth++;
    if(depth == 1)
	{
	const ModelType *type = model.getTypeRef(className);
	const ModelClassifier *classifier = type->getClass();
	if(classifier)
	    {
	    if(clear)
		{
		clearGraph();
		}
	    if(mNodes.size() == 0 && options.drawRelationKey)
		{
		addRelationKeyNode(options);
		}
	    addRelatedNodesRecurse(model, classifier, options, ClassGraph::AN_All, nodeDepth);
	    updateGraph(model, options);
	    }
	mModified = false;
	}
    depth--;
    }

void ClassGraph::addRelationKeyNode(const ClassNodeDrawOptions &options)
    {
    mNodes.push_back(ClassNode(nullptr, options));
    }

GraphSize ClassGraph::getGraphSize() const
    {
    GraphSize size;
    for(const auto &node : mNodes)
	{
	GraphRect rect = node.getRect();
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
	GraphRect rect = mNodes[i].getRect();
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

GraphSize ClassGraph::updateGraph(const ModelData &modelData, const ClassDrawOptions &options)
    {
    updateGenes(modelData, options);
    return(getGraphSize());
    }

