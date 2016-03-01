/*
 * OperationGraph.cpp
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "OperationGraph.h"
#include "Debug.h"
#include <algorithm>
#include <map>

#define DEBUG_OPERGRAPH 0

#if(DEBUG_OPERGRAPH)
static DebugFile sLog("OperGraph.txt");
#endif

class OperationClass *OperationNode::getClass()
    {
    return (getNodeType()==NT_Class) ? static_cast<OperationClass*>(this) : nullptr;
    }

class OperationClass const *OperationNode::getClass() const
    {
    return (getNodeType()==NT_Class) ? static_cast<OperationClass const*>(this) : nullptr;
    }

class OperationVariable *OperationNode::getVariable()
    {
    return (getNodeType()==NT_Variable) ? static_cast<OperationVariable*>(this) : nullptr;
    }

class OperationVariable const *OperationNode::getVariable() const
    {
    return (getNodeType()==NT_Variable) ? static_cast<OperationVariable const*>(this) : nullptr;
    }

OperationStatement::~OperationStatement()
    {
    }

OperationCall *OperationStatement::getCall()
    {
    return(mStatementType == ST_Call ? static_cast<OperationCall*>(this) : nullptr);
    }

OperationVarRef *OperationStatement::getVarRef()
    {
    return(mStatementType == ST_VarRef ? static_cast<OperationVarRef*>(this) : nullptr);
    }

OperationNestStart::~OperationNestStart()
    {
    }

const OperationNestStart *OperationStatement::getNestStart() const
    {
    return(mStatementType == ST_OpenNest ?
            static_cast<const OperationNestStart*>(this) : nullptr);
    }

OperationNestEnd::~OperationNestEnd()
    {
    }

const OperationNestEnd *OperationStatement::getNestEnd() const
    {
    return(mStatementType == ST_CloseNest ?
            static_cast<const OperationNestEnd*>(this) : nullptr);
    }

OperationCall::~OperationCall()
    {
    }

OperationVarRef::~OperationVarRef()
    {
    }

bool OperationCall::compareOperation(const OperationCall &call) const
    {
    return(getDestNode() == call.getDestNode() &&
            ModelStatement::compareFuncNames(getOverloadFuncName(), call.getOverloadFuncName()));
    }

OperationDefinition::~OperationDefinition()
    {
    removeStatements();
    }

size_t OperationDefinition::getNestDepth() const
    {
    size_t maxDepth = 0;
    size_t depth = 0;
    for(const auto &stmt : mStatements)
        {
        if(stmt->getStatementType() == ST_OpenNest)
            {
            depth++;
            if(depth > maxDepth)
                maxDepth = depth;
            }
        else if(stmt->getStatementType() == ST_CloseNest)
            {
            depth--;
            }
        }
    return maxDepth;
    }

const OperationCall *OperationDefinition::getCall(int x, int y) const
    {
    OperationCall *retCall = NULL;
    for(const auto &stmt : mStatements)
        {
        if(stmt->getStatementType() == ST_Call)
            {
            OperationCall *call = stmt->getCall();
            GraphRect rect;
            call->getRect(rect);
            if(rect.isPointIn(GraphPoint(x, y)))
                {
                retCall = call;
                break;
                }
            }
        }
    return retCall;
    }

bool OperationDefinition::isCalled(const OperationCall &opcall) const
    {
    bool called = false;
    for(const auto &stmt : mStatements)
        {
        if(stmt->getStatementType() == ST_Call)
            {
            const OperationCall *stmtcall = stmt->getCall();
            if(stmtcall->compareOperation(opcall))
                {
                called = true;
                break;
                }
            }
        }
    return called;
    }

bool OperationDefinition::isClassReferred(OperationNode const *destNode) const
    {
    bool referred = (getDestNode() == destNode);
    if(!referred)
        {
        for(const auto &stmt : mStatements)
            {
            OperationCall *call = stmt->getCall();
            if(call && call->getDestNode() == destNode)
                {
                referred = true;
                break;
                }
            }
        }
    return referred;
    }


size_t OperationGraph::addOrGetClass(const ModelClassifier *cls, eGetClass gc)
    {
    size_t index = NO_INDEX;
    for(size_t i=0; i<mNodes.size(); i++)
        {
        OperationClass const *opClass = mNodes[i]->getClass();
        if(opClass)
            {
            if(opClass->getType() == cls)
                {
                index = i;
                break;
                }
            }
        }
    if(index == NO_INDEX && gc == GC_AddClasses)
        {
        OperationClass *opCls = new OperationClass(cls);
        /// @todo - use make_unique when supported.
        mNodes.push_back(std::unique_ptr<OperationNode>(opCls));
        index = mNodes.size()-1;
        }
    return index;
    }

size_t OperationGraph::addOrGetVariable(ModelClassifier const *ownerClass,
    OovStringRef varName, eGetClass gc)
    {
    size_t index = NO_INDEX;
    for(size_t i=0; i<mNodes.size(); i++)
        {
        OperationVariable const *opVar = mNodes[i]->getVariable();
        if(opVar)
            {
            if(OovString(opVar->getName()).compare(
                OperationVariable::getName(ownerClass, varName)) == 0)
                {
                index = i;
                break;
                }
            }
        }
    if(index == NO_INDEX && gc == GC_AddClasses)
        {
        OperationVariable *opVar = new OperationVariable(ownerClass, varName);
        /// @todo - use make_unique when supported.
        mNodes.push_back(std::unique_ptr<OperationNode>(opVar));
        index = mNodes.size()-1;
        }
    return index;
    }

/// @todo - these two classes are a bit of a kludge.  Since not all classes
/// are defined in xmi files (such as external libraries), then there is no
/// class definition, but there is still a call to the method in the class.
/// This allows displaying the call even though there is no operation defined
/// in the model.
class DummyOperation:public ModelOperation
    {
    public:
        DummyOperation(const std::string &name, bool isConst, bool isVirt):
            ModelOperation(name, Visibility::Public, isConst, isVirt)
            {}
    };

class DummyOperationCall:public OperationCall
    {
    public:
        DummyOperationCall(OperationNode const *destNode, const std::string &name,
        		bool isConst):
            OperationCall(destNode, *(mDo=new DummyOperation(name,
            	isConst, false)))
            {}
        virtual ~DummyOperationCall();
    private:
        DummyOperation *mDo;
    };

DummyOperationCall::~DummyOperationCall()
    {
    delete mDo;
    }

static bool hasCall(const ModelStatements &stmts, size_t stmtIndex)
    {
    int nest=0;
    bool hasCall = false;
    for(size_t i=stmtIndex; i<stmts.size(); i++)
        {
        if(stmts[i].getStatementType() == ST_OpenNest)
            {
            nest++;
            }
        else if(stmts[i].getStatementType() == ST_Call)
            {
            hasCall = true;
            }
        else if(stmts[i].getStatementType() == ST_CloseNest)
            {
            nest--;
            if(nest == 0)
                break;
            }
        }
    return hasCall;
    }

static const ModelStatements pruneEmpty(const ModelStatements &stmts)
    {
    ModelStatements pruned;
    int discardClose = 0;
    for(size_t i=0; i<stmts.size(); i++)
        {
        if(stmts[i].getStatementType() == ST_OpenNest)
            {
            if(hasCall(stmts, i))
                {
                pruned.addStatement(stmts[i]);
                }
            else
                {
                discardClose++;
                }
            }
        else if(stmts[i].getStatementType() == ST_CloseNest)
            {
            if(discardClose == 0)
                {
                pruned.addStatement(stmts[i]);
                }
            else
                {
                discardClose--;
                }
            }
        else
            {
            pruned.addStatement(stmts[i]);
            }
        }
    return pruned;
    }

int OperationGraph::getNodeIndex(OperationNode const *refNode) const
    {
    auto iter = std::find_if(mNodes.begin(), mNodes.end(),
        [&refNode](std::unique_ptr<OperationNode> const &node)
        { return node.get() == refNode; });
    return(iter-mNodes.begin());
    }

void OperationGraph::fillDefinition(const ModelStatements &stmts, OperationDefinition &opDef,
        eGetClass gc)
    {
    ModelStatements prunedStmts = pruneEmpty(stmts);
    for(auto &stmt : prunedStmts)
        {
        if(stmt.getStatementType() == ST_OpenNest)
            {
            opDef.addNestStart(stmt.getCondName());
            }
        else if(stmt.getStatementType() == ST_CloseNest)
            {
            opDef.addNestEnd();
            }
        else if(stmt.getStatementType() == ST_VarRef)
            {
            const ModelClassifier *cls = ModelType::getClass(stmt.getClassDecl().getDeclType());
            if(cls)
                {
//                OovString varName = cls->getName();
//                varName += "::";
//                varName += stmt.getAttrName();
                size_t nodeIndex = addOrGetVariable(cls, stmt.getAttrName(), gc);
                if(nodeIndex != NO_INDEX)
                    {
                    /// @todo - use make_unique when supported.
                    opDef.addStatement(std::unique_ptr<OperationStatement>(
                        new OperationVarRef(mNodes[nodeIndex].get())));
                    }
                }
            }
        else if(stmt.getStatementType() == ST_Call)
            {
            const ModelClassifier *cls = ModelClassifier::getClass(
                stmt.getClassDecl().getDeclType());
            // If there is no class, it must be an [else]
/*
            if(!cls)
                {
                opDef.addStatement(std::unique_ptr<OperationStatement>(
                        new DummyOperationCall(NO_INDEX, stmt.getName(),
                        stmt.getDecl().isConst())));
                }
*/
            if(cls)
                {
                size_t classIndex = addOrGetClass(cls, gc);
                if(classIndex != NO_INDEX)
                    {
                    const ModelOperation *targetOper = cls->getMatchingOperation(stmt);
                    if(targetOper)
                        {
                        /// @todo - use make_unique when supported.
                        opDef.addStatement(std::unique_ptr<OperationStatement>(
                            new OperationCall(mNodes[classIndex].get(), *targetOper)));
#if(DEBUG_OPERGRAPH)
                        fprintf(sLog.mFp, "%d %s\n", classIndex, targetOper->getName().c_str());
#endif
                        }
                    else
                        {
                        opDef.addStatement(std::unique_ptr<OperationStatement>(
                            new DummyOperationCall(mNodes[classIndex].get(),
                            stmt.getFuncName(), stmt.getClassDecl().isConst())));
#if(DEBUG_OPERGRAPH)
                        fprintf(sLog.mFp, "Bad Oper %d %s %d\n", classIndex,
                                call->getName().c_str(), call->getDecl().isConst());
                        for(const auto oper : cls->getOperations())
                            {
                            fprintf(sLog.mFp, "  %s %d\n", oper->getName().c_str(),
                                    oper->isConst());
                            }
#endif
                        }
                    }
                else
                    {
#if(DEBUG_OPERGRAPH)
                    fprintf(sLog.mFp, "Bad Class %d\n", classIndex);
#endif
                    }
                }
            }
        }
    }

