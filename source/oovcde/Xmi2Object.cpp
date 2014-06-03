/**
*   Copyright (C) 2008 by dcblaha
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*  @file      xmi2object.cpp
*
*  @author    dcblaha
*
*  @date      2/1/2009
*
*/
#include "Xmi2Object.h"
#include <string.h>
#include <stdlib.h>     // for atoi
#include <stdio.h>
#include <algorithm>
#include <cassert>
#include "Debug.h"


enum elementTypes { ET_None, ET_Class, ET_DataType,
    ET_Attr, ET_Function, ET_FuncParam, ET_BodyVarDecl,
    ET_Generalization, ET_Module, ET_OperCall, ET_CondStatement };


#define DEBUG_LOAD 0
#if(DEBUG_LOAD)
    static DebugFile sLog("DebugXmiParse.txt");
    static ModelModule const *sCurrentModule;
    static bool sDumpFile = false;
    // This can be used for debugging. FN format is something like "DiagramDrawer_h.xmi".
    static char const *sCurrentFilename;
    static char const * const getObjectTypeName(ObjectType ot)
	{
	char const *p = nullptr;
	switch(ot)
	    {
	    case otClass:	p = "Class";	break;
	    case otAssociation:	p = "Assoc";	break;
	    case otAttribute:	p = "Attr ";	break;
	    case otOperation:	p = "Oper ";	break;
	    case otCondStatement:	p = "Cond ";	break;
	    case otBodyVarDecl:	p = "var  ";	break;
	    case otDatatype:	p = "DataT";	break;
	    case otOperCall:	p = "Call ";	break;
	    case otOperParam:	p = "Param";	break;
	    case otModule:	p = "Modul";	break;
	    }
	return p;
	}
    static void dumpFilename(char const * const fn, int typeIndex)
	{
	if(sDumpFile)
	    {
	    sCurrentFilename = fn;
	    fprintf(sLog.mFp, "\n***** %s - %d *****\n\n", fn, typeIndex);
	    }
	}
    static void dumpStatements(const ModelStatement *stmt, int level)
	{
	if(sDumpFile)
	    {
	    level++;
	    std::string ws(level*2, ' ');
	    if(stmt->getObjectType() == otCondStatement)
		{
		const ModelCondStatements *condStmts = static_cast<const ModelCondStatements*>(stmt);
		fprintf(sLog.mFp, "   %s%s   %s\n", ws.c_str(),
			getObjectTypeName(condStmts->getObjectType()),
			condStmts->getName().c_str());
		for(const auto &stmt : condStmts->getStatements())
		    {
		    dumpStatements(stmt, level);
		    }
		}
	    else if(stmt->getObjectType() == otOperCall)
		{
		const ModelOperationCall *call = static_cast<const ModelOperationCall*>(stmt);
		fprintf(sLog.mFp, "   %s%s   %d:%s\n", ws.c_str(),
			getObjectTypeName(call->getObjectType()),
			call->getDecl().getDeclTypeModelId(), call->getName().c_str());
		fflush(sLog.mFp);
		}
	    }
	}
    static void dumpTypes(const ModelData &graph)
	{
	if(sDumpFile)
	    {
	    fprintf(sLog.mFp, "\n** Type Dump **\n");
	    for(size_t i=0; i<graph.mTypes.size(); i++)
		{
		const ModelType *mt = graph.mTypes[i];
		fprintf(sLog.mFp, " %s   %s %d\n", getObjectTypeName(mt->getObjectType()),
			mt->getName().c_str(), mt->getModelId());
		fflush(sLog.mFp);
		if(mt->getObjectType() == otClass)
		    {
		    const ModelClassifier*c = ModelObject::getClass(mt);
		    if(c->getModule() == sCurrentModule)
			{
			for(const auto &attr : c->getAttributes())
			    {
			    fprintf(sLog.mFp, "   %s   %s\n", getObjectTypeName(attr->getObjectType()),
				    attr->getName().c_str());
			    }
			for(const auto &oper : c->getOperations())
			    {
			    fprintf(sLog.mFp, "   %s   %s", getObjectTypeName(oper->getObjectType()),
				    oper->getName().c_str());
			    if(oper->getModule())
				fprintf(sLog.mFp, " %d", oper->getLineNum());
			    fprintf(sLog.mFp, "\n");
			    dumpStatements(&oper->getCondStatements(), 0);
			    }
			}
		    }
		fflush(sLog.mFp);
		}
	    }
	}
    static void dumpRelations(const ModelData &graph)
	{
	if(sDumpFile)
	    {
	    fprintf(sLog.mFp, "\n** Relations Dump **\n");
	    for(const auto &assoc : graph.mAssociations)
		{
		const ModelClassifier *child = ModelObject::getClass(assoc->getChild());
		const ModelClassifier *parent = ModelObject::getClass(assoc->getParent());
		if(child && parent)
		    {
		    if(child->getModule() == sCurrentModule ||
			    parent->getModule() == sCurrentModule)
			{
			fprintf(sLog.mFp, " %s %s\n", parent->getName().c_str(), child->getName().c_str());
			}
		    }
		else
		    fprintf(sLog.mFp, " %d %d\n", assoc->getParentModelId(), assoc->getChildModelId());
		fflush(sLog.mFp);
		}
	    }
	}
