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
#include "DiagramDrawer.h"
#include "OovThreadedBackgroundQueue.h"
#include <map>


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
        drawOperBodyVarRelations(true), drawTemplateRelations(true),
        drawRelationKey(true)
        {}
    bool drawOovSymbols;
    bool drawOperParamRelations;
    bool drawOperBodyVarRelations;
    bool drawTemplateRelations;
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
            mType(type),  mNodeOptions(options)
            {}
        bool isKey() const
            { return(mType == nullptr);}
        const ModelType *getType() const
            { return mType; }
        /// Positions are only valid after updateNodeSizes is called.
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
        const ClassNodeDrawOptions &getNodeOptions() const
            { return mNodeOptions; }
        ClassNodeDrawOptions &getNodeOptions()
            { return mNodeOptions; }

    private:
        const ModelType *mType;
        GraphRect rect;
        ClassNodeDrawOptions mNodeOptions;
    };

struct nodePair
    {
    nodePair(int node1, int node2):
        n1(static_cast<uint16_t>(node1)), n2(static_cast<uint16_t>(node2))
        {}
    uint16_t n1;
    uint16_t n2;
    uint32_t getAsInt() const
        { return (static_cast<uint32_t>(n1)<<16) + n2; }
    bool operator<(const nodePair &np) const
        { return getAsInt() < np.getAsInt(); }
    };
typedef struct nodePair nodePair_t;