void OperationGraph::addDefinition(OperationNode const *destNode, const ModelOperation &oper)
    {
    OperationDefinition *opDef = new OperationDefinition(destNode, oper);
    fillDefinition(oper.getStatements(), *opDef, GC_AddClasses);
    bool found = false;
    for(size_t i=0; i<mOperations.size(); i++)
        {
        found = opDef->compareOperation(*mOperations[i]);
        if(found)
            {
            break;
            }
        }
/*
    auto iter = std::find(mOperations.begin(), mOperations.end(),
        [&opDef](std::unique_ptr<OperationDefinition> const &def)
            {
            return(opDef->compareOperation(*def));
            });
    if(iter == mOperations.end())
*/
    if(!found)
        {
        /// @todo - use make_unique when supported.
        mOperations.push_back(std::unique_ptr<OperationDefinition>(opDef));
        mModified = true;
        }
    }

/// @todo - prevent infinite recursion.
/// @todo - this doesn't work for overloaded functions.
void OperationGraph::addRelatedOperations(const ModelClassifier &sourceClass,
        const ModelOperation &sourceOper,
        eAddOperationTypes /*addType*/, int maxDepth)
    {
    -- maxDepth;
    if(maxDepth >= 0)
        {
        size_t sourceClassIndex = addOrGetClass(&sourceClass, GC_AddClasses);
        addDefinition(mNodes[sourceClassIndex].get(), sourceOper);
        }
    }

