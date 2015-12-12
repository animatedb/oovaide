/*
 * PortionGraph.cpp
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "PortionGraph.h"
#include "Debug.h"


PortionNode const *PortionGraph::getNode(OovStringRef name, PortionNodeTypes nt) const
    {
    PortionNode const *retNode = nullptr;
    for(auto const &node : mNodes)
        {
        if(node.getNodeType() == nt)
            {
            if(node.getName() == name.getStr())
                {
                retNode = &node;
                break;
                }
            }
        }
    if(!retNode)
        {
        DebugAssert(__FILE__, __LINE__);
        }
    return retNode;
    }

void PortionGraph::clearAndAddClass(OovStringRef classname)
    {
    const ModelType *type = mModel->findType(classname);
    if(type)
        {
        ModelClassifier const *cls = ModelClassifier::getClass(type);
        if(cls)
            {
            for(auto const &attr : cls->getAttributes())
                {
                mNodes.push_back(PortionNode(attr->getName(), PNT_Attribute));
                }
            for(auto const &oper : cls->getOperations())
                {
                mNodes.push_back(PortionNode(oper->getOverloadFuncName(), PNT_Operation,
                    oper->isVirtual()));
                }
            for(auto const &oper : cls->getOperations())
                {
                ModelStatements const &statements = oper->getStatements();
                for(auto const &stmt : statements)
                    {
                    if(stmt.hasBaseClassRef())
                        {
                        OovString operName = stmt.getFuncName();
                        /// @todo - this doesn't work in the case of overloading a
                        /// method in the base class of the same name.
                        if(!getNode(operName, PNT_Operation))
                            {
                            ModelClassifier const *calledClass =
                                ModelClassifier::getClass(stmt.getClassDecl().getDeclType());
                            OovString className = calledClass->getName();
                            if(className.length() > 0)
                                {
                                if(!getNode(className, PNT_NonMemberVariable))
                                    {
                                    mNodes.push_back(PortionNode(className,
                                        PNT_NonMemberVariable));
                                    }
                                size_t supIndex = getNodeIndex(getNode(
                                    className, PNT_NonMemberVariable));
                                size_t consIndex = getNodeIndex(getNode(
                                    oper->getOverloadFuncName(), PNT_Operation));
                                PortionConnection conn(supIndex, consIndex);
                                mConnections.push_back(conn);
                                }
                            }
                        }
                    }
                }
            addConnections(cls);
            }
        }
    }

void PortionGraph::addConnections(ModelClassifier const *cls)
    {
    // Operation to attribute connections
    for(auto const &attr : cls->getAttributes())
        {
        addAttrOperConnections(cls, attr->getName(), cls->getOperations());
        }
    // Operation to operation connections.
    for(auto const &oper : cls->getOperations())
        {
        ModelStatements const &statements = oper->getStatements();
        addOperationConnections(cls, statements, getNode(oper->getOverloadFuncName(),
                PNT_Operation));
        }
    }

void PortionGraph::addAttrOperConnections(ModelClassifier const *cls,
        OovStringRef attrName,
        std::vector<std::unique_ptr<ModelOperation>> const &opers)
    {
    for(auto const &oper : opers)
        {
        ModelStatements const &statements = oper->getStatements();
        if(statements.checkAttrUsed(cls, attrName))
            {
            PortionNode const *attrNode = getNode(attrName, PNT_Attribute);
            PortionNode const *operNode = getNode(oper->getOverloadFuncName(),
                    PNT_Operation);
            if(attrNode && operNode)
                {
                PortionConnection conn(getNodeIndex(attrNode),
                        getNodeIndex(operNode));
                mConnections.push_back(conn);
                }
            else
                {
                DebugAssert(__FILE__, __LINE__);
                }
            }
        }
    }

void PortionGraph::addOperationConnections(ModelClassifier const *classifier,
        ModelStatements const &statements, PortionNode const *callerOperNode)
    {
    if(callerOperNode)
        {
        for(auto const &stmt : statements)
            {
            if(stmt.getStatementType() == ST_Call)
                {
                ModelClassifier const *cls = ModelClassifier::getClass(
                    stmt.getClassDecl().getDeclType());
                if(cls == classifier)
                    {
                    const ModelOperation *oper = cls->getMatchingOperation(stmt);
                    if(oper)
                        {
                        PortionConnection conn(getNodeIndex(getNode(oper->getOverloadFuncName(),
                                PNT_Operation)), getNodeIndex(callerOperNode));
                        mConnections.push_back(conn);
                        }
                    else
                        {
                        DebugAssert(__FILE__, __LINE__);
                        }
                    }
                }
            }
        }
    }