#endif

static void replaceAttrChars(tString &str)
{
    static struct
    {
        char const * const mSrch;
        char const * const mRep;
    } words[] =
    { 
        { "&amp;", "&" },
        { "&lt;", "<" },
        { "&gt;", ">" },
	{ "&apos;", "\'" },
	{ "&quot;", "\"" },
    };
    for(unsigned int i=0; i<sizeof(words)/sizeof(words[0]); i++)
        {
	size_t starti = 0;
	do
	    {
	    starti = str.find(words[i].mSrch);
	    if(starti != tString::npos)
		{
		str.replace(starti, strlen(words[i].mSrch), words[i].mRep);
		}
	    } while(starti != tString::npos);
        }
}

bool XmiParser::parse(char const * const buf)
    {
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "----------\n");
#endif
    bool success = (parseXml(buf) == ERROR_NONE);
    if(success)
	{
	updateTypeIndices();
//	mModel.resolveModelIds();
	}
    return success;
    }

void XmiParser::onOpenElem(char const * const name, int len)
    {
    struct nameLookup
        {
        char const * const mName;
        elementTypes mElType;
        };
    static nameLookup names[] =
    {
	{ "Attribute", ET_Attr, },
        { "Class", ET_Class, },
        { "DataType", ET_DataType, },
        { "Generalization", ET_Generalization, },
        // { "Enumeration", ET_Enumeration, },
        { "Module", ET_Module },
        { "Operation", ET_Function },
        { "Parameter", ET_FuncParam },
        { "BodyVarDecl", ET_BodyVarDecl },
        { "Call", ET_OperCall },
        { "Condition", ET_CondStatement },
    };
    elementTypes elType = ET_None;
    for(size_t ni=0; ni<sizeof(names)/sizeof(names[0]); ni++)
        {
        tString elName(name, len);
        if(elName.compare(names[ni].mName) == 0)
            {
            elType = names[ni].mElType;
            break;
            }
        }
    ModelObject *elem = nullptr;
    switch(elType)
        {
	case ET_None:
	    break;

	case ET_Class:
	    elem = new ModelClassifier("");
	    break;

	case ET_DataType:
	    elem = new ModelType("");
	    break;

	case ET_Attr:
            elem = new ModelAttribute("", nullptr, Visibility::Public);
            break;

        case ET_Function:
            elem = new ModelOperation("", Visibility(), true);
            break;

        case ET_FuncParam:
            elem = new ModelFuncParam("", nullptr);
            break;

        case ET_BodyVarDecl:
            elem = new ModelBodyVarDecl("", nullptr);
            break;

        case ET_Generalization:
            {
            Visibility vis;
            elem = new ModelAssociation(nullptr, nullptr, vis);
            }
            break;

        case ET_Module:
            elem = new ModelModule();
            break;

        case ET_OperCall:
            elem = new ModelOperationCall("", nullptr);
            break;

        case ET_CondStatement:
            elem = new ModelCondStatements("");
            break;
        }
#if(DEBUG_LOAD)
    if(sDumpFile)
	{
	if(elem)
	    { fprintf(sLog.mFp, "< %s  ", getObjectTypeName(elem->getObjectType())); }
	}
#endif
    if(elType != ET_None)
	mElementStack.push_back(elem);
    }