void OperationGraph::clearGraphAndAddOperation(const ModelData &model,
        const OperationDrawOptions & /*options*/, char const * const className,
        char const * const operName, bool isConst, int /*nodeDepth*/)
    {
#if(DEBUG_OPERGRAPH)
    fprintf(sLog.mFp, "---------\n");
#endif
    clearGraph();
    const ModelClassifier *sourceClass = ModelClassifier::getClass(
        model.getTypeRef(className));
    if(sourceClass)
        {
        std::vector<const ModelOperation*> opers = sourceClass->getOperationsByName(operName);
        for(auto const &oper : opers)
            {
            addRelatedOperations(*sourceClass, *oper, OperationGraph::AO_All, 2);
            }
/*
        const ModelOperation *sourceOper = sourceClass->getOperationByName(operName, isConst);
        if(sourceOper)
            {
            addRelatedOperations(*sourceClass, *sourceOper, OperationGraph::AO_All, 2);
            }
*/
        }
    mModified = false;
    }

void OperationGraph::removeOperation(size_t index)
    {
    mOperations.erase(mOperations.begin() + static_cast<int>(index));
    mModified = true;
    }

void OperationGraph::removeNode(size_t index)
    {
    mNodes.erase(mNodes.begin() + static_cast<int>(index));
//    removeUnusedClasses();
    mModified = true;
    }

