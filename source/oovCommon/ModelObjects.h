/**
*   Copyright (C) 2013 by dcblaha
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
*  @file      ModelObjects.h
*
*  @date      5/27/2009
*/

#ifndef MODEL_OBJECTS_H
#define MODEL_OBJECTS_H
#include <list>
#include <vector>
#include <memory>
#include "OovString.h"

#define UNDEFINED_ID -1

class ModelType;


// The basic structure is that a class contains attributes and operations.
// A class can also inherit from another class, which must already be declared.


class Visibility
    {
  public:
    enum VisType {
        Public,
        Protected,
        Private,
    };
    Visibility(VisType v=Private)
        { vis = v; }
    Visibility(OovStringRef const umlStr);
    void setVis(VisType v)
	{ vis = v; }
    VisType getVis() const
	{ return vis; }
//    OovStringRef const asStr() const;
    OovStringRef const asUmlStr() const;
  private:
    VisType vis;
};

class ModelObject
{
public:
    ModelObject(OovStringRef const name):
        mName(name), mModelId(UNDEFINED_ID)
        {}
    const OovString &getName() const
        { return mName; }
    void setName(OovStringRef const name)
	{ mName = name; }
    void setModelId(int id)
	{ mModelId = id; }
    int getModelId() const
	{ return mModelId; }
private:
    OovString mName;
    int mModelId;
};


class ModelTypeRef
    {
    public:
	ModelTypeRef(ModelType  const *declType):
	    mDeclType(declType), mDeclTypeModelId(0), mStatic(false),
	    mConst(false), mRefer(false)
	    {}
	void setStatic(bool isStatic)
	    { mStatic = isStatic; }
	void setConst(bool isConst)
	    { mConst = isConst; }
	void setRefer(bool isRefer)
	    { mRefer = isRefer; }
	const ModelType *getDeclType() const
	    { return mDeclType; }
	void setDeclType(ModelType const *type)
	    { mDeclType = type; }
	void setDeclTypeModelId(int id)
	    { mDeclTypeModelId = id; }
	int getDeclTypeModelId() const
	    { return mDeclTypeModelId; }
	bool isConst() const
	    { return mConst; }
	bool isRefer() const
	    { return mRefer; }
	bool match(ModelTypeRef const &typeRef) const;

    private:
	ModelType const *mDeclType;
	unsigned int mDeclTypeModelId:29;
	unsigned int mStatic:1;
	unsigned int mConst:1;
	unsigned int mRefer:1;
    };

/// A C++ declarator is a type and a name.
/// A declarator can be a function return, function argument, attribute, etc.
class ModelDeclarator:public ModelObject, public ModelTypeRef
    {
    public:
	explicit ModelDeclarator(OovStringRef const name,
		ModelType const *declType):
	    ModelObject(name), ModelTypeRef(declType)
	    {}
	const class ModelClassifier *getDeclClassType() const;
	// Used for operation parameter matching
	bool match(ModelDeclarator const &decl) const
	    { return ModelTypeRef::match(decl); }
    };


typedef ModelDeclarator ModelFuncParam;
typedef ModelDeclarator ModelBodyVarDecl;


/// This is a record(class/struct) data member.
class ModelAttribute:public ModelDeclarator
{
public:
    explicit ModelAttribute(OovStringRef const name, ModelType const *attrType,
	    Visibility access):
        ModelDeclarator(name, attrType), mAccess(access)
        {}
    void setAccess(Visibility access)
        { mAccess = access; }
    Visibility getAccess() const
        { return mAccess; }
    private:
        Visibility mAccess;
};

enum eModelStatementTypes { ST_OpenNest, ST_CloseNest, ST_Call, ST_VarRef };

