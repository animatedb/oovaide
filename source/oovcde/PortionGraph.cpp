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
        if(node.getNodeType() == nt && node.getName() == name.getStr())
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
            for(auto const &attr : cls->getAttributes())
                {
                mNodes.push_back(PortionNode(attr->getName(), PNT_Attribute));
                }
            for(auto const &oper : cls->getOperations())
                {
                mNodes.push_back(PortionNode(oper->getName(), PNT_Operation));
#if(NonMemberVariables)
                ModelStatements const &statements = oper->getStatements();
                for(auto const &stmt : statements)
                    {
                    if(stmt.getStatementType() == ST_Call)
                	{
                	if(stmt.hasNonMemberVar())
                	    {
			    OovString attrName = stmt.getAttrName();
			    if(attrName.length() > 0 && !getNode(attrName, PNT_NonMemberVariable))
				{
				mNodes.push_back(PortionNode(attrName, PNT_NonMemberVariable));
				}
		            PortionConnection conn(getNodeIndex(getNode(attrName, PNT_NonMemberVariable)),
		            	getNodeIndex(getNode(oper->getName(), PNT_Operation)));
		            mConnections.push_back(conn);
                	    }
                	}
                    }
#endif
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
	addAttrOperConnections(attr->getName(), cls->getOperations());
	}
    // Operation to operation connections.
    for(auto const &oper : cls->getOperations())
	{
	ModelStatements const &statements = oper->getStatements();
	addOperationConnections(cls, statements, getNode(oper->getName(),
		PNT_Operation));
	}
    }

void PortionGraph::addAttrOperConnections(OovStringRef attrName,
	std::vector<std::unique_ptr<ModelOperation>> const &opers)
    {
    for(auto const &oper : opers)
        {
        ModelStatements const &statements = oper->getStatements();
        if(statements.checkAttrUsed(attrName))
            {
            PortionConnection conn(getNodeIndex(getNode(attrName, PNT_Attribute)),
            	getNodeIndex(getNode(oper->getName(), PNT_Operation)));
            mConnections.push_back(conn);
            }
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
			stmt.getFuncName(), false);
		if(oper)
		    {
		    PortionConnection conn(getNodeIndex(getNode(oper->getName(),
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

