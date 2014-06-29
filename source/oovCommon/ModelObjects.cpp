/*
 * ModelObject.cpp
 *
 *  Created on: Jun 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ModelObjects.h"
#include "OovString.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <algorithm>
#include <cassert>


#define DEBUG_TYPES 0

static void strReplace(const std::string &origPatt, const std::string &newPatt, std::string &str)
    {
    std::string::size_type pos = 0;
    while ((pos = str.find(origPatt, pos)) != std::string::npos)
	str.replace(pos, origPatt.length(), newPatt);
    }

void strRemoveSpaceAround(char const * const c, std::string &str)
    {
    strReplace(std::string(c) + " ", std::string(c), str);
    strReplace(std::string(" ") + c, std::string(c), str);
    }

/*
char const * const Visibility::asStr() const
{
    char const *str = "";
    switch(vis)
        {
        case Visibility::Public:        str = "public";       break;
        case Visibility::Protected:     str = "protected";    break;
        case Visibility::Private:       str = "private";      break;
        default:        break;
        }
    return str;
}
*/

char const * const Visibility::asUmlStr() const
    {
    char const *str = "";
    switch(vis)
        {
        case Visibility::Public:        str = "+";      break;
        case Visibility::Protected:     str = "#";    	break;
        case Visibility::Private:       str = "-";      break;
        default:        break;
        }
    return str;
    }

Visibility::Visibility(char const *umlStr)
    {
    switch(umlStr[0])
        {
	default:	// fall through
        case '+':	vis = Visibility::Public;      	break;
        case '#':	vis = Visibility::Protected;	break;
        case '-':	vis = Visibility::Private;      break;
        }
    }

const ModelClassifier *ModelType::getClass() const
    {
    const ModelClassifier *cl = nullptr;
    if(this)
	{
	if(mDataType == DT_Class)
	    cl = static_cast<const ModelClassifier*>(this);
	}
    return cl;
    }

ModelClassifier *ModelType::getClass()
    {
    ModelClassifier *cl = nullptr;
    if(this)
	{
	if(mDataType == DT_Class)
	    cl = static_cast<ModelClassifier*>(this);
	}
    return cl;
    }

bool ModelType::isTemplateType() const
    {
    return(getName().find('<') != std::string::npos);
    }

