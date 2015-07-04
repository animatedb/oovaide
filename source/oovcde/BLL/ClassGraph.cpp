/*
 * ClassGraph.cpp
 *
 *  Created on: Jun 20, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ClassGraph.h"
#include "ClassDrawer.h"
#include "FilePath.h"
#include "Debug.h"
#include <algorithm>

#define DEBUG_ADD 0
#if(DEBUG_ADD)
    static DebugFile sLog("DebugClassAddNode.txt");
    void DebugAdd(char const * const str, const ModelType *type)
        {
        if(type)
            fprintf(sLog.mFp, "%s  %s\n", str, type->getName().c_str());
        else    /// @todo - there should be no unknown types.
            fprintf(sLog.mFp, "Unknown Type\n");
        fflush(sLog.mFp);
        }
#endif

ClassGraph::~ClassGraph()
    {
    stopAndWaitForCompletion();
    }

void ClassGraph::initialize(ClassDrawOptions const &options, DiagramDrawer &nullDrawer,
    ClassGraphListener &graphListener, OovTaskStatusListener &foregroundTaskStatusListener,
    OovTaskStatusListener &backgroundTaskStatusListener)
    {
    mNullDrawer = &nullDrawer;
    mGraphListener = &graphListener;
    mForegroundTaskStatusListener = &foregroundTaskStatusListener;
    mBackgroundTaskStatusListener = &backgroundTaskStatusListener;
    mGraphOptions = options;
    }

void ClassGraph::updateNodeSizes()
    {
    mPad.x = mNullDrawer->getPad();
    mPad.y = mPad.x;
    ClassDrawer drawer(*mNullDrawer);
    for(auto &node : mNodes)
        {
        GraphSize size = drawer.drawNode(node);
        node.setSize(size);
        }
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
        DiagramDrawer &nullDrawer)
    {
    updateNodeSizes();
    updateConnections(modelData);
    if(mNodes.size() > 1)
        {
        mGenes.initialize(*this, getAvgNodeSize());
        mGenes.updatePositionsInGraph(*this, mBackgroundTaskStatusListener, *this);
        }
    else
        {
        // single node just ends up at 0,0
        }
    }

size_t ClassGraph::getNodeIndex(const ModelType *type) const
    {
    size_t nodeIndex = NO_INDEX;
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

void ClassGraph::addNodeToVector(ClassNode const &node, std::vector<ClassNode> &nodes)
    {
    if(!isNodeTypePresent(node, nodes))
        {
        nodes.push_back(node);
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

bool ClassGraph::isNodeTypePresent(ClassNode const &node,
        std::vector<ClassNode> const &nodes)
    {
    auto nodeIter = std::find_if(nodes.begin(), nodes.end(),
            [&node](ClassNode const &existingNode)
                { return(node.getType() == existingNode.getType()); });
    return (nodeIter != nodes.end());
    }


void ClassGraph::addRelatedNodesRecurseUserToVector(const ModelData &model,
        const ModelType *type, const ModelType *modelType, eAddNodeTypes addType,
        int maxDepth, std::vector<ClassNode> &nodes)
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
                getRelatedNodesRecurse(model, modelType, addType, maxDepth,
                        nodes);
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
                    getRelatedNodesRecurse(model, cl, addType, maxDepth, nodes);
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
            ConstModelClassifierVector relatedClasses;
            model.getRelatedFuncInterfaceClasses(*cl, relatedClasses);
            for(auto &cls : relatedClasses)
                {
                if(cls == type)
                    {
#if(DEBUG_ADD)
                    DebugAdd("Param User", cl);
#endif
                    getRelatedNodesRecurse(model, cl, addType, maxDepth, nodes);
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
            ConstModelDeclClasses relatedDeclClasses;
            model.getRelatedBodyVarClasses(*cl, relatedDeclClasses);
            for(auto &rdc : relatedDeclClasses)
                {
                if(rdc.getClass() == type)
                    {
#if(DEBUG_ADD)
                    DebugAdd("Var User", cl);
#endif
                    getRelatedNodesRecurse(model, cl, addType, maxDepth, nodes);
                    }
                }
            }
        }
    }

ClassNodeDrawOptions ClassGraph::getComponentOptions(ModelType const &type,
    ClassNodeDrawOptions const &options)
    {
    ClassNodeDrawOptions compOptions = options;
    bool mainComponent = false;
    if(mNodes.size() > FIRST_CLASS_INDEX)
        {
        ModelType const *mainType = mNodes[FIRST_CLASS_INDEX].getType();
        ModelClassifier const *mainClass = mainType->getClass();
        ModelClassifier const *testClass = type.getClass();
        if(mainClass && testClass)
            {
            ModelModule const *mainModule = mainClass->getModule();
            ModelModule const *testModule = testClass->getModule();
            if(mainModule && testModule)
                {
                FilePath mainFp(mainModule->getName(), FP_File);
                FilePath fp(testClass->getModule()->getName(), FP_File);
                if(mainFp.getDrivePath() == fp.getDrivePath())
                    {
                    mainComponent = true;
                    }
                }
            }
        }
    else
        {
        mainComponent = true;
        }
    if(!mainComponent)
        {
        compOptions.drawAttrTypes = false;
        compOptions.drawAttributes = false;
        compOptions.drawOperParams = false;
        compOptions.drawOperations = false;
        compOptions.drawPackageName = true;
        }
    return compOptions;
    }

void ClassGraph::addRelatedNodesRecurse(const ModelData &model, const ModelType *type,
        eAddNodeTypes addType, int maxDepth)
    {
    getRelatedNodesRecurse(model, type, addType, maxDepth, mNodes);
    }

void ClassGraph::getRelatedNodesRecurse(const ModelData &model, const ModelType *type,
        eAddNodeTypes addType, int maxDepth, std::vector<ClassNode> &nodes)
    {
    mBackgroundTaskLevel++;
    -- maxDepth;
    if(maxDepth >= 0 && type /* && type->getObjectType() == otClass*/)
        {
        const ModelClassifier *classifier = type->getClass();
        if(classifier)
            {
            addNodeToVector(ClassNode(classifier,
                    getComponentOptions(*type, mGraphOptions)), nodes);
            if((addType & AN_MemberChildren) > 0)
                {
                for(const auto &attr : classifier->getAttributes())
                    {
#if(DEBUG_ADD)
                    DebugAdd("Member", attr->getDeclType());
#endif
                    getRelatedNodesRecurse(model, attr->getDeclType(), addType,
                            maxDepth, nodes);
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
                            getRelatedNodesRecurse(model, assoc->getParent(),
                                    addType, maxDepth, nodes);
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
                            getRelatedNodesRecurse(model, assoc->getChild(),
                                    addType, maxDepth, nodes);
                            }
                        }
                    }
                }
            if((addType & AN_FuncParamsUsing) > 0)
                {
                ConstModelClassifierVector relatedClasses;
                model.getRelatedFuncInterfaceClasses(*classifier, relatedClasses);
                for(const auto &cls : relatedClasses)
                    {
#if(DEBUG_ADD)
                    DebugAdd("Param Using", cls);
#endif
                    getRelatedNodesRecurse(model, cls, addType, maxDepth, nodes);
                    }
                }
            if((addType & AN_FuncBodyUsing) > 0)
                {
                ConstModelDeclClasses relatedDeclClasses;
                model.getRelatedBodyVarClasses(*classifier, relatedDeclClasses);
                for(const auto &rdc : relatedDeclClasses)
                    {
#if(DEBUG_ADD)
                    DebugAdd("Body Using", rdc.cl);
#endif
                    getRelatedNodesRecurse(model, rdc.getClass(), addType,
                            maxDepth, nodes);
                    }
                }
            }
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
                getRelatedNodesRecurse(model, rc, addType, maxDepth, nodes);
                }
            addNodeToVector(ClassNode(type,
                    getComponentOptions(*type, mGraphOptions)), nodes);
            }
        int taskId = 0;
        if(mBackgroundTaskLevel == 1)
            {
            taskId = mForegroundTaskStatusListener->startTask("Adding relations.",
                    model.mTypes.size());
            }
        for(size_t i=0; i<model.mTypes.size(); i++)
            {
            addRelatedNodesRecurseUserToVector(model, type, model.mTypes[i].get(),
                    addType, maxDepth, nodes);
            if(mBackgroundTaskLevel == 1)
                {
                if(!mForegroundTaskStatusListener->updateProgressIteration(
                        taskId, i, nullptr))
                    {
                    break;
                    }
                }
            }
        if(mBackgroundTaskLevel == 1)
            {
            mForegroundTaskStatusListener->endTask(taskId);
            }
        }
    mBackgroundTaskLevel--;
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
        size_t n2Index = getNodeIndex(type);
        if(n2Index != NO_INDEX)
            {
            insertConnection(node1, n2Index, connectItem);
            }
        }
    }

