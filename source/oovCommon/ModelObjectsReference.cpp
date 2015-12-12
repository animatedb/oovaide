/*
 * ModelObjectsReplace.cpp
 *
 *  Created on: Dec 9, 2015
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

// This module is used only by OovAide and OovCppParser since they are the
// programs that check referenced types.

#include "ModelObjects.h"
#include "Debug.h"
#include <map>

bool ModelData::isTypeReferencedByStatements(ModelStatements const &stmts,
        ModelType const &type) const
    {
    bool referenced = false;
    for(auto &stmt : stmts)
        {
        if(stmt.getStatementType() == ST_Call &&
                stmt.getClassDecl().getDeclType() == &type)
            {
            referenced = true;
            break;
            }
        if(stmt.getStatementType() == ST_VarRef &&
                (stmt.getClassDecl().getDeclType() == &type ||
                stmt.getVarDecl().getDeclType() == &type))
            {
            referenced = true;
            break;
            }
        }
    return referenced;
    }

bool ModelData::isTypeReferencedByOperationInterface(ModelOperation const &oper,
    ModelType const &checkType) const
    {
    bool referenced = false;
    // Check function parameters.
    for(auto &param : oper.getParams())
        {
        if(param->getDeclType() == &checkType)
            {
            referenced = true;
            break;
            }
        }
    if(!referenced)
        {
        if(oper.getReturnType().getDeclType() == &checkType)
            {
            referenced = true;
            }
        }
    return referenced;
    }

bool ModelData::isTypeReferencedByOperation(ModelOperation const &oper,
    ModelType const &checkType) const
    {
    bool referenced = false;
    // Only defined operations in the translation unit have a module.
    if(oper.getModule())
        {
        referenced = isTypeReferencedByOperationInterface(oper, checkType);
        if(!referenced)
            {
            // Check function call decls.
            ModelStatements const &stmts = oper.getStatements();
            referenced = isTypeReferencedByStatements(stmts, checkType);
            }
        if(!referenced)
            {
            // Check body variables.
            for(auto &vd : oper.getBodyVarDeclarators())
                {
                if(vd->getDeclType() == &checkType)
                    {
                    referenced = true;
                    break;
                    }
                }
            }
        }
    return referenced;
    }

bool ModelData::isTypeReferencedByClassAttributes(ModelClassifier const &classifier,
    ModelType const &checkType) const
    {
    bool referenced = false;
    for(auto &attr : classifier.getAttributes())
        {
        if(attr->getDeclType() == &checkType)
            {
            referenced = true;
            if(referenced)
                {
                break;
                }
            }
        }
    return referenced;
    }

bool ModelData::isTypeReferencedByDefinedObjects(ModelType const &checkType) const
    {
    bool referenced = false;
    for(const auto &type : mTypes)
        {
        if(type->getDataType() == DT_Class)
            {
            ModelClassifier *classifier = ModelClassifier::getClass(type.get());
            // Only defined classes in the parsed translation unit have a module.
            if(classifier->getModule())
                {
                referenced = isTypeReferencedByClassAttributes(*classifier, checkType);
                }
            if(!referenced)
                {
                for(auto &oper : classifier->getOperations())
                    {
                    referenced = isTypeReferencedByOperation(*oper, checkType);
                    if(referenced)
                        {
                        break;
                        }
                    }
                }
            if(referenced)
                {
                break;
                }
            }
        }
    // Check relations.
    if(!referenced)
        {
        for(auto &assoc : mAssociations)
            {
            if(assoc->getChild() == &checkType || assoc->getParent() == &checkType)
                {
                referenced = true;
                break;
                }
            }
        }
    return referenced;
    }