const class ModelClassifier *ModelDeclarator::getDeclClassType() const
    {
    return getDeclType() ? getDeclType()->getClass() : nullptr;
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

int ModelClassifier::findMatchingOperationIndex(ModelOperation const * const oper)
    {
    return getOperationIndex(oper->getName(), oper->isConst());
    }

ModelOperation *ModelClassifier::findMatchingOperation(ModelOperation const * const oper)
    {
    return getOperation(oper->getName(), oper->isConst());
    }


void ModelClassifier::replaceOperation(ModelOperation const * const operToReplace,
	    std::unique_ptr<ModelOperation> &&newOper)
	{
	int index = findMatchingOperationIndex(operToReplace);
	if(index != -1)
	    {
//	    mOperations.erase(mOperations.begin() + index);
//	    mOperations.push_back(std::move(newOper));
	    mOperations[index] = std::move(newOper);
	    }
	}

void ModelClassifier::eraseAttribute(const ModelAttribute *attr)
    {
    for(size_t i=0; i<mAttributes.size(); i++)
	{
	if(mAttributes[i].get() == attr)
	    {
	    mAttributes.erase(mAttributes.begin() + i);
	    break;
	    }
	}
    }

void ModelClassifier::eraseOperation(const ModelOperation *oper)
    {
    for(size_t i=0; i<mOperations.size(); i++)
	{
	if(mOperations[i].get() == oper)
	    {
	    mOperations.erase(mOperations.begin() + i);
	    break;
	    }
	}
    }

void ModelClassifier::removeOperation(ModelOperation *oper)
    {
    int index = -1;
    for(size_t i=0; i<mOperations.size(); i++)
	{
	if(oper == mOperations[i].get())
	    index = i;
	}
    if(index != -1)
	{
	delete oper;
	mOperations.erase(mOperations.begin() + index);
	}
    }

int ModelClassifier::getAttributeIndex(const std::string &name) const
    {
    int index = -1;
    for(size_t i=0; i<mAttributes.size(); i++)
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
    int index = getAttributeIndex(name);
    if(index != -1)
	{
	attr = mAttributes[index].get();
	}
    return attr;
    }

int ModelClassifier::getOperationIndex(const std::string &name, bool isConst) const
    {
    int index = -1;
    for(size_t i=0; i<mOperations.size(); i++)
	{
	if(mOperations[i]->getName().compare(name) == 0 &&
		mOperations[i]->isConst() == isConst)
	    {
	    index = i;
	    break;
	    }
	}
    return index;
    }

const ModelOperation *ModelClassifier::getOperation(const std::string &name, bool isConst) const
    {
    ModelOperation *oper = nullptr;
    int index = getOperationIndex(name, isConst);
    if(index != -1)
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

void ModelData::resolveDecl(ModelTypeRef &decl)
    {
    if(decl.getDeclTypeModelId() != UNDEFINED_ID)
	{
	decl.setDeclType(findTypeByModelId(decl.getDeclTypeModelId()));
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
	    fprintf(fp, "%s %d\n", type->getName().c_str(), type->getModelId());
	    }
	fclose(fp);
	}
#endif
    }

void ModelData::resolveStatements(ModelStatements &stmts)
    {
    for(auto &stmt : stmts)
	{
	if(stmt.getStatementType() == ST_Call
#if(VAR_REF)
		||
		stmt.getStatementType() == ST_VarRef
#endif
		)
	    {
	    resolveDecl(stmt.getDecl());
	    }
	}
    }

void ModelData::resolveModelIds()
    {
    dumpTypes();
    // Resolve class member attributes and operations.
    for(const auto &type : mTypes)
	{
	if(type->getDataType() == DT_Class)
	    {
	    ModelClassifier *classifier = type->getClass();
	    for(auto &attr : classifier->getAttributes())
		{
		resolveDecl(*attr);
		}
	    for(auto &oper : classifier->getOperations())
		{
		// Resolve function parameters.
		for(auto &param : oper->getParams())
		    {
		    resolveDecl(*param);
		    }
		// Resolve function call decls.
		ModelStatements &stmts = oper->getStatements();
		resolveStatements(stmts);

		// Resolve body variables.
		for(auto &vd : oper->getBodyVarDeclarators())
		    {
		    resolveDecl(*vd);
		    }
		}
	    }
	}
    // Resolve relations.
    for(auto &assoc : mAssociations)
        {
	if(assoc->getChildModelId() != UNDEFINED_ID)
	    {
	    assoc->setChildClass(findClassByModelId(assoc->getChildModelId()));
	    assoc->setParentClass(findClassByModelId(assoc->getParentModelId()));
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
	if(stmt.getStatementType() == ST_Call && stmt.getDecl().getDeclType() == &type)
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
	ModelOperation *destOper = destType->findMatchingOperation(oper.get());
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
	if(stmt.getStatementType() == ST_Call
#if(VAR_REF)
		||
		stmt.getStatementType() == ST_VarRef
#endif
		)
	    {
	    if(stmt.getDecl().getDeclType() == existingType)
		{
		stmt.getDecl().setDeclType(newType);
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
	    mTypes.erase(mTypes.begin()+ci);
	}
    }

const ModelType *ModelData::getTypeRef(char const * const typeName) const
    {
    std::string baseTypeName = getBaseType(typeName);
    return findType(baseTypeName.c_str());
    }

ModelType *ModelData::createOrGetTypeRef(char const * const typeName, eModelDataTypes dtype)
    {
    std::string baseTypeName = getBaseType(typeName);
    ModelType *type = findType(baseTypeName.c_str());
    if(!type)
	{
	type = static_cast<ModelType*>(createDataType(dtype, baseTypeName.c_str()));
	}
    return type;
    }

ModelType *ModelData::createTypeRef(char const * const typeName, eModelDataTypes dtype)
    {
    std::string baseTypeName = getBaseType(typeName);
    return static_cast<ModelType*>(createDataType(dtype, baseTypeName.c_str()));
    }

ModelObject *ModelData::createDataType(eModelDataTypes type, const std::string &id)
    {
    ModelObject *obj = nullptr;
    switch(type)
	{
	case DT_DataType:
	    {
	    ModelType *type = new ModelType(id);
	    /// @todo - use make_unique when supported.
	    addType(std::unique_ptr<ModelType>(type));
	    obj = type;
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
static inline bool skipStr(char const * * const str, char const * const compareStr)
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

static char const *skipWhite(char const *p)
    {
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

std::string ModelData::getBaseType(char const * const fullStr) const
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
	    case '*':	p++;
		break;
	    case '&':	p++;
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
	    default:	str += *p++;
		break;
	    }
	}
    int len = str.length()-1;
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
    strRemoveSpaceAround(">", str);	// There are two spaces near >
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

static inline bool compareStrs(char const * const tstr1, char const * const tstr2)
    {
    return (strcmp(tstr1, tstr2) < 0);
    }

void ModelData::addType(std::unique_ptr<ModelType> &&type)
    {
#if(BINARYSPEED)
    std::string baseTypeName = getBaseType(type->getName().c_str());
    type->setName(baseTypeName.c_str());
    auto it = std::upper_bound(mTypes.begin(), mTypes.end(), baseTypeName.c_str(),
	[](char const * const mod1Name, std::unique_ptr<ModelType> &mod2) -> bool
	{ return(compareStrs(mod1Name, mod2->getName().c_str())); } );
    mTypes.insert(it, std::move(type));
#else
    mTypes.push_back(type);
#endif
    }

const ModelType *ModelData::findType(char const * const name) const
    {
    const ModelType *type = nullptr;
#if(BINARYSPEED)
    // This comparison must produce the same sort order as addType.
    std::string baseTypeName = getBaseType(name);
    auto iter = std::lower_bound(mTypes.begin(), mTypes.end(), baseTypeName.c_str(),
	[](std::unique_ptr<ModelType> const &mod1, char const * const mod2Name) -> bool
	{ return(compareStrs(mod1->getName().c_str(), mod2Name)); } );
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

ModelType *ModelData::findType(char const * const name)
    {
    return const_cast<ModelType*>(static_cast<const ModelData*>(this)->findType(name));
    }

ModelModule const * const ModelData::findModuleById(int id)
    {
    ModelModule *mod = nullptr;
    // Searching backwards is an effective optimization because onAttr
    // has calls to this function, and many of the recently added ID's
    // will be at the end.
    if(mModules.size() > 0)
	{
	for(int i=mModules.size()-1; i>=0; i--)
	    {
	    if(mModules[i]->getModelId() == id)
		{
		mod = mModules[i].get();
		break;
		}
	    }
	}
    assert(mod);
    return mod;
    }

const ModelClassifier *ModelData::findClassByModelId(int id) const
    {
    return findTypeByModelId(id)->getClass();
    }

const ModelType *ModelData::findTypeByModelId(int id) const
    {
    const ModelType *rettype = nullptr;
    for (auto &type : mTypes)
	{
	if (type->getModelId() == id)
	    {
	    rettype = type.get();
	    break;
	    }
	}
    return rettype;
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
	    const ModelClassifier *tempCl = findType(id.c_str())->getClass();
	    if(tempCl)
		classes.push_back(tempCl);
	    }
	}
    }

void ModelData::getRelatedFuncParamClasses(const ModelClassifier &classifier,
	ConstModelDeclClassVector &declClasses) const
    {
    declClasses.clear();
    for(auto &oper : classifier.getOperations())
	{
	for(auto &param : oper->getParams())
	    {
	    const ModelClassifier *cl = param->getDeclClassType();
	    if(cl)
		declClasses.push_back(ModelDeclClass(param.get(), cl));
	    }
	}
    }

void ModelData::getRelatedBodyVarClasses(const ModelClassifier &classifier,
	ConstModelDeclClassVector &declClasses) const
    {
    declClasses.clear();
    for(auto &oper : classifier.getOperations())
	{
	for(auto &vd : oper->getBodyVarDeclarators())
	    {
	    const ModelClassifier *cl = vd->getDeclClassType();
	    if(cl)
		declClasses.push_back(ModelDeclClass(vd.get(), cl));
	    }
	}
    }
