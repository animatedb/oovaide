/*
 * ModelObject.cpp
 *
 *  Created on: Jun 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ModelObjects.h"
#include "OovString.h"
#include "Debug.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <algorithm>
#include <map>

#define DEBUG_TYPES 0


OovStringRef const Visibility::asUmlStr() const
    {
    char const *str = "";
    switch(vis)
        {
        case Visibility::Public:        str = "+";      break;
        case Visibility::Protected:     str = "#";      break;
        case Visibility::Private:       str = "-";      break;
        default:        break;
        }
    return str;
    }

Visibility::Visibility(OovStringRef const umlStr)
    {
    switch(umlStr.getStr()[0])
        {
        default:        // fall through
        case '+':       vis = Visibility::Public;       break;
        case '#':       vis = Visibility::Protected;    break;
        case '-':       vis = Visibility::Private;      break;
        }
    }

const ModelClassifier *ModelType::getClass() const
    {
    return getClass(this);
    }

ModelClassifier *ModelType::getClass()
    {
    return getClass(this);
    }

class ModelClassifier *ModelType::getClass(ModelType *modelType)
    {
    ModelClassifier *cl = nullptr;
    if(modelType)
        {
        if(modelType->getDataType() == DT_Class)
            cl = static_cast<ModelClassifier *>(modelType);
        }
    return cl;
    }

class ModelClassifier const *ModelType::getClass(ModelType const *modelType)
    {
    ModelClassifier const *cl = nullptr;
    if(modelType)
        {
        if(modelType->getDataType() == DT_Class)
            cl = static_cast<ModelClassifier const *>(modelType);
        }
    return cl;
    }

bool ModelType::isTemplateType() const
    {
    return(getName().find('<') != std::string::npos);
    }

bool ModelTypeRef::match(ModelTypeRef const &typeRef) const
    {
    return(getDeclType() == typeRef.getDeclType() &&
            isConst() == typeRef.isConst() && isRefer() == typeRef.isRefer());
    }

const class ModelClassifier *ModelDeclarator::getDeclClassType() const
    {
    return getDeclType() ? getDeclType()->getClass() : nullptr;
    }


/// @todo - code copied from ParseBase.cpp
static size_t getRightSidePosFromMemberRefExpr(std::string &expr, bool afterSep)
    {
    size_t pos = expr.rfind('.');
    if(pos == std::string::npos)
        {
        pos = expr.rfind(':');          // Find end of ::
        }
    if(pos == std::string::npos)
        {
        pos = expr.rfind('>');          // Find end of ->
        }
    if(pos != std::string::npos)
        {
        size_t splitLen = (expr[pos] == '.') ? 1 : 2;
        pos++;
        if(!afterSep)
            {
            pos = static_cast<size_t>(pos - splitLen);
            }
        }
    return pos;
    }

OovString ModelStatement::getFuncName() const
    {
    OovString opName = getName();

    size_t pos = getRightSidePosFromMemberRefExpr(opName, true);
    if(pos != std::string::npos)
        opName.erase(0, pos);
    return opName;
    }

OovString ModelStatement::getAttrName() const
    {
    OovString attrName = getName();
    if(mStatementType == ST_Call)
        {
        size_t pos = getRightSidePosFromMemberRefExpr(attrName, false);
        if(pos != std::string::npos)
            attrName.erase(pos);
        else
            attrName.clear();
        }
    return attrName;
    }

bool ModelStatement::hasBaseClassMemberRef() const
    {
    OovString attrName = getName();
    size_t pos = getRightSidePosFromMemberRefExpr(attrName, false);
    return(attrName[pos] == '+');
    }

bool ModelStatements::checkAttrUsed(OovStringRef attrName) const
    {
    bool used = false;
    for(auto const &stmt : *this)
        {
        eModelStatementTypes stateType = stmt.getStatementType();
        if(stateType == ST_VarRef)
            {
            if(stmt.getAttrName() == attrName.getStr())
                {
                used = true;
                break;
                }
            }
        else if(stateType == ST_Call)
            {
            if(stmt.getAttrName() == attrName.getStr())
                {
                used = true;
                break;
                }
            }
        }
    return used;
    }


ModelFuncParam *ModelOperation::addMethodParameter(const std::string &name, const ModelType *type,
    bool isConst)
    {
    ModelFuncParam *param = new ModelFuncParam(name, type);
    param->setConst(isConst);
    /// @todo - use make_unique when supported.
    addMethodParameter(std::unique_ptr<ModelFuncParam>(param));
    return param;
    }

ModelBodyVarDecl *ModelOperation::addBodyVarDeclarator(const std::string &name, const ModelType *type,
    bool isConst, bool isRef)
    {
    ModelBodyVarDecl *decl = new ModelBodyVarDecl(name, type);
    decl->setConst(isConst);
    decl->setRefer(isRef);
    /// @todo - use make_unique when supported.
    mBodyVarDeclarators.push_back(std::unique_ptr<ModelBodyVarDecl>(decl));
    return decl;
    }

bool ModelOperation::isDefinition() const
    {
    return(mStatements.size() > 0);
    }

bool ModelOperation::paramsMatch(std::vector<std::unique_ptr<ModelFuncParam>> const &params) const
    {
    bool match = false;
    if(mParameters.size() == params.size())
        {
        match = true;
        for(size_t i=0; i<mParameters.size(); i++)
            {
            match = mParameters[i]->match(*params[i]);
            if(!match)
                {
                break;
                }
            }
        }
    return match;
    }

void ModelClassifier::clearAttributes()
    {
    mAttributes.clear();
    }

void ModelClassifier::clearOperations()
    {
    mOperations.clear();
    }

bool ModelClassifier::isDefinition() const
    {
    return(getAttributes().size() + getOperations().size() > 0);
    }

ModelAttribute *ModelClassifier::addAttribute(const std::string &name,
        ModelType const *attrType, Visibility scope)
    {
    ModelAttribute *attr = new ModelAttribute(name, attrType, scope);
    /// @todo - use make_unique when supported.
    addAttribute(std::unique_ptr<ModelAttribute>(attr));
    return attr;
    }

ModelOperation *ModelClassifier::addOperation(const std::string &name,
        Visibility access, bool isConst)
    {
    ModelOperation *oper = new ModelOperation(name, access, isConst);
    addOperation(std::unique_ptr<ModelOperation>(oper));
    return oper;
    }

void ModelClassifier::replaceOperation(ModelOperation const * const operToReplace,
            std::unique_ptr<ModelOperation> &&newOper)
        {
        size_t index = findExactMatchingOperationIndex(*operToReplace);
        if(index != NoIndex)
            {
//          mOperations.erase(mOperations.begin() + index);
//          mOperations.push_back(std::move(newOper));
            mOperations[index] = std::move(newOper);
            }
        }

void ModelClassifier::eraseAttribute(const ModelAttribute *attr)
    {
    for(size_t i=0; i<mAttributes.size(); ++i)
        {
        if(mAttributes[i].get() == attr)
            {
            mAttributes.erase(mAttributes.begin() +
                static_cast<int>(i));
            break;
            }
        }
    }

void ModelClassifier::eraseOperation(const ModelOperation *oper)
    {
    for(size_t i=0; i<mOperations.size(); ++i)
        {
        if(mOperations[i].get() == oper)
            {
            mOperations.erase(mOperations.begin() +
                static_cast<int>(i));
            break;
            }
        }
    }

void ModelClassifier::removeOperation(ModelOperation *oper)
    {
    size_t index = NoIndex;
    for(size_t i=0; i<mOperations.size(); ++i)
        {
        if(oper == mOperations[i].get())
            index = i;
        }
    if(index != NoIndex)
        {
        delete oper;
        mOperations.erase(mOperations.begin() +
            static_cast<int>(index));
        }
    }

size_t ModelClassifier::getAttributeIndex(const std::string &name) const
    {
    size_t index = NoIndex;
    for(size_t i=0; i<mAttributes.size(); ++i)
        {
        if(mAttributes[i]->getName().compare(name) == 0)
            {
            index = i;
            break;
            }
        }
    return index;
    }

const ModelAttribute *ModelClassifier::getAttribute(const std::string &name) const
    {
    ModelAttribute *attr = nullptr;
    size_t index = getAttributeIndex(name);
    if(index != NoIndex)
        {
        attr = mAttributes[index].get();
        }
    return attr;
    }

size_t ModelClassifier::getOperationIndex(const std::string &name, bool isConst) const
    {
    size_t index = NoIndex;
    for(size_t i=0; i<mOperations.size(); i++)
        {
        if(name.compare(mOperations[i]->getName()) == 0 &&
                mOperations[i]->isConst() == isConst)
            {
            index = i;
            break;
            }
        }
    return index;
    }

size_t ModelClassifier::findExactMatchingOperationIndex(const ModelOperation &matchOp) const
    {
    size_t index = NoIndex;
    for(size_t i=0; i<mOperations.size(); i++)
        {
        ModelOperation const &arrayOp = *mOperations[i];
        if(matchOp.getName().compare(arrayOp.getName()) == 0 &&
                matchOp.isConst() == arrayOp.isConst())
            {
            if(matchOp.paramsMatch(arrayOp.getParams()))
                {
                index = i;
                break;
                }
            }
        }
    return index;
    }

const ModelOperation *ModelClassifier::findExactMatchingOperation(const ModelOperation &op) const
    {
    ModelOperation *oper = nullptr;
    size_t index = findExactMatchingOperationIndex(op);
    if(index != NoIndex)
        {
        oper = mOperations[index].get();
        }
    return oper;
    }

const ModelOperation *ModelClassifier::getOperation(const std::string &name, bool isConst) const
    {
    ModelOperation *oper = nullptr;
    size_t index = getOperationIndex(name, isConst);
    if(index != NoIndex)
        {
        oper = mOperations[index].get();
        }
    return oper;
    }

const ModelOperation *ModelClassifier::getOperationAnyConst(const std::string &name, bool isConst) const
    {
    const ModelOperation *oper = getOperation(name, isConst);
    if(!oper)
        oper = getOperation(name, !isConst);
    return oper;
    }

void ModelData::clear()
    {
    mModules.clear();
    mAssociations.clear();
    mTypes.clear();
    }

// The resolveModelIds() function takes a long time for large projects.
// This reduces the time for resolving types to about 1/3 of the time of
// iterating through the type map to find the types.
class TypeIdMap:public std::map<int, ModelType *>
    {
    public:
        TypeIdMap(std::vector<std::unique_ptr<ModelType>> const &types)
            {
            for(auto &type : types)
                {
                insert(std::pair<int, ModelType*>(type->getModelId(), type.get()));
                }
            }
        ModelType *getTypeByModelId(int id) const
            {
            ModelType *type = nullptr;
            auto it = find(id);
            if(it != end())
                type = (*it).second;
            return type;
            }
    };

void ModelData::resolveDecl(TypeIdMap const &typeMap, ModelTypeRef &decl)
    {
    if(decl.getDeclTypeModelId() != UNDEFINED_ID)
        {
        decl.setDeclType(typeMap.getTypeByModelId(decl.getDeclTypeModelId()));
        decl.setDeclTypeModelId(UNDEFINED_ID);
        }
    }

void ModelData::dumpTypes()
    {
#if(DEBUG_TYPES)
    FILE *fp=fopen("DebugTypes.txt", "a");
    if(fp)
        {
        fprintf(fp, "-----\n");
        for(auto &type : mTypes)
            {
            fprintf(fp, "%s %d\n", type->getName().getStr(), type->getModelId());
            }
        fclose(fp);
        }
#endif
    }

void ModelData::resolveStatements(class TypeIdMap const &typeMap, ModelStatements &stmts)
    {
    for(auto &stmt : stmts)
        {
        if(stmt.getStatementType() == ST_Call ||
                stmt.getStatementType() == ST_VarRef)
            {
            resolveDecl(typeMap, stmt.getClassDecl());
            if(stmt.getStatementType() == ST_VarRef)
                {
                resolveDecl(typeMap, stmt.getVarDecl());
                }
            }
        }
    }

void ModelData::resolveModelIds()
    {
    dumpTypes();
    TypeIdMap typeMap(mTypes);
    // Resolve class member attributes and operations.
    for(const auto &type : mTypes)
        {
        if(type->getDataType() == DT_Class)
            {
            ModelClassifier *classifier = type->getClass();
            for(auto &attr : classifier->getAttributes())
                {
                resolveDecl(typeMap, *attr);
                }
            for(auto &oper : classifier->getOperations())
                {
                // Resolve function parameters.
                for(auto &param : oper->getParams())
                    {
                    resolveDecl(typeMap, *param);
                    }
                // Resolve function call decls.
                ModelStatements &stmts = oper->getStatements();
                resolveStatements(typeMap, stmts);

                // Resolve body variables.
                for(auto &vd : oper->getBodyVarDeclarators())
                    {
                    resolveDecl(typeMap, *vd);
                    }

#if(OPER_RET_TYPE)
                resolveDecl(typeMap, oper->getReturnType());
#endif
                }
            }
        }
    // Resolve relations.
    for(auto &assoc : mAssociations)
        {
        if(assoc->getChildModelId() != UNDEFINED_ID)
            {
            assoc->setChildClass(typeMap.getTypeByModelId(assoc->getChildModelId())->getClass());
            assoc->setParentClass(typeMap.getTypeByModelId(assoc->getParentModelId())->getClass());
/*
            assoc->setChildModelId(UNDEFINED_ID);
            assoc->setParentModelId(UNDEFINED_ID);
*/
            }
        }