enum eDiagramConnectType
    {
    ctNone = 0,
    ctAggregation = 0x01, ctIneritance = 0x02, ctAssociation = 0x04,
    ctTemplateDependency = 0x80,
    ctFuncParam = 0x10, ctFuncVar = 0x20,
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
        return (mConnectType & ctAggregation || mConnectType & ctIneritance);
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

class ClassGraphListener
    {
    public:
        ~ClassGraphListener()
            {}
        // WARNING - This is called from a background thread.
        virtual void doneRepositioning() = 0;
    };

/// This defines a class graph that holds class nodes and connections between
/// the class nodes.
class ClassGraph:public ThreadedWorkBackgroundQueue<ClassGraph, ClassGraphBackgroundItem>
    {
    public:
        ClassGraph(Diagram const &diagram):
            mDiagram(diagram), mModified(false), mBackgroundTaskLevel(0),
            mGraphListener(nullptr), mForegroundTaskStatusListener(nullptr),
            mBackgroundTaskStatusListener(nullptr), mNullDrawer(nullptr)
            {}
        virtual ~ClassGraph();
        /// The taskStatusListener's are used in the addNode functions, and in
        /// the genetic repositioning code.
        /// The background is on the background thread, and cannot directly
        /// call GUI functions.  The foreground has to call the GUI, since
        /// the function doesn't complete to let the GUI run.
        void initialize(ClassDrawOptions const &options, DiagramDrawer &nullDrawer,
                ClassGraphListener &graphListener,
                OovTaskStatusListener &foregroundTaskStatusListener,
                OovTaskStatusListener &backgroundTaskStatusListener);
        enum eAddNodeTypes
            {
            AN_Superclass=0x01, AN_Subclass=0x02,
            AN_MemberChildren=0x04, AN_MemberUsers=0x08,
            AN_FuncParamsUsing=0x10, AN_FuncParamsUsers=0x20,
            AN_FuncBodyUsing=0x40, AN_FuncBodyUsers=0x80,
            AN_Templates=0x100,
            AN_All=0xFFF,
            AN_AllStandard=AN_Superclass | AN_Subclass | AN_MemberChildren | AN_MemberUsers,
            AN_ClassesAndChildren=AN_Superclass | AN_Subclass | AN_MemberChildren
        };

        /// This relayouts the graph nodes.
        /// This is slow since it updates genes and repositions nodes.
        /// Updates node sizes and node connections in the graph.
        /// Initializes genes from the model.
        /// The nodes generally must have been added with the same modelData.
        GraphSize updateGraph(const ModelData &modelData, bool geneRepositioning);

        void clearGraph()
            {
            mNodes.clear();
            mConnectMap.clear();
            }

        /// See the addRelatedNodes function for more description.
        void addNode(const ModelData &model, char const * const className,
                eAddNodeTypes addType, int nodeDepth, bool reposition);

        /// Adds nodes/classes/types to a graph.
        /// @param model Used to look up related classes.
        /// @param type The type/class to be added to the graph.
        /// @param addType Defines the relationship types to look for.
        /// @param maxDepth Recurses to the specified depth.
        void addRelatedNodesRecurse(const ModelData &model, const ModelType *type,
                eAddNodeTypes addType=AN_All, int maxDepth=2);
        void getRelatedNodesRecurse(const ModelData &model, const ModelType *type,
                eAddNodeTypes addType, int maxDepth, std::vector<ClassNode> &nodes);

        void removeNode(const ClassNode &node, const ModelData &modelData)
            {
            removeNode(node);
            updateConnections(modelData);
            }

        bool isNodeTypePresent(ClassNode const &node) const
            { return isNodeTypePresent(node, mNodes); }

        /// Changing options doesn't change number of nodes, so only the size
        /// of nodes needs to be updated.
        void changeOptions(ClassDrawOptions const &options)
            {
            setModified();
            mGraphOptions = options;
            updateNodeSizes();
            }
        ClassDrawOptions const &getOptions() const
            { return mGraphOptions; }

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
        const std::vector<ClassNode> &getNodes() const
            { return mNodes; }
        std::vector<ClassNode> &getNodes()
            { return mNodes; }
        std::map<nodePair_t, ClassConnectItem> getConnections() const
            { return mConnectMap; }

        // Called from ThreadedWorkBackgroundQueue
        void processItem(ClassGraphBackgroundItem const &item)
            {
            updateGenes(*item.mModelData, *mNullDrawer);
            mGraphListener->doneRepositioning();
            }

        void setDrawer(DiagramDrawer &drawer)
            { mNullDrawer = &drawer; }

    private:
        Diagram const &mDiagram;
        /// @todo - this should be a set?
        // This may have some duplicate relations for the different types
        // of relations.
        std::vector<ClassNode> mNodes;
        std::map<nodePair_t, ClassConnectItem> mConnectMap;
        ClassGenes mGenes;
        GraphSize mPad;
        bool mModified;
        int mBackgroundTaskLevel;
        ClassGraphListener *mGraphListener;
        OovTaskStatusListener *mForegroundTaskStatusListener;
        OovTaskStatusListener *mBackgroundTaskStatusListener;
        DiagramDrawer *mNullDrawer;
        ClassDrawOptions mGraphOptions;
        static const int KEY_INDEX = 0;
        static const int FIRST_CLASS_INDEX = 1;

        void removeNode(const ClassNode &node);

        /// This updates quality information, runs the genetic algorithm for
        /// placing the nodes, and then draws them.
        void updateGenes(const ModelData &modelData,
            DiagramDrawer &nullDrawer);

        /// Update connections between nodes.
        void updateConnections(const ModelData &modelData);

        void insertConnection(int node1, int node2,
            const ClassConnectItem &connectItem);
        void insertConnection(int node1, const ModelType *type,
            const ClassConnectItem &connectItem);

        /// Checks the modelType to see if any parts of it refer to the type.
        /// @param type The base type to compare all other types against.
        /// @param modelType The type to check to see if any parts of it is a user
        ///                  of the base type.
        void addRelatedNodesRecurseUserToVector(const ModelData &model,
            const ModelType *type, const ModelType *modelType,
            eAddNodeTypes addType, int maxDepth, std::vector<ClassNode> &nodes);
        static void addNodeToVector(const ClassNode &node, std::vector<ClassNode> &nodes);
        void addRelationKeyNode();

        /// Update the visual node size based on font size, number of attributes, etc.
        /// Each node is a class.
        void updateNodeSizes();
        int getAvgNodeSize() const;

        static const size_t NO_INDEX = static_cast<size_t>(-1);
        /// Get the index to the class.
        size_t getNodeIndex(const ModelType *type) const;
        static bool isNodeTypePresent(ClassNode const &node,
                std::vector<ClassNode> const &nodes);

        ClassNodeDrawOptions getComponentOptions(ModelType const &type,
                ClassNodeDrawOptions const &options);
    };


#endif
