/*
 * ClassGraph.h
 *
 *  Created on: Jun 20, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef CLASSGRAPH_H_
#define CLASSGRAPH_H_

#include "ModelObjects.h"
#include "ClassGenes.h"
#include "CairoDrawer.h"
#include "Gui.h"
#include <gtk/gtk.h>	// For GtkWidget and cairo_t
#include <map>
#include "OovThreadedBackgroundQueue.h"


struct ClassNodeDrawOptions
    {
    ClassNodeDrawOptions():
	drawAttributes(true), drawOperations(true), drawOperParams(true),
	drawOperReturn(true),
	drawAttrTypes(true), drawOperTypes(true), drawPackageName(true)
	{}
    bool drawAttributes;
    bool drawOperations;
    bool drawOperParams;
    bool drawOperReturn;
    bool drawAttrTypes;
    bool drawOperTypes;
    bool drawPackageName;
    };

struct ClassRelationDrawOptions
    {
    ClassRelationDrawOptions():
	drawOovSymbols(true), drawOperParamRelations(true),
	drawOperBodyVarRelations(true), drawRelationKey(true)
	{}
    bool drawOovSymbols;
    bool drawOperParamRelations;
    bool drawOperBodyVarRelations;
    bool drawRelationKey;
    };

struct ClassDrawOptions:public ClassNodeDrawOptions, public ClassRelationDrawOptions
    {
    };



/// Each node in the diagram is a class.  A diagram node has information about
/// the size and position.
class ClassNode
    {
    public:
	// @param type Is null for relation key.
	ClassNode(const ModelType *type, const ClassNodeDrawOptions &options):
	    mType(type),  mDrawOptions(options)
	    {}
	bool isKey() const
	    { return(mType == nullptr);}
	const ModelType *getType() const
	    { return mType; }
	/// Positions are only valid after positionNodes is called.
	GraphSize getSize() const
	    { return rect.size; }
	void setPosition(const GraphPoint &pos)
	    { rect.start = pos; }
	GraphPoint getPosition() const
	    { return rect.start; }
	void setSize(GraphSize &size)
	    { rect.size = size; }
	GraphRect const &getRect() const
	    { return rect; }
	const ClassNodeDrawOptions &getDrawOptions() const
	    { return mDrawOptions; }
	ClassNodeDrawOptions &getDrawOptions()
	    { return mDrawOptions; }

    private:
	const ModelType *mType;
	GraphRect rect;
	ClassNodeDrawOptions mDrawOptions;
    };

struct nodePair
    {
    nodePair(int node1, int node2):
	n1(node1), n2(node2)
	{}
    uint16_t n1;
    uint16_t n2;
    uint32_t getAsInt() const
	{ return (n1<<16) + n2; }
    bool operator<(const nodePair &np) const
	{ return getAsInt() < np.getAsInt(); }
    };
typedef struct nodePair nodePair_t;

enum eDiagramConnectType
    {
    ctNone = 0,
    ctAggregation = 0x01, ctIneritance = 0x02, ctAssociation = 0x04,
    ctFuncParam = 0x10, ctFuncVar = 0x20,

    ctObjectRelationMask = 0x0F,
    ctFuncReferenceMask = 0xF0,
    };

/// Defines a connection between two class nodes
struct ClassConnectItem
    {
    ClassConnectItem(eDiagramConnectType ct = ctNone):
	mConnectType(ct), mConst(false), mRefer(false)
	{}
    ClassConnectItem(eDiagramConnectType conType,
	bool isConst, bool isRef, Visibility vis):
	mConnectType(conType), mConst(isConst), mRefer(isRef), mAccess(vis)
	{}
    ClassConnectItem(eDiagramConnectType conType, Visibility vis):
	mConnectType(conType), mConst(false), mRefer(false), mAccess(vis)
	{}
    bool hasAccess() const
	{
	return (mConnectType != ctAssociation && mConnectType != ctFuncParam &&
		mConnectType != ctFuncVar);
	}
    // For aggregation, node1 is the owner
    // For inheritance, node1 is the parent
    eDiagramConnectType mConnectType;
    bool mConst;
    bool mRefer;
    Visibility mAccess;
    };

class ClassGraphBackgroundItem
    {
    public:
	ClassGraphBackgroundItem(const ModelData *modelData=nullptr,
	    const ClassDrawOptions *options=nullptr):
	    mModelData(modelData), mOptions(options)
	    {}
    const ModelData *mModelData;
    const ClassDrawOptions *mOptions;
    };

/// This defines a class graph that holds class nodes and connections between
/// the class nodes.
class ClassGraph:public ThreadedWorkBackgroundQueue<ClassGraph, ClassGraphBackgroundItem>
    {
    public:
	ClassGraph():
	    mDrawingArea(nullptr), mModified(false), mBackgroundDialogLevel(0),
	    mTaskStatusListener(nullptr)
	    {}
	~ClassGraph()
	    {
            stopAndWaitForCompletion();
	    }
	void initialize(GtkWidget *drawingArea, OovTaskStatusListener *taskStatusListener);
	enum eAddNodeTypes
	    {
	    AN_Superclass=0x01, AN_Subclass=0x02,
	    AN_MemberChildren=0x04, AN_MemberUsers=0x08,
	    AN_FuncParamsUsing=0x10, AN_FuncParamsUsers=0x20,
	    AN_FuncBodyUsing=0x40, AN_FuncBodyUsers=0x80,
	    AN_All=0xFF,
	    AN_AllStandard=AN_Superclass | AN_Subclass | AN_MemberChildren | AN_MemberUsers,
	    AN_ClassesAndChildren=AN_Superclass | AN_Subclass | AN_MemberChildren,
	};

	/// This relayouts the graph nodes.
	/// This is slow since it updates genes and repositions nodes.
	/// Updates node sizes and node connections in the graph.
	/// Initializes genes from the model.
	/// The nodes generally must have been added with the same modelData.
	GraphSize updateGraph(const ModelData &modelData, const ClassDrawOptions &options);

	void clearGraph()
	    {
	    mNodes.clear();
	    mConnectMap.clear();
	    }

	/// See the addRelatedNodes function for more description.
	void addNode(const ModelData &model, const ClassDrawOptions &options,
		char const * const className, eAddNodeTypes addType, int nodeDepth);

	/// Adds nodes/classes/types to a graph.
	/// @param model Used to look up related classes.
	/// @param type The type/class to be added to the graph.
	/// @param addType Defines the relationship types to look for.
	/// @param maxDepth Recurses to the specified depth.
	void addRelatedNodesRecurse(const ModelData &model, const ModelType *type,
		const ClassNodeDrawOptions &options, eAddNodeTypes addType=AN_All,
		int maxDepth=2);

	void removeNode(const ClassNode &node, const ModelData &modelData,
		const ClassDrawOptions &options)
	    {
	    removeNode(node);
	    updateConnections(modelData, options);
	    }

	/// Changing options doesn't change number of nodes, so only the size
	/// of nodes needs to be updated.
	void changeDrawOptions(const ModelData &modelData, const ClassDrawOptions &options)
	    {
	    setModified();
	    mCairoContext.setContext(getDiagramWidget());
	    updateNodeSizes(options);
	    }

	GraphSize getNodeSizeWithPadding(int nodeIndex) const;
	GraphSize getGraphSize() const;
	ClassNode *getNode(int x, int y);
	bool isModified() const
	    { return mModified; }
	void setModified()
	    { mModified = true; }

	// Used by DiagramGenes.

	/// Return the connection between nodes.
	const ClassConnectItem *getNodeConnection(int node1, int node2) const;
	GtkWidget *getDiagramWidget() const
	    { return mDrawingArea; }
	const std::vector<ClassNode> &getNodes() const
	    { return mNodes; }
	std::vector<ClassNode> &getNodes()
	    { return mNodes; }
	std::map<nodePair_t, ClassConnectItem> getConnections() const
	    { return mConnectMap; }

	// Called from ThreadedWorkBackgroundQueue
	void processItem(ClassGraphBackgroundItem const &item)
	    { updateGenes(*item.mModelData, *item.mOptions); }

    private:
	/// @todo - this should be a set?
	// This may have some duplicate relations for the different types
	// of relations.
	std::vector<ClassNode> mNodes;
	std::map<nodePair_t, ClassConnectItem> mConnectMap;
	ClassGenes mGenes;
	GraphSize mPad;
	GtkWidget *mDrawingArea;
	bool mModified;
	int mBackgroundDialogLevel;
	OovTaskStatusListener *mTaskStatusListener;
	/// Used to calculate font sizes. Apparently the setContext call that
	/// calls gdk_cairo_create cannot be called from the background
	/// thread, but the other calls to get text extents can be made from
	/// the background thread.
	GtkCairoContext mCairoContext;

	void addNode(const ClassNode &node);
	void removeNode(const ClassNode &node);

	/// This updates quality information, runs the genetic algorithm for
	/// placing the nodes, and then draws them.
	void updateGenes(const ModelData &modelData,
		const ClassDrawOptions &options);

	/// Update connections between nodes.
	void updateConnections(const ModelData &modelData, const ClassRelationDrawOptions &options);
	void updateNodeSizes(const ClassDrawOptions &options);

	void insertConnection(int node1, int node2,
		const ClassConnectItem &connectItem);
	void insertConnection(int node1, const ModelType *type,
		const ClassConnectItem &connectItem);

	/// @param modelType The type to check to see if it is a user.
	void addRelatedNodesRecurseUser(const ModelData &model, const ModelType *type,
		const ModelType *modelType, const ClassNodeDrawOptions &options,
		eAddNodeTypes addType, int maxDepth);
	void addRelationKeyNode(const ClassNodeDrawOptions &options);

	/// Update the visual node size based on font size, number of attributes, etc.
	/// Each node is a class.
	void updateNodeSizes(GtkWidget *widget, const ClassDrawOptions &options);
	int getAvgNodeSize() const;

	/// Get the index to the class.
	int getNodeIndex(const ModelType *type) const;

	void redraw()
	    {
	    gtk_widget_queue_draw(getDiagramWidget());
	    }
    };


#endif