/// These aren't really statements. These are important things in a function
/// that must be displayed in sequence/operation diagrams.
class ModelStatement:public ModelObject
    {
    public:
	// Different types contain different parts of this class. This is done
	// to reduce memory in the ModelStatements vector.
	// OpenNest
	//	The name is the conditional statement and is optional.
	//	There is no decl
	// Call
	//	The name is the function name.
	//	The class decl points to the class type.
	// CloseNest
	//	There is no name or decl.
	// VarRef
	//	The name is the class's attribute name
	//	The class decl points to the class type.
	//	The var decl points to the variable type.
	ModelStatement(OovStringRef const name, eModelStatementTypes type):
	    ModelObject(name), mStatementType(type), mClassDecl(nullptr),
	    mVarDecl(nullptr), mVarAccessWrite(false)
	    {}
	OovString getFuncName() const;	// Only valid for call statement
	OovString getAttrName() const;	// Only valid for call statement
	eModelStatementTypes getStatementType() const
	    { return mStatementType; }
	// These are only valid for call statement.
	ModelTypeRef &getClassDecl()
	    { return mClassDecl; }
	const ModelTypeRef &getClassDecl() const
	    { return mClassDecl; }
	ModelTypeRef &getVarDecl()
	    { return mVarDecl; }
	const ModelTypeRef &getVarDecl() const
	    { return mVarDecl; }
	void setVarAccessWrite(bool write)
	    { mVarAccessWrite = write; }
	bool getVarAccessWrite() const
	    { return mVarAccessWrite; }

    private:
	eModelStatementTypes mStatementType;
	ModelTypeRef mClassDecl;
	// Only class member references are saved here.
	ModelTypeRef mVarDecl;
	bool mVarAccessWrite;	// Indicates whether the var decl is written or read.
    };

/// This is a list of statements in a function.
class ModelStatements:public std::vector<ModelStatement>
    {
    public:
	ModelStatements()
	    {}
	void addStatement(ModelStatement const &stmt)
	    {
	    push_back(stmt);
	    }
    };

#define OPER_RET_TYPE 1

/// This container owns all pointers (parameters, etc.) given to it, except for
/// types used to define parameters/variables.
class ModelOperation:public ModelObject
{
public:
    ModelOperation(OovStringRef const name, Visibility access,
	    bool isConst):
        ModelObject(name), mAccess(access),
        mIsConst(isConst), mModule(nullptr), mLineNum(0)
#if(OPER_RET_TYPE)
	, mReturnType(nullptr)
#endif
        {}
    // Returns a pointer to the added parameter so that it can be modified.
    ModelFuncParam *addMethodParameter(const std::string &name, const ModelType *type,
        bool isConst);
    void addMethodParameter(std::unique_ptr<ModelFuncParam> param)
	{
	mParameters.push_back(std::move(param));
        }
    // The operation's definition has declarations.
/*    void addMethodDefinitionDeclarator(const std::string &name, const ModelType *type,
        bool isConst, bool isRef)
        {
        ModelDeclarator *decl = new ModelDeclarator(name, otDatatype, type);
        decl->setConst(isConst);
        decl->setRefer(isRef);
        mDefinitionDeclarators.push_back(decl);
        }
*/
    ModelBodyVarDecl *addBodyVarDeclarator(const std::string &name, const ModelType *type,
        bool isConst, bool isRef);
    void addBodyVarDeclarator(std::unique_ptr<ModelBodyVarDecl> var)
	{
        mBodyVarDeclarators.push_back(std::move(var));
        }
    bool isDefinition() const;
    const std::vector<std::unique_ptr<ModelFuncParam>> &getParams() const
	{ return mParameters; }
    const std::vector<std::unique_ptr<ModelBodyVarDecl>> &getBodyVarDeclarators() const
	{ return mBodyVarDeclarators; }
#if(OPER_RET_TYPE)
    ModelTypeRef &getReturnType()
	{ return mReturnType; }
    ModelTypeRef const &getReturnType() const
	{ return mReturnType; }
#endif
    ModelStatements &getStatements()
	{ return mStatements; }
    const ModelStatements &getStatements() const
	{ return mStatements; }
    void setAccess(Visibility access)
        { mAccess = access; }
    Visibility getAccess() const
        { return mAccess; }
    void setConst(bool isConst)
	{ mIsConst = isConst; }
    bool isConst() const
	{ return mIsConst; }
    void setModule(const class ModelModule *module)
	{ mModule = module; }
    const class ModelModule *getModule() const
	{ return mModule; }
    void setLineNum(int lineNum)
	{ mLineNum = lineNum; }
    int getLineNum() const
	{ return mLineNum; }
    bool paramsMatch(std::vector<std::unique_ptr<ModelFuncParam>> const &params) const;

private:
    std::vector<std::unique_ptr<ModelFuncParam>> mParameters;
    std::vector<std::unique_ptr<ModelBodyVarDecl>> mBodyVarDeclarators;
    ModelStatements mStatements;
    Visibility mAccess;
    bool mIsConst;
    const class ModelModule *mModule;
    int mLineNum;
#if(OPER_RET_TYPE)
    ModelTypeRef mReturnType;
#endif
};