Visibility::VisType getAccess(char const * const accessStr)
    {
    Visibility::VisType access = Visibility::Public;
    switch(accessStr[2])
        {
        case 'b':   access = Visibility::Public;     break;
        case 'i':   access = Visibility::Private;    break;
        case 'o':   access = Visibility::Protected;  break;
        }
    return access;
    }

static int getInt(char const * const str)
    {
    int id = -1;
    sscanf(str, "%d", &id);
    return id;
    }

static bool isTrue(const std::string &attrVal)
    {
    return(attrVal[0] == 't');
    }

void XmiParser::setDeclAttr(const std::string &attrName,
	const std::string &attrVal, ModelDeclarator &decl)
    {
    if(strcmp(attrName.c_str(), "type") == 0)
        decl.setDeclTypeModelId(mStartingModuleTypeIndex + getInt(attrVal.c_str()));
    else if(strcmp(attrName.c_str(), "ref") == 0)
        decl.setRefer(isTrue(attrVal));
    else if(strcmp(attrName.c_str(), "const") == 0)
        decl.setConst(isTrue(attrVal));
    }

void XmiParser::onAttr(char const * const name, int &nameLen,
	char const * const val, int &valLen)
    {
    ModelObject *elItem = nullptr;
    if(mElementStack.size() > 0)	// XML version header is not a model object.
	elItem = mElementStack.back();
    if(elItem)
        {
        tString attrName(name, nameLen);
        tString attrVal(val, valLen);
        replaceAttrChars(attrVal);
        if(strcmp(attrName.c_str(), "xmi.id") == 0)
            {
            int index = getInt(attrVal.c_str());
            if(elItem->getObjectType() == otModule ||
        	    elItem->getObjectType() == otAssociation)
        	{
		elItem->setModelId(index);
        	}
            else
        	{
		elItem->setModelId(mStartingModuleTypeIndex + index);
		if(mEndingModuleTypeIndex < mStartingModuleTypeIndex + index)
		    mEndingModuleTypeIndex = mStartingModuleTypeIndex + index;
        	}
            }
        if(strcmp(attrName.c_str(), "name") == 0)
            {
            elItem->setName(attrVal.c_str());
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "  %s ", attrVal.c_str());
#endif
            }
        switch(elItem->getObjectType())
            {
	    case otClass:
                {
                ModelClassifier *cl = static_cast<ModelClassifier*>(elItem);
                if(strcmp(attrName.c_str(), "module") == 0)
                    {
                    int modId = getInt(attrVal.c_str());
                    const ModelModule *mod = mModel.findModuleById(modId);
                    if(mod)
                	cl->setModule(mod);
                    else
                	{
                	assert(false);
                	}
                    }
                else if(strcmp(attrName.c_str(), "line") == 0)
                    {
                    cl->setLineNum(getInt(attrVal.c_str()));
                    }
                }
                break;

            case otAttribute:
                {
                ModelAttribute *attr = static_cast<ModelAttribute*>(elItem);
                if(strcmp(attrName.c_str(), "access") == 0)
                    attr->setAccess(getAccess(attrVal.c_str()));
                else
                    setDeclAttr(attrName, attrVal, *attr);
                }
                break;

	    case otOperParam:
		{
		ModelFuncParam *fp = static_cast<ModelFuncParam*>(elItem);
		setDeclAttr(attrName, attrVal, *fp);
		}
		break;

	    case otBodyVarDecl:
		{
		ModelBodyVarDecl *vd = static_cast<ModelBodyVarDecl*>(elItem);
		setDeclAttr(attrName, attrVal, *vd);
		}
		break;

	    case otOperCall:
		{
		ModelOperationCall *oc = static_cast<ModelOperationCall*>(elItem);
		setDeclAttr(attrName, attrVal, oc->getDecl());
		}
		break;

            case otAssociation:
                {
                ModelAssociation *assoc = static_cast<ModelAssociation*>(elItem);
                if(strcmp(attrName.c_str(), "parent") == 0)
                    assoc->setParentModelId(mStartingModuleTypeIndex + getInt(attrVal.c_str()));
                else if(strcmp(attrName.c_str(), "child") == 0)
                    assoc->setChildModelId(mStartingModuleTypeIndex + getInt(attrVal.c_str()));
                else if(strcmp(attrName.c_str(), "access") == 0)
                    assoc->setAccess(getAccess(attrVal.c_str()));
                }
                break;

            case otModule:
                {
                ModelModule *mod = static_cast<ModelModule*>(elItem);
                if(strcmp(attrName.c_str(), "module") == 0)
                    mod->setModulePath(attrVal);
                }
                break;

            case otOperation:
        	{
                ModelOperation *oper = static_cast<ModelOperation*>(elItem);
                if(strcmp(attrName.c_str(), "access") == 0)
                    oper->setAccess(getAccess(attrVal.c_str()));
                else if(strcmp(attrName.c_str(), "const") == 0)
                    oper->setConst(isTrue(attrVal));
                else if(strcmp(attrName.c_str(), "module") == 0)
                    {
                    int modId = getInt(attrVal.c_str());
                    const ModelModule *mod = mModel.findModuleById(modId);
                    if(mod)
                	oper->setModule(mod);
                    }
                else if(strcmp(attrName.c_str(), "line") == 0)
                    {
                    oper->setLineNum(getInt(attrVal.c_str()));
                    }
        	}
        	break;

            case otDatatype:
	    case otCondStatement:
		break;
            }
        }
    }

