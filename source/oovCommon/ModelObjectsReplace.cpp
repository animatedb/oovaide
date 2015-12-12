/*
 * ModelObjectsReplace.cpp
 *
 *  Created on: Dec 9, 2015
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

// This module is used only by OovAide and OovCppParser since they are the
// programs that replace types.

#include "ModelObjects.h"
#include "Debug.h"
#include <map>

void ModelData::replaceType(ModelType *existingType, ModelClassifier *newType)
    {
    // Don't need to update function parameter types at this time, because the
    // existing type is a datatype, and datatypes are not referred to at this time.

    // update member attributes.
    for(const auto &type : mTypes)
        {
        if(type->getDataType() == DT_Class)
            {
            ModelClassifier *classifier = ModelClassifier::getClass(type.get());
            for(auto &attr : classifier->getAttributes())
                {
                if(attr->getDeclType() == existingType)
                    {
                    attr->setDeclType(newType);
                    }
                }
            for(auto &oper : classifier->getOperations())
                {
                for(auto &parm : oper->getParams())
                    {
                    if(parm->getDeclType() == existingType)
                        {
                        parm->setDeclType(newType);
                        }
                    }
                for(auto &vd : oper->getBodyVarDeclarators())
                    {
                    if(vd->getDeclType() == existingType)
                        {
                        vd->setDeclType(newType);
                        }
                    }
                if(oper->getReturnType().getDeclType() == existingType)
                    {
                    oper->getReturnType().setDeclType(newType);
                    }
                replaceStatementType(oper->getStatements(), existingType, newType);
                }
            }
        }
    // Resolve relations.
    for(auto &assoc : mAssociations)
        {
        if(assoc->getChild() == existingType)
            {
            assoc->setChildClass(newType);
            }
        if(assoc->getParent() == existingType)
            {
            assoc->setParentClass(newType);
            }
        }
    eraseType(existingType);
    }

void ModelData::replaceStatementType(ModelStatements &stmts, ModelType *existingType,
        ModelClassifier *newType)
    {
    for(auto &stmt : stmts)
        {
        if(stmt.getStatementType() == ST_Call ||
                stmt.getStatementType() == ST_VarRef)
            {
            if(stmt.getClassDecl().getDeclType() == existingType)
                {
                stmt.getClassDecl().setDeclType(newType);
                }
            if(stmt.getStatementType() == ST_VarRef)
                {
                if(stmt.getVarDecl().getDeclType() == existingType)
                    {
                    stmt.getVarDecl().setDeclType(newType);
                    }
                }
            }
        }
    }

void ModelData::eraseType(ModelType *existingType)
    {
    // Delete the old type
    for(size_t ci=0; ci<mTypes.size(); ci++)
        {
        if(mTypes[ci].get() == existingType)
            {
            mTypes.erase(mTypes.begin() +
                static_cast<int>(ci));
            }
        }
    }
