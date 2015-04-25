/*
 * PortionGraph.cpp
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "PortionGraph.h"


PortionNode const *PortionGraph::getNode(OovStringRef name) const
    {
    PortionNode const *retNode = nullptr;
    for(auto const &node : mNodes)
        {
        if(node.getName() == name.getStr())
            {
            retNode = &node;
            break;
            }
        }
    return retNode;
    }

void PortionGraph::clearAndAddClass(const ModelData &model, OovStringRef classname)
    {
    const ModelType *type = model.findType(classname);
    if(type)
        {
        ModelClassifier const *cls = type->getClass();
        if(cls)
            {
            for(auto const &oper : cls->getOperations())
                {
                mNodes.push_back(PortionNode(oper->getName(), PNT_Operation));
                }
            for(auto const &attr : cls->getAttributes())
                {
                mNodes.push_back(PortionNode(attr->getName(), PNT_Attribute));
                }
            addConnections(cls);
            }
        }
    }

void PortionGraph::addConnections(ModelClassifier const *cls)
    {
    for(auto const &attr : cls->getAttributes())
	{
        for(auto const &oper : cls->getOperations())
            {
            ModelStatements const &statements = oper->getStatements();
            if(statements.checkAttrUsed(attr->getName()))
                {
                PortionConnection conn(getNodeIndex(getNode(attr->getName())),
                	getNodeIndex(getNode(oper->getName())));
                mConnections.push_back(conn);
                }
            }
	}
    for(auto const &oper : cls->getOperations())
	{
	ModelStatements const &statements = oper->getStatements();
	addOperationConnections(cls, statements, getNode(oper->getName()));
	}
    }

void PortionGraph::addOperationConnections(ModelClassifier const *classifier,
	ModelStatements const &statements, PortionNode const *callerOperNode)
    {
    for(auto const &stmt : statements)
	{
	if(stmt.getStatementType() == ST_Call)
	    {
	    ModelClassifier const *cls = stmt.getClassDecl().getDeclType()->getClass();
	    if(cls == classifier)
		{
		/// @todo - have to handle overloaded operators.
		const ModelOperation *oper = cls->getOperationAnyConst(
			stmt.getName(), false);
		if(oper)
		    {
		    PortionConnection conn(getNodeIndex(getNode(oper->getName())),
			    getNodeIndex(callerOperNode));
		    mConnections.push_back(conn);
		    }
		}
	    }
	}
    }