ModelObject *XmiParser::findParentInStack(ObjectType type)
    {
    ModelObject *obj = nullptr;
    // -1 gets to highest element on stack, which is self, another -1 to get lower than self.
    for(int i=mElementStack.size()-2; i >= 0; i--)
	{
	if(mElementStack[i] && mElementStack[i]->getObjectType() == type)
	    {
	    obj = mElementStack[i];
	    break;
	    }
	}
    return obj;
    }

// The stack order is normally
// 	operation
//		cond
//		    cond
//
// So find the first parent condition, and if there aren't any, search for
// the operation.
ModelCondStatements *XmiParser::findStatementsParentInStack()
    {
    ModelCondStatements *stmts = NULL;
    ModelObject *obj = findParentInStack(otCondStatement);
    if(obj)
	{
	stmts = static_cast<ModelCondStatements*>(obj);
	}
    else
	{
	obj = findParentInStack(otOperation);
	if(obj)
	    {
	    stmts = &static_cast<ModelOperation*>(obj)->getCondStatements();
	    }
	}
    return stmts;
    }

void XmiParser::onCloseElem(char const * const /*name*/, int /*len*/)
    {
    ModelObject *elItem = nullptr;
    if(mElementStack.size() > 0)	// XML version header is not a model object.
	elItem = mElementStack.back();
    if(elItem)
        {
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "> ");
#endif
	switch(elItem->getObjectType())
	    {
	    case otClass:
	    case otDatatype:
		{
		ModelType *newType = static_cast<ModelType*>(elItem);
		ModelType *existingType = mModel.findType(newType->getName().c_str());
		if(existingType)
		    {
		    if(newType->getObjectType() == otClass &&
			    existingType->getObjectType() == otDatatype)
			{
			// Upgrade the type from a datatype to a class.
			mModel.replaceType(existingType, static_cast<ModelClassifier*>(newType));
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "Replaced and upgraded to class %s\n", newType->getName().c_str());
#endif
			}
		    else if(newType->getObjectType() == otClass &&
			    existingType->getObjectType() == otClass)
			{
			// Classes from the XMI files can either be defined struct/classes
			// with data members or function declarations or definitions, or
			// they may only contain defined functions
			mFileTypeIndexMap[newType->getModelId()] = existingType->getModelId();
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "Change new %d to exist %d\n", newType->getModelId(), existingType->getModelId());
#endif
			// This type must have indices remapped.
			mPotentialRemapIndicesTypes.push_back(existingType);
			// If the new class is a definition, then update
			// the existing class's module and line number.
			if(static_cast<const ModelClassifier*>(newType)->isDefinition())
			    {
			    ModelModule const *module = static_cast<ModelClassifier*>
				(newType)->getModule();
//			    assert(module);
			    if(module)
				{
				static_cast<ModelClassifier*>(existingType)->setModule(
				    module);
				static_cast<ModelClassifier*>(existingType)->setLineNum(
				    static_cast<ModelClassifier*>(newType)->getLineNum());
				}
			    }
			mModel.takeAttributes(static_cast<ModelClassifier*>(newType),
				static_cast<ModelClassifier*>(existingType));
			delete newType;
			newType = nullptr;
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "Used existing class %s\n", existingType->getName().c_str());
#endif
			}
		    // If the new type is a datatype, use the old type whether it
		    // was a class or datatype.
		    else if(newType->getObjectType() == otDatatype)
			{
			mFileTypeIndexMap[newType->getModelId()] = existingType->getModelId();
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "Change new %d to exist %d\n", newType->getModelId(), existingType->getModelId());
#endif
			delete newType;
			newType = nullptr;
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "Used existing type %s\n", existingType->getName().c_str());
#endif
			}
		    }
		if(newType)
		    {
		    mModel.addType(newType);
	            mPotentialRemapIndicesTypes.push_back(newType);
		    }
		}
		break;

	    case otAttribute:
		{
		// Search back up until the enclosing element with a class is found.
		ModelObject *obj = findParentInStack(otClass);
		if(obj)
		    {
		    ModelAttribute *attr = static_cast<ModelAttribute*>(elItem);
		    ModelClassifier *cls = static_cast<ModelClassifier*>(obj);
		    cls->addAttribute(attr);
		    }
		}
		break;

	    case otOperation:
		{
		ModelObject *obj = findParentInStack(otClass);
		if(obj)
		    {
		    ModelOperation *oper = static_cast<ModelOperation*>(elItem);
		    ModelClassifier *cls = static_cast<ModelClassifier*>(obj);
		    cls->addOperation(oper);
		    }
		}
		break;

	    case otBodyVarDecl:
		{
		// Search back up until the enclosing element with an operation is found.
		ModelObject *obj = findParentInStack(otOperation);
		if(obj)
		    {
		    ModelBodyVarDecl *dec = static_cast<ModelBodyVarDecl*>(elItem);
		    ModelOperation *op = static_cast<ModelOperation*>(obj);
		    op->addBodyVarDeclarator(dec);
		    }
		}
		break;

	    case otOperParam:
		{
		// Search back up until the enclosing element with an operation is found.
		ModelObject *obj = findParentInStack(otOperation);
		if(obj)
		    {
		    ModelFuncParam *param = static_cast<ModelFuncParam*>(elItem);
		    ModelOperation *op = static_cast<ModelOperation*>(obj);
		    op->addMethodParameter(param);
		    }
		}
		break;

	    case otCondStatement:
	    case otOperCall:
		{
		ModelCondStatements *condStmts = findStatementsParentInStack();
		/// @todo - use make_unique when supported.
		ModelStatement *stmt = static_cast<ModelStatement*>(elItem);
		condStmts->addStatement(std::unique_ptr<ModelStatement>(stmt));
		}
		break;

	    case otAssociation:
		{
		ModelAssociation *assoc = static_cast<ModelAssociation*>(elItem);
		mModel.mAssociations.push_back(assoc);
		}
		break;

	    case otModule:
		{
		ModelModule *mod = static_cast<ModelModule*>(elItem);
		mModel.mModules.push_back(mod);
#if(DEBUG_LOAD)
		sCurrentModule = mod;
#endif
		}
		break;
	    }
        }
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "\n");
#endif
    if(mElementStack.size() > 0)	// XML version header is not a model object.
	mElementStack.pop_back();
    }

