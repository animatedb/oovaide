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
#include <limits.h>
#include "Debug.h"
#include "OovError.h"


#define DEBUG_CLASS 0
#define DEBUG_LOAD 0
#if(DEBUG_LOAD)
    static DebugFile sLog("DebugXmiParse.txt");
    static ModelModule const *sCurrentModule;
    static bool sDumpFile = false;
    // This can be used for debugging. FN format is something like "DiagramDrawer_h.xmi".
    static char const *sCurrentFilename;
    static void dumpFilename(char const * const fn, int typeIndex)
        {
        if(sDumpFile)
            {
            sCurrentFilename = fn;
            fprintf(sLog.mFp, "\n***** %s - %d *****\n\n", fn, typeIndex);
            }
        }
    static void dumpStatements(const ModelStatements &stmts, int level)
        {
        if(sDumpFile)
            {
            level++;
            std::string ws(level*2, ' ');
            for(auto const &stmt : stmts)
                {
                if(stmt.getStatementType() == ST_OpenNest)
                    {
                    fprintf(sLog.mFp, "  %s OpenStmt   %s\n", ws.c_str(),
                            stmt.getName());
                    }
                else if(stmt.getStatementType() == ST_Call)
                    {
                    fprintf(sLog.mFp, "  %s Call   %d:%s\n", ws.c_str(),
                            stmt.getClassDecl().getDeclTypeModelId(), stmt.getName().c_str());
                    fflush(sLog.mFp);
                    }
                else if(stmt.getStatementType() == ST_VarRef)
                    {
                    fprintf(sLog.mFp, "  %s VarRef   %d:%s\n", ws.c_str(),
                            stmt.getVarDecl().getDeclTypeModelId(), stmt.getName().c_str());
                    fflush(sLog.mFp);
                    }
                }
            }
        }
    static void dumpTypes(const ModelData &graph)
        {
        if(sDumpFile)
            {
            fprintf(sLog.mFp, "\n** Type Dump **\n");
            for(auto &mt : graph.mTypes)
                {
                const char *typeStr = (mt->getDataType()==DT_Class) ? "Class": "DataType";
                fprintf(sLog.mFp, " %s   %s %d\n", typeStr,
                        mt->getName().c_str(), mt->getModelId());
                fflush(sLog.mFp);
                const ModelClassifier*c = mt->getClass();
                if(c && c->getModule() == sCurrentModule)
                    {
                    for(const auto &attr : c->getAttributes())
                        {
                        fprintf(sLog.mFp, "   Attr   %s\n", attr->getName().c_str());
                        }
                    for(const auto &oper : c->getOperations())
                        {
                        fprintf(sLog.mFp, "   Oper   %s", oper->getName().c_str());
                        if(oper->getModule())
                            fprintf(sLog.mFp, " %d", oper->getLineNum());
                        fprintf(sLog.mFp, "\n");
                        dumpStatements(oper->getStatements(), 0);
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
                const ModelClassifier *child = assoc->getChild()->getClass();
                const ModelClassifier *parent = assoc->getParent()->getClass();
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

static void replaceAttrChars(OovString &str)
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
        str.replaceStrs(words[i].mSrch, words[i].mRep);
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
//      mModel.resolveModelIds();
        }
    return success;
    }

void XmiParser::onOpenElem(char const * const name, int len)
    {
    struct nameLookup
        {
        char const * const mName;
        XmiElementTypes mElType;
        };
    static nameLookup names[] =
    {
        { "Attr", ET_Attr, },
        { "Class", ET_Class, },
        { "DataType", ET_DataType, },
        { "Genrl", ET_Generalization, },
        // { "Enumeration", ET_Enumeration, },
        { "Module", ET_Module },
        { "Oper", ET_Function },
        { "Parms", ET_FuncParams },
        { "BodyVarDecl", ET_BodyVarDecl },
        { "Statements", ET_Statements },
    };
    XmiElement elem;
    OovString elName(name, len);
    for(size_t ni=0; ni<sizeof(names)/sizeof(names[0]); ni++)
        {
        if(elName.compare(names[ni].mName) == 0)
            {
            elem.mType = names[ni].mElType;
            break;
            }
        }
    switch(elem.mType)
        {
        case ET_None:
            break;

        case ET_Class:
            elem.mModelObject = new ModelClassifier("");
            break;

        case ET_DataType:
            elem.mModelObject = new ModelType("");
            break;

        case ET_Attr:
            elem.mModelObject = new ModelAttribute("", nullptr, Visibility::Public);
            break;

        case ET_Function:
            elem.mModelObject = new ModelOperation("", Visibility(), true, false);
            break;

        case ET_FuncParams:
            elem.mModelObject = findParentInStack(ET_Function, false);
            break;

        case ET_BodyVarDecl:
            elem.mModelObject = new ModelBodyVarDecl("", nullptr);
            break;

        case ET_Generalization:
            {
            Visibility vis;
            elem.mModelObject = new ModelAssociation(nullptr, nullptr, vis);
            }
            break;

        case ET_Module:
            elem.mModelObject = new ModelModule();
            break;

        case ET_Statements:
            elem.mModelObject = findParentInStack(ET_Function, false);
            break;
        }
#if(DEBUG_LOAD)
    if(sDumpFile)
        {
        fprintf(sLog.mFp, "< %s  ", elName.c_str());
        }
#endif
    mElementStack.push_back(elem);
    }

Visibility::VisType getAccess(char const * const accessStr)
    {
    return Visibility(accessStr).getVis();
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

void XmiParser::addFuncParams(OovStringRef const &attrName,
        OovStringRef const &attrVal, ModelOperation &oper)
    {
    if(strcmp(attrName.getStr(), "list") == 0)
        {
        OovStringVec parms = attrVal.split('#');
        for(auto const &parm : parms)
            {
            OovStringVec parmVals = parm.split('@');
            if(parmVals.size() == 4)
                {
                ModelFuncParam *param = oper.addMethodParameter(parmVals[0],
                        nullptr, false);
                param->setDeclTypeModelId(mStartingModuleTypeIndex + getInt(parmVals[1].c_str()));
                param->setConst(parmVals[2][0] == 't');
                param->setRefer(parmVals[3][0] == 't');
                }
            }
        }
    }

void XmiParser::addFuncStatements(OovStringRef const &attrName,
        OovStringRef const &attrVal, ModelOperation &oper)
    {
    if(strcmp(attrName.getStr(), "list") == 0)
        {
        OovStringVec statements = attrVal.split('#');
        for(auto const &stmt : statements)
            {
            OovStringVec stmtVals = stmt.split('@');
            switch(stmtVals[0][0])
                {
                case '{':
                    {
                    ModelStatement modStmt(&stmtVals[0][1], ST_OpenNest);
                    oper.getStatements().addStatement(modStmt);
                    }
                    break;

                case '}':
                    {
                    ModelStatement modStmt("", ST_CloseNest);
                    oper.getStatements().addStatement(modStmt);
                    }
                    break;

                case 'c':
                    {
                    ModelStatement modStmt(&stmtVals[0][2], ST_Call);
                    int typeId = 0;
                    // -1 is used for [else]
                    if(stmtVals.getStr(1).getInt(-1, INT_MAX, typeId))
                        {
                        if(typeId != -1)
                            typeId += mStartingModuleTypeIndex;
                        modStmt.getClassDecl().setDeclTypeModelId(typeId);
                        oper.getStatements().addStatement(modStmt);
                        }
                    }
                    break;

                case 'v':
                    {
                    ModelStatement modStmt(&stmtVals[0][2], ST_VarRef);
                    int classTypeId = 0;
                    if(stmtVals.getStr(1).getInt(0, INT_MAX, classTypeId))
                        {
                        modStmt.getClassDecl().setDeclTypeModelId(
                                mStartingModuleTypeIndex + classTypeId);
                        }
                    int varTypeId = 0;
                    if(stmtVals.getStr(2).getInt(0, INT_MAX, varTypeId))
                        {
                        modStmt.getVarDecl().setDeclTypeModelId(
                                mStartingModuleTypeIndex + varTypeId);
                        }
                    modStmt.setVarAccessWrite(isTrue(stmtVals.getStr(3)));
                    oper.getStatements().addStatement(modStmt);
                    }
                    break;
                }
            }
        }
    }

void XmiParser::onAttr(char const * const name, int &nameLen,
        char const * const val, int &valLen)
    {
    if(mElementStack.size() > 0)
        {
        OovString attrName(name, nameLen);
        OovString attrVal(val, valLen);
        XmiElement const &elItem = mElementStack.back();
        if(elItem.mModelObject)
            {
            replaceAttrChars(attrVal);
            if(strcmp(attrName.c_str(), "id") == 0)
                {
                int index = getInt(attrVal.c_str());
                if(elItem.mType == ET_Module || elItem.mType == ET_Generalization)
                    {
                    elItem.mModelObject->setModelId(index);
                    }
                else
                    {
                    elItem.mModelObject->setModelId(mStartingModuleTypeIndex + index);
                    if(mEndingModuleTypeIndex < mStartingModuleTypeIndex + index)
                        mEndingModuleTypeIndex = mStartingModuleTypeIndex + index;
                    }
                }
            if(strcmp(attrName.c_str(), "name") == 0)
                {
                elItem.mModelObject->setName(attrVal.c_str());
    #if(DEBUG_LOAD)
        if(sDumpFile)
            fprintf(sLog.mFp, "  %s ", attrVal.c_str());
    #endif
                }
            }
        switch(elItem.mType)
            {
            case ET_Class:
                {
                ModelClassifier *cl = static_cast<ModelClassifier*>(elItem.mModelObject);
                if(strcmp(attrName.c_str(), "module") == 0)
                    {
                    int modId = getInt(attrVal.c_str());
                    const ModelModule *mod = mModel.findModuleById(modId);
                    if(mod)
                        cl->setModule(mod);
                    else
                        {
                        DebugAssert(__FILE__, __LINE__);
                        }
                    }
                else if(strcmp(attrName.c_str(), "line") == 0)
                    {
                    cl->setLineNum(getInt(attrVal.c_str()));
                    }
                }
                break;

            case ET_Attr:
                {
                ModelAttribute *attr = static_cast<ModelAttribute*>(elItem.mModelObject);
                if(strcmp(attrName.c_str(), "access") == 0)
                    attr->setAccess(getAccess(attrVal.c_str()));
                else
                    setDeclAttr(attrName, attrVal, *attr);
                }
                break;

            case ET_FuncParams:
                {
                ModelOperation *oper = static_cast<ModelOperation*>(elItem.mModelObject);
                if(oper)
                    {
                    addFuncParams(attrName.c_str(), attrVal.c_str(), *oper);
                    }
                }
                break;

            case ET_Statements:
                {
                ModelOperation *oper = static_cast<ModelOperation*>(elItem.mModelObject);
                if(oper)
                    {
                    addFuncStatements(attrName.c_str(), attrVal.c_str(), *oper);
                    }
                }
                break;


            case ET_BodyVarDecl:
                {
                ModelBodyVarDecl *vd = static_cast<ModelBodyVarDecl*>(elItem.mModelObject);
                setDeclAttr(attrName, attrVal, *vd);
                }
                break;

            case ET_Generalization:
                {
                ModelAssociation *assoc = static_cast<ModelAssociation*>(elItem.mModelObject);
                if(strcmp(attrName.c_str(), "parent") == 0)
                    assoc->setParentModelId(mStartingModuleTypeIndex + getInt(attrVal.c_str()));
                else if(strcmp(attrName.c_str(), "child") == 0)
                    assoc->setChildModelId(mStartingModuleTypeIndex + getInt(attrVal.c_str()));
                else if(strcmp(attrName.c_str(), "access") == 0)
                    assoc->setAccess(getAccess(attrVal.c_str()));
                }
                break;

            case ET_Module:
                {
                ModelModule *mod = static_cast<ModelModule*>(elItem.mModelObject);
                if(strcmp(attrName.c_str(), "module") == 0)
                    mod->setModulePath(attrVal);
                else if(strcmp(attrName.c_str(), "codeLines") == 0)
                    mod->mLineStats.mNumCodeLines = getInt(attrVal.c_str());
                else if(strcmp(attrName.c_str(), "commentLines") == 0)
                    mod->mLineStats.mNumCommentLines = getInt(attrVal.c_str());
                else if(strcmp(attrName.c_str(), "moduleLines") == 0)
                    mod->mLineStats.mNumModuleLines = getInt(attrVal.c_str());
                }
                break;

            case ET_Function:
                {
                ModelOperation *oper = static_cast<ModelOperation*>(elItem.mModelObject);
                if(strcmp(attrName.c_str(), "access") == 0)
                    {
                    oper->setAccess(getAccess(attrVal.c_str()));
                    }
                else if(strcmp(attrName.c_str(), "sym") == 0)
                    {
                    oper->setOverloadKeyFromKey(attrVal);
                    }
                else if(strcmp(attrName.c_str(), "const") == 0)
                    {
                    oper->setConst(isTrue(attrVal));
                    }
                else if(strcmp(attrName.c_str(), "virt") == 0)
                    {
                    oper->setVirtual(isTrue(attrVal));
                    }
                else if(strcmp(attrName.c_str(), "line") == 0)
                    {
                    if(mModel.mModules.size() > 0)
                        {
                        oper->setModule(
                                mModel.mModules[mModel.mModules.size()-1].get());
                        }
                    oper->setLineNum(getInt(attrVal.c_str()));
                    }
                else if(strcmp(attrName.c_str(), "ret") == 0)
                    {
                    ModelTypeRef &retType = oper->getReturnType();
                    retType.setDeclTypeModelId(mStartingModuleTypeIndex + getInt(attrVal.c_str()));
                    }
                else if(strcmp(attrName.c_str(), "retconst") == 0)
                    {
                    ModelTypeRef retType = oper->getReturnType();
                    retType.setConst(isTrue(attrVal));
                    }
                else if(strcmp(attrName.c_str(), "retref") == 0)
                    {
                    ModelTypeRef retType = oper->getReturnType();
                    retType.setRefer(isTrue(attrVal));
                    }
                }
                break;

            case ET_DataType:
            case ET_None:
                break;
            }
        }
    }

ModelObject *XmiParser::findParentInStack(XmiElementTypes type, bool afterAddingSelf)
    {
    ModelObject *obj = nullptr;
    // -1 gets to highest element on stack, which is self, another -1 to get lower than self.
    for(int i=mElementStack.size()-1-afterAddingSelf; i >= 0; i--)
        {
        if(mElementStack[i].mModelObject && mElementStack[i].mType == type)
            {
            obj = mElementStack[i].mModelObject;
            break;
            }
        }
    return obj;
    }

void XmiParser::onCloseElem(char const * const /*name*/, int /*len*/)
    {
    if(mElementStack.size() > 0)
        {
        XmiElement const &elItem = mElementStack.back();
    #if(DEBUG_LOAD)
        if(sDumpFile)
            fprintf(sLog.mFp, "> ");
    #endif
        switch(elItem.mType)
            {
            case ET_Class:
            case ET_DataType:
                {
                ModelType *newType = static_cast<ModelType*>(elItem.mModelObject);
                ModelType *existingType = mModel.findType(newType->getName().c_str());
                if(existingType)
                    {
                    if(newType->getDataType() == DT_Class &&
                            existingType->getDataType() == DT_DataType)
                        {
                        // Upgrade the type from a datatype to a class.
                        mModel.replaceType(existingType, static_cast<ModelClassifier*>(newType));
#if(DEBUG_LOAD)
    if(sDumpFile)
        fprintf(sLog.mFp, "Replaced and upgraded to class %s\n", newType->getName().c_str());
#endif
                        }
                    else if(newType->getDataType() == DT_Class &&
                            existingType->getDataType() == DT_Class)
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
                            if(module)
                                {
                                static_cast<ModelClassifier*>(existingType)->setModule(
                                    module);
                                static_cast<ModelClassifier*>(existingType)->setLineNum(
                                    static_cast<ModelClassifier*>(newType)->getLineNum());
                                }
                            else
                                {
//                              DebugAssert(__FILE__, __LINE__);
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
                    else if(newType->getDataType() == DT_DataType)
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
                    /// @todo - use make_unique when supported.
                    mModel.addType(std::unique_ptr<ModelType>(newType));
                    mPotentialRemapIndicesTypes.push_back(newType);
                    }
                }
                break;

            case ET_Attr:
                {
                // Search back up until the enclosing element with a class is found.
                ModelObject *obj = findParentInStack(ET_Class);
                if(obj)
                    {
                    ModelAttribute *attr = static_cast<ModelAttribute*>(elItem.mModelObject);
                    ModelClassifier *cls = static_cast<ModelClassifier*>(obj);
                    /// @todo - use make_unique when supported.
                    cls->addAttribute(std::unique_ptr<ModelAttribute>(attr));
                    }
                }
                break;

            case ET_Function:
                {
                ModelObject *obj = findParentInStack(ET_Class);
                if(obj)
                    {
                    ModelOperation *oper = static_cast<ModelOperation*>(elItem.mModelObject);
                    ModelClassifier *cls = static_cast<ModelClassifier*>(obj);
                    /// @todo - use make_unique when supported.
                    cls->addOperation(std::unique_ptr<ModelOperation>(oper));
                    }
                }
                break;

            case ET_BodyVarDecl:
                {
                // Search back up until the enclosing element with an operation is found.
                ModelObject *obj = findParentInStack(ET_Function);
                if(obj)
                    {
                    ModelBodyVarDecl *dec = static_cast<ModelBodyVarDecl*>(elItem.mModelObject);
                    ModelOperation *op = static_cast<ModelOperation*>(obj);
                    /// @todo - use make_unique when supported.
                    op->addBodyVarDeclarator(std::unique_ptr<ModelBodyVarDecl>(dec));
                    }
                }
                break;

            case ET_Generalization:
                {
                ModelAssociation *assoc = static_cast<ModelAssociation*>(elItem.mModelObject);
                /// @todo - use make_unique when supported.
                mModel.mAssociations.push_back(std::unique_ptr<ModelAssociation>(assoc));
                }
                break;

            case ET_Module:
                {
                ModelModule *mod = static_cast<ModelModule*>(elItem.mModelObject);
                /// @todo - use make_unique when supported.
                mModel.mModules.push_back(std::unique_ptr<ModelModule>(mod));
#if(DEBUG_LOAD)
                sCurrentModule = mod;
#endif
                }
                break;

            case ET_None:
            case ET_Statements:
            case ET_FuncParams:
                break;
            }
        }
#if(DEBUG_LOAD)
    if(sDumpFile)
        fprintf(sLog.mFp, "\n");
#endif
    if(mElementStack.size() > 0)
        mElementStack.pop_back();
    }

void XmiParser::updateDeclTypeIndices(ModelTypeRef &decl)
    {
    auto const &iter = mFileTypeIndexMap.find(decl.getDeclTypeModelId());
    if(iter != mFileTypeIndexMap.end())
        {
        decl.setDeclTypeModelId((*iter).second);
        }
    }

void XmiParser::updateStatementTypeIndices(ModelStatements &stmts)
    {
    for(auto &stmt : stmts)
        {
        if(stmt.getStatementType() == ST_Call ||
                stmt.getStatementType() == ST_VarRef)
            {
            updateDeclTypeIndices(stmt.getClassDecl());
            if(stmt.getStatementType() == ST_VarRef)
                {
                updateDeclTypeIndices(stmt.getVarDecl());
                }
            }
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
        if(type->getDataType() == DT_Class)
            {
            ModelClassifier *classifier = type->getClass();
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
                ModelStatements &stmts = oper->getStatements();
                updateStatementTypeIndices(stmts);

                // Resolve body variables.
                for(auto &vd : oper->getBodyVarDeclarators())
                    {
                    updateDeclTypeIndices(*vd);
                    }
                updateDeclTypeIndices(oper->getReturnType());
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
            DebugAssert(__FILE__, __LINE__);
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

bool loadXmiFile(File const &file, ModelData &graph, OovStringRef const fn, int &typeIndex)
    {
    int size = 0;
    bool success = file.getFileSize(size);
    if(success)
        {
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
            success = file.read(buf, size);
            if(success)
                success = loadXmiBuf(buf, graph, typeIndex);
            delete [] buf;
            }
#if(DEBUG_LOAD)
        dumpTypes(graph);
        dumpRelations(graph);
#endif
#if(DEBUG_CLASS)
        ModelType *type = graph.findType("Gui::");
        if(type)
            {
            printf("XMI %s\n", fn.getStr());
            ModelClassifier *classifier = ModelType::getClass(type);
            if(classifier)
                {
                for(auto const &oper : classifier->getOperations())
                    {
    //                if(oper->getName().find("appendPage") != std::string::npos)
                        {
    //                    printf("A");
                        }
                    printf("%s\n", oper->getName().getStr());
                    }
                fflush(stdout);
                }
            }
#endif
        }
    if(!success)
        {
        OovString err = "Unable to read file: ";
        err += fn;
        OovError::report(ET_Error, err);
        }
    return success;
    };