/*
    for(auto &type : mTypes)
        {
        type->setModelId(UNDEFINED_ID);
        }
    // Resolve modules.
    for(auto &mod : mModules)
        {
        mod->setModelId(UNDEFINED_ID);
        }
*/
    }

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

bool ModelData::isTypeReferencedByDefinedObjects(ModelType const &checkType) const
    {
    bool referenced = false;
    for(const auto &type : mTypes)
        {
        if(type->getDataType() == DT_Class)
            {
            ModelClassifier *classifier = type->getClass();
            // Only defined classes in the parsed translation unit have a module.
            if(classifier->getModule())
                {
                for(auto &attr : classifier->getAttributes())
                    {
                    if(attr->getDeclType() == &checkType)
                        {
                        referenced = true;
                        break;
                        }
                    }
                }
            if(!referenced)
                {
                for(auto &oper : classifier->getOperations())
                    {
                    // Only defined operations in the translation unit have a module.
                    if(oper->getModule())
                        {
                        // Check function parameters.
                        for(auto &param : oper->getParams())
                            {
                            if(param->getDeclType() == &checkType)
                                {
                                referenced = true;
                                break;
                                }
                            }
                        if(!referenced)
                            {
                            // Check function call decls.
                            ModelStatements &stmts = oper->getStatements();
                            referenced = isTypeReferencedByStatements(stmts, checkType);
                            }
                        if(!referenced)
                            {
                            // Check body variables.
                            for(auto &vd : oper->getBodyVarDeclarators())
                                {
                                if(vd->getDeclType() == &checkType)
                                    {
                                    referenced = true;
                                    break;
                                    }
                                }
                            }
#if(OPER_RET_TYPE)
                        if(!referenced)
                            {
                            if(oper->getReturnType().getDeclType() == &checkType)
                                {
                                referenced = true;
                                break;
                                }
                            }
#endif
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

void ModelData::takeAttributes(ModelClassifier *sourceType, ModelClassifier *destType)
    {
    for(auto &attr : sourceType->getAttributes())
        {
        ModelAttribute *attrPtr = attr.get();
        destType->addAttribute(std::move(attr));
        sourceType->eraseAttribute(attrPtr);
        }
    for(auto &oper : sourceType->getOperations())
        {
        ModelOperation const *destOper = destType->findExactMatchingOperation(*oper.get());
        if(destOper && destOper->isDefinition())
            {
            // dest operation already has a good definition.
            }
        else
            {
            ModelOperation *operPtr = oper.get();
            if(destOper)
                destType->replaceOperation(destOper, std::move(oper));
            else
                destType->addOperation(std::move(oper));
            sourceType->eraseOperation(operPtr);
            }
        }
    // Function parameters have references to types, but they do not need to
    // be handled since those types haven't been resolved yet.
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

void ModelData::replaceType(ModelType *existingType, ModelClassifier *newType)
    {
    // Don't need to update function parameter types at this time, because the
    // existing type is a datatype, and datatypes are not referred to at this time.

    // update member attributes.
    for(const auto &type : mTypes)
        {
        if(type->getDataType() == DT_Class)
            {
            ModelClassifier *classifier = type->getClass();
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
#if(OPER_RET_TYPE)
                if(oper->getReturnType().getDeclType() == existingType)
                    {
                    oper->getReturnType().setDeclType(newType);
                    }
#endif
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

const ModelType *ModelData::getTypeRef(OovStringRef const typeName) const
    {
    OovString baseTypeName = getBaseType(typeName);
    return findType(baseTypeName);
    }

ModelType *ModelData::createOrGetTypeRef(OovStringRef const typeName, eModelDataTypes dtype)
    {
    std::string baseTypeName = getBaseType(typeName);
    ModelType *type = findType(baseTypeName);
    if(!type)
        {
        type = static_cast<ModelType*>(createDataType(dtype, baseTypeName));
        }
    return type;
    }

ModelType *ModelData::createTypeRef(OovStringRef const typeName, eModelDataTypes dtype)
    {
    std::string baseTypeName = getBaseType(typeName);
    return static_cast<ModelType*>(createDataType(dtype, baseTypeName));
    }

ModelObject *ModelData::createDataType(eModelDataTypes type, const std::string &id)
    {
    ModelObject *obj = nullptr;
    switch(type)
        {
        case DT_DataType:
            {
            ModelType *dataType = new ModelType(id);
            /// @todo - use make_unique when supported.
            addType(std::unique_ptr<ModelType>(dataType));
            obj = dataType;
            }
            break;

        case DT_Class:
            {
            ModelClassifier *classifier = new ModelClassifier(id);
            /// @todo - use make_unique when supported.
            addType(std::unique_ptr<ModelType>(classifier));
            obj = classifier;
            break;
            }

        default:
            break;
        }
    return obj;
    }

#define BASESPEED 1
#define BINARYSPEED 1
#if(BASESPEED)
static inline bool skipStr(char const * * const str, OovStringRef const compareStr)
    {
    char const *needle = compareStr;
    char const *haystack = *str;
    while(*needle)
        {
        if(*needle != *haystack)
            break;
        needle++;
        haystack++;
        }
    bool match = (*needle == '\0');
    if(match)
        *str += needle-compareStr;
    return match;
    }
#endif

static bool isIdentC(char c)
    {
    return(isalnum(c) || c == '_');
    }

static char const *skipWhite(OovStringRef const s)
    {
    char const *p = s;
    while(p)
        {
        if(!isspace(*p))
            break;
        p++;
        }
    return p;
    }

static char getTokenType(char c)
    {
    if(isIdentC(c))
        c = 'i';
    return c;
    }

std::string ModelData::getBaseType(OovStringRef const fullStr) const
{
#if(BASESPEED)
    std::string str;
    char const *p = fullStr;
    // Skip leading spaces.
    while(*p == ' ')
        p++;
    while(*p)
        {
        switch(*p)
            {
            case 'c':
                if(!skipStr(&p, "const ") && !skipStr(&p, "class "))
                    str += *p++;
                break;
            case 'm':
                if(!skipStr(&p, "mutable "))
                    str += *p++;
                break;
            case 's':
                if(!skipStr(&p, "struct "))
                    str += *p++;
                break;
            case '*':   p++;
                break;
            case '&':   p++;
                break;
            case ' ':
                {
                char prevTokenType = getTokenType(str[str.length()-1]);
                char nextTokenType = getTokenType(*skipWhite(p));
                if(prevTokenType != nextTokenType)
                    p++;
                else
                    str += *p++;
                }
                break;
            default:    str += *p++;
                break;
            }
        }
    size_t len = str.length()-1;
    if(str[len] == ' ')
        str.resize(len);
#else
    std::string str = fullStr;
    strReplace("const ", " ", str);
    strReplace("class ", " ", str);
    strReplace("mutable ", " ", str);
    strReplace("*", "", str);
    strReplace("&", "", str);
    strRemoveSpaceAround("<", str);
    strRemoveSpaceAround(">", str);     // There are two spaces near >
    strRemoveSpaceAround(">", str);
    strRemoveSpaceAround(":", str);
    if(str[0] == ' ')
        str.erase(0, 1);
    int lastPos = str.length()-1;
    if(str[lastPos] == ' ')
        str.erase(lastPos, 1);
#endif
    return str;
}

static inline bool compareStrs(OovStringRef const tstr1, OovStringRef const tstr2)
    {
    return (strcmp(tstr1, tstr2) < 0);
    }

void ModelData::addType(std::unique_ptr<ModelType> &&type)
    {
#if(BINARYSPEED)
    std::string baseTypeName = getBaseType(type->getName());
    type->setName(baseTypeName);
    auto it = std::upper_bound(mTypes.begin(), mTypes.end(), baseTypeName,
        [](OovStringRef const mod1Name, std::unique_ptr<ModelType> &mod2) -> bool
        { return(compareStrs(mod1Name, mod2->getName())); } );
    mTypes.insert(it, std::move(type));
#else
    mTypes.push_back(type);
#endif
    }

const ModelType *ModelData::findType(OovStringRef const name) const
    {
    const ModelType *type = nullptr;
#if(BINARYSPEED)
    // This comparison must produce the same sort order as addType.
    std::string baseTypeName = getBaseType(name);
    auto iter = std::lower_bound(mTypes.begin(), mTypes.end(), baseTypeName,
        [](std::unique_ptr<ModelType> const &mod1, OovStringRef const mod2Name) -> bool
        { return(compareStrs(mod1->getName(), mod2Name)); } );
    if(iter != mTypes.end())
        {
        if(baseTypeName.compare((*iter)->getName()) == 0)
            type = (*iter).get();
        }
#else
    std::string baseTypeName = getBaseType(name);
    for(auto &iterType : mTypes)
        {
        if(iterType->getName().compare(baseTypeName) == 0)
            {
            type = iterType;
            break;
            }
        }
#endif
    return type;
    }

ModelType *ModelData::findType(OovStringRef const name)
    {
    return const_cast<ModelType*>(static_cast<const ModelData*>(this)->findType(name));
    }

ModelModule const * ModelData::findModuleById(int id)
    {
    ModelModule *mod = nullptr;
    // Searching backwards is an effective optimization because onAttr
    // has calls to this function, and many of the recently added ID's
    // will be at the end.
    if(mModules.size() > 0)
        {
        for(int i=static_cast<int>(mModules.size())-1; i>=0; i--)
            {
            size_t index = static_cast<size_t>(i);
            if(mModules[index]->getModelId() == id)
                {
                mod = mModules[index].get();
                break;
                }
            }
        }
    if(!mod)
        {
        DebugAssert(__FILE__, __LINE__);
        }
    return mod;
    }

static size_t findIdentC(const std::string &str, size_t pos)
    {
    size_t ret = std::string::npos;
    for(size_t i=pos; i<str.length(); i++)
        {
        if(isIdentC(str[i]))
            {
            ret = i;
            break;
            }
        }
    return ret;
    }

static size_t findNonIdentC(const std::string &str, size_t pos)
    {
    size_t ret = std::string::npos;
    for(size_t i=pos; i<str.length(); i++)
        {
        if(!isIdentC(str[i]))
            {
            ret = i;
            break;
            }
        }
    return ret;
    }

static void getIdents(const std::string &str, std::vector<std::string> &idents)
    {
    size_t startPos=0;
    while(startPos != std::string::npos)
        {
        startPos = findIdentC(str, startPos);
        if(startPos != std::string::npos)
            {
            size_t endPos = findNonIdentC(str, startPos);
            idents.push_back(str.substr(startPos, endPos-startPos));
            if(endPos != std::string::npos)
                startPos = endPos+1;
            else
                startPos = endPos;
            }
        }
    }

void ModelData::getRelatedTemplateClasses(const ModelType &type,
        ConstModelClassifierVector &classes) const
    {
    classes.clear();
    std::string name = type.getName();
    /// @todo - this does not handle templates of templates currently.
    /// Only the inner template classes are found.
    size_t startPos = name.rfind('<');
    if(startPos != std::string::npos)
        {
        size_t endPos = name.find('>', startPos);
        std::vector<std::string> idents;
        getIdents(name.substr(startPos, endPos-startPos), idents);
        for(auto &id : idents)
            {
            const ModelClassifier *tempCl = findType(id)->getClass();
            if(tempCl)
                classes.addUnique(tempCl);
            }
        }
    }

void ModelData::getRelatedFuncParamClasses(const ModelClassifier &classifier,
        ConstModelDeclClasses &declClasses) const
    {
    declClasses.clear();
    for(auto &oper : classifier.getOperations())
        {
        for(auto &param : oper->getParams())
            {
            const ModelClassifier *cl = param->getDeclClassType();
            if(cl)
                {
                declClasses.addUnique(param.get());
                }
            }
        }
    }

bool ConstModelDeclClasses::addUnique(ModelDeclClass const &decl)
    {
    bool added = false;
    auto declIter = std::find_if(begin(), end(),
            [&decl](ModelDeclClass const &existingDecl)
               { return(decl == existingDecl); });
    if(declIter == end())
        {
        added = true;
        push_back(decl);
        }
    return(added);
    }

bool ConstModelClassifierVector::addUnique(const ModelClassifier *cl)
    {
    bool added = false;
    auto clIter = std::find_if(begin(), end(),
            [&cl](ModelClassifier const *existingNode)
               { return(cl == existingNode); });
    if(clIter == end())
        {
        added = true;
        push_back(cl);
        }
    return(added);
    }

void ModelData::getRelatedFuncInterfaceClasses(const ModelClassifier &classifier,
        ConstModelClassifierVector &classes) const
    {
    classes.clear();
    for(auto &oper : classifier.getOperations())
        {
        for(auto &param : oper->getParams())
            {
            ModelClassifier const *cl = param.get()->getDeclClassType();
            classes.addUnique(cl);
            }
        }
// If the function's return is a relation, then the relation will already
// be present as either a param, member or body user
/*
#if(OPER_RET_TYPE)
        ModelTypeRef const &retType = oper->getReturnType();
        const ModelClassifier *cl = retType.getDeclType()->getClass();
        if(cl)
            {
            classes.push_back(cl);
            }
        }
#endif
*/
    }

void ModelData::getRelatedBodyVarClasses(const ModelClassifier &classifier,
        ConstModelDeclClasses &declClasses) const
    {
    declClasses.clear();
    for(auto &oper : classifier.getOperations())
        {
        for(auto &vd : oper->getBodyVarDeclarators())
            {
            declClasses.addUnique(vd.get());
            }
        // Normally the call statements would not have to be checked since most
        // calls are through either variable declarations, or through
        // the parameters, but calls to global classes cannot be found this way.
        for(auto &stmt : oper->getStatements())
            {
            if(stmt.getStatementType() == ST_Call)
                {
                const ModelClassifier *cl = stmt.getClassDecl().getDeclType()->getClass();
                if(cl)
                    {
                    declClasses.addUnique(ModelDeclClass(stmt.getAttrName(),
                            stmt.getClassDecl().getDeclType()));
                    }
                }
            }
        }
    }

void ModelData::addBaseClasses(ModelClassifier const &type,
        ConstModelClassifierVector &classes) const
    {
    for(auto const &assoc : mAssociations)
        {
        if(assoc.get()->getChild() == &type)
            {
            ModelClassifier const *parent = assoc.get()->getParent();
            if(classes.addUnique(parent))
                {
                addBaseClasses(*parent, classes);
                }
            }
        }
    }