void XmiParser::updateDeclTypeIndices(ModelDeclarator &decl)
    {
    auto const &iter = mFileTypeIndexMap.find(decl.getDeclTypeModelId());
    if(iter != mFileTypeIndexMap.end())
	{
	decl.setDeclTypeModelId((*iter).second);
	}
    }

void XmiParser::updateStatementTypeIndices(ModelStatement *stmt)
    {
    if(stmt->getObjectType() == otCondStatement)
	{
	ModelCondStatements *condStmt = static_cast<ModelCondStatements *>(stmt);
	for(auto &childstmt : condStmt->getStatements())
	    {
	    updateStatementTypeIndices(childstmt.get());
	    }
	}
    else if(stmt->getObjectType() == otOperCall)
	{
	updateDeclTypeIndices(static_cast<ModelOperationCall *>(stmt)->getDecl());
	}
    }

void XmiParser::updateTypeIndices()
    {
#if(DEBUG_LOAD)
    if(sDumpFile)
	{
	fprintf(sLog.mFp, "Index Map\n");
	for(auto const &iter : mFileTypeIndexMap)
	    {
	    fprintf(sLog.mFp, " %d %d\n", iter.first, iter.second);
	    }
	fflush(sLog.mFp);
	}
#endif
    for(const auto &type : mPotentialRemapIndicesTypes)
	{
	if(type->getObjectType() == otClass)
	    {
	    ModelClassifier *classifier = ModelObject::getClass(type);
	    for(auto &attr : classifier->getAttributes())
		{
		updateDeclTypeIndices(*attr);
		}
	    for(auto &oper : classifier->getOperations())
		{
		// Resolve function parameters.
		for(auto &param : oper->getParams())
		    {
		    updateDeclTypeIndices(*param);
		    }
		// Resolve function call decls.
		ModelCondStatements &stmts = oper->getCondStatements();
		updateStatementTypeIndices(&stmts);

		// Resolve body variables.
		for(auto &vd : oper->getBodyVarDeclarators())
		    {
		    updateDeclTypeIndices(*vd);
		    }
		}
	    }
	}
    // Not real efficient to go through all, but there should not be all that many
    // compared to the number of types.
    for(auto &assoc : mModel.mAssociations)
        {
	    for(auto const &iterChild : mFileTypeIndexMap)
		{
		if(iterChild.first == assoc->getChildModelId())
		    {
		    assoc->setChildModelId(iterChild.second);
#if(DEBUG_LOAD)
		    if(sDumpFile)
			{
			fprintf(sLog.mFp, "Updated Child Model Id %d %d\n",
				assoc->getChildModelId(), iterChild.second);
			}
#endif
		    break;
		    }
		}
	    for(auto const &iterChild : mFileTypeIndexMap)
		{
		if(iterChild.first == assoc->getParentModelId())
		    {
		    assoc->setParentModelId(iterChild.second);
#if(DEBUG_LOAD)
		    if(sDumpFile)
			{
			fprintf(sLog.mFp, "Updated Parent Model Id %d %d\n",
				assoc->getChildModelId(), iterChild.second);
			}
#endif
		    break;
		    }
		}
// For some reason, this iterator approach doesn't work
/*
	auto const iterChild = mFileTypeIndexMap.find(assoc->getChildModelId());
	if(iterChild != mFileTypeIndexMap.end())
	    {
#if(DEBUG_LOAD)
    if(sDumpFile)
	fprintf(sLog.mFp, "Updated Child Model Id %d %d\n", assoc->getChildModelId(), (*iterChild).second);
#endif
	    assoc->setChildModelId((*iterChild).second);
	    }
	auto const iter = mFileTypeIndexMap.find(assoc->getParentModelId());
	if(iter != mFileTypeIndexMap.end())
	    {
#if(DEBUG_LOAD)
    if(sDumpFile)
	{
	fprintf(sLog.mFp, "Updated Parent Model Id %d %d\n", assoc->getChildModelId(), (*iterChild).second);
	if((*iterChild).second > mEndingModuleTypeIndex)
	    {
	    fflush(sLog.mFp);
	    assert(false);
	    }
	}
#endif
	    assoc->setParentModelId((*iter).second);
	    }
*/
        }
    mPotentialRemapIndicesTypes.clear();
    mFileTypeIndexMap.clear();
    // Prevent using this module again.
    for(auto &mod : mModel.mModules)
	{
	mod->setModelId(UNDEFINED_ID);
	}
    }

static bool loadXmiBuf(char const * const buf, ModelData &model, int &typeIndex)
    {
    XmiParser parser(model);
    parser.setStartingTypeIndex(typeIndex);
    bool parsed = parser.parse(buf);
    typeIndex = parser.getNextTypeIndex();
    return(parsed);
    }

bool loadXmiFile(FILE *fp, ModelData &graph, char const * const fn, int &typeIndex)
    {
    bool success = false;
    fseek(fp , 0 , SEEK_END);
    int size = ftell(fp);
    rewind(fp);

#if(DEBUG_LOAD)
    std::string srcFn = fn;
    sDumpFile = true;
    // sDumpFile = (srcFn.find("ModelObjects_h") != std::string::npos);
    dumpFilename(fn, typeIndex);
#endif
    // allocate memory to contain the whole file plus a null byte
    char *buf = new char[size+1];
    if(buf)
        {
	buf[size] = 0;
	size_t actualSize = fread(buf, size, 1, fp);
	if(ferror(fp) == 0)
	    success = loadXmiBuf(buf, graph, typeIndex);
        delete [] buf;
        }
#if(DEBUG_LOAD)
    dumpTypes(graph);
    dumpRelations(graph);
#endif
    return success;
    };

