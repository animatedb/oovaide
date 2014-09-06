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
#include <string>
#include <list>
#include <vector>
#include <memory>

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
    Visibility(char const *umlStr);
    void setVis(VisType v)
	{ vis = v; }
    VisType getVis() const
	{ return vis; }
//    char const * const asStr() const;
    char const * const asUmlStr() const;
  private:
    VisType vis;
};

class ModelObject
{
public:
    ModelObject(const std::string &name):
        mName(name), mModelId(UNDEFINED_ID)
        {}
    const std::string &getName() const
        { return mName; }
    void setName(char const * const name)
	{ mName = name; }
    void setModelId(int id)
	{ mModelId = id; }
    int getModelId() const
	{ return mModelId; }
private:
    std::string mName;
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

    private:
	ModelType const *mDeclType;
	unsigned int mDeclTypeModelId:29;
	unsigned int mStatic:1;
	unsigned int mConst:1;
	unsigned int mRefer:1;
    };

// A C++ declarator is a type and a name.
// A declarator can be a function return, function argument, attribute, etc.
class ModelDeclarator:public ModelObject, public ModelTypeRef
    {
    public:
	explicit ModelDeclarator(const std::string &name,
		ModelType const *declType):
	    ModelObject(name), ModelTypeRef(declType)
	    {}
	const class ModelClassifier *getDeclClassType() const;
    };


typedef ModelDeclarator ModelFuncParam;
typedef ModelDeclarator ModelBodyVarDecl;


// This is a record(class/struct) data member.
class ModelAttribute:public ModelDeclarator
{
public:
    explicit ModelAttribute(const std::string &name, ModelType const *attrType,
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

#define VAR_REF 0
enum eModelStatementTypes { ST_OpenNest, ST_CloseNest, ST_Call,
#if(VAR_REF)
    ST_VarRef
#endif
};

// These aren't really statements. These are important things in a function
// that must be displayed in sequence/operation diagrams.
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
	//	The decl points to the type.
	// CloseNest
	//	There is no name or decl.
	// VarRef
	//	The name is the class's attribute name
	//	The decl points to the type.
	ModelStatement(const std::string &name, eModelStatementTypes type):
	    ModelObject(name), mStatementType(type), mDecl(nullptr)
	    {}
	eModelStatementTypes getStatementType() const
	    { return mStatementType; }
	// These are only valid if this statement is a call.
	ModelTypeRef &getDecl()
	    { return mDecl; }
	const ModelTypeRef &getDecl() const
	    { return mDecl; }

    private:
	eModelStatementTypes mStatementType;
	ModelTypeRef mDecl;
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

/// This container owns all pointers (parameters, etc.) given to it, except for
/// types used to define parameters/variables.
class ModelOperation:public ModelObject
{
public:
    ModelOperation(const std::string &name, Visibility access,
	    bool isConst):
        ModelObject(name), mAccess(access),
        mIsConst(isConst), mModule(nullptr), mLineNum(0)
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

private:
    std::vector<std::unique_ptr<ModelFuncParam>> mParameters;
    // This could be expanded to contain ref/ptr/const
//    ModelDeclarator* mDefinitionDeclarator;	// return type
    std::vector<std::unique_ptr<ModelBodyVarDecl>> mBodyVarDeclarators;
    ModelStatements mStatements;
    Visibility mAccess;
    bool mIsConst;
    const class ModelModule *mModule;
    int mLineNum;
};

enum eModelDataTypes { DT_DataType, DT_Class };

// This stores info about simple (non-class) types, and is the base for all types.
class ModelType:public ModelObject
    {
    public:
	explicit ModelType(const std::string &name, eModelDataTypes type=DT_DataType):
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
    explicit ModelClassifier(const std::string &name):
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
    ModelOperation *findMatchingOperation(ModelOperation const * const oper);
    int findMatchingOperationIndex(ModelOperation const * const oper);
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
    /// @todo - this doesn't work for overloaded functions.
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

// This is used for inheritance relations
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

// This stores the module where an operation or class was defined.
class ModelModule:public ModelObject
    {
    public:
	ModelModule():
	    ModelObject("")
	    {}
	void setModulePath(const std::string &str)
	    { setName(str.c_str()); }
	const std::string &getModulePath() const
	    { return getName(); }
    };

// This is used for references to types in other classes.
// It is used for function parameters, and function body variable references
// This is only used for temporary access, and is not stored in the model memory.
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
	ModelType *createOrGetTypeRef(char const * const typeName, eModelDataTypes otype);
	ModelType *createTypeRef(char const * const typeName, eModelDataTypes otype);
	const ModelType *getTypeRef(char const * const typeName) const;
	ModelType *findType(char const * const name);
	const ModelType *findType(char const * const name) const;
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
	void getRelatedFuncParamClasses(const ModelClassifier &type,
		ConstModelDeclClassVector &declclasses) const;
	void getRelatedBodyVarClasses(const ModelClassifier &type,
		ConstModelDeclClassVector &declclasses) const;

    private:
	ModelObject *createDataType(eModelDataTypes type, const std::string &id);
	std::string getBaseType(char const * const fullStr) const;
	const ModelClassifier *findClassByModelId(int id) const;
	const ModelType *findTypeByModelId(int id) const;
        void resolveStatements(ModelStatements &stmt);
        void resolveDecl(ModelTypeRef &decl);
	bool isTypeReferencedByStatements(ModelStatements const &stmts, ModelType const &type) const;
	void dumpTypes();
    };

#endif