void ClassGraph::updateConnections(const ModelData &modelData)
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

                if(mGraphOptions.drawOperParamRelations)
                    {
                    // Get operations parameters of classifier, and get the decl type
                    ConstModelDeclClasses declClasses;
                    modelData.getRelatedFuncParamClasses(*classifier, declClasses);
                    for(const auto &declCl : declClasses)
                        {
                        const ModelDeclarator *decl = declCl.getDecl();
                        insertConnection(ni, declCl.getClass(), ClassConnectItem(ctFuncParam,
                                decl->isConst(), decl->isRefer(), Visibility()));
                        }
                    }

                if(mGraphOptions.drawOperBodyVarRelations)
                    {
                    // Get operations parameters of classifier, and get the decl type
                    ConstModelDeclClasses declClasses;
                    modelData.getRelatedBodyVarClasses(*classifier, declClasses);
                    for(const auto &declCl : declClasses)
                        {
                        const ModelDeclarator *decl = declCl.getDecl();
                        insertConnection(ni, declCl.getClass(), ClassConnectItem(ctFuncVar,
                                decl->isConst(), decl->isRefer(), Visibility()));
                        }
                    }

                // Go through associations, and get related classes.
                for(const auto &assoc : modelData.mAssociations)
                    {
                    size_t n1Index = NO_INDEX;
                    size_t n2Index = NO_INDEX;
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
                    if(n1Index != NO_INDEX && n2Index != NO_INDEX)
                        {
                        insertConnection(n1Index, n2Index,
                                ClassConnectItem(ctIneritance, assoc->getAccess()));
                        }
                    }
                }
            }
        }
    }

void ClassGraph::addNode(const ModelData &model, char const * const className,
        ClassGraph::eAddNodeTypes addType, int nodeDepth, bool reposition)
    {
    const ModelType *type = model.getTypeRef(className);
    const ModelClassifier *classifier = type->getClass();
    if(classifier)
        {
        if(mNodes.size() == 0 && mGraphOptions.drawRelationKey)
            {
            addRelationKeyNode();
            }
        size_t startSize = mNodes.size();
        getRelatedNodesRecurse(model, classifier, addType, nodeDepth, mNodes);
        if(startSize != mNodes.size())
            {
            mModified = true;
            }
        updateGraph(model, reposition);
        }
    }

void ClassGraph::addRelationKeyNode()
    {
    mNodes.push_back(ClassNode(nullptr, mGraphOptions));
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

GraphSize ClassGraph::updateGraph(const ModelData &modelData, bool geneRepositioning)
    {
    if(geneRepositioning)
        {
        stopAndWaitForCompletion();
        addTask(ClassGraphBackgroundItem(&modelData, &mGraphOptions));
        }
    else
        {
        updateNodeSizes();
        updateConnections(modelData);
        mGraphListener->doneRepositioning();
        }
    return(getGraphSize());
    }