enum eModelDataTypes { DT_DataType, DT_Class };

/// This stores info about simple (non-class) types, and is the base for all types.
class ModelType:public ModelObject
    {
    public:
	explicit ModelType(OovStringRef const name, eModelDataTypes type=DT_DataType):
	    ModelObject(name), mDataType(type)
	    {}
	bool isTemplateType() const;
	eModelDataTypes getDataType() const
	    { return mDataType; }
	const class ModelClassifier *getClass() const;
	class ModelClassifier *getClass();

    private:
	eModelDataTypes mDataType;
    };

/// This stores data for class definitions.
/// This owns all pointers except for the modules, and other types.
class ModelClassifier:public ModelType
{
public:
    explicit ModelClassifier(OovStringRef const name):
        ModelType(name, DT_Class), mModule(nullptr), mLineNum(0)
        {}
    void clearOperations();
    void clearAttributes();
    ModelAttribute *addAttribute(const std::string &name, ModelType const *attrType,
	    Visibility scope);
    void addAttribute(std::unique_ptr<ModelAttribute> &&attr)
	{
        mAttributes.push_back(std::move(attr));
	}
    ModelOperation *addOperation(const std::string &name, Visibility access,
	    bool isConst);
    void removeOperation(ModelOperation *oper);
    void addOperation(std::unique_ptr<ModelOperation> &&oper)
        {
        mOperations.push_back(std::move(oper));
        }
    void replaceOperation(ModelOperation const * const operToReplace,
	    std::unique_ptr<ModelOperation> &&newOper);
    // These allow removing the pointer from the collections so that they
    // won't be destructed later.
    void eraseAttribute(const ModelAttribute *attr);
    void eraseOperation(const ModelOperation *oper);
    int getAttributeIndex(const std::string &name) const;
    const ModelAttribute *getAttribute(const std::string &name) const;

    int findExactMatchingOperationIndex(const ModelOperation &op) const;
    const ModelOperation *findExactMatchingOperation(const ModelOperation &op) const;

    /// @todo - these don't work for overloaded functions.
    const ModelOperation *getOperation(const std::string &name, bool isConst) const;
    int getOperationIndex(const std::string &name, bool isConst) const;
    ModelOperation *getOperation(const std::string &name, bool isConst)
	{
	return const_cast<ModelOperation*>(
		static_cast<const ModelClassifier*>(this)->getOperation(name, isConst));
	}
    /// @todo - This isn't really correct.
    const ModelOperation *getOperationAnyConst(const std::string &name, bool isConst) const;

    std::vector<std::unique_ptr<ModelAttribute>> &getAttributes()
	{ return mAttributes; }
    const std::vector<std::unique_ptr<ModelAttribute>> &getAttributes() const
	{ return mAttributes; }
    std::vector<std::unique_ptr<ModelOperation>> &getOperations()
	{ return mOperations; }
    const std::vector<std::unique_ptr<ModelOperation>> &getOperations() const
	{ return mOperations; }
    void setModule(const class ModelModule *module)
	{ mModule = module; }
    const class ModelModule *getModule() const
	{ return mModule; }
    void setLineNum(int lineNum)
	{ mLineNum = lineNum; }
    int getLineNum() const
	{ return mLineNum; }
    bool isDefinition() const;

private:
    std::vector<std::unique_ptr<ModelAttribute>> mAttributes;
    std::vector<std::unique_ptr<ModelOperation>> mOperations;
    const class ModelModule *mModule;
    int mLineNum;
};

/// This is used for inheritance relations
class ModelAssociation:public ModelObject
{
public:
    ModelAssociation(const ModelClassifier *child,
        const ModelClassifier *parent, Visibility access):
        ModelObject(""),
        mChildModelId(UNDEFINED_ID), mParentModelId(UNDEFINED_ID),
        mChild(child), mParent(parent), mAccess(access)
        {}
    void setChildModelId(int id)
	{ mChildModelId = id; }
    void setParentModelId(int id)
	{ mParentModelId = id; }
    int getChildModelId()
	{ return mChildModelId; }
    int getParentModelId()
	{ return mParentModelId; }
    const ModelClassifier *getChild() const
        { return mChild; }
    const ModelClassifier *getParent() const
        { return mParent; }
    void setAccess(Visibility vis)
	{ mAccess = vis; }
    const Visibility getAccess() const
        { return mAccess; }
    void setChildClass(const ModelClassifier *cl)
	{ mChild = cl; }
    void setParentClass(const ModelClassifier *cl)
	{ mParent = cl; }

private:
    int mChildModelId;
    int mParentModelId;
    const ModelClassifier *mChild;
    const ModelClassifier *mParent;
    Visibility mAccess;
};

