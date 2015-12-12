/*
 * ModelObjectsLoad.cpp
 *
 *  Created on: Dec 9, 2015
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

// This module is used only by OovAide since it is the only program that loads
// all of the XMI files and resolves them.

#include "ModelObjects.h"
#include "Debug.h"
#include <map>
#include <algorithm>

bool ModelType::isTemplateUseType() const
    {
    // Stereotypes have two "<", but template uses have one.
    // Look for a '<' that is not followed by a '<'.
    OovString const &name = getName();
    bool templateUse = false;
    for(size_t i=0; i<name.length(); i++)
        {
        if(name[i] == '<')
            {
            // Worst case, this could index the null terminator,
            // but normal class names should not have this format.
            if(name[i+1] != '<')
                {
                templateUse = true;
                }
            }
        }
    return(templateUse);
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
// Id's for intrinsic types do not exist.
/*
            else if(id != 0)
                {
                OovString str = "Unable to resolve Decl ID ";
                str.appendInt(id);
                OovError::report(ET_Error, str);
                }
*/
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

void ModelData::resolveModelIds()
    {
    dumpTypes();
    TypeIdMap typeMap(mTypes);
    // Resolve class member attributes and operations.
    for(const auto &type : mTypes)
        {
        if(type->getDataType() == DT_Class)
            {
            ModelClassifier *classifier = ModelClassifier::getClass(type.get());
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
                resolveDecl(typeMap, oper->getReturnType());
                }
            }
        }
    // Resolve relations.
    for(auto &assoc : mAssociations)
        {
        if(assoc->getChildModelId() != UNDEFINED_ID)
            {
            assoc->setChildClass(ModelClassifier::getClass(
                typeMap.getTypeByModelId(assoc->getChildModelId())));
            assoc->setParentClass(ModelClassifier::getClass(
                typeMap.getTypeByModelId(assoc->getParentModelId())));
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

void ModelData::resolveStatements(class TypeIdMap const &typeMap, ModelStatements &stmts)
    {
    for(auto &stmt : stmts)
        {
        if(stmt.getStatementType() == ST_Call ||
                stmt.getStatementType() == ST_VarRef)
            {
#if(DEBUG_OPER)
if(stmt.getFullName().find("appendPage") != std::string::npos)
    {
    OovString str;
    str = "XMI RESOLVE CALL ";
    str += stmt.getFullName();
    OovError::report(ET_Error, str);
    }
#endif
            resolveDecl(typeMap, stmt.getClassDecl());
            if(stmt.getStatementType() == ST_VarRef)
                {
                resolveDecl(typeMap, stmt.getVarDecl());
                }
            }
        }
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
        ModelOperation const *destOper = destType->getMatchingOperation(*oper.get());
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
        ModelTypeRef const &retType = oper->getReturnType();
        const ModelClassifier *cl = retType.getDeclType()->getClass();
        if(cl)
            {
            classes.push_back(cl);
            }
        }
*/
    }

void ModelData::getRelatedTypeArgClasses(const ModelType &type,
        ConstModelClassifierVector &classes) const
    {
    classes.clear();
    OovString name = type.getName();
    OovStringVec delimiters;
    delimiters.push_back("<");
    delimiters.push_back(">");
    delimiters.push_back(",");

    OovStringVec idents = name.split(delimiters, false);
    for(auto &id : idents)
        {
        if(id.findNonSpace() != OovString::npos)
            {
            const ModelClassifier *cl = ModelClassifier::getClass(findType(id));
            if(!cl)
                {
                cl = ModelClassifier::getClass(findTemplateType(id));
                }
            if(cl)
                classes.addUnique(cl);
            }
        }
    /*
    /// @todo - this does not handle typedefs of typedefs currently.
    /// Only the inner typedef classes are found.
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
        */
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

bool ModelData::isTypeReferencedByParentClass(ModelClassifier const &classifier,
    ModelType const &checkType) const
    {
    bool referenced = false;
    for(auto &assoc : mAssociations)
        {
        if(assoc->getParent() == &checkType)
            {
            referenced = true;
            break;
            }
        }
    return referenced;
    }

bool ModelData::isTypeReferencedByClassOperationInterfaces(ModelClassifier const &classifier,
    ModelType const &type) const
    {
    bool referenced = false;
    for(auto const &op : classifier.getOperations())
        {
        referenced = isTypeReferencedByOperationInterface(*op.get(), type);
        if(referenced)
            {
            break;
            }
        }
    return referenced;
    }

void ModelClassifier::replaceOperation(ModelOperation const * const operToReplace,
            std::unique_ptr<ModelOperation> &&newOper)
        {
        size_t index = getMatchingOperationIndex(*operToReplace);
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
#if(DEBUG_OPER)
            if(oper->getName().find("appendPage") != std::string::npos)
                {
                OovString str;
                str = "eraseOper ";
                str += oper->getName().getStr();
                str += ' ';
                str.appendInt(oper->getStatements().size());
                OovError::report(ET_Error, str);
                }
#endif

            mOperations.erase(mOperations.begin() +
                static_cast<int>(i));
            break;
            }
        }
    }


#define BINARYSPEED 1

#if(BINARYSPEED)

static OovString getTemplateDefBaseName(OovString const &name)
    {
    OovString templName;
    if(ModelType::isTemplateDefType(name))
        {
        size_t pos = name.find('<');
        templName = name.substr(0, pos);
        }
    return templName;
    }

/// Gets the template definition name, or the regular name if it
/// is not a template definition. Template definitions have the template
/// stereotype.
/// This discards the template type parameters
/// This must return most of the name otherwise the binary search will fail.
static OovString getTemplateSortedBaseName(OovString const &name)
    {
    OovString templName = getTemplateDefBaseName(name);
    if(templName.length() == 0)
        {
        templName = name;
        }
    return templName;
    }
#endif

static inline bool compareStrs(OovStringRef const tstr1, OovStringRef const tstr2)
    {
    return (strcmp(tstr1, tstr2) < 0);
    }

const ModelType *ModelData::findTemplateType(OovStringRef const baseIdentName) const
    {
    const ModelType *type = nullptr;
#if(BINARYSPEED)
    // This comparison must produce the same sort order as addType.
    auto iter = std::lower_bound(mTypes.begin(), mTypes.end(), baseIdentName,
        [](std::unique_ptr<ModelType> const &mod1, OovStringRef const mod2Name) -> bool
        { return(compareStrs(getTemplateSortedBaseName(mod1->getName()), mod2Name)); } );
    while(iter != mTypes.end())
        {
        // At this point, the iterator should be at the first template name of
        // either the usage or define, so search linearly to the end.
        // This relies on the fact that the template base name is alphabetically
        // less than all other uses, which it will be since it is shorter.
        // This also expects that there will always be a definition, so it
        // shouldn't take long to iterate until it is found instead of trying
        // to stop when the base name changes. It should be very rare to reach
        // the end.
        OovString templName = getTemplateSortedBaseName((*iter)->getName());
        if(templName.compare(baseIdentName) == 0)
            {
            type = (*iter).get();
            break;
            }
        iter++;
        }
#else
#endif
    return type;
    }