void OperationGraph::removeNode(const OperationNode *node)
    {
    size_t classIndex = NO_INDEX;
    for(size_t i=0; i<mNodes.size(); i++)
        {
        if(mNodes[i]->getPosition().x == node->getPosition().x)
            {
            removeNode(i);
            classIndex = i;
            break;
            }
        }
    if(classIndex != NO_INDEX)
        {
        for(size_t i=0; i<mOperations.size(); i++)
            {
            if(mOperations[i]->getDestNode() == mNodes[classIndex].get())
                {
                removeOperation(i);
                break;
                }
            }
        for(const auto &oper : mOperations)
            {
            oper->removeStatements();
            fillDefinition(oper->getOperation().getStatements(), *oper, FT_OnlyGetClasses);
            }
        }
    }

size_t OperationGraph::getNestDepth(size_t classIndex)
    {
    size_t maxDepth = 0;
    for(const auto &oper : mOperations)
        {
        if(oper->getDestNode() == mNodes[classIndex].get())
            {
            size_t depth = oper->getNestDepth();
            if(depth > maxDepth)
                maxDepth = depth;
            }
        }
    return maxDepth;
    }

OperationClass const *OperationGraph::getClass(int x, int y) const
    {
    OperationNode const *node = getNode(x, y);
    OperationClass const *opClass = nullptr;
    if(node)
        {
        opClass = node->getClass();
        }
    return opClass;
    }

OperationNode const *OperationGraph::getNode(int x, int y) const
    {
    const OperationNode *node = nullptr;
    for(size_t i=0; i<mNodes.size(); i++)
        {
        GraphRect rect;
        mNodes[i]->getRect(rect);
        if(rect.isPointIn(GraphPoint(x, y)))
            {
            node = mNodes[i].get();
            }
        }
    return node;
    }

const OperationCall *OperationGraph::getOperation(int x, int y) const
    {
    const OperationCall *call = nullptr;
    for(const auto &oper : mOperations)
        {
        GraphRect rect;
        oper->getRect(rect);
        if(rect.isPointIn(GraphPoint(x, y)))
            {
            call = oper.get();
            break;
            }
        call = oper->getCall(x, y);
        if(call)
            break;
        }
    return call;
    }

OovStringRef OperationGraph::getNodeName(const OperationCall &opcall) const
    {
    return opcall.getDestNode()->getName();
    }

void OperationGraph::addOperDefinition(const OperationCall &opcall)
    {
    if(opcall.getDestNode()->getNodeType() == NT_Class)
        {
        OperationClass const *opClass = opcall.getDestNode()->getClass();
        if(opClass)
            {
            const ModelClassifier *cls = ModelClassifier::getClass(opClass->getType());
            const ModelOperation *targetOper = cls->getMatchingOperation(opcall.getOperation());
            if(!isOperDefined(opcall) && targetOper)
                addDefinition(opcall.getDestNode(), *targetOper);
            }
        }
    }