/// There can be code and comments on the same lines. There can also
/// be blank lines. So totalling these does not equal the number of
/// lines in the module.
class ModelModuleLineStats
    {
    public:
	ModelModuleLineStats(int code=0, int comments=0, int total=0):
	    mNumCodeLines(code), mNumCommentLines(comments), mNumModuleLines(total)
	    {}
	/// The number of lines that have some code on them.
	unsigned int mNumCodeLines;
	/// The number of lines that have a comment on them. If the comment
	/// includes a blank line, it is included in the count.
	unsigned int mNumCommentLines;
	/// The total number of lines in the module.
	unsigned int mNumModuleLines;
    };

/// This stores the module where an operation or class was defined.
class ModelModule:public ModelObject
    {
    public:
	ModelModule():
	    ModelObject("")
	    {}
	void setModulePath(OovStringRef const str)
	    { setName(str); }
	const std::string &getModulePath() const
	    { return getName(); }
	ModelModuleLineStats mLineStats;
    };

/// This is used for references to types in other classes.
/// It is used for function parameters, and function body variable references
/// This is only used for temporary access, and is not stored in the model memory.
class ModelDeclClass
    {
    public:
	ModelDeclClass(const ModelDeclarator *d, const ModelClassifier *c):
	    decl(d), cl(c)
	    {}
	const ModelDeclarator *decl;
	const ModelClassifier *cl;
    };

typedef std::vector<ModelDeclClass> ConstModelDeclClassVector;
typedef std::vector<const ModelClassifier*> ConstModelClassifierVector;


/// Holds all data used to make class and sequence diagrams. This data is read
/// from the XMI files.
class ModelData
    {
    public:
	std::vector<std::unique_ptr<ModelType>> mTypes;			// Some of these (otClasses) are Nodes
	std::vector<std::unique_ptr<ModelAssociation>> mAssociations;	// Edges
	std::vector<std::unique_ptr<ModelModule>> mModules;

	void clear();
	/// Use the model ids from the file to resolve references.  This should
	/// be done for every loaded file since ID's are specific for each file.
        void resolveModelIds();
        bool isTypeReferencedByDefinedObjects(ModelType const &type) const;

        void addType(std::unique_ptr<ModelType> &&type);
        ModelModule const * const findModuleById(int id);

        // otype is only used if a type is created.
	ModelType *createOrGetTypeRef(OovStringRef const typeName, eModelDataTypes otype);
	ModelType *createTypeRef(OovStringRef const typeName, eModelDataTypes otype);
	const ModelType *getTypeRef(OovStringRef const typeName) const;
	ModelType *findType(OovStringRef const name);
	const ModelType *findType(OovStringRef const name) const;
	/// For all pointers to the old type, sets to the new type, then
	/// deletes the old type.
	void replaceType(ModelType *existingType, ModelClassifier *newType);
	void replaceStatementType(ModelStatements &stmts, ModelType *existingType,
		ModelClassifier *newType);
	void eraseType(ModelType *existingType);
	/// Takes the attributes from the source type, and moves them to the dest type.
	void takeAttributes(ModelClassifier *sourceType, ModelClassifier *destType);
	void getRelatedTemplateClasses(const ModelType &type,
		ConstModelClassifierVector &classes) const;
	void getRelatedFuncInterfaceClasses(const ModelClassifier &type,
		ConstModelClassifierVector &classes) const;
	void getRelatedFuncParamClasses(const ModelClassifier &type,
		ConstModelDeclClassVector &classes) const;
	void getRelatedBodyVarClasses(const ModelClassifier &type,
		ConstModelDeclClassVector &declclasses) const;

    private:
	ModelObject *createDataType(eModelDataTypes type, const std::string &id);
	std::string getBaseType(OovStringRef const fullStr) const;
        void resolveStatements(class TypeIdMap const &typeMap, ModelStatements &stmt);
        void resolveDecl(class TypeIdMap const &typeMap, ModelTypeRef &decl);
	bool isTypeReferencedByStatements(ModelStatements const &stmts, ModelType const &type) const;
	void dumpTypes();
    };

#endif