void OperationGraph::addOperCallers(const ModelStatements &stmts,
        const ModelClassifier &srcCls, const ModelOperation &oper,
        const OperationCall &callee)
    {
    OperationClass const *calleeClass = callee.getDestNode()->getClass();
    if(calleeClass)
        {
        for(auto const &stmt : stmts)
            {
            if(stmt.getStatementType() == ST_Call)
                {
                const ModelClassifier *callerCls = ModelClassifier::getClass(
                    stmt.getClassDecl().getDeclType());
                if(stmt.operMatch(callee.getOperation().getName()) &&
                        callerCls == calleeClass->getType())
                    {
                    addRelatedOperations(srcCls, oper, OperationGraph::AO_All, 1);
                    }
                }
            }
        }
    }

void OperationGraph::addOperCallers(const ModelData &model, const OperationCall &callee)
    {
    for(const auto &type : model.mTypes)
        {
        const ModelClassifier *srcCls = ModelClassifier::getClass(type.get());
        if(srcCls)
            {
            for(const auto &oper : srcCls->getOperations())
                {
                addOperCallers(oper->getStatements(), *srcCls, *oper, callee);
                }
            }
        }
    }

void OperationGraph::addVariableReferencesFromGraphNodes(OperationNode const *node)
    {
    struct ClsOper
        {
        ModelClassifier const *cls;
        ModelOperation *oper;

        bool operator<(ClsOper const &clsOper) const
            { return(cls<clsOper.cls ||
                (cls==clsOper.cls && oper<clsOper.oper)); }
        };
    std::set<ClsOper> newOpers;
    OperationVariable const *var = node->getVariable();
    if(var)
        {
        for(auto const &node : mNodes)
            {
            OperationClass *opClass = node->getClass();
            if(opClass)
                {
                const ModelClassifier *cls = ModelClassifier::getClass(opClass->getType());
                for(auto const &oper : cls->getOperations())
                    {
                    ModelStatements &stmts = oper->getStatements();
                    for(auto const &stmt : stmts)
                        {
                        if(stmt.getStatementType() == ST_VarRef)
                            {
                            ModelClassifier const *varClass = var->getOwnerClass();
                            if(varClass)
                                {
                                if(stmt.getClassDecl().getDeclType() == varClass &&
                                    stmt.getAttrName() == var->getAttrName())
                                    {
                                    ClsOper clsOp;
                                    clsOp.cls = cls;
                                    clsOp.oper = oper.get();
                                    newOpers.insert(clsOp);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    for(auto const &oper : newOpers)
        {
        addRelatedOperations(*oper.cls, *oper.oper, OperationGraph::AO_All, 1);
        }
    }

void OperationGraph::removeOperDefinition(const OperationCall &opcall)
    {
    OperationClass const *opClass = opcall.getDestNode()->getClass();
    if(opClass)
        {
        const ModelClassifier *cls = ModelClassifier::getClass(opClass->getType());
        const ModelOperation *targetOper = cls->getOperationByName(opcall.getName(),
                opcall.isConst());
        if(targetOper)
            {
            const OperationDefinition *operDef = getOperDefinition(opcall);
            if(operDef)
                {
                for(size_t i=0; i<mOperations.size(); i++)
                    {
                    if(mOperations[i].get() == operDef)
                        {
                        removeOperation(i);
                        break;
                        }
                    }
                }
            removeUnusedClasses();
            }
        }
    }

void OperationGraph::removeUnusedClasses()
    {
    for(size_t i=0; i<mNodes.size(); i++)
        {
        bool used = false;
        for(const auto &oper : mOperations)
            {
            if(oper->isClassReferred(oper->getDestNode()))
                used = true;
            }
        if(!used)
            {
            removeNode(mNodes[i].get());
            i--;
            }
        }
    }

bool OperationGraph::isOperCalled(const OperationCall &opcall) const
    {
    bool called = false;
    for(const auto &oper : mOperations)
        {
        if(oper->isCalled(opcall))
            {
            called = true;
            break;
            }
        }
    return called;
    }

OperationDefinition *OperationGraph::getOperDefinition(
        const OperationCall &opcall) const
    {
    OperationDefinition *operDef = NULL;
    for(const auto &oper : mOperations)
        {
        if(oper->getDestNode() == opcall.getDestNode() &&
                std::string(opcall.getName()).compare(oper->getName()) == 0)
            {
            operDef = oper.get();
            break;
            }
        }
    return operDef;
    }
